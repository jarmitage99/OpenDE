#pragma once

#include "core/object.h"
#include "engine/package.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dx {
    class Linker {
    public:
        Linker(Name name, std::unique_ptr<Package> file);

        UObject* object(int32_t ref);
        Name name(int32_t index) const;
        UObject* readObject(Reader& reader);
        Name readName(Reader& reader);
        int32_t findExport(Name objectName, Name className, std::string_view group) const;
        const Package& file() const;

        Name packageName;
        UObject* packageObject = nullptr;
        bool savedGame = false;
        uint64_t loadOrder = 0;

    private:
        UObject* createExport(int32_t ref);
        UObject* resolveImport(int32_t ref);

        std::unique_ptr<Package> package;
        std::vector<Name> names;
        std::vector<UObject*> exports;
        std::vector<UObject*> imports;
        std::vector<bool> importResolved;
    };

    using ObjectFactory = std::function<UObject*()>;

    namespace objects {
        void initialize(const std::filesystem::path& gameRoot);
        const std::filesystem::path& gameRoot();
        Linker* loadPackage(Name packageName);
        Linker* loadPackageFile(Name packageName, const std::filesystem::path& path, bool savedGame);
        Linker* findLinker(Name packageName);
        void unloadPackage(Name packageName);
        UObject* packageObject(Name packageName);
        const std::vector<std::string>& loadMismatches();
        void setEditorMode(bool enabled);
        bool editorMode();
        void registerFactory(Name nativeClass, ObjectFactory factory);
        void registerObject(UObject* object);
        UObject* fromHandle(uint32_t handle);
        const std::vector<UObject*>& all();
        void ensureLoaded(UObject* object);
        void flushLoads();
        void queueLoad(UObject* object);
        UClass* findClass(std::string_view path);
        UClass* intrinsicClass(Name className);
        bool isIntrinsic(const UObject* object);
        UObject* findObject(std::string_view path, UClass* cls);
        UObject* loadObject(std::string_view path, UClass* cls);
        UObject* newObject(UClass* cls, UObject* outer, Name objectName, uint32_t flags);
        UClass* createSubclass(UClass* parent, Name className);
        UObject* createNative(UClass* cls);
        void destroyObject(UObject* object);
        uint64_t destroyCount();
        Name uniqueName(UClass* cls);
        void readTaggedProperties(UStruct* structType, uint8_t* data, Reader& reader, Linker& linker);
        void syncObjectProperties(UObject* object);
        UClass* classClass();
        UClass* objectClass();
    }
}
