#pragma once

#include "core/math.h"
#include "core/object.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace dx {
    struct Frame {
        UObject* object = nullptr;
        UStruct* node = nullptr;
        const uint8_t* code = nullptr;
        uint8_t* locals = nullptr;
        Frame* previous = nullptr;
        uint8_t* propAddr = nullptr;
        UProperty* prop = nullptr;
        void* nullContext = nullptr;

        void step(UObject* context, void* result);
        uint8_t readByte();
        uint16_t readWord();
        int32_t readInt();
        float readFloat();
        UObject* readObject();
        Name readName();
        void jump(uint16_t offset);
    };

    struct StateFrame : Frame {
        UState* state = nullptr;
        int32_t latentAction = 0;
        uint64_t disabledProbes = 0;
        float sleepTime = 0;
        bool running = false;
        bool inEndState = false;
        bool stateChanged = false;
    };

    namespace vm {
        constexpr uint8_t EX_Nothing = 0x0B;
        constexpr uint8_t EX_EndFunctionParms = 0x16;
        constexpr uint8_t EX_Skip = 0x18;
        constexpr uint8_t EX_IteratorPop = 0x30;
        constexpr uint8_t EX_IteratorNext = 0x31;

        NativeFunction bindNative(UFunction* function);
        void registerNative(int32_t index, NativeFunction function);
        void registerNative(std::string_view className, std::string_view functionName, NativeFunction function);
        void callFunction(Frame& stack, UObject* context, UFunction* function, void* result);
        void execute(Frame& frame);
        UFunction* findFunction(UObject* object, Name functionName, bool global = false);
        UFunction* findEvent(UObject* object, Name eventName);
        bool isProbing(UObject* object, Name functionName);
        StateFrame& stateFrame(UObject* object);
        bool gotoState(UObject* object, Name stateName, Name label = Name());
        bool gotoLabel(UObject* object, Name label);
        Name stateName(UObject* object);
        void runStateCode(UObject* object);
        void restoreState(UObject* object, UState* state, UStruct* node, int32_t offset, uint64_t probeMask);
        void disable(UObject* object, Name eventName);
        void enable(UObject* object, Name eventName);
        int32_t probeIndex(Name eventName);
        void warn(const Frame& frame, const std::string& message);
        using CriticalHandler = std::function<void(const std::string&)>;
        CriticalHandler setCriticalHandler(CriticalHandler handler);
        void critical(const Frame& frame, const std::string& message);
        void resetRunaway();
        void fatal(const std::string& message);
        uint32_t assertionFailures();

        template <typename T>
        T get(Frame& stack)
        {
            T value {};
            stack.step(stack.object, &value);
            return value;
        }

        template <typename T>
        T getOpt(Frame& stack, T fallback)
        {
            T value = fallback;
            if (*stack.code == EX_EndFunctionParms) {
                return value;
            }
            stack.step(stack.object, &value);
            return value;
        }

        template <typename T>
        T* getRef(Frame& stack)
        {
            static thread_local T scratch[16];
            static thread_local int next = 0;
            T& value = scratch[next++ & 15];
            value = T {};
            stack.propAddr = nullptr;
            stack.step(stack.object, &value);
            if (stack.propAddr) {
                return reinterpret_cast<T*>(stack.propAddr);
            }
            return &value;
        }

        bool getBool(Frame& stack);
        bool getBoolOpt(Frame& stack, bool fallback);
        uint16_t getSkip(Frame& stack);
        void finish(Frame& stack);
        void iterate(Frame& stack, const std::function<bool()>& next);

        template <typename T>
        void ret(void* result, const T& value)
        {
            if (result) {
                *static_cast<T*>(result) = value;
            }
        }

        inline void retBool(void* result, bool value)
        {
            if (result) {
                *static_cast<uint32_t*>(result) = value ? 1 : 0;
            }
        }

        class Call {
        public:
            Call(UObject* target, Name functionName);
            Call(UObject* target, UFunction* callee);
            ~Call();
            Call(const Call&) = delete;
            Call& operator=(const Call&) = delete;

            bool valid() const;
            Call& obj(UObject* value);
            Call& f(float value);
            Call& i(int32_t value);
            Call& b(bool value);
            Call& byte(uint8_t value);
            Call& n(Name value);
            Call& s(const std::string& value);
            Call& v(const Vector& value);
            Call& r(const Rotator& value);
            Call& skip();
            void run();
            uint8_t* parameter(size_t index);
            uint8_t* returnValue();

        private:
            UProperty* nextParam();

            UObject* object = nullptr;
            UFunction* function = nullptr;
            std::vector<uint8_t> buffer;
            size_t argument = 0;
        };

        bool event(UObject* object, Name eventName);
    }
}
