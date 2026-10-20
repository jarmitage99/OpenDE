#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/canvas_hooks.h"
#include "engine/console.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace dx {
    CanvasHooks& canvasHooks()
    {
        static CanvasHooks hooks;
        return hooks;
    }

    LinearColor canvasColor(UObject* canvas)
    {
        Color color = dx::canvas::DrawColor(canvas);
        return { static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f, static_cast<float>(color.b) / 255.0f, 1.0f };
    }

    namespace {
        float sinTab(int32_t angle)
        {
            return std::sin(static_cast<float>((angle >> 2) & 16383) * (2.0f * Pi / 16384.0f));
        }

        float cosTab(int32_t angle)
        {
            return sinTab(angle + 16384);
        }
    }

    Vector transformPointBy(const Vector& point, const Coords& coords)
    {
        Vector local = point - coords.origin;
        return { local.dot(coords.xAxis), local.dot(coords.yAxis), local.dot(coords.zAxis) };
    }

    Vector transformVectorBy(const Vector& vector, const Coords& coords)
    {
        return { vector.dot(coords.xAxis), vector.dot(coords.yAxis), vector.dot(coords.zAxis) };
    }

    Coords coordsTranspose(const Coords& coords)
    {
        return { -transformVectorBy(coords.origin, coords), { coords.xAxis.x, coords.yAxis.x, coords.zAxis.x }, { coords.xAxis.y, coords.yAxis.y, coords.zAxis.y }, { coords.xAxis.z, coords.yAxis.z, coords.zAxis.z } };
    }

    Coords coordsTransform(const Coords& coords, const Coords& by)
    {
        return { transformPointBy(coords.origin, by), transformVectorBy(coords.xAxis, by), transformVectorBy(coords.yAxis, by), transformVectorBy(coords.zAxis, by) };
    }

    Coords coordsRotate(Coords coords, const Rotator& rotation)
    {
        coords = coordsTransform(coords, { {}, { cosTab(rotation.yaw), sinTab(rotation.yaw), 0 }, { -sinTab(rotation.yaw), cosTab(rotation.yaw), 0 }, { 0, 0, 1 } });
        coords = coordsTransform(coords, { {}, { cosTab(rotation.pitch), 0, sinTab(rotation.pitch) }, { 0, 1, 0 }, { -sinTab(rotation.pitch), 0, cosTab(rotation.pitch) } });
        return coordsTransform(coords, { {}, { 1, 0, 0 }, { 0, cosTab(rotation.roll), -sinTab(rotation.roll) }, { 0, sinTab(rotation.roll), cosTab(rotation.roll) } });
    }

    Coords coordsUnrotate(Coords coords, const Rotator& rotation)
    {
        coords = coordsTransform(coords, { {}, { 1, 0, 0 }, { 0, cosTab(rotation.roll), sinTab(rotation.roll) }, { 0, -sinTab(rotation.roll), cosTab(rotation.roll) } });
        coords = coordsTransform(coords, { {}, { cosTab(rotation.pitch), 0, -sinTab(rotation.pitch) }, { 0, 1, 0 }, { sinTab(rotation.pitch), 0, cosTab(rotation.pitch) } });
        return coordsTransform(coords, { {}, { cosTab(rotation.yaw), -sinTab(rotation.yaw), 0 }, { sinTab(rotation.yaw), cosTab(rotation.yaw), 0 }, { 0, 0, 1 } });
    }

    Rotator orthoRotation(const Coords& coords)
    {
        Rotator rotation;
        rotation.pitch = static_cast<int32_t>(std::atan2(static_cast<double>(coords.xAxis.z), std::sqrt(static_cast<double>(coords.xAxis.x * coords.xAxis.x + coords.xAxis.y * coords.xAxis.y))) * 32768.0 / static_cast<double>(Pi));
        rotation.yaw = static_cast<int32_t>(std::atan2(static_cast<double>(coords.xAxis.y), static_cast<double>(coords.xAxis.x)) * 32768.0 / static_cast<double>(Pi));
        rotation.roll = 0;
        Coords basis = coordsUnrotate({ {}, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, rotation);
        rotation.roll = static_cast<int32_t>(std::atan2(static_cast<double>(coords.zAxis.dot(basis.yAxis)), static_cast<double>(coords.yAxis.dot(basis.yAxis))) * 32768.0 / static_cast<double>(Pi));
        return rotation;
    }

    namespace {
        BlendStyle styleOf(uint8_t style)
        {
            switch (style) {
            case 0:
                return BlendStyle::None;
            case 2:
                return BlendStyle::Masked;
            case 3:
                return BlendStyle::Translucent;
            case 4:
                return BlendStyle::Modulated;
            default:
                return BlendStyle::Normal;
            }
        }

        UFont* fontOf(UObject* canvasObject)
        {
            UFont* font = cast<UFont>(dx::canvas::Font(canvasObject));
            objects::ensureLoaded(font);
            return font;
        }

        float wrappedSize(UFont* font, const std::string& text, float clipWidth, float spacing, std::vector<std::string>* lines, float& width)
        {
            float height = 0;
            width = 0;
            float lineHeight = textHeight(font);
            size_t lineStart = 0;
            while (lineStart < text.size()) {
                size_t best = lineStart;
                size_t probe = lineStart;
                while (true) {
                    size_t next = probe;
                    while (next < text.size() && text[next] != ' ') {
                        next++;
                    }
                    while (next < text.size() && text[next] == ' ') {
                        next++;
                    }
                    if (next == probe) {
                        break;
                    }
                    float w = textWidth(font, std::string_view(text).substr(lineStart, next - lineStart), spacing);
                    if (w > clipWidth && best != lineStart) {
                        break;
                    }
                    best = next;
                    probe = next;
                    if (w > clipWidth || next >= text.size()) {
                        break;
                    }
                }
                if (best == lineStart) {
                    best = text.size();
                }
                std::string line = text.substr(lineStart, best - lineStart);
                width = std::max(width, textWidth(font, line, spacing));
                height += lineHeight;
                if (lines) {
                    lines->push_back(line);
                }
                lineStart = best;
                while (lineStart < text.size() && text[lineStart] == ' ') {
                    lineStart++;
                }
            }
            if (text.empty()) {
                height = lineHeight;
            }
            return height;
        }

        void nativeStrLen(UObject* self, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            float* xl = vm::getRef<float>(stack);
            float* yl = vm::getRef<float>(stack);
            vm::finish(stack);
            float width = 0;
            float curX = static_cast<float>(static_cast<int32_t>(dx::canvas::CurX(self)));
            float curY = static_cast<float>(static_cast<int32_t>(dx::canvas::CurY(self)));
            float height = wrappedSize(fontOf(self), text, dx::canvas::ClipX(self), dx::canvas::SpaceX(self), nullptr, width);
            dx::canvas::CurX(self) = curX;
            dx::canvas::CurY(self) = curY;
            *xl = static_cast<float>(static_cast<int32_t>(width));
            *yl = static_cast<float>(static_cast<int32_t>(height));
        }

        void nativeTextSize(UObject* self, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            float* xl = vm::getRef<float>(stack);
            float* yl = vm::getRef<float>(stack);
            vm::finish(stack);
            UFont* font = fontOf(self);
            if (!font) {
                return;
            }
            int32_t width = 0;
            int32_t height = 0;
            for (char c : text) {
                const FontCharacter* glyph = font->glyph(static_cast<uint8_t>(c), nullptr);
                if (glyph) {
                    width += static_cast<int32_t>(glyph->width);
                    height = std::max(height, static_cast<int32_t>(glyph->height));
                }
            }
            *xl = static_cast<float>(width);
            *yl = static_cast<float>(height);
        }

        void nativeDrawText(UObject* self, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            bool carriageReturn = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            UFont* font = fontOf(self);
            if (!font) {
                return;
            }
            float clipX = dx::canvas::ClipX(self);
            float spacing = dx::canvas::SpaceX(self);
            std::vector<std::string> lines;
            float width = 0;
            float height = 0;
            uint8_t style = dx::canvas::Style(self);
            if (style != 0) {
                height = static_cast<float>(static_cast<int32_t>(wrappedSize(font, text, clipX, spacing, &lines, width)));
                width = static_cast<float>(static_cast<int32_t>(width));
            }
            Canvas2D* target = canvasHooks().target;
            if (target && style != 0) {
                float y = dx::canvas::CurY(self);
                float lineHeight = textHeight(font);
                for (const std::string& line : lines) {
                    float x = dx::canvas::CurX(self);
                    if (dx::canvas::bCenter(self)) {
                        x = dx::canvas::CurX(self) + (clipX - textWidth(font, line, spacing)) / 2;
                    }
                    drawText(*target, font, line, dx::canvas::OrgX(self) + x, dx::canvas::OrgY(self) + y, canvasColor(self), styleOf(style), spacing);
                    y += lineHeight;
                }
            }
            dx::canvas::CurX(self) += width;
            dx::canvas::CurYL(self) = std::max(dx::canvas::CurYL(self), height);
            if (carriageReturn) {
                dx::canvas::CurX(self) = 0;
                dx::canvas::CurY(self) += dx::canvas::CurYL(self);
                dx::canvas::CurYL(self) = 0;
            }
        }

        void nativeDrawTextClipped(UObject* self, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            vm::getBoolOpt(stack, false);
            vm::finish(stack);
            UFont* font = fontOf(self);
            Canvas2D* target = canvasHooks().target;
            if (!font || !target || dx::canvas::Style(self) == 0) {
                return;
            }
            float orgX = dx::canvas::OrgX(self);
            float orgY = dx::canvas::OrgY(self);
            target->setClip(orgX, orgY, dx::canvas::ClipX(self), dx::canvas::ClipY(self));
            drawText(*target, font, text, orgX + dx::canvas::CurX(self), orgY + dx::canvas::CurY(self), canvasColor(self), styleOf(dx::canvas::Style(self)), dx::canvas::SpaceX(self));
            target->clearClip();
        }

        void nativeDrawTile(UObject* self, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            float xl = vm::get<float>(stack);
            float yl = vm::get<float>(stack);
            float u = vm::get<float>(stack);
            float v = vm::get<float>(stack);
            float ul = vm::get<float>(stack);
            float vl = vm::get<float>(stack);
            vm::finish(stack);
            UTexture* tex = cast<UTexture>(texture);
            Canvas2D* target = canvasHooks().target;
            uint8_t style = dx::canvas::Style(self);
            if (!tex) {
                return;
            }
            if (target && style != 0) {
                objects::ensureLoaded(tex);
                target->drawTile(tex, dx::canvas::OrgX(self) + dx::canvas::CurX(self), dx::canvas::OrgY(self) + dx::canvas::CurY(self), xl, yl, u, v, ul, vl, canvasColor(self), styleOf(style), !dx::canvas::bNoSmooth(self));
            }
            dx::canvas::CurX(self) += xl + dx::canvas::SpaceX(self);
            dx::canvas::CurYL(self) = std::max(dx::canvas::CurYL(self), yl);
        }

        void nativeDrawTileClipped(UObject* self, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            float xl = vm::get<float>(stack);
            float yl = vm::get<float>(stack);
            float u = vm::get<float>(stack);
            float v = vm::get<float>(stack);
            float ul = vm::get<float>(stack);
            float vl = vm::get<float>(stack);
            vm::finish(stack);
            UTexture* tex = cast<UTexture>(texture);
            Canvas2D* target = canvasHooks().target;
            uint8_t style = dx::canvas::Style(self);
            if (!tex || xl <= 0 || yl <= 0) {
                return;
            }
            objects::ensureLoaded(tex);
            float curX = dx::canvas::CurX(self);
            float curY = dx::canvas::CurY(self);
            float clipX = dx::canvas::ClipX(self);
            float clipY = dx::canvas::ClipY(self);
            if (curX < 0) {
                float c = curX * ul / xl;
                u -= c;
                ul += c;
                xl += curX;
                curX = 0;
                dx::canvas::CurX(self) = 0;
            }
            if (curY < 0) {
                float c = curY * vl / yl;
                v -= c;
                vl += c;
                yl += curY;
                curY = 0;
                dx::canvas::CurY(self) = 0;
            }
            if (xl > clipX - curX) {
                ul += (clipX - curX - xl) * ul / xl;
                xl = clipX - curX;
            }
            if (yl > clipY - curY) {
                vl += (clipY - curY - yl) * vl / yl;
                yl = clipY - curY;
            }
            if (target && style != 0 && xl > 0 && yl > 0) {
                target->drawTile(tex, dx::canvas::OrgX(self) + curX, dx::canvas::OrgY(self) + curY, xl, yl, u, v, ul, vl, canvasColor(self), styleOf(style), !dx::canvas::bNoSmooth(self));
            }
            dx::canvas::CurX(self) = dx::canvas::SpaceX(self) + dx::canvas::CurX(self) + xl;
            dx::canvas::CurYL(self) = std::max(dx::canvas::CurYL(self), yl);
        }

        void nativeDrawActor(UObject*, Frame& stack, void*)
        {
            UObject* a = vm::get<UObject*>(stack);
            vm::getBool(stack);
            bool clearZ = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            if (!a) {
                return;
            }
            actor::bHidden(a) = 0;
            if (canvasHooks().drawActor) {
                canvasHooks().drawActor(a, clearZ);
            }
            actor::bHidden(a) = 1;
        }

        void nativeDrawClippedActor(UObject*, Frame& stack, void*)
        {
            UObject* a = vm::get<UObject*>(stack);
            vm::getBool(stack);
            vm::get<int32_t>(stack);
            vm::get<int32_t>(stack);
            vm::get<int32_t>(stack);
            vm::get<int32_t>(stack);
            bool clearZ = vm::getBoolOpt(stack, false);
            vm::finish(stack);
            if (!a) {
                return;
            }
            actor::bHidden(a) = 0;
            if (canvasHooks().drawActor) {
                canvasHooks().drawActor(a, clearZ);
            }
            actor::bHidden(a) = 1;
        }

        void nativeDrawPortal(UObject*, Frame& stack, void*)
        {
            int32_t x = vm::get<int32_t>(stack);
            int32_t y = vm::get<int32_t>(stack);
            int32_t width = vm::get<int32_t>(stack);
            int32_t height = vm::get<int32_t>(stack);
            UObject* camera = vm::get<UObject*>(stack);
            Vector location = vm::get<Vector>(stack);
            Rotator rotation = vm::get<Rotator>(stack);
            int32_t fov = vm::getOpt<int32_t>(stack, 90);
            bool clearZ = vm::getBoolOpt(stack, true);
            vm::finish(stack);
            if (canvasHooks().drawPortal) {
                canvasHooks().drawPortal(x, y, width, height, camera, location, rotation, static_cast<float>(fov), clearZ);
            }
        }

        void nativeZoneActors(UObject* self, Frame& stack, void*)
        {
            UClass* cls = cast<UClass>(vm::get<UObject*>(stack));
            UObject** out = vm::getRef<UObject*>(stack);
            vm::finish(stack);
            size_t index = 0;
            vm::iterate(stack, [&]() {
                while (World::current && index < World::current->actors.size()) {
                    UObject* a = World::current->actors[index++];
                    if (!a || a->destroyed || (cls && !a->isA(cls))) {
                        continue;
                    }
                    UObject* zone = region(a).zone;
                    if (zone != World::current->levelInfo ? zone == self : true) {
                        *out = a;
                        return true;
                    }
                }
                *out = nullptr;
                return false;
            });
        }

        void nativeInitEventManager(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            UObject*& manager = levelinfo::EventManager(self);
            if (!manager) {
                UEventManager* created = static_cast<UEventManager*>(objects::newObject(objects::intrinsicClass(Name("EventManager")), self->outer, Name(), 0));
                created->level = cast<ULevel>(actor::XLevel(self));
                manager = created;
            }
        }

        void nativeGetLocalURL(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<std::string>(result, World::current ? World::current->url : std::string());
        }

        void nativeGetAddressURL(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            std::string port = config::system().get("URL", "Port").value_or("7777");
            vm::ret<std::string>(result, ":" + std::to_string(std::atoi(port.c_str())));
        }

        void nativeDescribeSpec(UObject*, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            UObject** start = vm::getRef<UObject*>(stack);
            UObject** end = vm::getRef<UObject*>(stack);
            int32_t* flags = vm::getRef<int32_t>(stack);
            int32_t* distance = vm::getRef<int32_t>(stack);
            vm::finish(stack);
            *start = nullptr;
            *end = nullptr;
            *flags = 0;
            *distance = 0;
            if (World::current && World::current->level && index >= 0 && static_cast<size_t>(index) < World::current->level->reachSpecs.size()) {
                const ReachSpec& spec = World::current->level->reachSpecs[static_cast<size_t>(index)];
                *start = spec.start;
                *end = spec.end;
                *flags = spec.reachFlags;
                *distance = spec.distance;
            }
        }

        void nativeConsoleCommandBool(UObject* self, Frame& stack, void* result)
        {
            std::string command = vm::get<std::string>(stack);
            vm::finish(stack);
            std::string output = console::execute(self, command);
            if (!output.empty()) {
                vm::Call(self, Name("Message")).obj(nullptr).s(output).n(Name("Console")).run();
            }
            vm::retBool(result, true);
        }

        void nativePlayerConsoleCommand(UObject* self, Frame& stack, void* result)
        {
            std::string command = vm::get<std::string>(stack);
            vm::finish(stack);
            vm::ret<std::string>(result, console::execute(self, command));
        }

        void nativeClientTravel(UObject* self, Frame& stack, void*)
        {
            std::string url = vm::get<std::string>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            bool items = vm::getBool(stack);
            vm::finish(stack);
            if (!World::current || !playerpawn::Player(self)) {
                return;
            }
            vm::event(self, Name("PreClientTravel"));
            World::current->travelUrl = url;
            World::current->travelType = type;
            World::current->travelItems = items;
            World::current->travelPending = true;
            for (UObject* a : std::vector<UObject*>(World::current->actors)) {
                if (a && !a->destroyed) {
                    vm::Call preTravel(a, Name("PreTravel"));
                    if (preTravel.valid()) {
                        preTravel.run();
                    }
                }
            }
        }

        void nativeUpdateURL(UObject*, Frame& stack, void*)
        {
            std::string option = vm::get<std::string>(stack);
            std::string value = vm::get<std::string>(stack);
            bool saveDefault = vm::getBool(stack);
            vm::finish(stack);
            if (World::current) {
                std::string& url = World::current->url;
                std::string replaced;
                bool found = false;
                size_t mark = url.find('?');
                replaced = url.substr(0, mark);
                while (mark != std::string::npos) {
                    size_t next = url.find('?', mark + 1);
                    std::string item = url.substr(mark + 1, next == std::string::npos ? std::string::npos : next - mark - 1);
                    if (iequals(item.substr(0, item.find('=')), option)) {
                        item = option + "=" + value;
                        found = true;
                    }
                    replaced += "?" + item;
                    mark = next;
                }
                if (!found) {
                    replaced += "?" + option + "=" + value;
                }
                url = replaced;
            }
            if (saveDefault) {
                config::user().set("DefaultPlayer", option, value);
            }
        }

        void nativeGetDefaultURL(UObject*, Frame& stack, void* result)
        {
            std::string option = vm::get<std::string>(stack);
            vm::finish(stack);
            auto value = config::user().get("DefaultPlayer", option);
            vm::ret<std::string>(result, value ? *value : std::string());
        }

        void nativeResetKeyboard(UObject*, Frame& stack, void*)
        {
            vm::finish(stack);
            console::loadBindings();
        }

        void nativeGetEntryLevel(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<UObject*>(result, World::current ? World::current->levelInfo : nullptr);
        }

        void nativeCopyToClipboard(UObject*, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            if (canvasHooks().clipboardCopy) {
                canvasHooks().clipboardCopy(text);
            }
        }

        void nativePasteFromClipboard(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<std::string>(result, canvasHooks().clipboardPaste ? canvasHooks().clipboardPaste() : std::string());
        }

        void nativeEmptyString(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<std::string>(result, std::string());
        }

        void nativeParseKillMessage(UObject*, Frame& stack, void* result)
        {
            std::string killer = vm::get<std::string>(stack);
            std::string victim = vm::get<std::string>(stack);
            std::string weapon = vm::get<std::string>(stack);
            std::string message = vm::get<std::string>(stack);
            vm::finish(stack);
            auto replace = [&message](const std::string& token, const std::string& value) {
                size_t at = message.find(token);
                while (at != std::string::npos) {
                    message.replace(at, token.size(), value);
                    at = message.find(token, at + value.size());
                }
            };
            replace("%k", killer);
            replace("%o", victim);
            replace("%w", weapon);
            vm::ret<std::string>(result, message);
        }

        void warpBy(Frame& stack, const Coords& coords)
        {
            Vector* location = vm::getRef<Vector>(stack);
            Vector* velocity = vm::getRef<Vector>(stack);
            Rotator* rotation = vm::getRef<Rotator>(stack);
            vm::finish(stack);
            *location = transformPointBy(*location, coords);
            *velocity = transformVectorBy(*velocity, coords);
            *rotation = orthoRotation(coordsTransform(coordsUnrotate({ {}, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }, *rotation), coords));
        }

        void nativeWarp(UObject* self, Frame& stack, void*)
        {
            warpBy(stack, coordsTranspose(warpzoneinfo::WarpCoords(self)));
        }

        void nativeUnWarp(UObject* self, Frame& stack, void*)
        {
            warpBy(stack, warpzoneinfo::WarpCoords(self));
        }

        void nativeAttachDecal(UObject* self, Frame& stack, void* result)
        {
            float traceDistance = vm::get<float>(stack);
            Vector decalDir = vm::getOpt<Vector>(stack, Vector {});
            vm::finish(stack);
            vm::retBool(result, World::current && attachDecal(*World::current, self, traceDistance, decalDir));
        }

        void nativeDetachDecal(UObject* self, Frame& stack, void*)
        {
            vm::finish(stack);
            if (World::current) {
                detachDecal(*World::current, self);
            }
        }
    }

    void registerMiscNatives()
    {
        vm::registerNative(464, nativeStrLen);
        vm::registerNative(465, nativeDrawText);
        vm::registerNative(466, nativeDrawTile);
        vm::registerNative(467, nativeDrawActor);
        vm::registerNative(468, nativeDrawTileClipped);
        vm::registerNative(469, nativeDrawTextClipped);
        vm::registerNative(470, nativeTextSize);
        vm::registerNative(471, nativeDrawClippedActor);
        vm::registerNative(480, nativeDrawPortal);
        vm::registerNative(308, nativeZoneActors);
        vm::registerNative(650, nativeInitEventManager);
        vm::registerNative("LevelInfo", "GetLocalURL", nativeGetLocalURL);
        vm::registerNative("LevelInfo", "GetAddressURL", nativeGetAddressURL);
        vm::registerNative(519, nativeDescribeSpec);
        vm::registerNative("Console", "ConsoleCommand", nativeConsoleCommandBool);
        vm::registerNative("PlayerPawn", "ConsoleCommand", nativePlayerConsoleCommand);
        vm::registerNative("PlayerPawn", "ClientTravel", nativeClientTravel);
        vm::registerNative(546, nativeUpdateURL);
        vm::registerNative("PlayerPawn", "GetDefaultURL", nativeGetDefaultURL);
        vm::registerNative(544, nativeResetKeyboard);
        vm::registerNative("PlayerPawn", "GetEntryLevel", nativeGetEntryLevel);
        vm::registerNative("PlayerPawn", "CopyToClipboard", nativeCopyToClipboard);
        vm::registerNative("PlayerPawn", "PasteFromClipboard", nativePasteFromClipboard);
        vm::registerNative("PlayerPawn", "GetPlayerNetworkAddress", nativeEmptyString);
        vm::registerNative("GameInfo", "GetNetworkNumber", nativeEmptyString);
        vm::registerNative("GameInfo", "ParseKillMessage", nativeParseKillMessage);
        vm::registerNative(314, nativeWarp);
        vm::registerNative(315, nativeUnWarp);
        vm::registerNative("Decal", "AttachDecal", nativeAttachDecal);
        vm::registerNative("Decal", "DetachDecal", nativeDetachDecal);
    }
}
