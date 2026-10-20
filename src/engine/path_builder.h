#pragma once

#include "engine/resources.h"

namespace dx {
    struct CheckResult;

    int pawnPointReachable(UObject* pawn, Vector point, bool knowVisible);
    bool pawnFindPathToward(UObject* pawn, UObject* goal, bool singlePath, UObject*& bestPath, bool clear);
    int pawnWalkMove(UObject* pawn, Vector delta, CheckResult& hit, UObject* goal, float threshold, bool adjust);
    int pawnWalkReachable(UObject* pawn, const Vector& destination, float threshold, int reachFlags, UObject* goal);

    namespace paths {
        int buildPaths(ULevel* level, int optimization);
        int removePaths(ULevel* level);
        int showPaths(ULevel* level);
        int hidePaths(ULevel* level);
        void definePaths(ULevel* level);
        void undefinePaths(ULevel* level);
    }
}
