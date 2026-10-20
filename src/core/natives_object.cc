#include "core/natives.h"

#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>

namespace dx {
    namespace {
        using vm::finish;
        using vm::get;
        using vm::getBool;
        using vm::getRef;
        using vm::registerNative;
        using vm::ret;
        using vm::retBool;

        std::mt19937& randomEngine()
        {
            static std::mt19937 engine(12345);
            return engine;
        }

        float frand()
        {
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(randomEngine());
        }

        int32_t toInt(float value)
        {
            return static_cast<int32_t>(value);
        }

        Rotator scaleRotator(const Rotator& r, float s)
        {
            return { toInt(static_cast<float>(r.pitch) * s), toInt(static_cast<float>(r.yaw) * s), toInt(static_cast<float>(r.roll) * s) };
        }

        int32_t normalizeAxis(int32_t value)
        {
            value &= 0xffff;
            if (value > 32767) {
                value -= 0x10000;
            }
            return value;
        }

        std::string substring(const std::string& s, int32_t start, int32_t count)
        {
            if (start < 0) {
                count += start;
                start = 0;
            }
            if (count <= 0 || start >= static_cast<int32_t>(s.size())) {
                return {};
            }
            return s.substr(static_cast<size_t>(start), static_cast<size_t>(count));
        }

        bool isInState(UObject* object, Name stateName)
        {
            if (!object->stateFrame) {
                return false;
            }
            for (UState* state = object->stateFrame->state; state; state = cast<UState>(state->superField)) {
                if (state->name == stateName) {
                    return true;
                }
            }
            return false;
        }

