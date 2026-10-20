#include "extension/windows.h"

#include "core/objects.h"
#include "engine/reader.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace dx::extension {
    class UFlagBase : public UObject {
    public:
        struct Iterator {
            int32_t bucket = 0;
            UObject* next = nullptr;
            bool typed = false;
            uint8_t type = 0;
        };

        std::unordered_map<int32_t, Iterator> iterators;
        int32_t nextIterator = 1;
    };

    class UExtString : public UObject {
    public:
        void serialize(Reader& reader, Linker&) override
        {
            extstring::Text(this) = reader.string();
        }
    };

    namespace {
        constexpr int32_t HashSize = 64;

        const char* flagClassNames[] = {
            "Extension.FlagBool",
            "Extension.FlagByte",
            "Extension.FlagInt",
            "Extension.FlagFloat",
            "Extension.FlagName",
            "Extension.FlagVector",
            "Extension.FlagRotator",
        };

        const char* valueNames[] = {
            "bValue",
            "byteValue",
            "intValue",
            "floatValue",
            "nameValue",
            "vectorValue",
            "rotatorValue",
        };

        int32_t bucketOf(Name name)
        {
            const std::string& text = name.str();
            uint32_t hash = 0;
            for (char c : text) {
                hash = hash * 31u + static_cast<uint32_t>(std::tolower(static_cast<unsigned char>(c)));
            }
            return static_cast<int32_t>(hash % HashSize);
        }

        UFlagBase* self(UObject* object)
        {
            return cast<UFlagBase>(object);
        }

        UObject* findFlag(UObject* base, Name name, uint8_t type)
        {
            for (UObject* flag = flagbase::hashTable(base, bucketOf(name)); flag; flag = flag::nextFlag(flag)) {
                if (flag::FlagName(flag) == name && flag::flagType(flag) == type) {
                    return flag;
                }
            }
            return nullptr;
        }

        uint8_t* valueOf(UObject* flag, uint8_t type)
        {
            if (!flag || type >= 7) {
                return nullptr;
            }
            return flag->propertyAddress(Name(valueNames[type]));
        }

        UObject* obtainFlag(UObject* base, Name name, uint8_t type, bool add, bool hasExpiration, int32_t expiration)
        {
            UObject* flag = findFlag(base, name, type);
            if (flag) {
                if (hasExpiration) {
                    flag::expiration(flag) = expiration;
                }
                return flag;
            }
            if (!add || type >= 7) {
                return nullptr;
            }
            UClass* cls = extensionClass(flagClassNames[type]);
            if (!cls) {
                return nullptr;
            }
            flag = objects::newObject(cls, base, Name(), RF_Transient);
            int32_t bucket = bucketOf(name);
            flag::FlagName(flag) = name;
            flag::flagHash(flag) = bucket;
            flag::FlagBase(flag) = base;
            flag::flagType(flag) = type;
            flag::expiration(flag) = hasExpiration ? expiration : flagbase::defaultFlagExpiration(base);
            flag::nextFlag(flag) = flagbase::hashTable(base, bucket);
            flagbase::hashTable(base, bucket) = flag;
            return flag;
        }

        bool deleteFlag(UObject* base, Name name, uint8_t type)
        {
            int32_t bucket = bucketOf(name);
            UObject* previous = nullptr;
            for (UObject* flag = flagbase::hashTable(base, bucket); flag; flag = flag::nextFlag(flag)) {
                if (flag::FlagName(flag) == name && flag::flagType(flag) == type) {
                    if (previous) {
                        flag::nextFlag(previous) = flag::nextFlag(flag);
                    } else {
                        flagbase::hashTable(base, bucket) = flag::nextFlag(flag);
                    }
                    flag::nextFlag(flag) = nullptr;
                    objects::destroyObject(flag);
                    return true;
                }
                previous = flag;
            }
            return false;
        }

        template <typename T>
        void setFlag(UObject* object, Frame& stack, void* result, uint8_t type)
        {
            Name name = vm::get<Name>(stack);
            T value = vm::get<T>(stack);
            bool add = optionalBoolDefault(stack, true);
            int32_t expiration = 0;
            bool hasExpiration = optional(stack, expiration);
            vm::finish(stack);
            UObject* flag = object ? obtainFlag(object, name, type, add, hasExpiration, expiration) : nullptr;
            uint8_t* target = valueOf(flag, type);
            if (target) {
                std::memcpy(target, &value, sizeof(T));
            }
            vm::retBool(result, target != nullptr);
        }

        template <typename T>
        void getFlag(UObject* object, Frame& stack, void* result, uint8_t type)
        {
            Name name = vm::get<Name>(stack);
            vm::finish(stack);
            T value {};
            uint8_t* source = object ? valueOf(findFlag(object, name, type), type) : nullptr;
            if (source) {
                std::memcpy(&value, source, sizeof(T));
            }
            vm::ret<T>(result, value);
        }

        void nativeSetBool(UObject* object, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            bool value = vm::getBool(stack);
            bool add = optionalBoolDefault(stack, true);
            int32_t expiration = 0;
            bool hasExpiration = optional(stack, expiration);
            vm::finish(stack);
            UObject* flag = object ? obtainFlag(object, name, 0, add, hasExpiration, expiration) : nullptr;
            uint8_t* target = valueOf(flag, 0);
            if (target) {
                *reinterpret_cast<uint32_t*>(target) = value ? 1 : 0;
            }
            vm::retBool(result, target != nullptr);
        }

        void nativeSetByte(UObject* object, Frame& stack, void* result)
        {
            setFlag<uint8_t>(object, stack, result, 1);
        }

        void nativeSetInt(UObject* object, Frame& stack, void* result)
        {
            setFlag<int32_t>(object, stack, result, 2);
        }

        void nativeSetFloat(UObject* object, Frame& stack, void* result)
        {
            setFlag<float>(object, stack, result, 3);
        }

        void nativeSetName(UObject* object, Frame& stack, void* result)
        {
            setFlag<Name>(object, stack, result, 4);
        }

        void nativeSetVector(UObject* object, Frame& stack, void* result)
        {
            setFlag<Vector>(object, stack, result, 5);
        }

        void nativeSetRotator(UObject* object, Frame& stack, void* result)
        {
            setFlag<Rotator>(object, stack, result, 6);
        }

        void nativeGetBool(UObject* object, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            vm::finish(stack);
            uint8_t* source = object ? valueOf(findFlag(object, name, 0), 0) : nullptr;
            vm::retBool(result, source && *reinterpret_cast<uint32_t*>(source) != 0);
        }

        void nativeGetByte(UObject* object, Frame& stack, void* result)
        {
            getFlag<uint8_t>(object, stack, result, 1);
        }

        void nativeGetInt(UObject* object, Frame& stack, void* result)
        {
            getFlag<int32_t>(object, stack, result, 2);
        }

        void nativeGetFloat(UObject* object, Frame& stack, void* result)
        {
            getFlag<float>(object, stack, result, 3);
        }

        void nativeGetName(UObject* object, Frame& stack, void* result)
        {
            getFlag<Name>(object, stack, result, 4);
        }

        void nativeGetVector(UObject* object, Frame& stack, void* result)
        {
            getFlag<Vector>(object, stack, result, 5);
        }

        void nativeGetRotator(UObject* object, Frame& stack, void* result)
        {
            getFlag<Rotator>(object, stack, result, 6);
        }

        void nativeCheckFlag(UObject* object, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            vm::finish(stack);
            vm::retBool(result, object && findFlag(object, name, type));
        }

        void nativeDeleteFlag(UObject* object, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            vm::finish(stack);
            vm::retBool(result, object && deleteFlag(object, name, type));
        }

        void nativeSetExpiration(UObject* object, Frame& stack, void*)
        {
            Name name = vm::get<Name>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            int32_t expiration = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UObject* flag = object ? findFlag(object, name, type) : nullptr) {
                flag::expiration(flag) = expiration;
            }
        }

        void nativeGetExpiration(UObject* object, Frame& stack, void* result)
        {
            Name name = vm::get<Name>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            vm::finish(stack);
            UObject* flag = object ? findFlag(object, name, type) : nullptr;
            vm::ret<int32_t>(result, flag ? flag::expiration(flag) : -1);
        }

        void nativeDeleteExpiredFlags(UObject* object, Frame& stack, void*)
        {
            int32_t criteria = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (!object) {
                return;
            }
            std::vector<std::pair<Name, uint8_t>> expired;
            for (int32_t bucket = 0; bucket < HashSize; bucket++) {
                for (UObject* flag = flagbase::hashTable(object, bucket); flag; flag = flag::nextFlag(flag)) {
                    int32_t expiration = flag::expiration(flag);
                    if (expiration > 0 && expiration <= criteria) {
                        expired.emplace_back(flag::FlagName(flag), flag::flagType(flag));
                    }
                }
            }
            for (const auto& [name, type] : expired) {
                deleteFlag(object, name, type);
            }
        }

        void nativeSetDefaultExpiration(UObject* object, Frame& stack, void*)
        {
            int32_t expiration = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (object) {
                flagbase::defaultFlagExpiration(object) = expiration;
            }
        }

        void nativeCreateIterator(UObject* object, Frame& stack, void* result)
        {
            uint8_t type = 0;
            bool typed = optional(stack, type);
            vm::finish(stack);
            UFlagBase* base = self(object);
            if (!base) {
                vm::ret<int32_t>(result, 0);
                return;
            }
            int32_t id = base->nextIterator++;
            UFlagBase::Iterator iterator;
            iterator.typed = typed;
            iterator.type = type;
            iterator.bucket = 0;
            iterator.next = flagbase::hashTable(base, 0);
            base->iterators[id] = iterator;
            vm::ret<int32_t>(result, id);
        }

        void nativeDestroyIterator(UObject* object, Frame& stack, void*)
        {
            int32_t id = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UFlagBase* base = self(object)) {
                base->iterators.erase(id);
            }
        }

        bool advance(UFlagBase* base, int32_t id, Name& name, uint8_t& type)
        {
            auto it = base->iterators.find(id);
            if (it == base->iterators.end()) {
                return false;
            }
            UFlagBase::Iterator& iterator = it->second;
            while (iterator.bucket < HashSize) {
                while (iterator.next) {
                    UObject* flag = iterator.next;
                    iterator.next = flag::nextFlag(flag);
                    if (!iterator.typed || flag::flagType(flag) == iterator.type) {
                        name = flag::FlagName(flag);
                        type = flag::flagType(flag);
                        return true;
                    }
                }
                iterator.bucket++;
                if (iterator.bucket < HashSize) {
                    iterator.next = flagbase::hashTable(base, iterator.bucket);
                }
            }
            return false;
        }

        void nativeGetNextFlagName(UObject* object, Frame& stack, void* result)
        {
            int32_t id = vm::get<int32_t>(stack);
            Name* name = vm::getRef<Name>(stack);
            vm::finish(stack);
            UFlagBase* base = self(object);
            Name found;
            uint8_t type = 0;
            bool ok = base && advance(base, id, found, type);
            *name = found;
            vm::retBool(result, ok);
        }

        void nativeGetNextFlag(UObject* object, Frame& stack, void* result)
        {
            int32_t id = vm::get<int32_t>(stack);
            Name* name = vm::getRef<Name>(stack);
            uint8_t* type = vm::getRef<uint8_t>(stack);
            vm::finish(stack);
            UFlagBase* base = self(object);
            Name found;
            uint8_t foundType = 0;
            bool ok = base && advance(base, id, found, foundType);
            *name = found;
            *type = foundType;
            vm::retBool(result, ok);
        }

        void nativeDeleteAllFlags(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (!object) {
                return;
            }
            for (int32_t bucket = 0; bucket < HashSize; bucket++) {
                UObject* flag = flagbase::hashTable(object, bucket);
                while (flag) {
                    UObject* next = flag::nextFlag(flag);
                    objects::destroyObject(flag);
                    flag = next;
                }
                flagbase::hashTable(object, bucket) = nullptr;
            }
        }

        void nativeExtSetText(UObject* object, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            if (object) {
                extstring::Text(object) = text;
            }
        }

        void nativeExtAppendText(UObject* object, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            if (object) {
                extstring::Text(object) += text;
            }
        }

        void nativeExtGetText(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<std::string>(result, object ? extstring::Text(object) : std::string());
        }

        void nativeExtGetTextLength(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<int32_t>(result, object ? static_cast<int32_t>(extstring::Text(object).size()) : 0);
        }

        void nativeExtGetTextPart(UObject* object, Frame& stack, void* result)
        {
            int32_t start = vm::get<int32_t>(stack);
            int32_t count = vm::get<int32_t>(stack);
            std::string* out = vm::getRef<std::string>(stack);
            vm::finish(stack);
            std::string part;
            if (object) {
                const std::string& text = extstring::Text(object);
                int32_t length = static_cast<int32_t>(text.size());
                int32_t first = std::clamp(start, 0, length);
                int32_t last = std::clamp(start + count, first, length);
                part = text.substr(static_cast<size_t>(first), static_cast<size_t>(last - first));
            }
            *out = part;
            vm::ret<int32_t>(result, static_cast<int32_t>(part.size()));
        }

        constexpr size_t PageSize = 255;

        void nativeExtGetFirstTextPart(UObject* object, Frame& stack, void* result)
        {
            std::string* out = vm::getRef<std::string>(stack);
            vm::finish(stack);
            std::string part;
            if (object) {
                extstring::speechPage(object) = 0;
                part = extstring::Text(object).substr(0, PageSize);
            }
            *out = part;
            vm::ret<int32_t>(result, static_cast<int32_t>(part.size()));
        }

        void nativeExtGetNextTextPart(UObject* object, Frame& stack, void* result)
        {
            std::string* out = vm::getRef<std::string>(stack);
            vm::finish(stack);
            std::string part;
            if (object) {
                int32_t& page = extstring::speechPage(object);
                page++;
                size_t start = static_cast<size_t>(page) * PageSize;
                const std::string& text = extstring::Text(object);
                if (start < text.size()) {
                    part = text.substr(start, PageSize);
                }
            }
            *out = part;
            vm::ret<int32_t>(result, static_cast<int32_t>(part.size()));
        }
    }

    void registerFlagNatives()
    {
        vm::registerNative(1100, nativeSetBool);
        vm::registerNative(1101, nativeSetByte);
        vm::registerNative(1102, nativeSetInt);
        vm::registerNative(1103, nativeSetFloat);
        vm::registerNative(1104, nativeSetName);
        vm::registerNative(1105, nativeSetVector);
        vm::registerNative(1106, nativeSetRotator);
        vm::registerNative(1110, nativeGetBool);
        vm::registerNative(1111, nativeGetByte);
        vm::registerNative(1112, nativeGetInt);
        vm::registerNative(1113, nativeGetFloat);
        vm::registerNative(1114, nativeGetName);
        vm::registerNative(1115, nativeGetVector);
        vm::registerNative(1116, nativeGetRotator);
        vm::registerNative(1120, nativeCheckFlag);
        vm::registerNative(1121, nativeDeleteFlag);
        vm::registerNative(1122, nativeSetExpiration);
        vm::registerNative(1123, nativeGetExpiration);
        vm::registerNative(1124, nativeDeleteExpiredFlags);
        vm::registerNative(1125, nativeSetDefaultExpiration);
        vm::registerNative(1126, nativeCreateIterator);
        vm::registerNative(1127, nativeGetNextFlagName);
        vm::registerNative(1128, nativeGetNextFlag);
        vm::registerNative(1129, nativeDestroyIterator);
        vm::registerNative(1130, nativeDeleteAllFlags);
        vm::registerNative(1140, nativeExtSetText);
        vm::registerNative(1141, nativeExtAppendText);
        vm::registerNative(1142, nativeExtGetText);
        vm::registerNative(1143, nativeExtGetTextLength);
        vm::registerNative(1144, nativeExtGetTextPart);
        vm::registerNative(1145, nativeExtGetFirstTextPart);
        vm::registerNative(1146, nativeExtGetNextTextPart);
    }

    void registerFactories()
    {
        objects::registerFactory(Name("Window"), [] { return new UWindow(); });
        objects::registerFactory(Name("TabGroupWindow"), [] { return new UTabGroupWindow(); });
        objects::registerFactory(Name("ModalWindow"), [] { return new UModalWindow(); });
        objects::registerFactory(Name("RootWindow"), [] { return new URootWindow(); });
        objects::registerFactory(Name("ClipWindow"), [] { return new UClipWindow(); });
        objects::registerFactory(Name("RadioBoxWindow"), [] { return new URadioBoxWindow(); });
        objects::registerFactory(Name("TextWindow"), [] { return new UTextWindow(); });
        objects::registerFactory(Name("ButtonWindow"), [] { return new UButtonWindow(); });
        objects::registerFactory(Name("ToggleWindow"), [] { return new UToggleWindow(); });
        objects::registerFactory(Name("CheckboxWindow"), [] { return new UCheckboxWindow(); });
        objects::registerFactory(Name("LargeTextWindow"), [] { return new ULargeTextWindow(); });
        objects::registerFactory(Name("EditWindow"), [] { return new UEditWindow(); });
        objects::registerFactory(Name("TextLogWindow"), [] { return new UTextLogWindow(); });
        objects::registerFactory(Name("TileWindow"), [] { return new UTileWindow(); });
        objects::registerFactory(Name("BorderWindow"), [] { return new UBorderWindow(); });
        objects::registerFactory(Name("ScaleWindow"), [] { return new UScaleWindow(); });
        objects::registerFactory(Name("ScaleManagerWindow"), [] { return new UScaleManagerWindow(); });
        objects::registerFactory(Name("ScrollAreaWindow"), [] { return new UScrollAreaWindow(); });
        objects::registerFactory(Name("ListWindow"), [] { return new UListWindow(); });
        objects::registerFactory(Name("ComputerWindow"), [] { return new UComputerWindow(); });
        objects::registerFactory(Name("ViewportWindow"), [] { return new UViewportWindow(); });
        objects::registerFactory(Name("GC"), [] { return new UGC(); });
        objects::registerFactory(Name("FlagBase"), [] { return new UFlagBase(); });
        objects::registerFactory(Name("ExtString"), [] { return new UExtString(); });
        registerConSysFactories();
    }
}
