#include "engine/resources.h"

#include "core/objects.h"
#include "engine/reader.h"

#include <stdexcept>

namespace dx {
    namespace {
        Vector readVector(Reader& reader)
        {
            Vector value;
            value.x = reader.f32();
            value.y = reader.f32();
            value.z = reader.f32();
            return value;
        }

        size_t readCount(Reader& reader)
        {
            int32_t count = reader.index();
            if (count < 0) {
                throw std::runtime_error("negative array count");
            }
            return static_cast<size_t>(count);
        }

        void readMips(Reader& reader, std::vector<TextureMip>& mips, uint16_t version)
        {
            uint8_t count = reader.u8();
            mips.resize(count);
            for (TextureMip& mip : mips) {
                if (version > 61) {
                    reader.u32();
                }
                mip.data.resize(readCount(reader));
                reader.bytes(mip.data.data(), mip.data.size());
                mip.width = reader.u32();
                mip.height = reader.u32();
                mip.ubits = reader.u8();
                mip.vbits = reader.u8();
            }
        }

        void decodeDxt1(const TextureMip& mip, std::vector<uint8_t>& out)
        {
            uint32_t blocksX = (mip.width + 3) / 4;
            uint32_t blocksY = (mip.height + 3) / 4;
            if (mip.data.size() < static_cast<size_t>(blocksX) * blocksY * 8) {
                throw std::runtime_error("truncated DXT1 mipmap");
            }
            for (uint32_t by = 0; by < blocksY; by++) {
                for (uint32_t bx = 0; bx < blocksX; bx++) {
                    const uint8_t* block = mip.data.data() + (static_cast<size_t>(by) * blocksX + bx) * 8;
                    uint32_t c0 = static_cast<uint32_t>(block[0] | (block[1] << 8));
                    uint32_t c1 = static_cast<uint32_t>(block[2] | (block[3] << 8));
                    uint32_t colors[4][4] = {};
                    for (uint32_t i = 0; i < 2; i++) {
                        uint32_t c = i == 0 ? c0 : c1;
                        colors[i][0] = ((c >> 11) & 31) * 255 / 31;
                        colors[i][1] = ((c >> 5) & 63) * 255 / 63;
                        colors[i][2] = (c & 31) * 255 / 31;
                        colors[i][3] = 255;
                    }
                    for (uint32_t ch = 0; ch < 3; ch++) {
                        if (c0 > c1) {
                            colors[2][ch] = (2 * colors[0][ch] + colors[1][ch]) / 3;
                            colors[3][ch] = (colors[0][ch] + 2 * colors[1][ch]) / 3;
                        } else {
                            colors[2][ch] = (colors[0][ch] + colors[1][ch]) / 2;
                            colors[3][ch] = 0;
                        }
                    }
                    colors[2][3] = 255;
                    colors[3][3] = c0 > c1 ? 255 : 0;
                    uint32_t bits = static_cast<uint32_t>(block[4] | (block[5] << 8) | (block[6] << 16)) | (static_cast<uint32_t>(block[7]) << 24);
                    for (uint32_t py = 0; py < 4; py++) {
                        for (uint32_t px = 0; px < 4; px++) {
                            uint32_t x = bx * 4 + px;
                            uint32_t y = by * 4 + py;
                            if (x >= mip.width || y >= mip.height) {
                                continue;
                            }
                            uint32_t code = (bits >> (2 * (py * 4 + px))) & 3;
                            uint8_t* pixel = out.data() + (static_cast<size_t>(y) * mip.width + x) * 4;
                            for (uint32_t ch = 0; ch < 4; ch++) {
                                pixel[ch] = static_cast<uint8_t>(colors[code][ch]);
                            }
                        }
                    }
                }
            }
        }
    }

    void UPalette::serialize(Reader& reader, Linker&)
    {
        size_t count = readCount(reader);
        colors.resize(count);
        for (uint32_t& color : colors) {
            color = reader.u32();
        }
    }

