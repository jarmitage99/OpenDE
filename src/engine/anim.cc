#include "engine/anim.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/latent.h"
#include "engine/mesh.h"
#include "engine/world.h"

#include <algorithm>
#include <unordered_map>

namespace dx {
    bool findAnim(UObject* mesh, Name sequence, AnimInfo& info)
    {
        UMesh* object = cast<UMesh>(mesh);
        if (!object) {
            return false;
        }
        objects::ensureLoaded(object);
        const AnimSequence* found = object->findSequence(sequence);
        if (!found || found->numFrames <= 0) {
            return false;
        }
        info.group = found->group;
        info.numFrames = found->numFrames;
        info.rate = found->rate;
        info.notifys.clear();
        for (const AnimNotify& notify : found->notifys) {
            info.notifys.emplace_back(notify.time, notify.function);
        }
        return true;
    }

    namespace {
        bool isAnimating(UObject* a)
        {
            if (actor::AnimSequence(a).none()) {
                return false;
            }
            if (actor::AnimFrame(a) >= 0) {
                return actor::AnimRate(a) != 0;
            }
            return actor::TweenRate(a) != 0;
        }

        void startAnim(UObject* a, Name sequence, float playRate, float tweenTime, bool loop, float minRate)
        {
            UObject* mesh = actor::Mesh(a);
            AnimInfo info;
            if (!mesh || !findAnim(mesh, sequence, info)) {
                return;
            }
            float frames = static_cast<float>(info.numFrames);
            if (loop && actor::AnimSequence(a) == sequence && actor::bAnimLoop(a) && isAnimating(a)) {
                actor::AnimRate(a) = playRate * info.rate / frames;
                actor::bAnimFinished(a) = 0;
                actor::AnimMinRate(a) = minRate != 0 ? minRate * (info.rate / frames) : 0;
                actor::OldAnimRate(a) = actor::AnimRate(a);
                return;
            }
            if (actor::AnimSequence(a).none()) {
                tweenTime = 0;
            }
            actor::AnimSequence(a) = sequence;
            actor::AnimRate(a) = playRate * info.rate / frames;
            actor::AnimLast(a) = 1.0f - 1.0f / frames;
            actor::AnimMinRate(a) = loop && minRate != 0 ? minRate * (info.rate / frames) : 0;
            actor::bAnimNotify(a) = info.notifys.empty() ? 0 : 1;
            actor::bAnimFinished(a) = 0;
            actor::bAnimLoop(a) = loop ? 1 : 0;
            if (actor::AnimLast(a) == 0) {
                actor::AnimMinRate(a) = 0;
                actor::bAnimNotify(a) = 0;
                actor::OldAnimRate(a) = 0;
                actor::TweenRate(a) = tweenTime > 0 ? 1.0f / tweenTime : 10.0f;
                actor::AnimFrame(a) = -1.0f / frames;
                actor::AnimRate(a) = 0;
            } else if (tweenTime > 0) {
                actor::TweenRate(a) = 1.0f / (tweenTime * frames);
                actor::AnimFrame(a) = -1.0f / frames;
            } else if (tweenTime == -1.0f) {
                actor::AnimFrame(a) = -1.0f / frames;
                float old = actor::OldAnimRate(a);
                if (old > 0) {
                    actor::TweenRate(a) = old;
                } else if (old < 0) {
                    actor::TweenRate(a) = std::max(0.5f * actor::AnimRate(a), -actor::Velocity(a).size() * old);
                } else {
                    actor::TweenRate(a) = 1.0f / (0.025f * frames);
                }
            } else {
                actor::TweenRate(a) = 0;
                actor::AnimFrame(a) = loop ? 0.0001f : 0.001f;
            }
            actor::OldAnimRate(a) = actor::AnimRate(a);
        }

        void nativePlayAnim(UObject* self, Frame& stack, void*)
        {
            Name sequence = vm::get<Name>(stack);
            float rate = vm::getOpt<float>(stack, 1.0f);
            float tween = vm::getOpt<float>(stack, -1.0f);
            vm::finish(stack);
            startAnim(self, sequence, rate, tween, false, 0);
        }

