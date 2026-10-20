#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/actor.h"
#include "engine/props_consys.h"
#include "engine/props_deusex.h"
#include "engine/savegame.h"
#include "engine/world.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <random>
#include <unordered_map>

namespace dx {
    namespace {
        struct DeusExState {
            UObject* missionList = nullptr;
            UObject* mission = nullptr;
            UObject* levelInfo = nullptr;
        };

        DeusExState& state()
        {
            static DeusExState instance;
            return instance;
        }

        UObject* createObject(const char* path, UObject* outer)
        {
            UClass* cls = objects::findClass(path);
            if (!cls) {
                std::fprintf(stderr, "missing class %s\n", path);
                return nullptr;
            }
            return objects::newObject(cls, outer ? outer : objects::packageObject(Name("Transient")), Name(), 0);
        }

        uint8_t* field(UObject* object, const char* name)
        {
            if (!object) {
                return nullptr;
            }
            static std::unordered_map<const char*, Name> names;
            auto it = names.find(name);
            if (it == names.end()) {
                it = names.emplace(name, Name(name)).first;
            }
            return object->propertyAddress(it->second);
        }

        UObject* objectField(UObject* object, const char* name)
        {
            uint8_t* address = field(object, name);
            return address ? *reinterpret_cast<UObject**>(address) : nullptr;
        }

        std::string stringField(UObject* object, const char* name)
        {
            uint8_t* address = field(object, name);
            return address ? *reinterpret_cast<std::string*>(address) : std::string();
        }

        void bindConversations(UObject* self)
        {
            UObject* mission = state().mission;
            UClass* listItemClass = objects::findClass("ConSys.ConListItem");
            UObject* head = nullptr;
            if (mission && listItemClass) {
                std::string names[2] = { stringField(self, "BindName"), stringField(self, "BarkBindName") };
                for (const std::string& bindName : names) {
                    if (bindName.empty()) {
                        continue;
                    }
                    for (UObject* item = objectField(mission, "conversations"); item; item = objectField(item, "Next")) {
                        UObject* conversation = objectField(item, "ConObject");
                        if (!conversation) {
                            continue;
                        }
                        objects::ensureLoaded(conversation);
                        if (!iequals(stringField(conversation, "conOwnerName"), bindName)) {
                            continue;
                        }
                        UObject* entry = objects::newObject(listItemClass, objects::packageObject(Name("Transient")), Name(), RF_Transient);
                        *reinterpret_cast<UObject**>(field(entry, "con")) = conversation;
                        *reinterpret_cast<UObject**>(field(entry, "Next")) = head;
                        head = entry;
                    }
                }
            }
            uint8_t* list = field(self, "ConListItems");
            if (list) {
                *reinterpret_cast<UObject**>(list) = head;
            }
        }

        void nativeConBindEvents(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            bindConversations(self);
        }

        uint8_t allianceType(UObject* self, Name alliance)
        {
            uint8_t result = 1;
            static const Name nameAlliancesEx("AlliancesEx");
            UProperty* property = self->cls->findProperty(nameAlliancesEx);
            if (property && property->structType) {
                UStruct* layout = property->structType;
                static const Name nameAllianceName("AllianceName");
                UProperty* nameProperty = layout->findProperty(nameAllianceName);
                static const Name nameAllianceLevel("AllianceLevel");
                UProperty* levelProperty = layout->findProperty(nameAllianceLevel);
                static const Name nameAgitationLevel("AgitationLevel");
                UProperty* agitationProperty = layout->findProperty(nameAgitationLevel);
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    uint8_t* entry = self->data + property->offset + static_cast<size_t>(i) * property->elementSize();
                    if (*reinterpret_cast<Name*>(entry + nameProperty->offset) != alliance) {
                        continue;
                    }
                    float level = *reinterpret_cast<float*>(entry + levelProperty->offset);
                    float agitation = *reinterpret_cast<float*>(entry + agitationProperty->offset);
                    if (level < 0 || agitation >= 1.0f) {
                        result = 2;
                    } else if (level > 0) {
                        result = 0;
                    }
                    break;
                }
            }
            if (scriptedpawn::bLikesNeutral(self) && result == 1) {
                result = 0;
            }
            if (scriptedpawn::bReverseAlliances(self)) {
                if (result == 0) {
                    return 2;
                }
                if (result == 2) {
                    return 0;
                }
            }
            return result;
        }

        uint8_t pawnAllianceType(UObject* self, UObject* other)
        {
            UClass* scripted = engineClasses().scriptedPawn;
            if (other && scripted && other->isA(scripted)) {
                if (allianceType(other, pawn::Alliance(self)) == 2) {
                    return 2;
                }
            }
            return other ? allianceType(self, pawn::Alliance(other)) : 1;
        }

        void nativeGetAllianceType(UObject* self, Frame& stack, void* result)
        {
            Name alliance = vm::get<Name>(stack);
            vm::finish(stack);
            vm::ret<uint8_t>(result, allianceType(self, alliance));
        }