    void UTexture::serialize(Reader& reader, Linker& source)
    {
        uint16_t version = source.file().version();
        readMips(reader, mips, version);
        uint8_t* hasComp = propertyAddress(Name("bHasComp"));
        if (hasComp && *reinterpret_cast<uint32_t*>(hasComp)) {
            readMips(reader, compressedMips, version);
        }
    }

    uint8_t UTexture::format() const
    {
        uint8_t* address = propertyAddress(Name("Format"));
        return address ? *address : 0;
    }

    UPalette* UTexture::palette() const
    {
        if (!data) {
            return nullptr;
        }
        uint8_t* address = propertyAddress(Name("Palette"));
        if (!address) {
            return nullptr;
        }
        UObject* object = *reinterpret_cast<UObject**>(address);
        objects::ensureLoaded(object);
        return cast<UPalette>(object);
    }

    bool UTexture::masked() const
    {
        uint8_t* address = propertyAddress(Name("bMasked"));
        return address && *reinterpret_cast<uint32_t*>(address) != 0;
    }

    uint32_t UTexture::width() const
    {
        return mips.empty() ? 0 : mips[0].width;
    }

    uint32_t UTexture::height() const
    {
        return mips.empty() ? 0 : mips[0].height;
    }

    bool UTexture::decodable() const
    {
        if (mips.empty() || mips[0].width == 0 || mips[0].height == 0) {
            return false;
        }
        size_t pixels = static_cast<size_t>(mips[0].width) * mips[0].height;
        switch (format()) {
        case 0:
            return mips[0].data.size() >= pixels && palette() && palette()->colors.size() >= 256;
        case 3:
            return mips[0].data.size() >= pixels / 2;
        case 5:
            return mips[0].data.size() >= pixels * 4;
        default:
            return false;
        }
    }

    std::vector<uint8_t> UTexture::rgba(size_t mipIndex) const
    {
        const TextureMip& mip = mips.at(mipIndex);
        size_t pixels = static_cast<size_t>(mip.width) * mip.height;
        std::vector<uint8_t> out(pixels * 4);
        switch (format()) {
        case 0: {
            UPalette* pal = palette();
            if (!pal || pal->colors.size() < 256 || mip.data.size() < pixels) {
                throw std::runtime_error("bad P8 texture " + name.str());
            }
            bool isMasked = masked();
            for (size_t i = 0; i < pixels; i++) {
                uint8_t index = mip.data[i];
                uint32_t color = pal->colors[index];
                out[i * 4 + 0] = static_cast<uint8_t>(color & 0xff);
                out[i * 4 + 1] = static_cast<uint8_t>((color >> 8) & 0xff);
                out[i * 4 + 2] = static_cast<uint8_t>((color >> 16) & 0xff);
                out[i * 4 + 3] = (isMasked && index == 0) ? 0 : 255;
            }
            break;
        }
        case 3:
            decodeDxt1(mip, out);
            break;
        case 5:
            if (mip.data.size() < pixels * 4) {
                throw std::runtime_error("bad RGBA8 texture " + name.str());
            }
            for (size_t i = 0; i < pixels; i++) {
                out[i * 4 + 0] = mip.data[i * 4 + 2];
                out[i * 4 + 1] = mip.data[i * 4 + 1];
                out[i * 4 + 2] = mip.data[i * 4 + 0];
                out[i * 4 + 3] = mip.data[i * 4 + 3];
            }
            break;
        default:
            throw std::runtime_error("unsupported texture format in " + name.str());
        }
        return out;
    }

    void UFont::serialize(Reader& reader, Linker& source)
    {
        size_t count = readCount(reader);
        pages.resize(count);
        for (FontPage& page : pages) {
            page.texture = cast<UTexture>(source.readObject(reader));
            page.characters.resize(readCount(reader));
            for (FontCharacter& character : page.characters) {
                character.u = reader.i32();
                character.v = reader.i32();
                character.width = reader.i32();
                character.height = reader.i32();
            }
        }
        charactersPerPage = reader.u32();
        if (charactersPerPage == 0 && !pages.empty()) {
            charactersPerPage = static_cast<uint32_t>(pages[0].characters.size());
        }
    }