        void registerIntNatives()
        {
            registerNative(146, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b)));
            });
            registerNative(147, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)));
            });
            registerNative(144, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b)));
            });
            registerNative(145, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                if (b == 0) {
                    vm::warn(s, "divide by zero");
                    ret<int32_t>(r, 0);
                    return;
                }
                ret<int32_t>(r, a / b);
            });
            registerNative(148, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(static_cast<uint32_t>(a) << (b & 31)));
            });
            registerNative(149, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, a >> (b & 31));
            });
            registerNative(196, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(static_cast<uint32_t>(a) >> (b & 31)));
            });
            registerNative(150, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a < b);
            });
            registerNative(151, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a > b);
            });
            registerNative(152, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a <= b);
            });
            registerNative(153, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a >= b);
            });
            registerNative(154, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(155, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                retBool(r, a != b);
            });
            registerNative(156, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, a & b);
            });
            registerNative(157, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, a ^ b);
            });
            registerNative(158, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, a | b);
            });
            registerNative(159, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                float b = get<float>(s);
                finish(s);
                *a = toInt(static_cast<float>(*a) * b);
                ret<int32_t>(r, *a);
            });
            registerNative(160, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                float b = get<float>(s);
                finish(s);
                if (b != 0) {
                    *a = toInt(static_cast<float>(*a) / b);
                }
                ret<int32_t>(r, *a);
            });
            registerNative(161, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                *a += b;
                ret<int32_t>(r, *a);
            });
            registerNative(162, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                *a -= b;
                ret<int32_t>(r, *a);
            });
            registerNative(163, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                finish(s);
                ++*a;
                ret<int32_t>(r, *a);
            });
            registerNative(164, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                finish(s);
                --*a;
                ret<int32_t>(r, *a);
            });
            registerNative(165, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                finish(s);
                ret<int32_t>(r, (*a)++);
            });
            registerNative(166, [](UObject*, Frame& s, void* r) {
                int32_t* a = getRef<int32_t>(s);
                finish(s);
                ret<int32_t>(r, (*a)--);
            });
            registerNative(143, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, -a);
            });
            registerNative(141, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, ~a);
            });
            registerNative(167, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                finish(s);
                int32_t value = 0;
                if (a > 0) {
                    value = static_cast<int32_t>(randomEngine()() % static_cast<uint32_t>(a));
                }
                ret<int32_t>(r, value);
            });
            registerNative(249, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, std::min(a, b));
            });
            registerNative(250, [](UObject*, Frame& s, void* r) {
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, std::max(a, b));
            });
            registerNative(251, [](UObject*, Frame& s, void* r) {
                int32_t v = get<int32_t>(s);
                int32_t a = get<int32_t>(s);
                int32_t b = get<int32_t>(s);
                finish(s);
                ret<int32_t>(r, v < a ? a : (v > b ? b : v));
            });
        }

        void registerByteBoolNatives()
        {
            registerNative(133, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                uint8_t b = get<uint8_t>(s);
                finish(s);
                *a = static_cast<uint8_t>(*a * b);
                ret<uint8_t>(r, *a);
            });
            registerNative(134, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                uint8_t b = get<uint8_t>(s);
                finish(s);
                if (b != 0) {
                    *a = static_cast<uint8_t>(*a / b);
                }
                ret<uint8_t>(r, *a);
            });
            registerNative(135, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                uint8_t b = get<uint8_t>(s);
                finish(s);
                *a = static_cast<uint8_t>(*a + b);
                ret<uint8_t>(r, *a);
            });
            registerNative(136, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                uint8_t b = get<uint8_t>(s);
                finish(s);
                *a = static_cast<uint8_t>(*a - b);
                ret<uint8_t>(r, *a);
            });
            registerNative(137, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                finish(s);
                *a = static_cast<uint8_t>(*a + 1);
                ret<uint8_t>(r, *a);
            });
            registerNative(138, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                finish(s);
                *a = static_cast<uint8_t>(*a - 1);
                ret<uint8_t>(r, *a);
            });
            registerNative(139, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                finish(s);
                uint8_t old = *a;
                *a = static_cast<uint8_t>(old + 1);
                ret<uint8_t>(r, old);
            });
            registerNative(140, [](UObject*, Frame& s, void* r) {
                uint8_t* a = getRef<uint8_t>(s);
                finish(s);
                uint8_t old = *a;
                *a = static_cast<uint8_t>(old - 1);
                ret<uint8_t>(r, old);
            });
            registerNative(129, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                finish(s);
                retBool(r, !a);
            });
            registerNative(130, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                uint16_t skip = vm::getSkip(s);
                if (!a && skip != 0) {
                    s.code += skip;
                    retBool(r, false);
                    return;
                }
                bool b = getBool(s);
                finish(s);
                retBool(r, a && b);
            });
            registerNative(131, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                bool b = getBool(s);
                finish(s);
                retBool(r, a != b);
            });
            registerNative(132, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                uint16_t skip = vm::getSkip(s);
                if (a && skip != 0) {
                    s.code += skip;
                    retBool(r, true);
                    return;
                }
                bool b = getBool(s);
                finish(s);
                retBool(r, a || b);
            });
            registerNative(242, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                bool b = getBool(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(243, [](UObject*, Frame& s, void* r) {
                bool a = getBool(s);
                bool b = getBool(s);
                finish(s);
                retBool(r, a != b);
            });
        }

        void registerFloatNatives()
        {
            registerNative(169, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, -a);
            });
            registerNative(170, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, std::pow(a, b));
            });
            registerNative(171, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a * b);
            });
            registerNative(172, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a / b);
            });
            registerNative(173, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, b != 0 ? std::fmod(a, b) : 0.0f);
            });
            registerNative(174, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a + b);
            });
            registerNative(175, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a - b);
            });
            registerNative(176, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a < b);
            });
            registerNative(177, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a > b);
            });
            registerNative(178, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a <= b);
            });
            registerNative(179, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a >= b);
            });
            registerNative(180, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(181, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, a != b);
            });
            registerNative(210, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                retBool(r, std::fabs(a - b) < 0.0001f);
            });
            registerNative(182, [](UObject*, Frame& s, void* r) {
                float* a = getRef<float>(s);
                float b = get<float>(s);
                finish(s);
                *a *= b;
                ret<float>(r, *a);
            });
            registerNative(183, [](UObject*, Frame& s, void* r) {
                float* a = getRef<float>(s);
                float b = get<float>(s);
                finish(s);
                *a /= b;
                ret<float>(r, *a);
            });
            registerNative(184, [](UObject*, Frame& s, void* r) {
                float* a = getRef<float>(s);
                float b = get<float>(s);
                finish(s);
                *a += b;
                ret<float>(r, *a);
            });
            registerNative(185, [](UObject*, Frame& s, void* r) {
                float* a = getRef<float>(s);
                float b = get<float>(s);
                finish(s);
                *a -= b;
                ret<float>(r, *a);
            });
            registerNative(186, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::fabs(a));
            });
            registerNative(187, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::sin(a));
            });
            registerNative(188, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::cos(a));
            });
            registerNative(189, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::tan(a));
            });
            registerNative(190, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::atan(a));
            });
            registerNative(191, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::exp(a));
            });
            registerNative(192, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, std::log(a));
            });
            registerNative(193, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, a > 0 ? std::sqrt(a) : 0.0f);
            });
            registerNative(194, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                finish(s);
                ret<float>(r, a * a);
            });
            registerNative(195, [](UObject*, Frame& s, void* r) {
                finish(s);
                ret<float>(r, frand());
            });
            registerNative(244, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, std::min(a, b));
            });
            registerNative(245, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, std::max(a, b));
            });
            registerNative(246, [](UObject*, Frame& s, void* r) {
                float v = get<float>(s);
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, v < a ? a : (v > b ? b : v));
            });
            registerNative(247, [](UObject*, Frame& s, void* r) {
                float alpha = get<float>(s);
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a + alpha * (b - a));
            });
            registerNative(248, [](UObject*, Frame& s, void* r) {
                float alpha = get<float>(s);
                float a = get<float>(s);
                float b = get<float>(s);
                finish(s);
                ret<float>(r, a + (3.0f * alpha * alpha - 2.0f * alpha * alpha * alpha) * (b - a));
            });
        }

        void registerVectorNatives()
        {
            registerNative(211, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                finish(s);
                ret<Vector>(r, -a);
            });
            registerNative(212, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                float b = get<float>(s);
                finish(s);
                ret<Vector>(r, a * b);
            });
            registerNative(213, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<Vector>(r, b * a);
            });
            registerNative(296, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<Vector>(r, a * b);
            });
            registerNative(214, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                float b = get<float>(s);
                finish(s);
                ret<Vector>(r, a / b);
            });
            registerNative(215, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<Vector>(r, a + b);
            });
            registerNative(216, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<Vector>(r, a - b);
            });
            registerNative(275, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                ret<Vector>(r, transformVector(rotationAxes(b), a));
            });
            registerNative(276, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                ret<Vector>(r, untransformVector(rotationAxes(b), a));
            });
            registerNative(217, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(218, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                retBool(r, !(a == b));
            });
            registerNative(219, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<float>(r, a.dot(b));
            });
            registerNative(220, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                ret<Vector>(r, a.cross(b));
            });
            registerNative(221, [](UObject*, Frame& s, void* r) {
                Vector* a = getRef<Vector>(s);
                float b = get<float>(s);
                finish(s);
                *a = *a * b;
                ret<Vector>(r, *a);
            });
            registerNative(297, [](UObject*, Frame& s, void* r) {
                Vector* a = getRef<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                *a = *a * b;
                ret<Vector>(r, *a);
            });
            registerNative(222, [](UObject*, Frame& s, void* r) {
                Vector* a = getRef<Vector>(s);
                float b = get<float>(s);
                finish(s);
                *a = *a / b;
                ret<Vector>(r, *a);
            });
            registerNative(223, [](UObject*, Frame& s, void* r) {
                Vector* a = getRef<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                *a = *a + b;
                ret<Vector>(r, *a);
            });
            registerNative(224, [](UObject*, Frame& s, void* r) {
                Vector* a = getRef<Vector>(s);
                Vector b = get<Vector>(s);
                finish(s);
                *a = *a - b;
                ret<Vector>(r, *a);
            });
            registerNative(225, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                finish(s);
                ret<float>(r, a.size());
            });
            registerNative(226, [](UObject*, Frame& s, void* r) {
                Vector a = get<Vector>(s);
                finish(s);
                ret<Vector>(r, a.normal());
            });
            registerNative(227, [](UObject*, Frame& s, void*) {
                Vector* x = getRef<Vector>(s);
                Vector* y = getRef<Vector>(s);
                Vector* z = getRef<Vector>(s);
                finish(s);
                Vector a = *x;
                Vector b = *y;
                Vector c = *z;
                *x = { a.x, b.x, c.x };
                *y = { a.y, b.y, c.y };
                *z = { a.z, b.z, c.z };
            });
            registerNative(252, [](UObject*, Frame& s, void* r) {
                finish(s);
                Vector v;
                do {
                    v = { frand() * 2.0f - 1.0f, frand() * 2.0f - 1.0f, frand() * 2.0f - 1.0f };
                } while (v.sizeSquared() > 1.0f || v.sizeSquared() < 0.0001f);
                ret<Vector>(r, v.normal());
            });
            registerNative(300, [](UObject*, Frame& s, void* r) {
                Vector v = get<Vector>(s);
                Vector n = get<Vector>(s).normal();
                finish(s);
                ret<Vector>(r, v - n * (2.0f * v.dot(n)));
            });
        }

        void registerRotatorNatives()
        {
            registerNative(142, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(203, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                retBool(r, !(a == b));
            });
            registerNative(287, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                float b = get<float>(s);
                finish(s);
                ret<Rotator>(r, scaleRotator(a, b));
            });
            registerNative(288, [](UObject*, Frame& s, void* r) {
                float a = get<float>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                ret<Rotator>(r, scaleRotator(b, a));
            });
            registerNative(289, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                float b = get<float>(s);
                finish(s);
                ret<Rotator>(r, scaleRotator(a, 1.0f / b));
            });
            registerNative(290, [](UObject*, Frame& s, void* r) {
                Rotator* a = getRef<Rotator>(s);
                float b = get<float>(s);
                finish(s);
                *a = scaleRotator(*a, b);
                ret<Rotator>(r, *a);
            });
            registerNative(291, [](UObject*, Frame& s, void* r) {
                Rotator* a = getRef<Rotator>(s);
                float b = get<float>(s);
                finish(s);
                *a = scaleRotator(*a, 1.0f / b);
                ret<Rotator>(r, *a);
            });
            registerNative(316, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                ret<Rotator>(r, Rotator { a.pitch + b.pitch, a.yaw + b.yaw, a.roll + b.roll });
            });
            registerNative(317, [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                ret<Rotator>(r, Rotator { a.pitch - b.pitch, a.yaw - b.yaw, a.roll - b.roll });
            });
            registerNative(318, [](UObject*, Frame& s, void* r) {
                Rotator* a = getRef<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                *a = Rotator { a->pitch + b.pitch, a->yaw + b.yaw, a->roll + b.roll };
                ret<Rotator>(r, *a);
            });
            registerNative(319, [](UObject*, Frame& s, void* r) {
                Rotator* a = getRef<Rotator>(s);
                Rotator b = get<Rotator>(s);
                finish(s);
                *a = Rotator { a->pitch - b.pitch, a->yaw - b.yaw, a->roll - b.roll };
                ret<Rotator>(r, *a);
            });
            registerNative(229, [](UObject*, Frame& s, void*) {
                Rotator a = get<Rotator>(s);
                Vector* x = getRef<Vector>(s);
                Vector* y = getRef<Vector>(s);
                Vector* z = getRef<Vector>(s);
                finish(s);
                Coords c = rotationAxes(a);
                *x = c.xAxis;
                *y = c.yAxis;
                *z = c.zAxis;
            });
            registerNative(230, [](UObject*, Frame& s, void*) {
                Rotator a = get<Rotator>(s);
                Vector* x = getRef<Vector>(s);
                Vector* y = getRef<Vector>(s);
                Vector* z = getRef<Vector>(s);
                finish(s);
                Coords c = rotationAxes(a);
                *x = { c.xAxis.x, c.yAxis.x, c.zAxis.x };
                *y = { c.xAxis.y, c.yAxis.y, c.zAxis.y };
                *z = { c.xAxis.z, c.yAxis.z, c.zAxis.z };
            });
            registerNative(320, [](UObject*, Frame& s, void* r) {
                bool roll = vm::getBoolOpt(s, false);
                finish(s);
                Rotator value;
                value.pitch = static_cast<int32_t>(randomEngine()() & 0xffff);
                value.yaw = static_cast<int32_t>(randomEngine()() & 0xffff);
                value.roll = roll ? static_cast<int32_t>(randomEngine()() & 0xffff) : 0;
                ret<Rotator>(r, value);
            });
            registerNative("Object", "Normalize", [](UObject*, Frame& s, void* r) {
                Rotator a = get<Rotator>(s);
                finish(s);
                ret<Rotator>(r, Rotator { normalizeAxis(a.pitch), normalizeAxis(a.yaw), normalizeAxis(a.roll) });
            });
            registerNative("Object", "OrthoRotation", [](UObject*, Frame& s, void* r) {
                Vector x = get<Vector>(s);
                Vector y = get<Vector>(s);
                Vector z = get<Vector>(s);
                finish(s);
                Rotator value;
                value.yaw = static_cast<int32_t>(std::atan2(x.y, x.x) * 32768.0f / Pi);
                value.pitch = static_cast<int32_t>(std::atan2(x.z, std::sqrt(x.x * x.x + x.y * x.y)) * 32768.0f / Pi);
                Vector baseY = rotationAxes(Rotator { value.pitch, value.yaw, 0 }).yAxis;
                value.roll = static_cast<int32_t>(std::atan2(z.dot(baseY), y.dot(baseY)) * 32768.0f / Pi);
                ret<Rotator>(r, value);
            });
        }

        void registerStringNatives()
        {
            registerNative(112, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                ret<std::string>(r, a + b);
            });
            registerNative(168, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                ret<std::string>(r, a + " " + b);
            });
            registerNative(115, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a.compare(b) < 0);
            });
            registerNative(116, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a.compare(b) > 0);
            });
            registerNative(120, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a.compare(b) <= 0);
            });
            registerNative(121, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a.compare(b) >= 0);
            });
            registerNative(122, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(123, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, a != b);
            });
            registerNative(124, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string b = get<std::string>(s);
                finish(s);
                retBool(r, iequals(a, b));
            });
            registerNative(125, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                finish(s);
                ret<int32_t>(r, static_cast<int32_t>(a.size()));
            });
            registerNative(126, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                std::string t = get<std::string>(s);
                finish(s);
                size_t pos = a.find(t);
                ret<int32_t>(r, pos == std::string::npos ? -1 : static_cast<int32_t>(pos));
            });
            registerNative(127, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                int32_t i = get<int32_t>(s);
                int32_t j = vm::getOpt<int32_t>(s, 0xffff);
                finish(s);
                uint32_t length = static_cast<uint32_t>(a.size());
                uint32_t start = std::min(static_cast<uint32_t>(i), length);
                uint32_t end = static_cast<uint32_t>(i) + static_cast<uint32_t>(j);
                end = end < start ? start : std::min(end, length);
                ret<std::string>(r, a.substr(start, end - start));
            });
            registerNative(128, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                int32_t i = get<int32_t>(s);
                finish(s);
                ret<std::string>(r, substring(a, 0, i));
            });
            registerNative(234, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                int32_t i = get<int32_t>(s);
                finish(s);
                int32_t length = static_cast<int32_t>(a.size());
                ret<std::string>(r, substring(a, length - i, i));
            });
            registerNative(235, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                finish(s);
                for (char& c : a) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                }
                ret<std::string>(r, a);
            });
            registerNative(236, [](UObject*, Frame& s, void* r) {
                int32_t i = get<int32_t>(s);
                finish(s);
                std::string value;
                uint16_t c = static_cast<uint16_t>(i);
                if (c != 0) {
                    value.push_back(c < 256 ? static_cast<char>(c) : '?');
                }
                ret<std::string>(r, value);
            });
            registerNative(237, [](UObject*, Frame& s, void* r) {
                std::string a = get<std::string>(s);
                finish(s);
                ret<int32_t>(r, a.empty() ? 0 : static_cast<int32_t>(static_cast<uint8_t>(a[0])));
            });
        }

        void registerObjectNamesNatives()
        {
            registerNative(114, [](UObject*, Frame& s, void* r) {
                UObject* a = get<UObject*>(s);
                UObject* b = get<UObject*>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(119, [](UObject*, Frame& s, void* r) {
                UObject* a = get<UObject*>(s);
                UObject* b = get<UObject*>(s);
                finish(s);
                retBool(r, a != b);
            });
            registerNative(254, [](UObject*, Frame& s, void* r) {
                Name a = get<Name>(s);
                Name b = get<Name>(s);
                finish(s);
                retBool(r, a == b);
            });
            registerNative(255, [](UObject*, Frame& s, void* r) {
                Name a = get<Name>(s);
                Name b = get<Name>(s);
                finish(s);
                retBool(r, a != b);
            });
        }

        void registerSystemNatives()
        {
            registerNative(231, [](UObject*, Frame& s, void*) {
                std::string text = get<std::string>(s);
                Name tag = vm::getOpt<Name>(s, Name("ScriptLog"));
                finish(s);
                std::fprintf(stderr, "%s: %s\n", tag.c_str(), text.c_str());
            });
            registerNative(232, [](UObject*, Frame& s, void*) {
                std::string text = get<std::string>(s);
                finish(s);
                vm::warn(s, text);
            });
            registerNative("Object", "Localize", [](UObject*, Frame& s, void* r) {
                std::string section = get<std::string>(s);
                std::string key = get<std::string>(s);
                std::string package = get<std::string>(s);
                finish(s);
                ret<std::string>(r, config::localize(section, key, package));
            });
            registerNative(113, [](UObject* self, Frame& s, void*) {
                Name current = vm::stateName(self);
                Name state = vm::getOpt<Name>(s, current);
                Name label = vm::getOpt<Name>(s, Name());
                finish(s);
                if (state == current && !state.none()) {
                    vm::gotoLabel(self, label.none() ? Name("Begin") : label);
                    return;
                }
                if (state == current && label.none()) {
                    return;
                }
                vm::gotoState(self, state, label);
            });
            registerNative(281, [](UObject* self, Frame& s, void* r) {
                Name state = get<Name>(s);
                finish(s);
                retBool(r, isInState(self, state));
            });
            registerNative(284, [](UObject* self, Frame& s, void* r) {
                finish(s);
                ret<Name>(r, vm::stateName(self));
            });
            registerNative(117, [](UObject* self, Frame& s, void*) {
                Name probe = get<Name>(s);
                finish(s);
                vm::enable(self, probe);
            });
            registerNative(118, [](UObject* self, Frame& s, void*) {
                Name probe = get<Name>(s);
                finish(s);
                vm::disable(self, probe);
            });
            registerNative(303, [](UObject* self, Frame& s, void* r) {
                Name className = get<Name>(s);
                finish(s);
                retBool(r, self->isA(className));
            });
            registerNative(258, [](UObject*, Frame& s, void* r) {
                UObject* test = get<UObject*>(s);
                UObject* parent = get<UObject*>(s);
                finish(s);
                UClass* testClass = cast<UClass>(test);
                UClass* parentClass = cast<UClass>(parent);
                retBool(r, testClass && parentClass && testClass->isChildOf(parentClass));
            });
            registerNative(751, [](UObject*, Frame& s, void* r) {
                get<UObject*>(s);
                finish(s);
                ret<int32_t>(r, 0);
            });
            registerNative("Object", "DynamicLoadObject", [](UObject*, Frame& s, void* r) {
                std::string path = get<std::string>(s);
                UObject* classObject = get<UObject*>(s);
                bool mayFail = vm::getBoolOpt(s, false);
                finish(s);
                UClass* cls = cast<UClass>(classObject);
                UObject* object = objects::loadObject(path, cls);
                objects::flushLoads();
                if (!object && !mayFail) {
                    vm::warn(s, "DynamicLoadObject failed: " + path);
                }
                ret<UObject*>(r, object);
            });
            registerNative("Object", "GetEnum", [](UObject*, Frame& s, void* r) {
                UObject* object = get<UObject*>(s);
                int32_t index = get<int32_t>(s);
                finish(s);
                UEnum* values = cast<UEnum>(object);
                Name value;
                if (values && index >= 0 && static_cast<size_t>(index) < values->names.size()) {
                    value = values->names[static_cast<size_t>(index)];
                }
                ret<Name>(r, value);
            });
            registerNative("Object", "GetPropertyText", [](UObject* self, Frame& s, void* r) {
                std::string propertyName = get<std::string>(s);
                finish(s);
                std::string value;
                UProperty* property = self->cls ? self->cls->findProperty(Name(propertyName)) : nullptr;
                if (property && self->data) {
                    value = property->toText(self->data + property->offset);
                }
                ret<std::string>(r, value);
            });
            registerNative("Object", "SetPropertyText", [](UObject* self, Frame& s, void*) {
                std::string propertyName = get<std::string>(s);
                std::string value = get<std::string>(s);
                finish(s);
                UProperty* property = self->cls ? self->cls->findProperty(Name(propertyName)) : nullptr;
                if (property && self->data) {
                    property->fromText(self->data + property->offset, value);
                }
            });
            registerNative(536, [](UObject* self, Frame& s, void*) {
                finish(s);
                config::saveObjectConfig(self, self->cls);
            });
            registerNative("Object", "StaticSaveConfig", [](UObject* self, Frame& s, void*) {
                finish(s);
                UClass* cls = self->cls;
                if (cls && cls->defaultObject) {
                    config::saveObjectConfig(cls->defaultObject, cls);
                }
            });
            registerNative("Object", "ResetConfig", [](UObject* self, Frame& s, void*) {
                finish(s);
                UClass* cls = self->cls;
                if (cls && cls->defaultObject) {
                    config::applyObjectConfig(cls->defaultObject);
                }
            });
            registerNative("Object", "GetConfig", [](UObject*, Frame& s, void* r) {
                std::string section = get<std::string>(s);
                std::string key = get<std::string>(s);
                finish(s);
                auto value = config::system().get(section, key);
                if (!value) {
                    value = config::user().get(section, key);
                }
                ret<std::string>(r, value.value_or(""));
            });
            registerNative(1001, [](UObject*, Frame& s, void*) {
                UClass* baseClass = cast<UClass>(get<UObject*>(s));
                UObject** out = getRef<UObject*>(s);
                finish(s);
                size_t index = 0;
                vm::iterate(s, [&]() {
                    const std::vector<UObject*>& all = objects::all();
                    while (index < all.size()) {
                        UObject* candidate = all[index++];
                        if (candidate && !candidate->destroyed && candidate->loaded && (!baseClass || candidate->isA(baseClass))) {
                            *out = candidate;
                            return true;
                        }
                    }
                    *out = nullptr;
                    return false;
                });
            });
            registerNative(1005, [](UObject*, Frame& s, void*) {
                int32_t* cycles = getRef<int32_t>(s);
                finish(s);
                *cycles = static_cast<int32_t>(std::chrono::steady_clock::now().time_since_epoch().count() / 1000);
            });
            registerNative(1006, [](UObject*, Frame& s, void*) {
                int32_t* cycles = getRef<int32_t>(s);
                finish(s);
                *cycles = static_cast<int32_t>(std::chrono::steady_clock::now().time_since_epoch().count() / 1000) - *cycles;
            });
            registerNative(1007, [](UObject*, Frame& s, void* r) {
                int32_t cycles = get<int32_t>(s);
                finish(s);
                ret<float>(r, static_cast<float>(cycles) / 1000000.0f);
            });
            registerNative("Commandlet", "Main", [](UObject*, Frame& s, void* r) {
                get<std::string>(s);
                finish(s);
                ret<int32_t>(r, 0);
            });
            registerNative(4000, [](UObject*, Frame& s, void*) {
                get<std::string>(s);
                get<std::string>(s);
                get<int32_t>(s);
                finish(s);
            });
            registerNative(4001, [](UObject*, Frame& s, void*) {
                get<std::string>(s);
                finish(s);
            });
            registerNative(4002, [](UObject*, Frame& s, void*) {
                get<std::string>(s);
                get<std::string>(s);
                finish(s);
            });
            registerNative(4003, [](UObject*, Frame& s, void* r) {
                get<std::string>(s);
                finish(s);
                ret<std::string>(r, std::string());
            });
        }
    }

    void registerObjectNatives()
    {
        registerIntNatives();
        registerByteBoolNatives();
        registerFloatNatives();
        registerVectorNatives();
        registerRotatorNatives();
        registerStringNatives();
        registerObjectNamesNatives();
        registerSystemNatives();
    }
}