        void nativeGetPawnAllianceType(UObject* self, Frame& stack, void* result)
        {
            UObject* other = vm::get<UObject*>(stack);
            vm::finish(stack);
            vm::ret<uint8_t>(result, pawnAllianceType(self, other));
        }

        void nativeIsValidEnemy(UObject* self, Frame& stack, void* result)
        {
            UObject* enemy = vm::get<UObject*>(stack);
            bool checkAlliance = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            bool valid = enemy && enemy != self && !enemy->destroyed && isPawn(enemy) && pawn::Health(enemy) > 0 && !actor::bDeleteMe(enemy);
            if (valid) {
                uint8_t* detectable = field(enemy, "bDetectable");
                if (detectable && !*reinterpret_cast<uint32_t*>(detectable)) {
                    valid = false;
                }
                uint8_t* ignore = field(enemy, "bIgnore");
                if (ignore && *reinterpret_cast<uint32_t*>(ignore)) {
                    valid = false;
                }
            }
            if (valid && checkAlliance) {
                valid = pawnAllianceType(self, enemy) == 2;
            }
            vm::retBool(result, valid);
        }

        void nativeAddCarcass(UObject* self, Frame& stack, void*)
        {
            Name carcass = vm::get<Name>(stack);
            vm::finish(stack);
            int32_t& count = scriptedpawn::NumCarcasses(self);
            for (int32_t i = 0; i < count; i++) {
                if (scriptedpawn::Carcasses(self, i) == carcass) {
                    return;
                }
            }
            if (count < 4) {
                scriptedpawn::Carcasses(self, count) = carcass;
                count++;
            }
        }

        void nativeHaveSeenCarcass(UObject* self, Frame& stack, void* result)
        {
            Name carcass = vm::get<Name>(stack);
            vm::finish(stack);
            bool seen = false;
            for (int32_t i = 0; i < scriptedpawn::NumCarcasses(self); i++) {
                if (scriptedpawn::Carcasses(self, i) == carcass) {
                    seen = true;
                }
            }
            vm::retBool(result, seen);
        }

        void nativeCreateObject(Frame& stack, void* result, const char* path, UObject* outer)
        {
            vm::finish(stack);
            vm::ret<UObject*>(result, createObject(path, outer));
        }

