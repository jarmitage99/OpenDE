#include "core/objects.h"
#include "deusex/deusex.h"
#include "engine/resources.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace dx::deusex {
    void spliceMenuBackground(UObject* menu, int cut, int resume)
    {
        int cols = *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("textureCols")));
        int rows = *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("textureRows")));
        std::vector<UTexture*> tiles;
        int height = 0;
        for (int i = 0; i < cols * rows; i++) {
            UObject* tile = *reinterpret_cast<UObject**>(menu->propertyAddress(Name("clientTextures"), i));
            objects::ensureLoaded(tile);
            tiles.push_back(cast<UTexture>(tile));
            height = std::max(height, (i / cols) * 256 + static_cast<int>(tiles.back()->height()));
        }
        int width = cols * 256;
        std::vector<uint8_t> pixels(static_cast<size_t>(width * height) * 4);
        for (int i = 0; i < cols * rows; i++) {
            UTexture* tile = tiles[static_cast<size_t>(i)];
            std::vector<uint8_t> rgba = tile->rgba(0);
            int tileWidth = static_cast<int>(tile->width());
            for (int y = 0; y < static_cast<int>(tile->height()); y++) {
                for (int x = 0; x < tileWidth; x++) {
                    size_t from = static_cast<size_t>(y * tileWidth + x);
                    uint8_t* to = &pixels[(static_cast<size_t>((i / cols) * 256 + y) * static_cast<size_t>(width) + static_cast<size_t>((i % cols) * 256 + x)) * 4];
                    if (tile->mips[0].data[from] != 0) {
                        to[0] = rgba[from * 4 + 2];
                        to[1] = rgba[from * 4 + 1];
                        to[2] = rgba[from * 4 + 0];
                        to[3] = 255;
                    }
                }
            }
        }
        std::vector<int> sourceRows;
        for (int y = 0; y < cut; y++) {
            sourceRows.push_back(y);
        }
        for (int y = resume; y < height; y++) {
            sourceRows.push_back(y);
        }
        int newHeight = static_cast<int>(sourceRows.size());
        int newRows = (newHeight + 255) / 256;
        UClass* textureClass = objects::findClass("Engine.Texture");
        for (int i = 0; i < 6; i++) {
            *reinterpret_cast<UObject**>(menu->propertyAddress(Name("clientTextures"), i)) = nullptr;
        }
        for (int row = 0; row < newRows; row++) {
            for (int col = 0; col < cols; col++) {
                UTexture* source = tiles[static_cast<size_t>(col)];
                UTexture* texture = cast<UTexture>(objects::newObject(textureClass, source->outer, Name(), RF_Public | RF_Transient));
                *texture->propertyAddress(Name("Format")) = 5;
                *reinterpret_cast<uint32_t*>(texture->propertyAddress(Name("bNoSmooth"))) = *reinterpret_cast<uint32_t*>(source->propertyAddress(Name("bNoSmooth")));
                TextureMip mip;
                mip.width = source->width();
                mip.height = static_cast<uint32_t>(std::min(256, newHeight - row * 256));
                for (uint32_t y = 0; y < mip.height; y++) {
                    size_t from = (static_cast<size_t>(sourceRows[static_cast<size_t>(row * 256) + y]) * static_cast<size_t>(width) + static_cast<size_t>(col * 256)) * 4;
                    mip.data.insert(mip.data.end(), pixels.begin() + static_cast<std::ptrdiff_t>(from), pixels.begin() + static_cast<std::ptrdiff_t>(from + mip.width * 4));
                }
                texture->mips.push_back(std::move(mip));
                *reinterpret_cast<UObject**>(menu->propertyAddress(Name("clientTextures"), row * cols + col)) = texture;
            }
        }
        *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("textureRows"))) = newRows;
    }

    void insertMenuButton(UObject* menu, int index, const std::string& label, UClass* invoke)
    {
        UStruct* button = menu->cls->findProperty(Name("buttonDefaults"))->structType;
        size_t yOffset = button->findProperty(Name("Y"))->offset;
        size_t invokeOffset = button->findProperty(Name("Invoke"))->offset;
        bool names = menu->cls->findProperty(Name("ButtonNames")) != nullptr;
        for (int i = 9; i > index; i--) {
            button->copyValue(menu->propertyAddress(Name("buttonDefaults"), i), menu->propertyAddress(Name("buttonDefaults"), i - 1));
            if (names) {
                *reinterpret_cast<std::string*>(menu->propertyAddress(Name("ButtonNames"), i)) = *reinterpret_cast<std::string*>(menu->propertyAddress(Name("ButtonNames"), i - 1));
            }
        }
        button->copyValue(menu->propertyAddress(Name("buttonDefaults"), index), menu->propertyAddress(Name("buttonDefaults"), index - 1));
        if (names) {
            *reinterpret_cast<std::string*>(menu->propertyAddress(Name("ButtonNames"), index)) = label;
        } else {
            *reinterpret_cast<std::string*>(menu->propertyAddress(Name("buttonDefaults"), index) + button->findProperty(Name("Text"))->offset) = label;
        }
        *reinterpret_cast<UObject**>(menu->propertyAddress(Name("buttonDefaults"), index) + invokeOffset) = invoke;
        int gap = *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("buttonDefaults"), 1) + yOffset) - *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("buttonDefaults"), 0) + yOffset);
        for (int i = index; i < 10; i++) {
            *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("buttonDefaults"), i) + yOffset) += gap;
        }
        int slot = *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("buttonDefaults"), index) + yOffset);
        spliceMenuBackground(menu, slot, slot - gap);
        *reinterpret_cast<int32_t*>(menu->propertyAddress(Name("ClientHeight"))) += gap;
    }
}