    const FontCharacter* UFont::glyph(uint8_t character, UTexture** texture) const
    {
        if (pages.empty() || charactersPerPage == 0) {
            return nullptr;
        }
        size_t page = character / charactersPerPage;
        size_t index = character % charactersPerPage;
        if (page >= pages.size() || index >= pages[page].characters.size()) {
            return nullptr;
        }
        if (texture) {
            *texture = pages[page].texture;
            objects::ensureLoaded(*texture);
        }
        return &pages[page].characters[index];
    }

    void USound::serialize(Reader& reader, Linker& source)
    {
        format = source.readName(reader);
        reader.u32();
        bytes.resize(readCount(reader));
        reader.bytes(bytes.data(), bytes.size());
    }

    void UMusic::serialize(Reader& reader, Linker& source)
    {
        format = source.readName(reader);
        reader.u32();
        bytes.resize(readCount(reader));
        reader.bytes(bytes.data(), bytes.size());
    }

    void UPolys::serialize(Reader& reader, Linker& source)
    {
        int32_t count = reader.i32();
        reader.i32();
        polys.resize(static_cast<size_t>(std::max(count, 0)));
        for (Poly& poly : polys) {
            size_t vertices = readCount(reader);
            poly.base = readVector(reader);
            poly.normal = readVector(reader);
            poly.textureU = readVector(reader);
            poly.textureV = readVector(reader);
            poly.vertices.resize(vertices);
            for (Vector& vertex : poly.vertices) {
                vertex = readVector(reader);
            }
            poly.polyFlags = reader.u32();
            poly.actor = source.readObject(reader);
            poly.texture = source.readObject(reader);
            poly.itemName = source.readName(reader);
            poly.link = reader.index();
            poly.brushPoly = reader.index();
            poly.panU = reader.i16();
            poly.panV = reader.i16();
        }
    }

    void UPrimitive::serialize(Reader& reader, Linker&)
    {
        bounds.min = readVector(reader);
        bounds.max = readVector(reader);
        bounds.valid = reader.u8() != 0;
        sphere.x = reader.f32();
        sphere.y = reader.f32();
        sphere.z = reader.f32();
        sphere.w = reader.f32();
    }

    void UModel::serialize(Reader& reader, Linker& source)
    {
        UPrimitive::serialize(reader, source);
        size_t count = readCount(reader);
        vectors.resize(count);
        for (Vector& v : vectors) {
            v = readVector(reader);
        }
        count = readCount(reader);
        points.resize(count);
        for (Vector& v : points) {
            v = readVector(reader);
        }
        count = readCount(reader);
        nodes.resize(count);
        for (BspNode& node : nodes) {
            node.plane.x = reader.f32();
            node.plane.y = reader.f32();
            node.plane.z = reader.f32();
            node.plane.w = reader.f32();
            node.zoneMask = reader.u64();
            node.nodeFlags = reader.u8();
            node.vertPool = reader.index();
            node.surf = reader.index();
            node.back = reader.index();
            node.front = reader.index();
            node.iPlane = reader.index();
            node.collisionBound = reader.index();
            node.renderBound = reader.index();
            node.zone[0] = static_cast<uint8_t>(reader.index());
            node.zone[1] = static_cast<uint8_t>(reader.index());
            node.numVertices = reader.u8();
            node.leaf[0] = reader.i32();
            node.leaf[1] = reader.i32();
        }
        count = readCount(reader);
        surfs.resize(count);
        for (BspSurf& surf : surfs) {
            surf.texture = source.readObject(reader);
            surf.polyFlags = reader.u32();
            surf.pBase = reader.index();
            surf.vNormal = reader.index();
            surf.vTextureU = reader.index();
            surf.vTextureV = reader.index();
            surf.lightMap = reader.index();
            surf.brushPoly = reader.index();
            surf.panU = reader.i16();
            surf.panV = reader.i16();
            surf.actor = source.readObject(reader);
        }
        count = readCount(reader);
        verts.resize(count);
        for (BspVert& vert : verts) {
            vert.vertex = reader.index();
            vert.side = reader.index();
        }
        numSharedSides = reader.i32();
        int32_t zoneCount = reader.i32();
        zones.resize(64);
        for (int32_t i = 0; i < zoneCount; i++) {
            ZoneProperties zone;
            zone.zoneActor = source.readObject(reader);
            zone.connectivity = reader.u64();
            zone.visibility = reader.u64();
            if (i < 64) {
                zones[static_cast<size_t>(i)] = zone;
            }
        }
        polys = cast<UPolys>(source.readObject(reader));
        count = readCount(reader);
        lightMaps.resize(count);
        for (LightMapIndex& entry : lightMaps) {
            entry.dataOffset = reader.i32();
            entry.pan = readVector(reader);
            entry.uClamp = reader.index();
            entry.vClamp = reader.index();
            entry.uScale = reader.f32();
            entry.vScale = reader.f32();
            entry.lightActors = reader.i32();
        }
        lightBits.resize(readCount(reader));
        reader.bytes(lightBits.data(), lightBits.size());
        count = readCount(reader);
        nodeBounds.resize(count);
        for (BoundingBox& box : nodeBounds) {
            box.min = readVector(reader);
            box.max = readVector(reader);
            box.valid = reader.u8() != 0;
        }
        count = readCount(reader);
        leafHulls.resize(count);
        for (int32_t& hull : leafHulls) {
            hull = reader.i32();
        }
        count = readCount(reader);
        leaves.resize(count);
        for (ConvexLeaf& leaf : leaves) {
            leaf.zone = reader.index();
            leaf.permeating = reader.index();
            leaf.volumetric = reader.index();
            leaf.visibleZones = reader.u64();
        }
        count = readCount(reader);
        lights.resize(count);
        for (UObject*& light : lights) {
            light = source.readObject(reader);
        }
        rootOutside = reader.i32();
        linked = reader.i32();
    }