        void nativeLoopAnim(UObject* self, Frame& stack, void*)
        {
            Name sequence = vm::get<Name>(stack);
            float rate = vm::getOpt<float>(stack, 1.0f);
            float tween = vm::getOpt<float>(stack, -1.0f);
            float minRate = vm::getOpt<float>(stack, 0.0f);
            vm::finish(stack);
            startAnim(self, sequence, rate, tween, true, minRate);
        }

        void nativeTweenAnim(UObject* self, Frame& stack, void*)
        {
            Name sequence = vm::get<Name>(stack);
            float tween = vm::get<float>(stack);
            vm::finish(stack);
            AnimInfo info;
            UObject* mesh = actor::Mesh(self);
            if (!mesh || !findAnim(mesh, sequence, info)) {
                return;
            }
            float frames = static_cast<float>(info.numFrames);
            actor::AnimSequence(self) = sequence;
            actor::AnimLast(self) = 0;
            actor::AnimMinRate(self) = 0;
            actor::bAnimNotify(self) = 0;
            actor::bAnimFinished(self) = 0;
            actor::bAnimLoop(self) = 0;
            actor::AnimRate(self) = 0;
            actor::OldAnimRate(self) = 0;
            if (tween > 0) {
                actor::TweenRate(self) = 1.0f / (tween * frames);
                actor::AnimFrame(self) = -1.0f / frames;
            } else {
                actor::TweenRate(self) = 0;
                actor::AnimFrame(self) = 0;
            }
        }

        void nativeFinishAnim(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (actor::bAnimLoop(self)) {
                actor::bAnimLoop(self) = 0;
                actor::bAnimFinished(self) = 0;
            }
            if (isAnimating(self) && actor::AnimFrame(self) < actor::AnimLast(self)) {
                vm::stateFrame(self).latentAction = latent::FinishAnim;
            }
        }

        void nativeIsAnimating(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::retBool(result, isAnimating(self));
        }

        void nativeGetAnimGroup(UObject* self, Frame& stack, void* result)
        {
            Name sequence = vm::get<Name>(stack);
            vm::finish(stack);
            AnimInfo info;
            Name group;
            if (actor::Mesh(self) && findAnim(actor::Mesh(self), sequence, info)) {
                group = info.group;
            }
            vm::ret<Name>(result, group);
        }

        void nativeHasAnim(UObject* self, Frame& stack, void* result)
        {
            Name sequence = vm::get<Name>(stack);
            vm::finish(stack);
            AnimInfo info;
            vm::retBool(result, actor::Mesh(self) && findAnim(actor::Mesh(self), sequence, info));
        }

        void nativePlayBlendAnim(UObject* self, Frame& stack, void*)
        {
            Name sequence = vm::get<Name>(stack);
            float rate = vm::getOpt<float>(stack, 1.0f);
            float tween = vm::getOpt<float>(stack, -1.0f);
            int32_t slot = vm::getOpt<int32_t>(stack, 0);
            vm::finish(stack);
            AnimInfo info;
            if (slot < 0 || slot > 3 || !actor::Mesh(self) || !findAnim(actor::Mesh(self), sequence, info)) {
                return;
            }
            float frames = static_cast<float>(info.numFrames);
            actor::BlendAnimSequence(self, slot) = sequence;
            actor::BlendAnimRate(self, slot) = rate * info.rate / frames;
            actor::BlendAnimLast(self, slot) = 1.0f - 1.0f / frames;
            if (actor::BlendAnimLast(self, slot) == 0) {
                actor::BlendAnimMinRate(self, slot) = 0;
                actor::OldBlendAnimRate(self, slot) = 0;
                actor::BlendTweenRate(self, slot) = tween > 0 ? 1.0f / tween : 10.0f;
                actor::BlendAnimFrame(self, slot) = -1.0f / frames;
                actor::BlendAnimRate(self, slot) = 0;
            } else if (tween > 0) {
                actor::BlendTweenRate(self, slot) = 1.0f / (tween * frames);
                actor::BlendAnimFrame(self, slot) = -1.0f / frames;
            } else if (tween == -1.0f) {
                actor::BlendAnimFrame(self, slot) = -1.0f / frames;
                float old = actor::OldBlendAnimRate(self, slot);
                if (old > 0) {
                    actor::BlendTweenRate(self, slot) = old;
                } else {
                    actor::BlendTweenRate(self, slot) = 1.0f / (0.025f * frames);
                }
            } else {
                actor::BlendTweenRate(self, slot) = 0;
                actor::BlendAnimFrame(self, slot) = 0.001f;
            }
            actor::OldBlendAnimRate(self, slot) = actor::BlendAnimRate(self, slot);
        }