        void nativeCreateDumpLocation(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "DeusEx.DumpLocation", self);
        }

        void nativeCreateDataVaultImageNote(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "DeusEx.DataVaultImageNote", self);
        }

        void nativeCreateGameDirectory(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "DeusEx.GameDirectory", self);
        }

        void nativeCreateLog(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "DeusEx.DeusExLog", self);
        }

        void nativeCreateHistoryEvent(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "ConSys.ConHistoryEvent", self);
        }

        void nativeCreateHistory(UObject* self, Frame& stack, void* result)
        {
            nativeCreateObject(stack, result, "ConSys.ConHistory", self);
        }

        void nativeSetBoolFlagFromString(UObject* self, Frame& stack, void* result)
        {
            std::string flagName = vm::get<std::string>(stack);
            bool value = vm::getBool(stack);
            vm::finish(stack);
            Name flag(flagName);
            UObject* flagBase = objectField(self, "FlagBase");
            if (flagBase) {
                vm::Call call(flagBase, Name("SetBool"));
                if (call.valid()) {
                    call.n(flag).b(value).run();
                }
            }
            vm::ret<Name>(result, flag);
        }

        void nativeGetDeusExVersion(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            Ini manifest;
            manifest.load(objects::gameRoot() / "System" / "Manifest.ini");
            std::string product = manifest.get("Setup", "MasterProduct").value_or("");
            if (manifest.get(product, "Version").value_or("") == "1002f_DEMO") {
                vm::ret<std::string>(result, "Mon Jun 12 11:43:26 2000 v1.002f_DEMO");
            } else {
                vm::ret<std::string>(result, "Mon Mar 19 12:06:14 2001 v1.112fm");
            }
        }

        void nativeSaveGame(UObject*, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            std::string description = vm::getOpt<std::string>(stack, std::string());
            vm::finish(stack);
            if (World::current) {
                savegame::saveGame(*World::current, index, description);
            }
        }

        void nativeDeleteSaveGameFiles(UObject*, Frame& stack, void*)
        {
            std::string directory = vm::getOpt<std::string>(stack, std::string());
            vm::finish(stack);
            if (directory.empty()) {
                savegame::deleteCurrent();
                return;
            }
            std::filesystem::path target = std::filesystem::path(directory).lexically_normal();
            std::filesystem::path root = savegame::saveRoot().lexically_normal();
            auto mismatch = std::mismatch(root.begin(), root.end(), target.begin(), target.end());
            if (mismatch.first == root.end() && target != root) {
                std::error_code error;
                std::filesystem::remove_all(target, error);
            }
        }

        struct DirectoryListing {
            uint8_t type = 0;
            std::string filter;
            std::vector<std::string> entries;
        };

        std::unordered_map<UObject*, DirectoryListing>& listings()
        {
            static std::unordered_map<UObject*, DirectoryListing> instance;
            return instance;
        }

        int32_t clampToInt(uint64_t value)
        {
            return value > static_cast<uint64_t>(INT_MAX) ? INT_MAX : static_cast<int32_t>(value);
        }

        void nativeGetGameDirectory(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            DirectoryListing& listing = listings()[self];
            listing.entries.clear();
            if (listing.type == 1) {
                for (int32_t index : savegame::slotIndices()) {
                    listing.entries.push_back(savegame::slotDirectory(index).filename().string());
                }
                return;
            }
            std::filesystem::path maps = objects::gameRoot() / "Maps";
            if (!std::filesystem::is_directory(maps)) {
                return;
            }
            std::string filter = listing.filter;
            filter.erase(std::remove(filter.begin(), filter.end(), '*'), filter.end());
            for (const auto& entry : std::filesystem::directory_iterator(maps)) {
                std::string name = entry.path().filename().string();
                if (filter.empty() || name.find(filter) != std::string::npos) {
                    listing.entries.push_back(name);
                }
            }
            std::sort(listing.entries.begin(), listing.entries.end());
        }

        void nativeGetNewSaveFileIndex(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<int32_t>(result, savegame::newSlotIndex());
        }

        void nativeGenerateNewSaveFilename(UObject*, Frame& stack, void* result)
        {
            int32_t index = vm::getOpt<int32_t>(stack, 0);
            vm::finish(stack);
            vm::ret<std::string>(result, savegame::slotDirectory(index > 0 ? index : savegame::newSlotIndex()).string());
        }

        void nativeGetDirCount(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<int32_t>(result, static_cast<int32_t>(listings()[self].entries.size()));
        }

        void nativeGetDirFilename(UObject* self, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            const std::vector<std::string>& entries = listings()[self].entries;
            vm::ret<std::string>(result, index >= 0 && static_cast<size_t>(index) < entries.size() ? entries[static_cast<size_t>(index)] : std::string());
        }

        void nativeSetDirType(UObject* self, Frame& stack, void*)
        {
            uint8_t type = vm::get<uint8_t>(stack);
            vm::finish(stack);
            listings()[self].type = type;
            gamedirectory::gameDirectoryType(self) = type;
        }

        void nativeSetDirFilter(UObject* self, Frame& stack, void*)
        {
            std::string filter = vm::get<std::string>(stack);
            vm::finish(stack);
            listings()[self].filter = filter;
            gamedirectory::currentFilter(self) = filter;
        }

        void nativeGetSaveInfo(UObject*, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            vm::ret<UObject*>(result, savegame::readSaveInfo(index));
        }

        void nativeGetSaveInfoFromDirectoryIndex(UObject* self, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            const std::vector<std::string>& entries = listings()[self].entries;
            UObject* info = nullptr;
            if (index >= 0 && static_cast<size_t>(index) < entries.size() && entries[static_cast<size_t>(index)].size() > 4) {
                info = savegame::readSaveInfo(std::atoi(entries[static_cast<size_t>(index)].c_str() + 4));
            }
            vm::ret<UObject*>(result, info);
        }

        void nativeGetTempSaveInfo(UObject* self, Frame& stack, void* result)
        {
            vm::finish(stack);
            UObject*& temporary = gamedirectory::tempSaveInfo(self);
            if (!temporary) {
                temporary = createObject("DeusEx.DeusExSaveInfo", nullptr);
            }
            vm::ret<UObject*>(result, temporary);
        }

        void nativeGetSaveFreeSpace(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            std::error_code error;
            std::filesystem::space_info space = std::filesystem::space(savegame::saveRoot(), error);
            vm::ret<int32_t>(result, error ? 0 : clampToInt(space.available / 1024));
        }

        void nativeGetSaveDirectorySize(UObject*, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            vm::ret<int32_t>(result, clampToInt(savegame::slotSize(index) / 1024));
        }

        void nativeUnloadTexture(UObject*, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            vm::finish(stack);
        }

        void nativeUpdateTimeStamp(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            std::time_t now = std::time(nullptr);
            std::tm local = *std::localtime(&now);
            auto set = [self](const char* name, int32_t value) {
                uint8_t* address = field(self, name);
                if (address) {
                    *reinterpret_cast<int32_t*>(address) = value;
                }
            };
            set("Year", local.tm_year + 1900);
            set("Month", local.tm_mon + 1);
            set("Day", local.tm_mday);
            set("Hour", local.tm_hour);
            set("Minute", local.tm_min);
            set("Second", local.tm_sec);
        }

        void nativeZeroInt(UObject*, Frame& stack, void* result)
        {
            while (*stack.code != vm::EX_EndFunctionParms) {
                stack.step(stack.object, nullptr);
            }
            vm::finish(stack);
            vm::ret<int32_t>(result, 0);
        }

        void nativeFalse(UObject*, Frame& stack, void* result)
        {
            while (*stack.code != vm::EX_EndFunctionParms) {
                stack.step(stack.object, nullptr);
            }
            vm::finish(stack);
            vm::retBool(result, false);
        }

        void nativeEmptyString(UObject*, Frame& stack, void* result)
        {
            while (*stack.code != vm::EX_EndFunctionParms) {
                stack.step(stack.object, nullptr);
            }
            vm::finish(stack);
            vm::ret<std::string>(result, std::string());
        }

        float particleRandom()
        {
            static std::mt19937 engine(4242);
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(engine);
        }

        struct ElementLayout {
            UProperty* array = nullptr;
            std::unordered_map<std::string, size_t> members;

            uint8_t* element(UObject* object, int32_t index) const
            {
                return object->data + array->offset + static_cast<size_t>(index) * array->elementSize();
            }

            template <typename T>
            T& at(UObject* object, int32_t index, const char* member) const
            {
                return *reinterpret_cast<T*>(element(object, index) + members.at(member));
            }
        };

        const ElementLayout& elementLayout(UObject* object, const char* arrayName)
        {
            static std::unordered_map<UClass*, std::unordered_map<std::string, ElementLayout>> cache;
            ElementLayout& layout = cache[object->cls][arrayName];
            if (!layout.array) {
                layout.array = object->cls->findProperty(Name(arrayName));
                for (UProperty* member : layout.array->structType->properties) {
                    layout.members[member->name.str()] = member->offset;
                }
            }
            return layout;
        }

        void deleteParticle(UObject* iterator, int32_t index)
        {
            const ElementLayout& particles = elementLayout(iterator, "Particles");
            if (index >= 0 && index < particles.array->arrayDim && particles.at<uint32_t>(iterator, index, "bActive")) {
                particles.at<uint32_t>(iterator, index, "bActive") = 0;
                particleiterator::nextFreeParticle(iterator) = index;
            }
        }

        void nativeUpdateParticles(UObject* self, Frame& stack, void*)
        {
            float deltaTime = vm::get<float>(stack);
            vm::finish(stack);
            const ElementLayout& particles = elementLayout(self, "Particles");
            bool noneActive = true;
            float ownerLifeSpan = particleiterator::OwnerLifeSpan(self);
            float ownerDrawScale = particleiterator::OwnerDrawScale(self);
            for (int32_t i = 0; i < particles.array->arrayDim; i++) {
                if (!particles.at<uint32_t>(self, i, "bActive")) {
                    continue;
                }
                float& lifeSpan = particles.at<float>(self, i, "LifeSpan");
                if (lifeSpan > 0) {
                    lifeSpan -= deltaTime;
                    if (lifeSpan <= 0) {
                        lifeSpan = -1.0f;
                    }
                }
                if (lifeSpan < 0) {
                    deleteParticle(self, i);
                    continue;
                }
                noneActive = false;
                const Vector& initVel = particles.at<Vector>(self, i, "initVel");
                Vector& velocity = particles.at<Vector>(self, i, "Velocity");
                velocity.x = (initVel.x + 2.0f) - particleRandom() * 5.0f;
                velocity.y = (initVel.y + 2.0f) - particleRandom() * 5.0f;
                if (particleiterator::bOwnerUsesGravity(self)) {
                    velocity.z = velocity.z + deltaTime * particleiterator::OwnerZoneGravity(self) * 0.2f;
                } else {
                    velocity.z = initVel.z + (particleRandom() * 0.2f + 0.9f) * ((ownerLifeSpan - lifeSpan) * particleiterator::OwnerRiseRate(self));
                }
                float& drawScale = particles.at<float>(self, i, "DrawScale");
                if (particleiterator::bOwnerScales(self)) {
                    drawScale = std::clamp((ownerLifeSpan - lifeSpan + 1.0f) * ownerDrawScale, 0.01f, 3.0f);
                } else {
                    drawScale = ownerDrawScale;
                }
                particles.at<float>(self, i, "ScaleGlow") = particleiterator::bOwnerFades(self) ? lifeSpan / ownerLifeSpan : 1.0f;
                Vector& location = particles.at<Vector>(self, i, "Location");
                location = location + velocity * deltaTime;
            }
            if (UObject* proxy = particleiterator::proxy(self)) {
                actor::bHidden(proxy) = noneActive ? 1 : 0;
            }
        }

        UObject* particleCurrentItem(World& world, UObject* iterator, int32_t index)
        {
            const ElementLayout& particles = elementLayout(iterator, "Particles");
            UObject* proxy = particleiterator::proxy(iterator);
            if (proxy && index >= 0 && index < particles.array->arrayDim && particles.at<uint32_t>(iterator, index, "bActive")) {
                actor::DrawScale(proxy) = particles.at<float>(iterator, index, "DrawScale");
                actor::ScaleGlow(proxy) = particles.at<float>(iterator, index, "ScaleGlow");
                if (world.farMoveActor(proxy, particles.at<Vector>(iterator, index, "Location"), false, false)) {
                    return proxy;
                }
                deleteParticle(iterator, index);
            }
            return iterator->outer;
        }

        UObject* laserCurrentItem(World& world, UObject* iterator, int32_t index, int32_t maxItems)
        {
            UObject* proxy = *reinterpret_cast<UObject**>(field(iterator, "proxy"));
            if (!proxy) {
                return iterator->outer;
            }
            const ElementLayout& beams = elementLayout(iterator, "Beams");
            int32_t end = 0;
            int32_t selected = 0;
            for (int32_t b = 0; b < beams.array->arrayDim; b++) {
                if (beams.at<uint32_t>(iterator, b, "bActive")) {
                    end += beams.at<int32_t>(iterator, b, "numSegments");
                    if (index < end) {
                        selected = b;
                        break;
                    }
                }
            }
            int32_t segments = beams.at<int32_t>(iterator, selected, "numSegments");
            float t = std::clamp(1.0f - static_cast<float>(end - index) / static_cast<float>(segments), 0.0f, 1.0f);
            Rotator rotation = beams.at<Rotator>(iterator, selected, "Rotation");
            Vector point = beams.at<Vector>(iterator, selected, "Location") + rotationVector(rotation) * (t * beams.at<float>(iterator, selected, "Length"));
            Vector& prevLoc = *reinterpret_cast<Vector*>(field(iterator, "prevLoc"));
            Vector& prevRand = *reinterpret_cast<Vector*>(field(iterator, "prevRand"));
            if (*reinterpret_cast<uint32_t*>(field(iterator, "bRandomBeam"))) {
                Vector v;
                do {
                    v = { 2.0f * particleRandom() - 1.0f, 2.0f * particleRandom() - 1.0f, 2.0f * particleRandom() - 1.0f };
                } while (v.sizeSquared() > 1.0f);
                Vector n = v / v.size();
                point = point + n + prevRand;
                Vector q = (point - prevLoc) + point;
                rotation = vectorRotation(prevLoc - q);
                prevLoc = q;
                prevRand = n;
            }
            CheckResult hit;
            world.farMoveActor(proxy, point, false, false);
            world.moveActor(proxy, {}, rotation, hit, false, false, false, false);
            Vector& savedLoc = *reinterpret_cast<Vector*>(field(iterator, "savedLoc"));
            Rotator& savedRot = *reinterpret_cast<Rotator*>(field(iterator, "savedRot"));
            int32_t& nextItem = *reinterpret_cast<int32_t*>(field(iterator, "nextItem"));
            if (savedLoc == actor::Location(iterator->outer) && static_cast<float>(nextItem) / static_cast<float>(maxItems) > particleRandom()) {
                savedLoc = point;
                savedRot = rotation;
            }
            if (++nextItem == maxItems) {
                world.farMoveActor(proxy, savedLoc, false, false);
                world.moveActor(proxy, {}, savedRot, hit, false, false, false, false);
            }
            return proxy;
        }

        void nativeNothing(UObject*, Frame& stack, void*)
        {
            while (*stack.code != vm::EX_EndFunctionParms) {
                stack.step(stack.object, nullptr);
            }
            vm::finish(stack);
        }

        void nativeGenerateSaveFilename(UObject*, Frame& stack, void* result)
        {
            int32_t index = vm::getOpt<int32_t>(stack, 0);
            vm::finish(stack);
            vm::ret<std::string>(result, savegame::slotDirectory(index).string());
        }

        void linkConversations()
        {
            UObject* mission = state().mission;
            if (!mission) {
                return;
            }
            for (UObject* item = objectField(mission, "conversations"); item; item = objectField(item, "Next")) {
                UObject* conversation = objectField(item, "ConObject");
                if (!conversation) {
                    continue;
                }
                objects::ensureLoaded(conversation);
                uint8_t* eventList = field(conversation, "eventList");
                if (!eventList) {
                    continue;
                }
                UObject*& head = *reinterpret_cast<UObject**>(eventList);
                auto isComment = [](UObject* event) {
                    if (!event) {
                        return false;
                    }
                    uint8_t* type = field(event, "eventType");
                    return type && *type == 17;
                };
                while (head && isComment(head)) {
                    head = objectField(head, "nextEvent");
                }
                for (UObject* event = head; event; event = objectField(event, "nextEvent")) {
                    objects::ensureLoaded(event);
                    uint8_t* next = field(event, "nextEvent");
                    while (next && isComment(*reinterpret_cast<UObject**>(next))) {
                        *reinterpret_cast<UObject**>(next) = objectField(*reinterpret_cast<UObject**>(next), "nextEvent");
                    }
                    uint8_t* giveObject = field(event, "giveObject");
                    uint8_t* checkObject = field(event, "checkObject");
                    uint8_t* objectName = field(event, "ObjectName");
                    if (!objectName) {
                        continue;
                    }
                    std::string className = *reinterpret_cast<std::string*>(objectName);
                    if (className.empty()) {
                        continue;
                    }
                    UClass* cls = className.rfind("NK_", 0) == 0 ? nullptr : objects::findClass("DeusEx." + className);
                    if (giveObject) {
                        *reinterpret_cast<UObject**>(giveObject) = cls;
                    }
                    if (checkObject) {
                        *reinterpret_cast<UObject**>(checkObject) = cls;
                    }
                }
            }
        }
    }

    namespace deusex {
        bool renderIterate(World& world, UObject* owner, const std::function<void(UObject* item)>& emit)
        {
            UObject* cls = actor::RenderIteratorClass(owner);
            UObject*& iterator = actor::RenderInterface(owner);
            if (iterator && iterator->destroyed) {
                iterator = nullptr;
            }
            if (cls && !iterator) {
                iterator = objects::newObject(cast<UClass>(cls), owner, Name(), RF_Transient);
            } else if (!cls && iterator) {
                objects::destroyObject(iterator);
                iterator = nullptr;
            }
            if (!iterator) {
                return false;
            }
            UObject* player = world.player();
            static const Name nameInit("Init");
            vm::Call init(iterator, nameInit);
            if (init.valid()) {
                init.obj(player).run();
            }
            *reinterpret_cast<UObject**>(field(iterator, "Observer")) = player;
            static const Name nameParticleIterator("ParticleIterator");
            static const Name nameLaserIterator("LaserIterator");
            int32_t& index = *reinterpret_cast<int32_t*>(field(iterator, "Index"));
            int32_t& maxItems = *reinterpret_cast<int32_t*>(field(iterator, "MaxItems"));
            for (index = 0; index < maxItems; index++) {
                UObject* item = iterator->outer;
                if (iterator->isA(nameParticleIterator)) {
                    item = particleCurrentItem(world, iterator, index);
                } else if (iterator->isA(nameLaserIterator)) {
                    item = laserCurrentItem(world, iterator, index, maxItems);
                }
                emit(item);
            }
            return true;
        }

        bool scriptedPawnTick(World& world, UObject* pawn, float deltaSeconds)
        {
            float lastRendered = std::max(world.timeSeconds - actor::LastRenderTime(pawn), 0.0f);
            if (scriptedpawn::bDisappear(pawn) && lastRendered > 5.0f) {
                world.destroy(pawn);
                return false;
            }
            float& prePivotTime = scriptedpawn::PrePivotTime(pawn);
            if (prePivotTime > 0 && deltaSeconds < prePivotTime) {
                actor::PrePivot(pawn) = actor::PrePivot(pawn) + (scriptedpawn::DesiredPrePivot(pawn) - actor::PrePivot(pawn)) * deltaSeconds / prePivotTime;
                prePivotTime -= deltaSeconds;
            } else {
                actor::PrePivot(pawn) = scriptedpawn::DesiredPrePivot(pawn);
                prePivotTime = 0;
            }
            bool lowPriority = !(scriptedpawn::bTickVisibleOnly(pawn) && actor::DistanceFromPlayer(pawn) > 1200.0f);

            auto decrement = [deltaSeconds](float& timer) {
                timer = std::max(timer - deltaSeconds, 0.0f);
            };
            auto sustainDecay = [deltaSeconds](float& timer, float rate) {
                float decay = 0;
                if (timer > 0) {
                    if (timer < deltaSeconds) {
                        decay = (1.0f - timer / deltaSeconds) * deltaSeconds * rate;
                        timer = 0;
                    } else {
                        timer -= deltaSeconds;
                    }
                } else {
                    decay = deltaSeconds * rate;
                }
                return decay;
            };

            decrement(scriptedpawn::AgitationCheckTimer(pawn));
            float agitationDecay = sustainDecay(scriptedpawn::AgitationTimer(pawn), scriptedpawn::AgitationDecayRate(pawn));
            if (scriptedpawn::bAlliancesChanged(pawn) && agitationDecay > 0) {
                scriptedpawn::bAlliancesChanged(pawn) = 0;
                static const Name nameAlliancesEx("AlliancesEx");
                UProperty* property = pawn->cls->findProperty(nameAlliancesEx);
                if (property && property->structType) {
                    static const Name nameAllianceName("AllianceName");
                    static const Name nameAgitationLevel("AgitationLevel");
                    static const Name namePermanent("bPermanent");
                    UProperty* nameProperty = property->structType->findProperty(nameAllianceName);
                    UProperty* agitationProperty = property->structType->findProperty(nameAgitationLevel);
                    UProperty* permanentProperty = property->structType->findProperty(namePermanent);
                    for (int32_t i = 0; i < property->arrayDim; i++) {
                        uint8_t* entry = pawn->data + property->offset + static_cast<size_t>(i) * property->elementSize();
                        float& agitation = *reinterpret_cast<float*>(entry + agitationProperty->offset);
                        bool permanent = *reinterpret_cast<uint32_t*>(entry + permanentProperty->offset) != 0;
                        if (*reinterpret_cast<Name*>(entry + nameProperty->offset) != Name() && !permanent && agitation > 0) {
                            scriptedpawn::bAlliancesChanged(pawn) = 1;
                            agitation = std::max(agitation - agitationDecay, 0.0f);
                        }
                    }
                }
            }
            float fearDecay = sustainDecay(scriptedpawn::FearTimer(pawn), scriptedpawn::FearDecayRate(pawn));
            scriptedpawn::FearLevel(pawn) = std::max(scriptedpawn::FearLevel(pawn) - fearDecay, 0.0f);

            decrement(scriptedpawn::AlarmTimer(pawn));
            if (pawn::Weapon(pawn)) {
                scriptedpawn::WeaponTimer(pawn) += deltaSeconds;
            } else {
                scriptedpawn::WeaponTimer(pawn) = 0;
            }
            decrement(scriptedpawn::FireTimer(pawn));
            decrement(scriptedpawn::SpecialTimer(pawn));
            if (pawn::Weapon(pawn)) {
                scriptedpawn::ReloadTimer(pawn) -= deltaSeconds;
            } else {
                scriptedpawn::ReloadTimer(pawn) = 0;
            }
            decrement(scriptedpawn::AvoidWallTimer(pawn));
            decrement(scriptedpawn::AvoidBumpTimer(pawn));
            decrement(scriptedpawn::ObstacleTimer(pawn));
            decrement(scriptedpawn::CloakEMPTimer(pawn));
            decrement(scriptedpawn::TakeHitTimer(pawn));
            decrement(scriptedpawn::CarcassCheckTimer(pawn));
            float& potentialEnemyTimer = scriptedpawn::PotentialEnemyTimer(pawn);
            potentialEnemyTimer -= deltaSeconds;
            if (potentialEnemyTimer <= 0) {
                potentialEnemyTimer = 0;
                scriptedpawn::PotentialEnemyAlliance(pawn) = Name();
            }
            decrement(scriptedpawn::BeamCheckTimer(pawn));
            decrement(scriptedpawn::FutzTimer(pawn));
            decrement(scriptedpawn::PlayerAgitationTimer(pawn));
            float& distressTimer = scriptedpawn::DistressTimer(pawn);
            if (distressTimer >= 0) {
                distressTimer += deltaSeconds;
                if (distressTimer > scriptedpawn::FearSustainTime(pawn)) {
                    distressTimer = -1;
                }
            }

            if (scriptedpawn::bHasCloak(pawn)) {
                static const Name nameEnableCloak("EnableCloak");
                vm::Call call(pawn, nameEnableCloak);
                if (call.valid()) {
                    call.b(pawn::Health(pawn) <= scriptedpawn::CloakThreshold(pawn)).run();
                }
            }
            if (pawn::bAdvancedTactics(pawn) && (actor::Acceleration(pawn).zero() || actor::Physics(pawn) != PHYS_Walking || scriptedpawn::TurnDirection(pawn) == 0)) {
                pawn::bAdvancedTactics(pawn) = 0;
                if (scriptedpawn::TurnDirection(pawn) != 0) {
                    pawn::MoveTimer(pawn) -= 4.0f;
                }
                scriptedpawn::ActorAvoiding(pawn) = nullptr;
                scriptedpawn::NextDirection(pawn) = 0;
                scriptedpawn::TurnDirection(pawn) = 0;
                scriptedpawn::bClearedObstacle(pawn) = 1;
                scriptedpawn::ObstacleTimer(pawn) = 0;
            }
            if (pawn::bOnFire(pawn)) {
                pawn::burnTimer(pawn) += deltaSeconds;
                if (pawn::burnTimer(pawn) >= scriptedpawn::BurnPeriod(pawn)) {
                    static const Name nameExtinguishFire("ExtinguishFire");
                    vm::Call(pawn, nameExtinguishFire).run();
                }
            }
            if (pawn->destroyed) {
                return false;
            }
            if (lowPriority && scriptedpawn::BleedRate(pawn) > 0 && scriptedpawn::bCanBleed(pawn)) {
                float speed = std::clamp(actor::Velocity(pawn).size() / 512.0f, 0.05f, 1.0f);
                float period = ((1.0f - std::clamp(scriptedpawn::BleedRate(pawn), 0.0f, 1.0f)) + 0.1f) / speed;
                float& dropCounter = scriptedpawn::DropCounter(pawn);
                dropCounter += deltaSeconds;
                static const Name nameSpurtBlood("SpurtBlood");
                while (dropCounter >= period && !pawn->destroyed) {
                    vm::Call(pawn, nameSpurtBlood).run();
                    dropCounter -= period;
                }
                scriptedpawn::BleedRate(pawn) -= deltaSeconds / scriptedpawn::ClotPeriod(pawn);
                if (scriptedpawn::BleedRate(pawn) <= 0) {
                    dropCounter = 0;
                    scriptedpawn::BleedRate(pawn) = 0;
                }
            }
            return !pawn->destroyed;
        }

        void levelLoaded(World& world)
        {
            DeusExState& s = state();
            s.levelInfo = nullptr;
            s.mission = nullptr;
            UClass* levelInfoClass = objects::findClass("DeusEx.DeusExLevelInfo");
            for (UObject* a : world.actors) {
                if (a && levelInfoClass && a->isA(levelInfoClass)) {
                    s.levelInfo = a;
                    break;
                }
            }
            if (!s.missionList) {
                objects::loadPackage(Name("DeusExConText"));
                s.missionList = objects::findObject("DeusExConText.ConMissionList", nullptr);
                objects::flushLoads();
            }
            if (!s.missionList || !s.levelInfo) {
                return;
            }
            int32_t missionNumber = deusexlevelinfo::missionNumber(s.levelInfo);
            for (UObject* item = objectField(s.missionList, "missions"); item; item = objectField(item, "Next")) {
                UObject* mission = objectField(item, "ConObject");
                if (!mission) {
                    continue;
                }
                objects::ensureLoaded(mission);
                uint8_t* number = field(mission, "missionNumber");
                if (number && *reinterpret_cast<int32_t*>(number) == missionNumber) {
                    s.mission = mission;
                    break;
                }
            }
            objects::flushLoads();
            linkConversations();
        }
    }

    namespace engine {
        void registerDeusExNatives()
        {
            vm::registerNative(2100, nativeConBindEvents);
            vm::registerNative(2101, nativeConBindEvents);
            vm::registerNative(2102, nativeConBindEvents);
            vm::registerNative(2106, nativeGetAllianceType);
            vm::registerNative(2107, nativeGetPawnAllianceType);
            vm::registerNative(2105, nativeIsValidEnemy);
            vm::registerNative(2109, nativeAddCarcass);
            vm::registerNative(2108, nativeHaveSeenCarcass);
            vm::registerNative(3015, nativeCreateDumpLocation);
            vm::registerNative(3014, nativeCreateDataVaultImageNote);
            vm::registerNative(3013, nativeCreateGameDirectory);
            vm::registerNative(3010, nativeCreateLog);
            vm::registerNative(3003, nativeCreateHistoryEvent);
            vm::registerNative(3002, nativeCreateHistory);
            vm::registerNative(3001, nativeSetBoolFlagFromString);
            vm::registerNative(1099, nativeGetDeusExVersion);
            vm::registerNative(3011, nativeSaveGame);
            vm::registerNative(3012, nativeDeleteSaveGameFiles);
            vm::registerNative(3016, nativeUnloadTexture);
            vm::registerNative(3075, nativeUpdateTimeStamp);
            vm::registerNative(3017, nativeUpdateParticles);
            for (int32_t index = 3020; index <= 3040; index++) {
                vm::registerNative(index, nativeNothing);
            }
            vm::registerNative(3020, nativeEmptyString);
            vm::registerNative(3021, nativeEmptyString);
            vm::registerNative(3022, nativeZeroInt);
            vm::registerNative(3023, nativeZeroInt);
            vm::registerNative(3024, nativeFalse);
            vm::registerNative(3027, nativeFalse);
            vm::registerNative(3028, nativeFalse);
            vm::registerNative(3029, nativeZeroInt);
            vm::registerNative(3030, nativeFalse);
            vm::registerNative(3034, nativeZeroInt);
            vm::registerNative(3035, nativeEmptyString);
            vm::registerNative(3039, nativeFalse);
            vm::registerNative(3040, nativeZeroInt);
            vm::registerNative(3080, nativeGetGameDirectory);
            vm::registerNative(3081, nativeGetNewSaveFileIndex);
            vm::registerNative(3082, nativeGenerateSaveFilename);
            vm::registerNative(3083, nativeGenerateNewSaveFilename);
            vm::registerNative(3084, nativeGetDirCount);
            vm::registerNative(3085, nativeGetDirFilename);
            vm::registerNative(3086, nativeSetDirType);
            vm::registerNative(3087, nativeSetDirFilter);
            vm::registerNative(3088, nativeGetSaveInfo);
            vm::registerNative(3089, nativeGetSaveInfoFromDirectoryIndex);
            vm::registerNative(3090, nativeGetTempSaveInfo);
            vm::registerNative(3091, nativeNothing);
            vm::registerNative(3092, nativeNothing);
            vm::registerNative(3093, nativeGetSaveFreeSpace);
            vm::registerNative(3094, nativeGetSaveDirectorySize);
        }
    }
}
