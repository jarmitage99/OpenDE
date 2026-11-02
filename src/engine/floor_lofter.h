#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dx {
    namespace brushbuilder {
        struct FloorGrid {
            int sizeX = 4;
            int sizeY = 4;
            double cellSize = 64;
            double renderScale = 1;
            double baseDepth = 64;
            std::vector<double> heights;
            std::vector<uint8_t> reversed;
        };

        struct GreyImage {
            int width = 0;
            int height = 0;
            std::vector<uint8_t> pixels;
        };

        constexpr int FloorMaxCells = 32;

        void resizeFloor(FloorGrid& grid);
        double& floorHeight(FloorGrid& grid, int column, int row);
        bool buildFloor(const FloorGrid& grid, UObject* brushActor, std::string* error);
        bool loadGreyImage(const std::filesystem::path& file, GreyImage& image, std::string* error);
        void applyHeightMap(FloorGrid& grid, const GreyImage& image, double heightScale);
        FloorGrid heightMapGrid(const GreyImage& image, double cellSize, double heightScale, double baseDepth);
    }
}
