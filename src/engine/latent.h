#pragma once

#include "core/vm.h"

#include <cstdint>

namespace dx {
    using LatentPoll = void (*)(UObject* self, StateFrame& frame, float deltaSeconds);

    namespace latent {
        constexpr int32_t Sleep = 384;
        constexpr int32_t FinishAnim = 385;
        constexpr int32_t FinishInterpolation = 302;
        constexpr int32_t MoveTo = 501;
        constexpr int32_t MoveToward = 503;
        constexpr int32_t StrafeTo = 505;
        constexpr int32_t StrafeFacing = 507;
        constexpr int32_t TurnTo = 509;
        constexpr int32_t TurnToward = 511;
        constexpr int32_t WaitForLanding = 528;

        void registerPoll(int32_t action, LatentPoll poll);
        void poll(UObject* self, float deltaSeconds);
    }
}
