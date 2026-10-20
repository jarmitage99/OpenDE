#pragma once

#include "core/object.h"

#include <functional>
#include <string>

namespace dx {
    class World;

    namespace deusex {
        void levelLoaded(World& world);
        bool scriptedPawnTick(World& world, UObject* pawn, float deltaSeconds);
        bool renderIterate(World& world, UObject* owner, const std::function<void(UObject* item)>& emit);
        void spliceMenuBackground(UObject* menu, int cut, int resume);
        void insertMenuButton(UObject* menu, int index, const std::string& label, UClass* invoke);
    }
}
