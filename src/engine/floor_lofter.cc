#include "engine/floor_lofter.h"

#include "engine/builder_mesh.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace dx {
    namespace brushbuilder {
        namespace {
            constexpr int MaxGridCells = 128;

            uint16_t read16(const std::vector<uint8_t>& bytes, size_t at)
            {
                return static_cast<uint16_t>(bytes[at] | (bytes[at + 1] << 8));
            }

            uint32_t read32(const std::vector<uint8_t>& bytes, size_t at)
            {
                return static_cast<uint32_t>(read16(bytes, at)) | (static_cast<uint32_t>(read16(bytes, at + 2)) << 16);
            }

            uint8_t luminance(uint8_t r, uint8_t g, uint8_t b)
            {
                return static_cast<uint8_t>((r * 299 + g * 587 + b * 114 + 500) / 1000);
            }

            bool decodeBmp(const std::vector<uint8_t>& bytes, GreyImage& image, std::string* error)
            {
                uint32_t offBits = read32(bytes, 10);
                int32_t width = static_cast<int32_t>(read32(bytes, 18));
                int32_t height = static_cast<int32_t>(read32(bytes, 22));
                uint16_t bitCount = read16(bytes, 28);
                uint32_t compression = read32(bytes, 30);
                uint32_t colorsUsed = read32(bytes, 46);
                uint32_t headerSize = read32(bytes, 14);
                bool topDown = height < 0;
                height = topDown ? -height : height;
                if (compression != 0 || (bitCount != 8 && bitCount != 24) || width <= 0 || height <= 0) {
                    return failWith(error, "Height maps must be uncompressed 8 or 24 bit BMP files");
                }
                uint8_t grey[256] = {};
                if (bitCount == 8) {
                    uint32_t used = colorsUsed ? std::min<uint32_t>(colorsUsed, 256) : 256;
                    for (uint32_t i = 0; i < used; i++) {
                        size_t at = 14 + headerSize + i * 4;
                        if (at + 2 >= bytes.size()) {
                            return failWith(error, "BMP palette truncated");
                        }
                        grey[i] = luminance(bytes[at + 2], bytes[at + 1], bytes[at]);
                    }
                }
                size_t pixelBytes = bitCount / 8;
                size_t stride = (static_cast<size_t>(width) * pixelBytes + 3) & ~static_cast<size_t>(3);
                image.width = width;
                image.height = height;
                image.pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
                for (int32_t y = 0; y < height; y++) {
                    size_t from = offBits + static_cast<size_t>(topDown ? y : height - 1 - y) * stride;
                    if (from + static_cast<size_t>(width) * pixelBytes > bytes.size()) {
                        return failWith(error, "BMP data truncated");
                    }
                    for (int32_t x = 0; x < width; x++) {
                        const uint8_t* p = bytes.data() + from + static_cast<size_t>(x) * pixelBytes;
                        image.pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = bitCount == 8 ? grey[p[0]] : luminance(p[2], p[1], p[0]);
                    }
                }
                return true;
            }

            bool decodePcx(const std::vector<uint8_t>& bytes, GreyImage& image, std::string* error)
            {
                int width = read16(bytes, 8) + 1 - read16(bytes, 4);
                int height = read16(bytes, 10) + 1 - read16(bytes, 6);
                size_t bytesPerLine = read16(bytes, 66);
                if (bytes[3] != 8 || bytes[65] != 1 || width <= 0 || height <= 0 || bytesPerLine < static_cast<size_t>(width)) {
                    return failWith(error, "Height maps must be 256 color PCX files");
                }
                std::vector<uint8_t> decoded(bytesPerLine * static_cast<size_t>(height), 0);
                size_t dest = 0;
                size_t at = 128;
                size_t end = bytes.size() - 769;
                while (dest < decoded.size() && at < end) {
                    uint8_t color = bytes[at++];
                    size_t run = 1;
                    if ((color & 0xc0) == 0xc0) {
                        run = color & 0x3f;
                        color = bytes[at++];
                    }
                    for (size_t i = 0; i < run && dest < decoded.size(); i++) {
                        decoded[dest++] = color;
                    }
                }
                size_t paletteAt = bytes.size() - 768;
                image.width = width;
                image.height = height;
                image.pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        size_t entry = paletteAt + decoded[static_cast<size_t>(y) * bytesPerLine + static_cast<size_t>(x)] * 3u;
                        image.pixels[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = luminance(bytes[entry], bytes[entry + 1], bytes[entry + 2]);
                    }
                }
                return true;
            }

            Vector floorPoint(const FloorGrid& grid, int index, bool base)
            {
                int column = index % (grid.sizeX + 1);
                int row = index / (grid.sizeX + 1);
                double x = column * grid.cellSize - (grid.sizeX * grid.cellSize) / 2;
                double y = row * grid.cellSize - (grid.sizeY * grid.cellSize) / 2;
                double z = base ? -grid.baseDepth : grid.heights[static_cast<size_t>(index)];
                return { static_cast<float>(x * grid.renderScale), static_cast<float>(y * grid.renderScale), static_cast<float>(z * grid.renderScale) };
            }

            void putFloor(Mesh& mesh, const FloorGrid& grid, int slot, int index, bool base)
            {
                Vector v = floorPoint(grid, index, base);
                mesh.put(slot, v.x, v.y, v.z);
            }
        }

        void resizeFloor(FloorGrid& grid)
        {
            grid.heights.resize(static_cast<size_t>((grid.sizeX + 1) * (grid.sizeY + 1)), 0.0);
            grid.reversed.resize(static_cast<size_t>(grid.sizeX * grid.sizeY), 0);
        }

        double& floorHeight(FloorGrid& grid, int column, int row)
        {
            return grid.heights[static_cast<size_t>(row * (grid.sizeX + 1) + column)];
        }

        bool buildFloor(const FloorGrid& grid, UObject* brushActor, std::string* error)
        {
            if (grid.sizeX < 1 || grid.sizeY < 1 || grid.sizeX > MaxGridCells || grid.sizeY > MaxGridCells) {
                return failWith(error, "The grid must have between 1 and 128 cells on each side");
            }
            if (grid.cellSize <= 0 || grid.renderScale <= 0) {
                return failWith(error, "Cell size and render scale must be positive");
            }
            if (grid.heights.size() != static_cast<size_t>((grid.sizeX + 1) * (grid.sizeY + 1)) || grid.reversed.size() != static_cast<size_t>(grid.sizeX * grid.sizeY)) {
                return failWith(error, "The floor grid is not initialized");
            }
            for (double height : grid.heights) {
                if (height <= -grid.baseDepth) {
                    return failWith(error, "Every vertex must be above the base");
                }
            }
            int row = grid.sizeX + 1;
            Mesh mesh;
            for (int i = 0; i < grid.sizeY; i++) {
                for (int j = 0; j < grid.sizeX; j++) {
                    int cell = i * row + j;
                    int corners[4] = { cell, cell + 1, cell + 1 + row, cell + row };
                    double key = grid.heights[static_cast<size_t>(cell)];
                    bool flat = key == grid.heights[static_cast<size_t>(corners[1])] && key == grid.heights[static_cast<size_t>(corners[2])] && key == grid.heights[static_cast<size_t>(corners[3])];
                    if (flat) {
                        mesh.begin("FloorTOP", 4);
                        for (int k = 0; k < 4; k++) {
                            putFloor(mesh, grid, k + 1, corners[k], false);
                        }
                    } else if (!grid.reversed[static_cast<size_t>(i * grid.sizeX + j)]) {
                        mesh.begin("FloorTOP", 3);
                        putFloor(mesh, grid, 1, corners[0], false);
                        putFloor(mesh, grid, 2, corners[1], false);
                        putFloor(mesh, grid, 3, corners[2], false);
                        mesh.begin("FloorTOP", 3);
                        putFloor(mesh, grid, 1, corners[0], false);
                        putFloor(mesh, grid, 2, corners[2], false);
                        putFloor(mesh, grid, 3, corners[3], false);
                    } else {
                        mesh.begin("FloorTOP", 3);
                        putFloor(mesh, grid, 1, corners[1], false);
                        putFloor(mesh, grid, 2, corners[2], false);
                        putFloor(mesh, grid, 3, corners[3], false);
                        mesh.begin("FloorTOP", 3);
                        putFloor(mesh, grid, 1, corners[1], false);
                        putFloor(mesh, grid, 2, corners[3], false);
                        putFloor(mesh, grid, 3, corners[0], false);
                    }
                }
            }
            for (int j = 0; j < grid.sizeX; j++) {
                mesh.begin("FloorXSIDE1", 4);
                putFloor(mesh, grid, 4, j, false);
                putFloor(mesh, grid, 3, j + 1, false);
                putFloor(mesh, grid, 2, j + 1, true);
                putFloor(mesh, grid, 1, j, true);
            }
            for (int j = 0; j < grid.sizeX; j++) {
                int at = grid.sizeY * row + j;
                mesh.begin("FloorXSIDE2", 4);
                putFloor(mesh, grid, 1, at, false);
                putFloor(mesh, grid, 2, at + 1, false);
                putFloor(mesh, grid, 3, at + 1, true);
                putFloor(mesh, grid, 4, at, true);
            }
            for (int i = 0; i < grid.sizeY; i++) {
                int at = i * row;
                mesh.begin("FloorYSIDE1", 4);
                putFloor(mesh, grid, 1, at, false);
                putFloor(mesh, grid, 2, at + row, false);
                putFloor(mesh, grid, 3, at + row, true);
                putFloor(mesh, grid, 4, at, true);
                mesh.begin("FloorYSIDE2", 4);
                putFloor(mesh, grid, 4, at + grid.sizeX, false);
                putFloor(mesh, grid, 3, at + grid.sizeX + row, false);
                putFloor(mesh, grid, 2, at + grid.sizeX + row, true);
                putFloor(mesh, grid, 1, at + grid.sizeX, true);
            }
            mesh.begin("FloorBOTTOM", 4);
            putFloor(mesh, grid, 4, 0, true);
            putFloor(mesh, grid, 3, grid.sizeX, true);
            putFloor(mesh, grid, 2, grid.sizeY * row + grid.sizeX, true);
            putFloor(mesh, grid, 1, grid.sizeY * row, true);
            return sendMesh(mesh, brushActor, "Floor", false, error);
        }

        bool loadGreyImage(const std::filesystem::path& file, GreyImage& image, std::string* error)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in) {
                return failWith(error, "Cannot open " + file.string());
            }
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (bytes.size() >= 54 && bytes[0] == 'B' && bytes[1] == 'M') {
                return decodeBmp(bytes, image, error);
            }
            if (bytes.size() >= 128 + 769 && bytes[0] == 10) {
                return decodePcx(bytes, image, error);
            }
            return failWith(error, "Height maps must be BMP or PCX files");
        }

        void applyHeightMap(FloorGrid& grid, const GreyImage& image, double heightScale)
        {
            resizeFloor(grid);
            for (int row = 0; row <= grid.sizeY; row++) {
                for (int column = 0; column <= grid.sizeX; column++) {
                    bool inside = column < image.width && row < image.height;
                    double pixel = inside ? image.pixels[static_cast<size_t>(row * image.width + column)] : 0;
                    floorHeight(grid, column, row) = pixel * heightScale;
                }
            }
        }

        FloorGrid heightMapGrid(const GreyImage& image, double cellSize, double heightScale, double baseDepth)
        {
            FloorGrid grid;
            grid.sizeX = image.width - 1;
            grid.sizeY = image.height - 1;
            grid.cellSize = cellSize;
            grid.baseDepth = baseDepth;
            if (grid.sizeX > 0 && grid.sizeY > 0) {
                applyHeightMap(grid, image, heightScale);
            }
            return grid;
        }
    }
}