    void ULevel::serialize(Reader& reader, Linker& source)
    {
        int32_t count = reader.i32();
        reader.i32();
        actors.resize(static_cast<size_t>(std::max(count, 0)));
        for (UObject*& actor : actors) {
            actor = source.readObject(reader);
        }
        std::string protocol = reader.string();
        std::string host = reader.string();
        std::string map = reader.string();
        std::string portal = reader.string();
        url = map;
        int32_t options = reader.index();
        for (int32_t i = 0; i < options; i++) {
            url += "?" + reader.string();
        }
        if (!portal.empty()) {
            url += "#" + portal;
        }
        reader.i32();
        reader.u32();
        model = cast<UModel>(source.readObject(reader));
        count = reader.index();
        for (int32_t i = 0; i < count; i++) {
            ReachSpec spec;
            spec.distance = reader.i32();
            spec.start = source.readObject(reader);
            spec.end = source.readObject(reader);
            spec.collisionRadius = reader.i32();
            spec.collisionHeight = reader.i32();
            spec.reachFlags = reader.i32();
            spec.pruned = reader.u8();
            reachSpecs.push_back(spec);
        }
        timeSeconds = reader.f32();
        source.readObject(reader);
        for (UObject*& block : textBlocks) {
            block = source.readObject(reader);
        }
        int32_t travelCount = reader.index();
        for (int32_t i = 0; i < travelCount; i++) {
            std::string key = reader.string();
            std::string value = reader.string();
            travelInfo.emplace_back(key, value);
        }
    }

    void registerEngineResources()
    {
        objects::registerFactory(Name("Palette"), [] { return new UPalette(); });
        objects::registerFactory(Name("Texture"), [] { return new UTexture(); });
        objects::registerFactory(Name("Font"), [] { return new UFont(); });
        objects::registerFactory(Name("Sound"), [] { return new USound(); });
        objects::registerFactory(Name("Music"), [] { return new UMusic(); });
        objects::registerFactory(Name("Polys"), [] { return new UPolys(); });
        objects::registerFactory(Name("Primitive"), [] { return new UPrimitive(); });
        objects::registerFactory(Name("Model"), [] { return new UModel(); });
        objects::registerFactory(Name("Level"), [] { return new ULevel(); });
    }
}
