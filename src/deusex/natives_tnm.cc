#include "audio/audio.h"
#include "core/config.h"
#include "core/vm.h"
#include "engine/world.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>

namespace dx {
    namespace {
        std::unordered_map<UObject*, std::unique_ptr<std::ofstream>>& logFiles()
        {
            static std::unordered_map<UObject*, std::unique_ptr<std::ofstream>> files;
            return files;
        }

        void nativeLogOpen(UObject* self, Frame& stack, void*)
        {
            std::string name = vm::get<std::string>(stack);
            vm::finish(stack);
            std::filesystem::path file = config::userDirectory() / std::filesystem::path(name).filename();
            logFiles()[self] = std::make_unique<std::ofstream>(file, std::ios::binary);
        }

        void nativeLogClose(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            logFiles().erase(self);
        }

        void nativeLogLine(UObject* self, Frame& stack, void*)
        {
            std::string line = vm::get<std::string>(stack);
            vm::finish(stack);
            auto it = logFiles().find(self);
            if (it != logFiles().end()) {
                *it->second << line << "\r\n";
            }
        }

        void nativeRegexMatch(UObject*, Frame& stack, void* result)
        {
            std::string text = vm::get<std::string>(stack);
            std::string pattern = vm::get<std::string>(stack);
            vm::finish(stack);
            bool matched = false;
            try {
                matched = std::regex_match(text, std::regex(pattern));
            } catch (const std::regex_error&) {
                matched = false;
            }
            vm::retBool(result, matched);
        }

        std::tm localNow()
        {
            std::time_t now = std::time(nullptr);
            std::tm local {};
#ifdef _WIN32
            localtime_s(&local, &now);
#else
            localtime_r(&now, &local);
#endif
            return local;
        }

        template <int std::tm::* Field, int Offset>
        void nativeTimeField(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<int32_t>(result, localNow().*Field + Offset);
        }

        void nativeConfigureSound(UObject*, Frame& stack, void*)
        {
            audio::SoundModifier modifier = audio::soundModifier();
            modifier.enabled = vm::getBoolOpt(stack, modifier.enabled);
            modifier.volume = vm::getOpt<float>(stack, modifier.volume);
            modifier.radius = vm::getOpt<float>(stack, modifier.radius);
            modifier.pitch = vm::getOpt<float>(stack, modifier.pitch);
            vm::finish(stack);
            audio::setSoundModifier(modifier);
        }

        void nativeAssertHook(UObject*, Frame& stack, void*)
        {
            vm::finish(stack);
        }

        std::filesystem::path oggFile(const std::string& name)
        {
            if (name.empty()) {
                return {};
            }
            std::string directory = config::system().get("Core.System", "OggPath").value_or("..\\Music\\");
            return config::findFile(directory + name);
        }

        void nativeSetCurrentOgg(UObject* self, Frame& stack, void*)
        {
            std::string intro = vm::get<std::string>(stack);
            std::string loop = vm::get<std::string>(stack);
            int32_t position = vm::get<int32_t>(stack);
            uint8_t transition = vm::get<uint8_t>(stack);
            vm::finish(stack);
            audio::playOgg(self, oggFile(intro), oggFile(loop), static_cast<float>(position), transition);
        }

        void nativeGetCurrentSongPosition(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<float>(result, audio::oggPosition());
        }
    }

    namespace engine {
        void registerTnmNatives()
        {
            vm::registerNative(3051, nativeLogOpen);
            vm::registerNative(3052, nativeLogClose);
            vm::registerNative(3053, nativeLogLine);
            vm::registerNative(3056, nativeRegexMatch);
            vm::registerNative("tnmTime", "GetYear", nativeTimeField<&std::tm::tm_year, 1900>);
            vm::registerNative("tnmTime", "GetMonth", nativeTimeField<&std::tm::tm_mon, 1>);
            vm::registerNative("tnmTime", "GetDate", nativeTimeField<&std::tm::tm_mday, 0>);
            vm::registerNative("tnmTime", "GetDayOfWeek", nativeTimeField<&std::tm::tm_wday, 0>);
            vm::registerNative("tnmTime", "GetHour", nativeTimeField<&std::tm::tm_hour, 0>);
            vm::registerNative("tnmTime", "GetMinute", nativeTimeField<&std::tm::tm_min, 0>);
            vm::registerNative("tnmTime", "GetSecond", nativeTimeField<&std::tm::tm_sec, 0>);
            vm::registerNative("tnmSoundModifier", "ConfigureSound", nativeConfigureSound);
            vm::registerNative("tnmNativeHookObject", "AssertHook", nativeAssertHook);
            vm::registerNative("DXOgg", "SetCurrentOgg", nativeSetCurrentOgg);
            vm::registerNative("DXOgg", "GetCurrentSongPosition", nativeGetCurrentSongPosition);
        }
    }
}