        void nativeTweenBlendAnim(UObject* self, Frame& stack, void*)
        {
            Name sequence = vm::get<Name>(stack);
            float tween = vm::get<float>(stack);
            int32_t slot = vm::getOpt<int32_t>(stack, 0);
            vm::finish(stack);
            AnimInfo info;
            if (slot < 0 || slot > 3 || !actor::Mesh(self) || !findAnim(actor::Mesh(self), sequence, info)) {
                return;
            }
            float frames = static_cast<float>(info.numFrames);
            actor::BlendAnimSequence(self, slot) = sequence;
            actor::BlendAnimLast(self, slot) = 0;
            actor::BlendAnimMinRate(self, slot) = 0;
            actor::BlendAnimRate(self, slot) = 0;
            actor::OldBlendAnimRate(self, slot) = 0;
            if (tween > 0) {
                actor::BlendTweenRate(self, slot) = 1.0f / (tween * frames);
                actor::BlendAnimFrame(self, slot) = -1.0f / frames;
            } else {
                actor::BlendTweenRate(self, slot) = 0;
                actor::BlendAnimFrame(self, slot) = 0;
            }
        }

        void updateBlendAnimation(UObject* a, float deltaSeconds)
        {
            for (int slot = 0; slot < 4; slot++) {
                if (actor::BlendAnimSequence(a, slot).none()) {
                    continue;
                }
                float& frame = actor::BlendAnimFrame(a, slot);
                if (frame < 0) {
                    frame += actor::BlendTweenRate(a, slot) * deltaSeconds;
                    if (frame >= 0) {
                        frame = 0;
                    }
                    continue;
                }
                float rate = actor::BlendAnimRate(a, slot);
                float last = actor::BlendAnimLast(a, slot);
                if (rate == 0 || frame >= last) {
                    continue;
                }
                if (rate > 0) {
                    frame += rate * deltaSeconds;
                } else {
                    frame += std::max(actor::BlendAnimMinRate(a, slot), actor::Velocity(a).size() * -rate) * deltaSeconds;
                }
                if (frame >= last) {
                    frame = last;
                    actor::BlendAnimRate(a, slot) = 0;
                }
            }
        }

        void nativeSleep(UObject* self, Frame& stack, void*)
        {
            float seconds = vm::get<float>(stack);
            vm::finish(stack);
            StateFrame& frame = vm::stateFrame(self);
            frame.sleepTime = seconds;
            actor::LatentFloat(self) = seconds;
            frame.latentAction = latent::Sleep;
        }

        void nativeFinishInterpolation(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            vm::stateFrame(self).latentAction = latent::FinishInterpolation;
        }

        void pollSleep(UObject* self, StateFrame& frame, float deltaSeconds)
        {
            float& remaining = actor::LatentFloat(self);
            remaining -= deltaSeconds;
            if (remaining < 0.5f * deltaSeconds) {
                frame.latentAction = 0;
            }
        }

        void pollFinishAnim(UObject* self, StateFrame& frame, float)
        {
            if (actor::bAnimFinished(self)) {
                frame.latentAction = 0;
            }
        }

        void pollFinishInterpolation(UObject* self, StateFrame& frame, float)
        {
            if (!actor::bInterpolating(self)) {
                frame.latentAction = 0;
            }
        }

        std::unordered_map<int32_t, LatentPoll>& polls()
        {
            static std::unordered_map<int32_t, LatentPoll> table;
            return table;
        }
    }

    namespace latent {
        void registerPoll(int32_t action, LatentPoll pollFunction)
        {
            polls()[action] = pollFunction;
        }

