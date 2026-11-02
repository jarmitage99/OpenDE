#pragma once

#include "engine/mesh.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace dx::compiler {
    template <typename T, typename Compare>
    void msvcQsort(T* base, size_t count, Compare compare)
    {
        if (count < 2) {
            return;
        }
        constexpr size_t Cutoff = 8;
        T* loStack[64];
        T* hiStack[64];
        int stackPointer = 0;
        T* lo = base;
        T* hi = base + (count - 1);
        while (true) {
            size_t size = static_cast<size_t>(hi - lo) + 1;
            if (size <= Cutoff) {
                T* top = hi;
                while (top > lo) {
                    T* max = lo;
                    for (T* p = lo + 1; p <= top; p++) {
                        if (compare(*p, *max) > 0) {
                            max = p;
                        }
                    }
                    std::swap(*max, *top);
                    top--;
                }
            } else {
                T* mid = lo + (size / 2);
                std::swap(*mid, *lo);
                T* loGuy = lo;
                T* hiGuy = hi + 1;
                while (true) {
                    do {
                        loGuy++;
                    } while (loGuy <= hi && compare(*loGuy, *lo) <= 0);
                    do {
                        hiGuy--;
                    } while (hiGuy > lo && compare(*hiGuy, *lo) >= 0);
                    if (hiGuy < loGuy) {
                        break;
                    }
                    std::swap(*loGuy, *hiGuy);
                }
                std::swap(*lo, *hiGuy);
                if (hiGuy - 1 - lo >= hi - loGuy) {
                    if (lo + 1 < hiGuy) {
                        loStack[stackPointer] = lo;
                        hiStack[stackPointer] = hiGuy - 1;
                        stackPointer++;
                    }
                    if (loGuy < hi) {
                        lo = loGuy;
                        continue;
                    }
                } else {
                    if (loGuy < hi) {
                        loStack[stackPointer] = loGuy;
                        hiStack[stackPointer] = hi;
                        stackPointer++;
                    }
                    if (lo + 1 < hiGuy) {
                        hi = hiGuy - 1;
                        continue;
                    }
                }
            }
            stackPointer--;
            if (stackPointer < 0) {
                return;
            }
            lo = loStack[stackPointer];
            hi = hiStack[stackPointer];
        }
    }

    struct MeshVert {
        int16_t x = 0;
        int16_t y = 0;
        int16_t z = 0;
        int16_t pad = 0;
    };

    struct LodSettings {
        int32_t style = 0;
        int32_t sampleFrame = 0;
        bool noUVData = false;
    };

    struct LodProcessed {
        std::vector<MeshTri> tris;
        std::vector<MeshVert> verts;
        int32_t frameVerts = 0;
        std::vector<uint16_t> collapsePointThus;
        std::vector<uint16_t> faceLevel;
        std::vector<MeshFace> faces;
        std::vector<uint16_t> collapseWedgeThus;
        std::vector<MeshWedge> wedges;
        std::vector<MeshMaterial> materials;
        std::vector<MeshFace> specialFaces;
        int32_t modelVerts = 0;
        int32_t specialVerts = 0;
    };

    struct LodTrace {
        const std::vector<int32_t>* forcedOrder = nullptr;
        std::vector<std::pair<int32_t, int32_t>> precedes;
        int32_t mismatches = 0;
        int32_t staleTargets = 0;
    };

    LodProcessed meshLodProcess(std::vector<MeshTri> tris, const std::vector<MeshVert>& verts, int32_t frameVerts, int32_t animFrames, const LodSettings& settings, LodTrace* trace);
}
