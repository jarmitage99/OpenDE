#include "engine/editor_ops.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/props_engine.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dx {
    namespace editorops {
        namespace {
            constexpr uint32_t PolyNoEdit = 0x01000000 | 0x02000000 | 0x40000000 | 0x00010000 | 0x80000000;

            std::string vectorText(const Vector& v)
            {
                char text[128];
                std::snprintf(text, sizeof(text), "%+013.6f,%+013.6f,%+013.6f", static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z));
                return text;
            }

            std::string upper(std::string_view text)
            {
                std::string result(text);
                for (char& c : result) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                }
                return result;
            }

            bool parseCommand(std::string_view& line, const char* command)
            {
                size_t start = 0;
                while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
                    start++;
                }
                std::string word(command);
                if (upper(line.substr(start, word.size())) != word) {
                    return false;
                }
                size_t end = start + word.size();
                if (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) {
                    return false;
                }
                line.remove_prefix(end);
                while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
                    line.remove_prefix(1);
                }
                return true;
            }

            bool parseValue(std::string_view line, const char* match, std::string& value)
            {
                size_t found = upper(line).find(match);
                if (found == std::string::npos) {
                    return false;
                }
                size_t start = found + std::string_view(match).size();
                size_t end = start;
                while (end < line.size() && line[end] != ' ' && line[end] != '\t' && line[end] != ',' && line[end] != ')' && line[end] != '\r' && line[end] != '\n') {
                    end++;
                }
                value = std::string(line.substr(start, end - start));
                return true;
            }

            Vector parseVector(std::string_view line)
            {
                std::string text(line);
                Vector v;
                const char* cursor = text.c_str();
                v.x = std::strtof(cursor, nullptr);
                const char* comma = std::strchr(cursor, ',');
                if (!comma) {
                    return v;
                }
                v.y = std::strtof(comma + 1, nullptr);
                comma = std::strchr(comma + 1, ',');
                if (!comma) {
                    return v;
                }
                v.z = std::strtof(comma + 1, nullptr);
                return v;
            }
        }

        void updateMaster(UModel& model, int32_t surfIndex, bool updateTexCoords, bool updateBase)
        {
            BspSurf& surf = model.surfs[static_cast<size_t>(surfIndex)];
            if (!surf.actor || !surf.actor->data) {
                return;
            }
            UModel* brush = cast<UModel>(actor::Brush(surf.actor));
            if (!brush || !brush->polys || surf.brushPoly < 0) {
                return;
            }
            bsp::ModelCoords uncoords;
            if (updateTexCoords || updateBase) {
                bsp::buildCoords(surf.actor, nullptr, &uncoords);
            }
            std::vector<Poly>& polys = brush->polys->polys;
            for (size_t i = static_cast<size_t>(surf.brushPoly); i < polys.size(); i++) {
                Poly& master = polys[i];
                if (static_cast<int32_t>(i) != surf.brushPoly && master.link != surf.brushPoly) {
                    continue;
                }
                master.texture = surf.texture;
                master.panU = surf.panU;
                master.panV = surf.panV;
                master.polyFlags = surf.polyFlags & ~PolyNoEdit;
                if (updateTexCoords) {
                    master.textureU = bsp::transformVector(model.vectors[static_cast<size_t>(surf.vTextureU)], uncoords.vectorXform);
                    master.textureV = bsp::transformVector(model.vectors[static_cast<size_t>(surf.vTextureV)], uncoords.vectorXform);
                }
                if (updateBase) {
                    master.base = bsp::transformVector(model.points[static_cast<size_t>(surf.pBase)] - actor::Location(surf.actor), uncoords.pointXform) + actor::PrePivot(surf.actor);
                }
            }
        }

        void setTexture(UModel& model, const std::set<int32_t>& surfs, UObject* texture)
        {
            for (int32_t i : surfs) {
                model.surfs[static_cast<size_t>(i)].texture = texture;
                updateMaster(model, i, false, false);
            }
        }

        void setAndClearFlags(UModel& model, const std::set<int32_t>& surfs, uint32_t setBits, uint32_t clearBits)
        {
            for (int32_t i : surfs) {
                BspSurf& surf = model.surfs[static_cast<size_t>(i)];
                uint32_t flags = (surf.polyFlags & ~clearBits) | setBits;
                if (flags != surf.polyFlags) {
                    surf.polyFlags = flags;
                    updateMaster(model, i, false, false);
                }
            }
        }

        void texPan(UModel& model, const std::set<int32_t>& surfs, int panU, int panV, bool absolute)
        {
            for (int32_t i : surfs) {
                BspSurf& surf = model.surfs[static_cast<size_t>(i)];
                if (absolute) {
                    surf.panU = static_cast<int16_t>(panU);
                    surf.panV = static_cast<int16_t>(panV);
                } else {
                    surf.panU = static_cast<int16_t>(surf.panU + panU);
                    surf.panV = static_cast<int16_t>(surf.panV + panV);
                }
                updateMaster(model, i, false, false);
            }
        }

        void texScale(UModel& model, const std::set<int32_t>& surfs, float uu, float uv, float vu, float vv, bool absolute)
        {
            for (int32_t i : surfs) {
                BspSurf& surf = model.surfs[static_cast<size_t>(i)];
                Vector originalU = model.vectors[static_cast<size_t>(surf.vTextureU)];
                Vector originalV = model.vectors[static_cast<size_t>(surf.vTextureV)];
                if (absolute) {
                    originalU *= static_cast<float>(1.0 / static_cast<double>(originalU.size()));
                    originalV *= static_cast<float>(1.0 / static_cast<double>(originalV.size()));
                }
                Vector newU = originalU * uu + originalV * uv;
                Vector newV = originalU * vu + originalV * vv;
                int32_t u = bsp::vectorIndex(model, newU, false);
                int32_t v = bsp::vectorIndex(model, newV, false);
                BspSurf& target = model.surfs[static_cast<size_t>(i)];
                target.vTextureU = u;
                target.vTextureV = v;
                updateMaster(model, i, true, false);
                target.lightMap = -1;
            }
        }

        void texAlign(UModel& model, const std::set<int32_t>& surfs, TexAlign align)
        {
            for (int32_t i : surfs) {
                Vector normal = model.vectors[static_cast<size_t>(model.surfs[static_cast<size_t>(i)].vNormal)];
                switch (align) {
                case TexAlign::Default: {
                    BspSurf& surf = model.surfs[static_cast<size_t>(i)];
                    if (!surf.actor || !surf.actor->data) {
                        break;
                    }
                    UModel* brush = cast<UModel>(actor::Brush(surf.actor));
                    if (!brush || !brush->polys || surf.brushPoly < 0 || static_cast<size_t>(surf.brushPoly) >= brush->polys->polys.size()) {
                        break;
                    }
                    Poly edPoly = brush->polys->polys[static_cast<size_t>(surf.brushPoly)];
                    bsp::ModelCoords coords;
                    bsp::ModelCoords uncoords;
                    float orientation = bsp::buildCoords(surf.actor, &coords, &uncoords);
                    edPoly.textureU = {};
                    edPoly.textureV = {};
                    edPoly.base = edPoly.vertices.empty() ? edPoly.base : edPoly.vertices[0];
                    edPoly.panU = 0;
                    edPoly.panV = 0;
                    bsp::finalizePoly(edPoly);
                    bsp::transformPoly(edPoly, coords, {}, {}, orientation);
                    int32_t u = bsp::vectorIndex(model, edPoly.textureU, false);
                    int32_t v = bsp::vectorIndex(model, edPoly.textureV, false);
                    BspSurf& target = model.surfs[static_cast<size_t>(i)];
                    target.vTextureU = u;
                    target.vTextureV = v;
                    target.panU = edPoly.panU;
                    target.panV = edPoly.panV;
                    target.lightMap = -1;
                    updateMaster(model, i, true, true);
                    break;
                }
                case TexAlign::Floor:
                    if (std::abs(normal.z) > 0.05f) {
                        Vector base = model.points[static_cast<size_t>(model.surfs[static_cast<size_t>(i)].pBase)];
                        base = { 0.0f, 0.0f, base.dot(normal) / normal.z };
                        int32_t p = bsp::pointIndex(model, base, true);
                        Vector temp { 1, 0, 0 };
                        temp = temp - normal * temp.dot(normal);
                        int32_t u = bsp::vectorIndex(model, temp, false);
                        temp = { 0, 1, 0 };
                        temp = temp - normal * temp.dot(normal);
                        int32_t v = bsp::vectorIndex(model, temp, false);
                        BspSurf& target = model.surfs[static_cast<size_t>(i)];
                        target.pBase = p;
                        target.vTextureU = u;
                        target.vTextureV = v;
                        target.panU = 0;
                        target.panV = 0;
                        target.lightMap = -1;
                    }
                    updateMaster(model, i, true, true);
                    break;
                case TexAlign::WallDir:
                    if (std::abs(normal.z) < 0.95f) {
                        Vector u { normal.y, -normal.x, 0.0f };
                        u *= static_cast<float>(1.0 / static_cast<double>(u.size()));
                        Vector v = u.cross(normal);
                        v *= static_cast<float>(1.0 / static_cast<double>(v.size()));
                        if (v.z > 0.0f) {
                            v *= -1.0f;
                            u *= -1.0f;
                        }
                        int32_t ui = bsp::vectorIndex(model, u, false);
                        int32_t vi = bsp::vectorIndex(model, v, false);
                        BspSurf& target = model.surfs[static_cast<size_t>(i)];
                        target.vTextureU = ui;
                        target.vTextureV = vi;
                        target.panU = 0;
                        target.panV = 0;
                        target.lightMap = -1;
                        updateMaster(model, i, true, false);
                    }
                    break;
                case TexAlign::WallPan: {
                    BspSurf& surf = model.surfs[static_cast<size_t>(i)];
                    Vector base = model.points[static_cast<size_t>(surf.pBase)];
                    Vector v = model.vectors[static_cast<size_t>(surf.vTextureV)];
                    if (std::abs(normal.z) < 0.95f && std::abs(v.z) > 0.05f) {
                        float k = -base.z / v.z;
                        v *= k;
                        base += v;
                        int32_t p = bsp::pointIndex(model, base, true);
                        BspSurf& target = model.surfs[static_cast<size_t>(i)];
                        target.pBase = p;
                        target.lightMap = -1;
                        updateMaster(model, i, true, true);
                    }
                    break;
                }
                case TexAlign::OneTile:
                    model.surfs[static_cast<size_t>(i)].lightMap = -1;
                    updateMaster(model, i, true, true);
                    break;
                }
            }
        }

        std::string exportPolys(const UPolys& polys)
        {
            std::string out = "Begin PolyList\r\n";
            for (const Poly& poly : polys.polys) {
                out += "   Begin Polygon";
                if (poly.itemName != Name()) {
                    out += " Item=" + poly.itemName.str();
                }
                if (poly.texture) {
                    out += " Texture=" + poly.texture->name.str();
                }
                if (poly.polyFlags != 0) {
                    out += " Flags=" + std::to_string(static_cast<int32_t>(poly.polyFlags));
                }
                if (poly.link != -1) {
                    out += " Link=" + std::to_string(poly.link);
                }
                out += "\r\n";
                out += "      Origin   " + vectorText(poly.base) + "\r\n";
                out += "      Normal   " + vectorText(poly.normal) + "\r\n";
                if (poly.panU != 0 || poly.panV != 0) {
                    out += "      Pan      U=" + std::to_string(poly.panU) + " V=" + std::to_string(poly.panV) + "\r\n";
                }
                out += "      TextureU " + vectorText(poly.textureU) + "\r\n";
                out += "      TextureV " + vectorText(poly.textureV) + "\r\n";
                for (const Vector& vertex : poly.vertices) {
                    out += "      Vertex   " + vectorText(vertex) + "\r\n";
                }
                out += "   End Polygon\r\n";
            }
            out += "End PolyList\r\n";
            return out;
        }

        void importPolys(UPolys& polys, std::string_view text)
        {
            polys.polys.clear();
            UClass* textureClass = objects::findClass("Engine.Texture");
            Poly poly;
            bool gotBase = false;
            while (!text.empty()) {
                size_t end = text.find('\n');
                std::string_view line = text.substr(0, end);
                text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                    line.remove_suffix(1);
                }
                std::string_view rest = line;
                std::string value;
                if (parseCommand(rest, "END") && parseCommand(rest, "POLYLIST")) {
                    break;
                }
                rest = line;
                if (parseCommand(rest, "BEGIN") && parseCommand(rest, "POLYGON")) {
                    poly = Poly();
                    if (parseValue(rest, "LINK=", value)) {
                        poly.link = std::atoi(value.c_str());
                    }
                    if (parseValue(rest, "ITEM=", value)) {
                        poly.itemName = Name(value);
                    }
                    if (parseValue(rest, "TEXTURE=", value)) {
                        poly.texture = objects::findObject(value, textureClass);
                    }
                    if (parseValue(rest, "FLAGS=", value)) {
                        poly.polyFlags = static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 10));
                    }
                    poly.polyFlags &= ~PolyNoEdit;
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "PAN")) {
                    if (parseValue(rest, "U=", value)) {
                        poly.panU = static_cast<int16_t>(std::atoi(value.c_str()));
                    }
                    if (parseValue(rest, "V=", value)) {
                        poly.panV = static_cast<int16_t>(std::atoi(value.c_str()));
                    }
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "ORIGIN")) {
                    gotBase = true;
                    poly.base = parseVector(rest);
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "VERTEX")) {
                    if (poly.vertices.size() < 16) {
                        poly.vertices.push_back(parseVector(rest));
                    }
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "TEXTUREU")) {
                    poly.textureU = parseVector(rest);
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "TEXTUREV")) {
                    poly.textureV = parseVector(rest);
                    continue;
                }
                rest = line;
                if (parseCommand(rest, "END") && parseCommand(rest, "POLYGON")) {
                    if (!gotBase && !poly.vertices.empty()) {
                        poly.base = poly.vertices[0];
                    }
                    if (bsp::finalizePoly(poly) == 0) {
                        polys.polys.push_back(poly);
                    }
                    gotBase = false;
                }
            }
        }
    }
}