        void poll(UObject* self, float deltaSeconds)
        {
            if (!self->stateFrame || self->stateFrame->latentAction == 0) {
                return;
            }
            auto it = polls().find(self->stateFrame->latentAction);
            if (it == polls().end()) {
                self->stateFrame->latentAction = 0;
                return;
            }
            it->second(self, *self->stateFrame, deltaSeconds);
        }
    }

    void updateAnimation(UObject* a, float deltaSeconds)
    {
        updateBlendAnimation(a, deltaSeconds);
        int iterations = 0;
        float seconds = deltaSeconds;
        while (isAnimating(a) && seconds > 0 && ++iterations <= 4) {
            float& frame = actor::AnimFrame(a);
            float oldFrame = frame;
            if (frame >= 0) {
                float rate = actor::AnimRate(a);
                if (rate >= 0) {
                    frame += rate * seconds;
                } else {
                    frame += std::max(actor::AnimMinRate(a), actor::Velocity(a).size() * -rate) * seconds;
                }
                if (actor::bAnimNotify(a) && actor::Mesh(a)) {
                    AnimInfo info;
                    if (findAnim(actor::Mesh(a), actor::AnimSequence(a), info)) {
                        float bestElapsed = 100000.0f;
                        const std::pair<float, Name>* best = nullptr;
                        for (const auto& notify : info.notifys) {
                            if (oldFrame < notify.first && frame >= notify.first) {
                                float elapsed = notify.first - oldFrame;
                                if (!best || elapsed < bestElapsed) {
                                    bestElapsed = elapsed;
                                    best = &notify;
                                }
                            }
                        }
                        if (best) {
                            seconds = seconds * (frame - best->first) / (frame - oldFrame);
                            frame = best->first;
                            UFunction* function = vm::findFunction(a, best->second);
                            if (function) {
                                vm::Call(a, function).run();
                            }
                            if (a->destroyed) {
                                return;
                            }
                            continue;
                        }
                    }
                }
                float last = actor::AnimLast(a);
                if (frame < last) {
                    break;
                }
                if (actor::bAnimLoop(a)) {
                    if (frame < 1.0f) {
                        seconds = 0;
                    } else {
                        seconds = seconds * (frame - 1.0f) / (frame - oldFrame);
                        frame = 0;
                    }
                    if (oldFrame < last) {
                        if (a->stateFrame && a->stateFrame->latentAction == latent::FinishAnim) {
                            actor::bAnimFinished(a) = 1;
                        }
                        vm::event(a, Name("AnimEnd"));
                    }
                } else {
                    seconds = seconds * (frame - last) / (frame - oldFrame);
                    frame = last;
                    actor::bAnimFinished(a) = 1;
                    actor::AnimRate(a) = 0;
                    vm::event(a, Name("AnimEnd"));
                }
            } else {
                frame += actor::TweenRate(a) * seconds;
                if (frame >= 0) {
                    seconds = seconds * frame / (frame - oldFrame);
                    frame = 0;
                    if (actor::AnimRate(a) == 0) {
                        actor::bAnimFinished(a) = 1;
                        vm::event(a, Name("AnimEnd"));
                    }
                } else {
                    break;
                }
            }
            if (a->destroyed) {
                return;
            }
        }
    }

    void registerAnimNatives()
    {
        vm::registerNative(259, nativePlayAnim);
        vm::registerNative(260, nativeLoopAnim);
        vm::registerNative(294, nativeTweenAnim);
        vm::registerNative(261, nativeFinishAnim);
        vm::registerNative(282, nativeIsAnimating);
        vm::registerNative(293, nativeGetAnimGroup);
        vm::registerNative(263, nativeHasAnim);
        vm::registerNative(1010, nativePlayBlendAnim);
        vm::registerNative(1012, nativeTweenBlendAnim);
        vm::registerNative(256, nativeSleep);
        vm::registerNative(301, nativeFinishInterpolation);
        latent::registerPoll(latent::Sleep, pollSleep);
        latent::registerPoll(latent::FinishAnim, pollFinishAnim);
        latent::registerPoll(latent::FinishInterpolation, pollFinishInterpolation);
    }
}
