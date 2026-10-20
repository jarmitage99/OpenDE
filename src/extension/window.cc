#include "extension/windows.h"

#include "audio/audio.h"
#include "core/config.h"
#include "core/objects.h"
#include "engine/input.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace dx::extension {
    namespace {
        bool runBool(vm::Call& call)
        {
            if (!call.valid()) {
                return false;
            }
            call.run();
            uint8_t* result = call.returnValue();
            return result && *reinterpret_cast<uint32_t*>(result) != 0;
        }

        void runVoid(vm::Call& call)
        {
            if (call.valid()) {
                call.run();
            }
        }

        UWindow* windowOf(UObject* object)
        {
            UWindow* window = cast<UWindow>(object);
            if (!window || window->destroyed) {
                return nullptr;
            }
            return window;
        }

        UWindow* commonAncestor(UWindow* a, UWindow* b)
        {
            for (UWindow* first = a; first; first = first->parent()) {
                for (UWindow* second = b; second; second = second->parent()) {
                    if (first == second) {
                        return first;
                    }
                }
            }
            return nullptr;
        }

        void collectFocusable(UWindow* window, std::vector<UWindow*>& out)
        {
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (!child->visible() || !child->sensitive()) {
                    continue;
                }
                if (child->selectable()) {
                    out.push_back(child);
                }
                collectFocusable(child, out);
            }
        }

        bool isMouseKey(uint8_t key)
        {
            return key == IK_MouseX || key == IK_MouseY || key == IK_MouseZ || key == IK_MouseW || key == IK_LeftMouse || key == IK_RightMouse || key == IK_MiddleMouse || key == IK_MouseWheelUp || key == IK_MouseWheelDown;
        }

        bool chainSensitive(UWindow* window)
        {
            for (UWindow* current = window; current; current = current->parent()) {
                if (!current->sensitive()) {
                    return false;
                }
            }
            return true;
        }

        UWindow* acceleratorWindow(UWindow* window, int wanted)
        {
            int32_t accel = window::acceleratorKey(window);
            if (accel > 0 && accel < 255 && std::toupper(accel) == wanted && window->selectable() && window->effectivelyVisible() && chainSensitive(window)) {
                return window;
            }
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (child->destroyed || cast<UModalWindow>(child) || !chainSensitive(child)) {
                    continue;
                }
                if (UWindow* found = acceleratorWindow(child, wanted)) {
                    return found;
                }
            }
            return nullptr;
        }

        UWindow* tabGroupOf(UWindow* window)
        {
            for (UWindow* current = window; current; current = current->parent()) {
                if (cast<UTabGroupWindow>(current)) {
                    return current;
                }
            }
            return nullptr;
        }

        void collectGroupMembers(UWindow* window, std::vector<UWindow*>& out)
        {
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (child->destroyed || cast<UTabGroupWindow>(child)) {
                    continue;
                }
                if (child->selectable()) {
                    out.push_back(child);
                }
                collectGroupMembers(child, out);
            }
        }

        struct Located {
            UWindow* window;
            float x;
            float y;
        };

        std::vector<UWindow*> groupOrder(UWindow* group, bool rowMajor)
        {
            std::vector<UWindow*> members;
            if (group->selectable()) {
                members.push_back(group);
            }
            collectGroupMembers(group, members);
            std::vector<Located> located;
            for (UWindow* member : members) {
                Located entry { member, 0, 0 };
                member->absolutePosition(entry.x, entry.y);
                located.push_back(entry);
            }
            std::stable_sort(located.begin(), located.end(), [rowMajor](const Located& a, const Located& b) {
                if (rowMajor) {
                    return a.y != b.y ? a.y < b.y : a.x < b.x;
                }
                return a.x != b.x ? a.x < b.x : a.y < b.y;
            });
            std::vector<UWindow*> ordered;
            for (const Located& entry : located) {
                ordered.push_back(entry.window);
            }
            return ordered;
        }

        void collectTabGroups(UWindow* window, std::vector<UWindow*>& out)
        {
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (child->destroyed || cast<UModalWindow>(child)) {
                    continue;
                }
                if (cast<UTabGroupWindow>(child)) {
                    out.push_back(child);
                }
                collectTabGroups(child, out);
            }
        }
    }

    State& state()
    {
        static State instance;
        return instance;
    }

    float fittedHeight()
    {
        return std::min(state().viewportHeight, state().viewportWidth * 0.75f);
    }

    float virtualScale()
    {
        return std::max(std::round(fittedHeight() / 600.0f), 1.0f);
    }

    float virtualHeight()
    {
        return std::ceil(state().viewportHeight / virtualScale());
    }

    float virtualWidth()
    {
        return std::round(std::ceil(fittedHeight() / virtualScale()) * 4.0f / 3.0f);
    }

    float extendedVirtualWidth()
    {
        float realWidth = std::ceil(state().viewportWidth / virtualScale());
        return std::max(std::min(realWidth, std::ceil(fittedHeight() / virtualScale()) * 16.0f / 9.0f), virtualWidth());
    }

    UClass* extensionClass(const char* path)
    {
        UClass* cls = objects::findClass(path);
        objects::flushLoads();
        return cls;
    }

    UGC* sharedGC()
    {
        State& s = state();
        if (!s.gc) {
            UClass* cls = extensionClass("Extension.GC");
            if (cls) {
                s.gc = cast<UGC>(objects::newObject(cls, objects::packageObject(Name("Transient")), Name("RootGC"), RF_Transient));
            }
        }
        return s.gc;
    }

    UObject* defaultFont()
    {
        static UObject* font = nullptr;
        if (!font) {
            font = objects::loadObject("Extension.TechMedium", nullptr);
            objects::ensureLoaded(font);
        }
        return font;
    }

    bool optionalBool(Frame& stack, bool& out)
    {
        uint32_t value = 0;
        if (!optional(stack, value)) {
            return false;
        }
        out = value != 0;
        return true;
    }

    bool optionalBoolDefault(Frame& stack, bool fallback)
    {
        bool value = fallback;
        optionalBool(stack, value);
        return value;
    }

    LinearColor toLinear(const Color& color)
    {
        LinearColor result;
        result.r = static_cast<float>(color.r) / 255.0f;
        result.g = static_cast<float>(color.g) / 255.0f;
        result.b = static_cast<float>(color.b) / 255.0f;
        result.a = 1.0f;
        return result;
    }

    float textureWidth(UObject* texture)
    {
        UTexture* tex = cast<UTexture>(texture);
        if (!tex) {
            return 0;
        }
        objects::ensureLoaded(tex);
        return static_cast<float>(tex->width());
    }

    float textureHeight(UObject* texture)
    {
        UTexture* tex = cast<UTexture>(texture);
        if (!tex) {
            return 0;
        }
        objects::ensureLoaded(tex);
        return static_cast<float>(tex->height());
    }

    float fontHeight(UObject* font)
    {
        UFont* value = cast<UFont>(font);
        if (!value) {
            return 0;
        }
        objects::ensureLoaded(value);
        const FontCharacter* glyph = value->glyph(static_cast<uint8_t>('X'), nullptr);
        return glyph ? static_cast<float>(glyph->height) : 0;
    }

    void playSound(UWindow* window, UObject* sound, float volume, float pitch)
    {
        if (!sound) {
            return;
        }
        UObject* player = window ? window->playerPawn() : nullptr;
        State& s = state();
        if (s.soundPlayer) {
            s.soundPlayer(sound, player, volume, pitch);
            return;
        }
        USound* value = cast<USound>(sound);
        if (!value) {
            return;
        }
        objects::ensureLoaded(value);
        Vector location;
        if (player && player->data) {
            location = actor::Location(player);
        }
        audio::playSound(value, player, 0, location, volume, 4096, pitch, false);
    }

    std::string formatNumber(const std::string& format, double value)
    {
        std::string out;
        size_t pos = 0;
        while (pos < format.size()) {
            char c = format[pos];
            if (c != '%') {
                out.push_back(c);
                pos++;
                continue;
            }
            if (pos + 1 < format.size() && format[pos + 1] == '%') {
                out.push_back('%');
                pos += 2;
                continue;
            }
            pos++;
            bool left = false;
            bool zero = false;
            while (pos < format.size() && (format[pos] == '-' || format[pos] == '0' || format[pos] == '+' || format[pos] == ' ')) {
                left = left || format[pos] == '-';
                zero = zero || format[pos] == '0';
                pos++;
            }
            int width = 0;
            while (pos < format.size() && std::isdigit(static_cast<unsigned char>(format[pos]))) {
                width = width * 10 + (format[pos] - '0');
                pos++;
            }
            int precision = -1;
            if (pos < format.size() && format[pos] == '.') {
                pos++;
                precision = 0;
                while (pos < format.size() && std::isdigit(static_cast<unsigned char>(format[pos]))) {
                    precision = precision * 10 + (format[pos] - '0');
                    pos++;
                }
            }
            char spec = pos < format.size() ? format[pos] : 'g';
            pos++;
            char buffer[128];
            int signedWidth = left ? -width : width;
            if (spec == 'd' || spec == 'i') {
                if (zero && !left) {
                    std::snprintf(buffer, sizeof(buffer), "%0*ld", width, std::lround(value));
                } else {
                    std::snprintf(buffer, sizeof(buffer), "%*ld", signedWidth, std::lround(value));
                }
            } else if (spec == 'f' || spec == 'F') {
                std::snprintf(buffer, sizeof(buffer), "%*.*f", signedWidth, precision < 0 ? 6 : precision, value);
            } else if (spec == 'e' || spec == 'E') {
                std::snprintf(buffer, sizeof(buffer), "%*.*e", signedWidth, precision < 0 ? 6 : precision, value);
            } else {
                std::snprintf(buffer, sizeof(buffer), "%*.*g", signedWidth, precision < 0 ? 6 : precision, value);
            }
            out += buffer;
        }
        return out;
    }

    UWindow* UWindow::parent()
    {
        return cast<UWindow>(window::parentOwner(this));
    }

    UWindow* UWindow::firstChild()
    {
        return cast<UWindow>(window::firstChild(this));
    }

    UWindow* UWindow::lastChild()
    {
        return cast<UWindow>(window::lastChild(this));
    }

    UWindow* UWindow::nextSibling()
    {
        return cast<UWindow>(window::nextSibling(this));
    }

    UWindow* UWindow::prevSibling()
    {
        return cast<UWindow>(window::prevSibling(this));
    }

    bool UWindow::visible()
    {
        return window::bIsVisible(this) != 0;
    }

    bool UWindow::sensitive()
    {
        return window::bIsSensitive(this) != 0;
    }

    bool UWindow::selectable()
    {
        return window::bIsSelectable(this) != 0;
    }

    float& UWindow::x()
    {
        return window::X(this);
    }

    float& UWindow::y()
    {
        return window::Y(this);
    }

    float& UWindow::width()
    {
        return window::Width(this);
    }

    float& UWindow::height()
    {
        return window::Height(this);
    }

    URootWindow* UWindow::root()
    {
        UWindow* current = this;
        while (current->parent()) {
            current = current->parent();
        }
        return cast<URootWindow>(current);
    }

    UObject* UWindow::playerPawn()
    {
        URootWindow* rootWindow = root();
        return rootWindow ? rootwindow::parentPawn(rootWindow) : nullptr;
    }

    UWindow* UWindow::modalWindow()
    {
        for (UWindow* current = this; current; current = current->parent()) {
            if (cast<UModalWindow>(current)) {
                return current;
            }
        }
        return nullptr;
    }

    void UWindow::absolutePosition(float& outX, float& outY)
    {
        outX = 0;
        outY = 0;
        for (UWindow* current = this; current; current = current->parent()) {
            outX += current->usedX;
            outY += current->usedY;
        }
    }

    void UWindow::detachFromParent()
    {
        UWindow* owner = parent();
        UWindow* previous = prevSibling();
        UWindow* following = nextSibling();
        if (previous) {
            window::nextSibling(previous) = following;
        }
        if (following) {
            window::prevSibling(following) = previous;
        }
        if (owner) {
            if (owner->firstChild() == this) {
                window::firstChild(owner) = following;
            }
            if (owner->lastChild() == this) {
                window::lastChild(owner) = previous;
            }
        }
        window::prevSibling(this) = nullptr;
        window::nextSibling(this) = nullptr;
        window::parentOwner(this) = nullptr;
        window::winParent(this) = nullptr;
    }

    void UWindow::attachLast(UWindow* owner)
    {
        window::parentOwner(this) = owner;
        window::winParent(this) = owner;
        window::prevSibling(this) = owner->lastChild();
        window::nextSibling(this) = nullptr;
        if (owner->lastChild()) {
            window::nextSibling(owner->lastChild()) = this;
        }
        window::lastChild(owner) = this;
        if (!owner->firstChild()) {
            window::firstChild(owner) = this;
        }
    }

    void UWindow::attachFirst(UWindow* owner)
    {
        window::parentOwner(this) = owner;
        window::winParent(this) = owner;
        window::nextSibling(this) = owner->firstChild();
        window::prevSibling(this) = nullptr;
        if (owner->firstChild()) {
            window::prevSibling(owner->firstChild()) = this;
        }
        window::firstChild(owner) = this;
        if (!owner->lastChild()) {
            window::lastChild(owner) = this;
        }
    }

    UWindow* UWindow::newChild(UClass* childClass, bool show)
    {
        if (!childClass) {
            return nullptr;
        }
        UObject* object = objects::newObject(childClass, objects::packageObject(Name("Transient")), Name(), RF_Transient);
        objects::flushLoads();
        UWindow* child = cast<UWindow>(object);
        if (!child) {
            std::fprintf(stderr, "extension: %s is not a window class\n", childClass->name.c_str());
            return nullptr;
        }
        child->attachLast(this);
        window::firstChild(child) = nullptr;
        window::lastChild(child) = nullptr;
        window::bIsVisible(child) = 0;
        window::bIsSensitive(child) = 1;
        child->initWindow();
        window::bIsInitialized(child) = 1;
        childAdded(child);
        for (UWindow* ancestor = parent(); ancestor; ancestor = ancestor->parent()) {
            ancestor->descendantAdded(child);
        }
        if (show && !child->destroyed) {
            child->show(true);
        }
        askParentForReconfigure();
        window::bNeedsReconfigure(this) = 1;
        return child;
    }

    void UWindow::destroyWindowTree()
    {
        if (window::bBeingDestroyed(this) || destroyed) {
            return;
        }
        window::bBeingDestroyed(this) = 1;
        if (effectivelyVisible()) {
            show(false);
        }
        while (UWindow* child = firstChild()) {
            child->destroyWindowTree();
            if (firstChild() == child) {
                child->detachFromParent();
            }
        }
        destroyWindow();
        URootWindow* rootWindow = root();
        UWindow* owner = parent();
        if (UWindow* modal = owner ? owner->modalWindow() : nullptr) {
            if (modalwindow::preferredFocus(modal) == this) {
                modalwindow::preferredFocus(modal) = nullptr;
            }
        }
        detachFromParent();
        if (owner) {
            owner->childRemoved(this);
            for (UWindow* ancestor = owner->parent(); ancestor; ancestor = ancestor->parent()) {
                ancestor->descendantRemoved(this);
            }
            window::bNeedsReconfigure(owner) = 1;
        }
        if (rootWindow) {
            rootWindow->forgetWindow(this);
        }
        if (state().root == this) {
            state().root = nullptr;
        }
        timers.clear();
        objects::destroyObject(this);
    }

    void UWindow::updateLayout()
    {
        if (!visible()) {
            return;
        }
        UWindow* owner = parent();
        if (owner) {
            uint8_t halign = window::winHAlign(this);
            uint8_t valign = window::winVAlign(this);
            float leftMargin = window::hMargin0(this);
            float rightMargin = window::hMargin1(this);
            float topMargin = window::vMargin0(this);
            float bottomMargin = window::vMargin1(this);
            float parentWidth = owner->width();
            float parentHeight = owner->height();
            bool parentIsRoot = owner == state().root;
            float rootWidth = isA(Name("DeusExScopeView")) ? std::ceil(state().viewportWidth / virtualScale()) : extendedVirtualWidth();
            if (halign == HALIGN_Full || valign == VALIGN_Full) {
                float newWidth = width();
                float newHeight = height();
                if (halign == HALIGN_Full) {
                    x() = 0;
                    newWidth = std::max((parentIsRoot ? rootWidth : parentWidth) - leftMargin - rightMargin, 0.0f);
                }
                if (valign == VALIGN_Full) {
                    y() = 0;
                    newHeight = std::max(parentHeight - topMargin - bottomMargin, 0.0f);
                }
                if (width() != newWidth || height() != newHeight) {
                    width() = newWidth;
                    height() = newHeight;
                    window::bNeedsReconfigure(this) = 1;
                }
            }
            float offsetX = 0;
            if (parentIsRoot) {
                float extra = std::max(rootWidth - parentWidth, 0.0f);
                offsetX = -std::round(extra * 0.5f);
                parentWidth += extra;
            }
            float px = 0;
            float py = 0;
            if (halign == HALIGN_Center) {
                px = (parentWidth - width()) * 0.5f;
            } else if (halign == HALIGN_Right) {
                px = parentWidth - rightMargin - width() - x();
            } else {
                px = x() + leftMargin;
            }
            if (valign == VALIGN_Center) {
                py = (parentHeight - height()) * 0.5f;
            } else if (valign == VALIGN_Bottom) {
                py = parentHeight - bottomMargin - height() - y();
            } else {
                py = y() + topMargin;
            }
            usedX = px + offsetX;
            usedY = py;
        } else {
            float w = virtualWidth();
            float h = virtualHeight();
            if (width() != w || height() != h) {
                x() = 0;
                y() = 0;
                width() = w;
                height() = h;
                window::bConfigured(this) = 1;
                window::bNeedsReconfigure(this) = 1;
            }
            usedX = std::round((state().viewportWidth - w * virtualScale()) * 0.5f) / virtualScale();
            usedY = std::round((state().viewportHeight - h * virtualScale()) * 0.5f) / virtualScale();
        }

        bool reconfigured = window::bNeedsReconfigure(this) != 0;
        if (reconfigured) {
            window::bNeedsReconfigure(this) = 0;
            configurationChanged();
        }

        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            bool needsQuery = window::bNeedsQuery(child) != 0;
            window::bNeedsQuery(child) = 0;
            if (needsQuery || ((reconfigured || child->firstDraw) && !window::bConfigured(child))) {
                float newWidth = window::hardcodedWidth(child);
                float newHeight = window::hardcodedHeight(child);
                if (!child->fixedWidth && !child->fixedHeight) {
                    child->queryPreferredSize(newWidth, newHeight);
                } else if (child->fixedWidth && !child->fixedHeight) {
                    newHeight = child->queryPreferredHeight(newWidth);
                } else if (child->fixedHeight && !child->fixedWidth) {
                    newWidth = child->queryPreferredWidth(newHeight);
                }
                if (child->width() != newWidth || child->height() != newHeight || child->firstDraw) {
                    child->width() = newWidth;
                    child->height() = newHeight;
                    window::bNeedsReconfigure(child) = 1;
                }
            }
        }

        UWindow* child = firstChild();
        while (child) {
            UWindow* following = child->nextSibling();
            child->updateLayout();
            child = following;
        }

        if (firstDraw) {
            firstDraw = false;
            windowReady();
        }
    }

    void UWindow::show(bool value)
    {
        if (visible() == value) {
            return;
        }
        if (UWindow* owner = parent()) {
            owner->childRequestedVisibilityChange(this, value);
        } else {
            setChildVisibility(value);
        }
    }

    bool UWindow::effectivelyVisible()
    {
        for (UWindow* current = this; current; current = current->parent()) {
            if (!current->visible()) {
                return false;
            }
        }
        return true;
    }

    void UWindow::setChildVisibility(bool value)
    {
        if (visible() == value) {
            return;
        }
        bool before = effectivelyVisible();
        window::bIsVisible(this) = value ? 1 : 0;
        bool after = effectivelyVisible();
        if (before != after) {
            if (URootWindow* rootWindow = root()) {
                rootWindow->checkFocusWindow();
            }
            invokeVisibilityChange(after);
        }
        if (!destroyed) {
            askParentForReconfigure();
        }
    }

    void UWindow::invokeVisibilityChange(bool value)
    {
        visibilityChanged(value);
        if (destroyed) {
            return;
        }
        playWindowSound(value ? window::visibleSound(this) : window::invisibleSound(this));
        UWindow* child = firstChild();
        while (child) {
            UWindow* following = child->nextSibling();
            if (!child->destroyed && child->visible()) {
                child->invokeVisibilityChange(value);
            }
            child = following;
        }
    }

    void UWindow::playWindowSound(UObject* sound)
    {
        if (sound) {
            float volume = window::SoundVolume(this);
            playSound(this, sound, volume > 0 ? volume : 1.0f, 1.0f);
        }
    }

    void UWindow::setSensitivity(bool value)
    {
        if (sensitive() != value) {
            window::bIsSensitive(this) = value ? 1 : 0;
            if (URootWindow* rootWindow = root()) {
                rootWindow->checkFocusWindow();
            }
            sensitivityChanged(value);
        }
    }

    void UWindow::setWidth(float value)
    {
        window::hardcodedWidth(this) = value;
        fixedWidth = true;
        window::bNeedsReconfigure(this) = 1;
        askParentForReconfigure();
    }

    void UWindow::setHeight(float value)
    {
        window::hardcodedHeight(this) = value;
        fixedHeight = true;
        window::bNeedsReconfigure(this) = 1;
        askParentForReconfigure();
    }

    void UWindow::configureChild(float newX, float newY, float newWidth, float newHeight)
    {
        if (fixedWidth) {
            newWidth = window::hardcodedWidth(this);
        }
        if (fixedHeight) {
            newHeight = window::hardcodedHeight(this);
        }
        if (UWindow* owner = parent()) {
            if (window::winHAlign(this) == HALIGN_Full) {
                newX = 0;
                float available = owner == state().root ? extendedVirtualWidth() : owner->width();
                newWidth = std::max(available - window::hMargin0(this) - window::hMargin1(this), 0.0f);
            }
            if (window::winVAlign(this) == VALIGN_Full) {
                newY = 0;
                newHeight = std::max(owner->height() - window::vMargin0(this) - window::vMargin1(this), 0.0f);
            }
        }
        x() = newX;
        y() = newY;
        window::bConfigured(this) = 1;
        window::bNeedsQuery(this) = 0;
        width() = newWidth;
        height() = newHeight;
        window::bNeedsReconfigure(this) = 1;
    }

    void UWindow::queryPreferredSize(float& preferredWidth, float& preferredHeight)
    {
        preferredWidth = 0;
        preferredHeight = 0;
        if (!fixedWidth || !fixedHeight) {
            UObject* background = window::Background(this);
            if (background) {
                preferredWidth = textureWidth(background);
                preferredHeight = textureHeight(background);
            }
            if (fixedWidth) {
                preferredWidth = window::hardcodedWidth(this);
                parentRequestedPreferredSize(true, preferredWidth, false, preferredHeight);
            } else if (fixedHeight) {
                preferredHeight = window::hardcodedHeight(this);
                parentRequestedPreferredSize(false, preferredWidth, true, preferredHeight);
            } else {
                parentRequestedPreferredSize(false, preferredWidth, false, preferredHeight);
            }
        }
        if (fixedWidth) {
            preferredWidth = window::hardcodedWidth(this);
        }
        if (fixedHeight) {
            preferredHeight = window::hardcodedHeight(this);
        }
        window::lastQueryWidth(this) = preferredWidth;
        window::lastQueryHeight(this) = preferredHeight;
    }

    float UWindow::queryPreferredWidth(float queryHeight)
    {
        float result = 0;
        if (fixedWidth) {
            result = window::hardcodedWidth(this);
        } else {
            UObject* background = window::Background(this);
            if (background) {
                result = textureWidth(background);
            }
            parentRequestedPreferredSize(false, result, true, queryHeight);
        }
        window::lastQueryWidth(this) = result;
        return result;
    }

    float UWindow::queryPreferredHeight(float queryWidth)
    {
        float result = 0;
        if (fixedHeight) {
            result = window::hardcodedHeight(this);
        } else {
            UObject* background = window::Background(this);
            if (background) {
                result = textureHeight(background);
            }
            parentRequestedPreferredSize(true, queryWidth, false, result);
        }
        window::lastQueryHeight(this) = result;
        return result;
    }

    void UWindow::askParentForReconfigure()
    {
        for (UWindow* current = parent(); current; current = current->parent()) {
            window::bNeedsReconfigure(current) = 1;
            if (current->childRequestedReconfiguration(this)) {
                break;
            }
        }
    }

    void UWindow::askParentToShowArea(float areaX, float areaY, float areaWidth, float areaHeight)
    {
        if (UWindow* owner = parent()) {
            owner->childRequestedShowArea(this, areaX, areaY, areaWidth, areaHeight);
        }
    }

    bool UWindow::setFocusWindow(UWindow* window)
    {
        URootWindow* rootWindow = root();
        return rootWindow && rootWindow->setRootFocusWindow(window);
    }

    void UWindow::grabMouse()
    {
        if (URootWindow* rootWindow = root()) {
            rootwindow::grabbedWindow(rootWindow) = this;
        }
    }

    void UWindow::ungrabMouse()
    {
        if (URootWindow* rootWindow = root()) {
            if (rootwindow::grabbedWindow(rootWindow) == this) {
                rootwindow::grabbedWindow(rootWindow) = nullptr;
            }
        }
    }

    void UWindow::setFont(UObject* font)
    {
        window::normalFont(this) = font;
        window::boldFont(this) = font;
    }

    bool UWindow::sendButtonActivated(UWindow* button)
    {
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->buttonActivated(button)) {
                return true;
            }
        }
        return false;
    }

    void UWindow::initWindow()
    {
        window::TextColor(this) = { 0, 255, 0, 255 };
        window::tileColor(this) = { 255, 255, 255, 255 };
        window::textVSpacing(this) = 1.0f;
        window::bSpecialText(this) = 1;
        baselineOffset = 2.0f;
        underlineHeight = 1.0f;
        window::backgroundStyle(this) = DSTY_Normal;
        static const Name eventName("InitWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::destroyWindow()
    {
        static const Name eventName("DestroyWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::windowReady()
    {
        static const Name eventName("WindowReady");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        static const Name eventName("ParentRequestedPreferredSize");
        vm::Call call(this, eventName);
        if (!call.valid()) {
            return;
        }
        call.b(widthSpecified).f(preferredWidth).b(heightSpecified).f(preferredHeight);
        call.run();
        preferredWidth = *reinterpret_cast<float*>(call.parameter(1));
        preferredHeight = *reinterpret_cast<float*>(call.parameter(3));
    }

    void UWindow::parentRequestedGranularity(float& hGranularity, float& vGranularity)
    {
        static const Name eventName("ParentRequestedGranularity");
        vm::Call call(this, eventName);
        if (!call.valid()) {
            return;
        }
        call.f(hGranularity).f(vGranularity);
        call.run();
        hGranularity = *reinterpret_cast<float*>(call.parameter(0));
        vGranularity = *reinterpret_cast<float*>(call.parameter(1));
    }

    bool UWindow::childRequestedReconfiguration(UWindow* child)
    {
        static const Name eventName("ChildRequestedReconfiguration");
        vm::Call call(this, eventName);
        call.obj(child);
        return runBool(call);
    }

    void UWindow::childRequestedShowArea(UWindow* child, float showX, float showY, float showWidth, float showHeight)
    {
        static const Name eventName("ChildRequestedShowArea");
        vm::Call call(this, eventName);
        if (!call.valid()) {
            askParentToShowArea(child->x() + showX, child->y() + showY, showWidth, showHeight);
            return;
        }
        call.obj(child).f(showX).f(showY).f(showWidth).f(showHeight);
        call.run();
    }

    void UWindow::childRequestedVisibilityChange(UWindow* child, bool visible)
    {
        static const Name eventName("ChildRequestedVisibilityChange");
        vm::Call call(this, eventName);
        call.obj(child).b(visible);
        runVoid(call);
    }

    void UWindow::configurationChanged()
    {
        static const Name eventName("ConfigurationChanged");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::visibilityChanged(bool value)
    {
        static const Name eventName("VisibilityChanged");
        vm::Call call(this, eventName);
        call.b(value);
        runVoid(call);
    }

    void UWindow::sensitivityChanged(bool value)
    {
        static const Name eventName("SensitivityChanged");
        vm::Call call(this, eventName);
        call.b(value);
        runVoid(call);
    }

    void UWindow::mouseMoved(float newX, float newY)
    {
        static const Name eventName("MouseMoved");
        vm::Call call(this, eventName);
        call.f(newX).f(newY);
        runVoid(call);
    }

    void UWindow::mouseEnteredWindow()
    {
        static const Name eventName("MouseEnteredWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::mouseLeftWindow()
    {
        static const Name eventName("MouseLeftWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    bool UWindow::rawMouseButtonPressed(float pointX, float pointY, uint8_t button, uint8_t inputState)
    {
        static const Name eventName("RawMouseButtonPressed");
        vm::Call call(this, eventName);
        call.f(pointX).f(pointY).byte(button).byte(inputState);
        return runBool(call);
    }

    bool UWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        static const Name eventName("MouseButtonPressed");
        vm::Call call(this, eventName);
        call.f(pointX).f(pointY).byte(button).i(numClicks);
        return runBool(call);
    }

    bool UWindow::mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        static const Name eventName("MouseButtonReleased");
        vm::Call call(this, eventName);
        call.f(pointX).f(pointY).byte(button).i(numClicks);
        return runBool(call);
    }

    bool UWindow::rawKeyPressed(uint8_t key, uint8_t inputState, bool repeat)
    {
        static const Name eventName("RawKeyPressed");
        vm::Call call(this, eventName);
        call.byte(key).byte(inputState).b(repeat);
        return runBool(call);
    }

    bool UWindow::keyPressed(const std::string& key)
    {
        static const Name eventName("KeyPressed");
        vm::Call call(this, eventName);
        call.s(key);
        return runBool(call);
    }

    bool UWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        static const Name eventName("VirtualKeyPressed");
        vm::Call call(this, eventName);
        call.byte(key).b(repeat);
        return runBool(call);
    }

    bool UWindow::acceleratorKeyPressed(const std::string& key)
    {
        static const Name eventName("AcceleratorKeyPressed");
        vm::Call call(this, eventName);
        call.s(key);
        return runBool(call);
    }

    void UWindow::focusEnteredWindow()
    {
        static const Name eventName("FocusEnteredWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::focusLeftWindow()
    {
        static const Name eventName("FocusLeftWindow");
        vm::Call call(this, eventName);
        runVoid(call);
    }

    void UWindow::focusEnteredDescendant(UWindow* window)
    {
        static const Name eventName("FocusEnteredDescendant");
        vm::Call call(this, eventName);
        call.obj(window);
        runVoid(call);
    }

    void UWindow::focusLeftDescendant(UWindow* window)
    {
        static const Name eventName("FocusLeftDescendant");
        vm::Call call(this, eventName);
        call.obj(window);
        runVoid(call);
    }

    bool UWindow::buttonActivated(UWindow* button)
    {
        static const Name eventName("ButtonActivated");
        vm::Call call(this, eventName);
        call.obj(button);
        return runBool(call);
    }

    bool UWindow::toggleChanged(UWindow* button, bool newToggle)
    {
        static const Name eventName("ToggleChanged");
        vm::Call call(this, eventName);
        call.obj(button).b(newToggle);
        return runBool(call);
    }

    bool UWindow::boxOptionSelected(UWindow* box, int32_t buttonNumber)
    {
        static const Name eventName("BoxOptionSelected");
        vm::Call call(this, eventName);
        call.obj(box).i(buttonNumber);
        return runBool(call);
    }

    bool UWindow::scalePositionChanged(UWindow* scale, int32_t newTickPosition, float newValue, bool final)
    {
        static const Name eventName("ScalePositionChanged");
        vm::Call call(this, eventName);
        call.obj(scale).i(newTickPosition).f(newValue).b(final);
        return runBool(call);
    }

    bool UWindow::scaleRangeChanged(UWindow* scale, int32_t fromTick, int32_t toTick, float fromValue, float toValue, bool final)
    {
        static const Name eventName("ScaleRangeChanged");
        vm::Call call(this, eventName);
        call.obj(scale).i(fromTick).i(toTick).f(fromValue).f(toValue).b(final);
        return runBool(call);
    }

    bool UWindow::scaleAttributesChanged(UWindow* scale, int32_t tickPosition, int32_t tickSpan, int32_t numTicks)
    {
        static const Name eventName("ScaleAttributesChanged");
        vm::Call call(this, eventName);
        call.obj(scale).i(tickPosition).i(tickSpan).i(numTicks);
        return runBool(call);
    }

    bool UWindow::clipAttributesChanged(UWindow* clip, int32_t newClipWidth, int32_t newClipHeight, int32_t newChildWidth, int32_t newChildHeight)
    {
        static const Name eventName("ClipAttributesChanged");
        vm::Call call(this, eventName);
        call.obj(clip).i(newClipWidth).i(newClipHeight).i(newChildWidth).i(newChildHeight);
        return runBool(call);
    }

    bool UWindow::clipPositionChanged(UWindow* clip, int32_t newCol, int32_t newRow)
    {
        static const Name eventName("ClipPositionChanged");
        vm::Call call(this, eventName);
        call.obj(clip).i(newCol).i(newRow);
        return runBool(call);
    }

    void UWindow::queryGranularity(float& hGranularity, float& vGranularity)
    {
        hGranularity = 1.0f;
        vGranularity = 1.0f;
        parentRequestedGranularity(hGranularity, vGranularity);
        hGranularity = std::max(hGranularity, 1.0f);
        vGranularity = std::max(vGranularity, 1.0f);
    }

    bool UWindow::listRowActivated(UWindow* list, int32_t rowId)
    {
        static const Name eventName("ListRowActivated");
        vm::Call call(this, eventName);
        call.obj(list).i(rowId);
        return runBool(call);
    }

    bool UWindow::listSelectionChanged(UWindow* list, int32_t numSelections, int32_t focusRowId)
    {
        static const Name eventName("ListSelectionChanged");
        vm::Call call(this, eventName);
        call.obj(list).i(numSelections).i(focusRowId);
        return runBool(call);
    }

    bool UWindow::textChanged(UWindow* edit, bool modified)
    {
        static const Name eventName("TextChanged");
        vm::Call call(this, eventName);
        call.obj(edit).b(modified);
        return runBool(call);
    }

    bool UWindow::editActivated(UWindow* edit, bool modified)
    {
        static const Name eventName("EditActivated");
        vm::Call call(this, eventName);
        call.obj(edit).b(modified);
        return runBool(call);
    }

    void UWindow::drawBackground(UGC* gc)
    {
        if (!window::bDrawRawBackground(this)) {
            return;
        }
        UObject* background = window::Background(this);
        if (!background) {
            return;
        }
        float x = 0;
        float y = 0;
        float w = width();
        float h = height();
        if (!parent()) {
            x = -usedX;
            y = -usedY;
            w = state().viewportWidth / virtualScale();
            h = state().viewportHeight / virtualScale();
        }
        gc->setStyle(window::backgroundStyle(this));
        gc::bSmoothed(gc) = window::bSmoothBackground(this);
        gc::tileColor(gc) = window::tileColor(this);
        if (window::bStretchBackground(this)) {
            gc->drawStretched(x, y, w, h, 0, 0, textureWidth(background), textureHeight(background), background);
        } else {
            gc->drawPattern(x, y, w, h, 0, 0, background);
        }
        gc->resetState(this, gc->offsetX, gc->offsetY);
    }

    void UWindow::scriptDrawWindow(UGC* gc)
    {
        static const Name eventName("DrawWindow");
        vm::Call call(this, eventName);
        call.obj(gc);
        runVoid(call);
    }

    void UWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        scriptDrawWindow(gc);
    }

    void UWindow::postDrawWindow(UGC* gc)
    {
        static const Name eventName("PostDrawWindow");
        vm::Call call(this, eventName);
        call.obj(gc);
        runVoid(call);
    }

    void UWindow::childAdded(UWindow* child)
    {
        static const Name eventName("ChildAdded");
        vm::Call call(this, eventName);
        call.obj(child);
        runVoid(call);
    }

    void UWindow::childRemoved(UWindow* child)
    {
        static const Name eventName("ChildRemoved");
        vm::Call call(this, eventName);
        call.obj(child);
        runVoid(call);
    }

    void UWindow::descendantAdded(UWindow* descendant)
    {
        static const Name eventName("DescendantAdded");
        vm::Call call(this, eventName);
        call.obj(descendant);
        runVoid(call);
    }

    void UWindow::descendantRemoved(UWindow* descendant)
    {
        static const Name eventName("DescendantRemoved");
        vm::Call call(this, eventName);
        call.obj(descendant);
        runVoid(call);
    }

    void UWindow::tick(float deltaSeconds)
    {
        if (window::bTickEnabled(this)) {
            static const Name eventName("Tick");
            vm::Call call(this, eventName);
            call.f(deltaSeconds);
            runVoid(call);
        }
        if (destroyed) {
            return;
        }
        std::vector<int32_t> expired;
        for (auto& [id, timer] : timers) {
            timer.left -= deltaSeconds;
            if (timer.left <= 0) {
                expired.push_back(id);
            }
        }
        for (int32_t id : expired) {
            auto it = timers.find(id);
            if (it == timers.end()) {
                continue;
            }
            Name function = it->second.function;
            int32_t clientData = it->second.clientData;
            if (it->second.loop && it->second.timeout > 0.0001f) {
                it->second.left = std::max(it->second.left + it->second.timeout, 0.0f);
            } else {
                timers.erase(it);
            }
            vm::Call call(this, function);
            if (call.valid()) {
                call.i(clientData);
                call.run();
            }
            if (destroyed) {
                return;
            }
        }
        UWindow* child = firstChild();
        while (child) {
            UWindow* following = child->nextSibling();
            child->tick(deltaSeconds);
            child = following;
        }
    }

    void UModalWindow::initWindow()
    {
        modalwindow::focusMode(this) = 2;
        UTabGroupWindow::initWindow();
    }

    bool UModalWindow::keyPressed(const std::string& key)
    {
        if (UTabGroupWindow::keyPressed(key)) {
            return true;
        }
        if (key.empty() || !isCurrentModal() || !state().keys[IK_Alt]) {
            return false;
        }
        UWindow* target = acceleratorWindow(this, std::toupper(static_cast<unsigned char>(key[0])));
        return target && target->acceleratorKeyPressed(key);
    }

    void UModalWindow::visibilityChanged(bool value)
    {
        UTabGroupWindow::visibilityChanged(value);
        if (!parent() || !root()) {
            return;
        }
        State& s = state();
        if (value) {
            if (s.keyboardGrabs <= 0) {
                for (size_t key = 0; key < s.keys.size(); key++) {
                    if (!isMouseKey(static_cast<uint8_t>(key))) {
                        s.keys[key] = false;
                    }
                }
            }
            s.keyboardGrabs++;
        } else if (s.keyboardGrabs > 0) {
            s.keyboardGrabs--;
        }
    }

    bool UModalWindow::isCurrentModal()
    {
        URootWindow* rootWindow = root();
        return rootWindow && rootWindow->topModal() == this;
    }

    float& URootWindow::mouseX()
    {
        return rootwindow::MouseX(this);
    }

    float& URootWindow::mouseY()
    {
        return rootwindow::MouseY(this);
    }

    UWindow* URootWindow::focusWindow()
    {
        return windowOf(rootwindow::FocusWindow(this));
    }

    UWindow* URootWindow::topModal()
    {
        for (UWindow* child = lastChild(); child; child = child->prevSibling()) {
            if (child->visible() && cast<UModalWindow>(child)) {
                return child;
            }
        }
        return this;
    }

    bool URootWindow::isTraversable(UWindow* window, bool checkModal)
    {
        if (!window || !window->selectable()) {
            return false;
        }
        for (UWindow* current = window; current; current = current->parent()) {
            if (!current->visible() || !current->sensitive()) {
                return false;
            }
        }
        return !checkModal || window->modalWindow() == topModal();
    }

    UWindow* URootWindow::moveTabGroup(int32_t move)
    {
        UWindow* modal = topModal();
        std::vector<UWindow*> groups { modal };
        collectTabGroups(modal, groups);
        std::vector<Located> located;
        for (UWindow* group : groups) {
            Located entry { group, 0, 0 };
            for (UWindow* member : groupOrder(group, true)) {
                if (member->width() > 0 && member->height() > 0 && isTraversable(member, false)) {
                    member->absolutePosition(entry.x, entry.y);
                    break;
                }
            }
            located.push_back(entry);
        }
        std::stable_sort(located.begin(), located.end(), [](const Located& a, const Located& b) {
            return a.y != b.y ? a.y < b.y : a.x < b.x;
        });
        int32_t count = static_cast<int32_t>(located.size());
        if (count <= 0) {
            return focusWindow();
        }
        bool back = move == 0 || move == 2;
        int32_t start = 0;
        UWindow* focus = focusWindow();
        if (focus && focus->modalWindow() == modal) {
            UWindow* current = tabGroupOf(focus);
            for (int32_t i = 0; i < count; i++) {
                if (located[static_cast<size_t>(i)].window == current) {
                    start = back ? (i - 1 < 0 ? count - 1 : i - 1) : (i + 1 >= count ? 0 : i + 1);
                    break;
                }
            }
        }
        int32_t index = start;
        UWindow* target = nullptr;
        do {
            for (UWindow* member : groupOrder(located[static_cast<size_t>(index)].window, true)) {
                if (isTraversable(member, false)) {
                    target = member;
                    break;
                }
            }
            if (target) {
                break;
            }
            index = back ? (index - 1 < 0 ? count - 1 : index - 1) : (index + 1 >= count ? 0 : index + 1);
        } while (index != start);
        if (target) {
            setRootFocusWindow(target);
        }
        return focusWindow();
    }

    UWindow* URootWindow::moveFocus(int32_t move)
    {
        UWindow* focus = focusWindow();
        if (!focus) {
            moveTabGroup(move);
            return focusWindow();
        }
        UWindow* group = tabGroupOf(focus);
        if (!group) {
            return focus;
        }
        std::vector<UWindow*> list = groupOrder(group, move == 0 || move == 1);
        int32_t count = static_cast<int32_t>(list.size());
        int32_t index = static_cast<int32_t>(std::find(list.begin(), list.end(), focus) - list.begin());
        if (index >= count) {
            moveTabGroup(move);
            return focusWindow();
        }
        bool back = move == 0 || move == 2;
        for (int32_t remaining = count; remaining > 0; remaining--) {
            index = back ? (index - 1 < 0 ? count - 1 : index - 1) : (index + 1 >= count ? 0 : index + 1);
            if (isTraversable(list[static_cast<size_t>(index)], true)) {
                setRootFocusWindow(list[static_cast<size_t>(index)]);
                return focusWindow();
            }
        }
        moveTabGroup(move);
        return focusWindow();
    }

    void URootWindow::checkFocusWindow()
    {
        UWindow* old = focusWindow();
        if (!old || isTraversable(old, true)) {
            return;
        }
        UWindow* preferred = windowOf(modalwindow::preferredFocus(topModal()));
        if (preferred && isTraversable(preferred, true)) {
            setRootFocusWindow(preferred);
        }
        if (focusWindow() == old && moveFocus(1) == old) {
            setRootFocusWindow(nullptr);
        }
    }

    void URootWindow::tick(float deltaSeconds)
    {
        if (!focusWindow()) {
            UWindow* modal = topModal();
            if (modalwindow::focusMode(modal) < 3) {
                updateLayout();
                UWindow* preferred = windowOf(modalwindow::preferredFocus(modal));
                if (preferred && isTraversable(preferred, true)) {
                    setRootFocusWindow(preferred);
                }
                if (!focusWindow()) {
                    moveTabGroup(3);
                }
            }
        }
        UModalWindow::tick(deltaSeconds);
    }

    bool URootWindow::isModalOpen()
    {
        for (UWindow* child = lastChild(); child; child = child->prevSibling()) {
            if (child->visible() && cast<UModalWindow>(child)) {
                return true;
            }
        }
        return false;
    }

    bool URootWindow::isCursorVisible()
    {
        return isModalOpen() && rootwindow::bCursorVisible(this) != 0;
    }

    void URootWindow::forgetWindow(UWindow* window)
    {
        if (rootwindow::FocusWindow(this) == window) {
            rootwindow::FocusWindow(this) = nullptr;
        }
        if (rootwindow::grabbedWindow(this) == window) {
            rootwindow::grabbedWindow(this) = nullptr;
        }
        if (rootwindow::lastMouseWindow(this) == window) {
            rootwindow::lastMouseWindow(this) = nullptr;
        }
        if (lastClickWindow == window) {
            lastClickWindow = nullptr;
        }
    }

    void URootWindow::postDrawWindow(UGC* gc)
    {
        UModalWindow::postDrawWindow(gc);
        if (!isCursorVisible()) {
            return;
        }
        float relativeX = 0;
        float relativeY = 0;
        UWindow* focus = cursorFocus(relativeX, relativeY);
        UObject* cursor = nullptr;
        UObject* shadow = nullptr;
        Color color { 255, 255, 255, 255 };
        float hotX = 0;
        float hotY = 0;
        for (UWindow* current = focus; current; current = current->parent()) {
            if (window::defaultCursor(current)) {
                cursor = window::defaultCursor(current);
                shadow = window::defaultCursorShadow(current);
                hotX = window::defaultHotX(current);
                hotY = window::defaultHotY(current);
                Color custom = window::defaultCursorColor(current);
                if (custom.r || custom.g || custom.b) {
                    color = custom;
                }
                break;
            }
        }
        if (!cursor) {
            return;
        }
        float cursorWidth = textureWidth(cursor);
        float cursorHeight = textureHeight(cursor);
        if (hotX < 0 || hotX >= cursorWidth) {
            hotX = std::floor(cursorWidth * 0.5f) - 1;
        }
        if (hotY < 0 || hotY >= cursorHeight) {
            hotY = std::floor(cursorHeight * 0.5f) - 1;
        }
        gc->resetState(this, usedX, usedY);
        if (shadow) {
            gc->setStyle(DSTY_Modulated);
            gc::tileColor(gc) = { 255, 255, 255, 255 };
            gc->drawIcon(mouseX() - hotX, mouseY() - hotY, shadow);
        }
        gc->setStyle(DSTY_Masked);
        gc::tileColor(gc) = color;
        gc->drawIcon(mouseX() - hotX, mouseY() - hotY, cursor);
    }

    bool URootWindow::setRootFocusWindow(UWindow* newFocus)
    {
        if (newFocus) {
            UWindow* selectableAncestor = newFocus;
            while (selectableAncestor && !selectableAncestor->selectable()) {
                selectableAncestor = selectableAncestor->parent();
            }
            if (selectableAncestor) {
                newFocus = selectableAncestor;
            }
        }
        UWindow* oldFocus = focusWindow();
        if (oldFocus == newFocus) {
            return true;
        }
        if (newFocus) {
            if (!isTraversable(newFocus, true)) {
                return false;
            }
            if (UWindow* modal = newFocus->modalWindow()) {
                modalwindow::preferredFocus(modal) = newFocus;
            }
        }
        rootwindow::FocusWindow(this) = newFocus;
        if (oldFocus) {
            oldFocus->focusLeftWindow();
            oldFocus->playWindowSound(window::unfocusSound(oldFocus));
            for (UWindow* current = oldFocus; current; current = current->parent()) {
                current->focusLeftDescendant(oldFocus);
            }
        }
        if (newFocus) {
            newFocus->askParentToShowArea(0, 0, newFocus->width(), newFocus->height());
            newFocus->playWindowSound(window::focusSound(newFocus));
            newFocus->focusEnteredWindow();
            for (UWindow* current = newFocus; current; current = current->parent()) {
                current->focusEnteredDescendant(newFocus);
            }
        }
        return true;
    }

    UWindow* URootWindow::cursorFocus(float& relativeX, float& relativeY)
    {
        UWindow* grabbed = windowOf(rootwindow::grabbedWindow(this));
        if (grabbed) {
            float gx = 0;
            float gy = 0;
            grabbed->absolutePosition(gx, gy);
            relativeX = mouseX() + usedX - gx;
            relativeY = mouseY() + usedY - gy;
            return grabbed;
        }
        float px = mouseX();
        float py = mouseY();
        UWindow* current = this;
        while (true) {
            UWindow* found = nullptr;
            for (UWindow* child = current->lastChild(); child; child = child->prevSibling()) {
                if (child->visible() && child->usedX <= px && child->usedY <= py && child->usedX + child->width() > px && child->usedY + child->height() > py) {
                    found = child;
                    break;
                }
            }
            if (!found) {
                break;
            }
            current = found;
            px -= found->usedX;
            py -= found->usedY;
        }
        relativeX = px;
        relativeY = py;
        return current;
    }

    void URootWindow::setRootCursorPos(float newMouseX, float newMouseY)
    {
        float scale = virtualScale();
        float realWidth = std::ceil(state().viewportWidth / scale);
        float realHeight = std::ceil(state().viewportHeight / scale);
        newMouseX = std::clamp(newMouseX + usedX, 0.0f, realWidth) - usedX;
        newMouseY = std::clamp(newMouseY + usedY, 0.0f, realHeight) - usedY;
        rootwindow::prevMouseX(this) = mouseX();
        rootwindow::prevMouseY(this) = mouseY();
        mouseX() = newMouseX;
        mouseY() = newMouseY;
        rootwindow::bMouseMoved(this) = 1;

        float relativeX = 0;
        float relativeY = 0;
        UWindow* focus = cursorFocus(relativeX, relativeY);
        UWindow* last = windowOf(rootwindow::lastMouseWindow(this));
        if (last != focus) {
            uint8_t mouseFocusMode = modalwindow::focusMode(topModal());
            UWindow* ancestor = commonAncestor(last, focus);
            for (UWindow* current = last; current && current != ancestor; current = current->parent()) {
                current->mouseLeftWindow();
            }
            if (last && mouseFocusMode >= 3) {
                setRootFocusWindow(nullptr);
            }
            rootwindow::lastMouseWindow(this) = focus;
            std::vector<UWindow*> entered;
            for (UWindow* current = focus; current && current != ancestor; current = current->parent()) {
                entered.push_back(current);
            }
            for (auto it = entered.rbegin(); it != entered.rend(); ++it) {
                (*it)->mouseEnteredWindow();
            }
            if (focus && mouseFocusMode >= 2) {
                setRootFocusWindow(focus);
            }
        }
        if (focus && !focus->destroyed) {
            focus->mouseMoved(relativeX, relativeY);
        }
    }

    bool URootWindow::onRawMouseMove(float dx, float dy)
    {
        if (isCursorVisible()) {
            float speed = 1.0f / virtualScale();
            setRootCursorPos(mouseX() + dx * speed, mouseY() + dy * speed);
        }
        return isModalOpen();
    }

    bool URootWindow::onMouseButton(uint8_t key, bool pressed)
    {
        float relativeX = 0;
        float relativeY = 0;
        UWindow* focus = cursorFocus(relativeX, relativeY);
        if (!focus) {
            return isModalOpen();
        }
        if (pressed) {
            if (!focus->sensitive()) {
                return isModalOpen();
            }
            double now = state().time;
            float timeout = rootwindow::multiClickTimeout(this);
            if (timeout <= 0) {
                timeout = 0.4f;
            }
            if (lastClickWindow == focus && lastClickButton == key && now - lastClickTime < static_cast<double>(timeout)) {
                clicks++;
            } else {
                clicks = 1;
            }
            lastClickWindow = focus;
            lastClickButton = key;
            lastClickTime = now;
            rootwindow::clickCount(this) = clicks;
            if (focus->rawMouseButtonPressed(relativeX, relativeY, key, IST_Press)) {
                return true;
            }
            if (focus->selectable()) {
                setRootFocusWindow(focus);
            }
            float px = relativeX;
            float py = relativeY;
            for (UWindow* current = focus; current; current = current->parent()) {
                if (current->destroyed) {
                    break;
                }
                if (current->mouseButtonPressed(px, py, key, clicks)) {
                    return true;
                }
                px += current->usedX;
                py += current->usedY;
            }
            return isModalOpen();
        }
        if (focus->rawMouseButtonPressed(relativeX, relativeY, key, IST_Release)) {
            return true;
        }
        float px = relativeX;
        float py = relativeY;
        for (UWindow* current = focus; current; current = current->parent()) {
            if (current->destroyed) {
                break;
            }
            if (current->mouseButtonReleased(px, py, key, clicks)) {
                return true;
            }
            px += current->usedX;
            py += current->usedY;
        }
        return isModalOpen();
    }

    bool URootWindow::onKey(uint8_t key, bool pressed, bool repeat)
    {
        UWindow* start = focusWindow();
        if (!start) {
            for (UWindow* child = lastChild(); child; child = child->prevSibling()) {
                if (child->visible() && cast<UModalWindow>(child)) {
                    start = child;
                    break;
                }
            }
        }
        if (!start) {
            start = this;
        }
        if (!pressed) {
            for (UWindow* current = start; current; current = current->parent()) {
                if (current->rawKeyPressed(key, IST_Release, false)) {
                    return true;
                }
            }
            return isModalOpen();
        }
        for (UWindow* current = start; current; current = current->parent()) {
            if (current->destroyed) {
                break;
            }
            if (!repeat && current->rawKeyPressed(key, IST_Press, false)) {
                return true;
            }
            if (current->virtualKeyPressed(key, repeat)) {
                return true;
            }
        }
        return isModalOpen();
    }

    bool URootWindow::onChar(const std::string& chars)
    {
        UWindow* start = focusWindow();
        if (!start) {
            start = topModal();
        }
        bool alt = state().keys[IK_Alt];
        for (UWindow* current = start; current; current = current->parent()) {
            if (current->destroyed) {
                break;
            }
            if (alt && !cast<UModalWindow>(current)) {
                continue;
            }
            if (current->keyPressed(chars)) {
                return true;
            }
        }
        return isModalOpen();
    }

    namespace {
        UWindow* self(UObject* object)
        {
            return cast<UWindow>(object);
        }

        void drawTree(UWindow* window, float offsetX, float offsetY)
        {
            if (!window->visible() || window->destroyed) {
                return;
            }
            UGC* gc = sharedGC();
            offsetX += window->usedX;
            offsetY += window->usedY;
            bool clipped = window->parent() != nullptr;
            if (clipped) {
                gc->pushClip(offsetX, offsetY, window->width(), window->height());
            }
            gc->resetState(window, offsetX, offsetY);
            window->drawWindow(gc);
            UWindow* child = window->firstChild();
            while (child) {
                UWindow* following = child->nextSibling();
                drawTree(child, offsetX, offsetY);
                child = following;
            }
            if (!window->destroyed) {
                gc->resetState(window, offsetX, offsetY);
                window->postDrawWindow(gc);
            }
            if (clipped) {
                gc->popClip();
            }
        }

        void drawRoot(Canvas2D& canvas)
        {
            State& s = state();
            URootWindow* rootWindow = s.root;
            UGC* gc = sharedGC();
            if (!rootWindow || rootWindow->destroyed || !gc) {
                return;
            }
            s.viewportWidth = canvas.width();
            s.viewportHeight = canvas.height();
            Canvas2D* previous = s.canvas;
            s.canvas = &canvas;
            rootWindow->updateLayout();
            float scale = virtualScale();
            gc->resetClip(0, 0, canvas.width() / scale, canvas.height() / scale);
            UObject* raw = rootwindow::rawBackground(rootWindow);
            if (raw && !rootwindow::bRender(rootWindow)) {
                gc->resetState(rootWindow, 0, 0);
                gc->setStyle(DSTY_Normal);
                gc::tileColor(gc) = rootwindow::rawColor(rootWindow);
                float w = rootwindow::rawBackgroundWidth(rootWindow);
                float h = rootwindow::rawBackgroundHeight(rootWindow);
                gc->drawStretched(0, 0, canvas.width() / scale, canvas.height() / scale, 0, 0, w > 0 ? w : textureWidth(raw), h > 0 ? h : textureHeight(raw), raw);
            }
            drawTree(rootWindow, 0, 0);
            s.canvas = previous;
        }

        void nativeDestroy(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->destroyWindowTree();
            }
        }

        void nativeNewChild(UObject* object, Frame& stack, void* result)
        {
            UObject* cls = vm::get<UObject*>(stack);
            bool show = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UWindow* window = self(object);
            UWindow* child = window ? window->newChild(cast<UClass>(cls), show) : nullptr;
            vm::ret<UObject*>(result, child);
        }

        void nativeRaise(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            UWindow* owner = window ? window->parent() : nullptr;
            if (owner) {
                window->detachFromParent();
                window->attachLast(owner);
            }
        }

        void nativeLower(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            UWindow* owner = window ? window->parent() : nullptr;
            if (owner) {
                window->detachFromParent();
                window->attachFirst(owner);
            }
        }

        void nativeShow(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->show(value);
            }
        }

        void nativeHide(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->show(false);
            }
        }

        bool anyVisible(UWindow* window)
        {
            if (window->visible()) {
                return true;
            }
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (anyVisible(child)) {
                    return true;
                }
            }
            return false;
        }

        bool anySensitive(UWindow* window)
        {
            if (window->sensitive()) {
                return true;
            }
            for (UWindow* child = window->firstChild(); child; child = child->nextSibling()) {
                if (anySensitive(child)) {
                    return true;
                }
            }
            return false;
        }

        void nativeIsVisible(UObject* object, Frame& stack, void* result)
        {
            bool recurse = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UWindow* window = self(object);
            bool value = false;
            if (window) {
                value = window->visible();
                if (value && recurse) {
                    for (UWindow* current = window->parent(); current; current = current->parent()) {
                        if (!current->visible()) {
                            value = false;
                        }
                    }
                }
            }
            vm::retBool(result, value);
        }

        void nativeSetSensitivity(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setSensitivity(value);
            }
        }

        void nativeEnableWindow(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setSensitivity(value);
            }
        }

        void nativeDisableWindow(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setSensitivity(false);
            }
        }

        void nativeIsSensitive(UObject* object, Frame& stack, void* result)
        {
            bool recurse = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UWindow* window = self(object);
            bool value = false;
            if (window) {
                value = window->sensitive();
                if (value && recurse) {
                    for (UWindow* current = window->parent(); current; current = current->parent()) {
                        if (!current->sensitive()) {
                            value = false;
                        }
                    }
                }
            }
            vm::retBool(result, value);
        }

        void nativeSetSelectability(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            if (window && (window::bIsSelectable(window) != 0) != value) {
                window::bIsSelectable(window) = value ? 1 : 0;
                if (URootWindow* rootWindow = window->root()) {
                    rootWindow->checkFocusWindow();
                }
            }
        }

        void nativeSetBackground(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::Background(window) = texture;
                window::bDrawRawBackground(window) = 1;
            }
        }

        void nativeSetBackgroundStyle(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::backgroundStyle(window) = value;
            }
        }

        void nativeSetBackgroundSmoothing(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::bSmoothBackground(window) = value ? 1 : 0;
            }
        }

        void nativeSetBackgroundStretching(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::bStretchBackground(window) = value ? 1 : 0;
            }
        }

        void nativeGetRootWindow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window ? window->root() : nullptr);
        }

        void nativeGetModalWindow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window ? window->modalWindow() : nullptr);
        }

        void nativeGetTabGroupWindow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* found = nullptr;
            for (UWindow* current = self(object); current; current = current->parent()) {
                if (cast<UTabGroupWindow>(current)) {
                    found = current;
                    break;
                }
            }
            vm::ret<UObject*>(result, found);
        }

        void nativeGetParent(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window ? window->parent() : nullptr);
        }

        void nativeGetPlayerPawn(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window ? window->playerPawn() : nullptr);
        }

        void nativeSetConfiguration(UObject* object, Frame& stack, void*)
        {
            float newX = vm::get<float>(stack);
            float newY = vm::get<float>(stack);
            float newWidth = vm::get<float>(stack);
            float newHeight = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->x() = newX;
                window->y() = newY;
                window->setWidth(newWidth);
                window->setHeight(newHeight);
            }
        }

        void nativeSetSize(UObject* object, Frame& stack, void*)
        {
            float newWidth = vm::get<float>(stack);
            float newHeight = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setWidth(newWidth);
                window->setHeight(newHeight);
            }
        }

        void nativeSetPos(UObject* object, Frame& stack, void*)
        {
            float newX = vm::get<float>(stack);
            float newY = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->x() = newX;
                window->y() = newY;
            }
        }

        void nativeSetWidth(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setWidth(value);
            }
        }

        void nativeSetHeight(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setHeight(value);
            }
        }

        void nativeResetSize(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->fixedWidth = false;
                window->fixedHeight = false;
                window::bNeedsReconfigure(window) = 1;
                window->askParentForReconfigure();
            }
        }

        void nativeResetWidth(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->fixedWidth = false;
                window::bNeedsReconfigure(window) = 1;
                window->askParentForReconfigure();
            }
        }

        void nativeResetHeight(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->fixedHeight = false;
                window::bNeedsReconfigure(window) = 1;
                window->askParentForReconfigure();
            }
        }

        void nativeSetWindowAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t halign = vm::get<uint8_t>(stack);
            uint8_t valign = vm::get<uint8_t>(stack);
            float h0 = 0;
            float v0 = 0;
            float h1 = 0;
            float v1 = 0;
            bool hasH0 = optional(stack, h0);
            bool hasV0 = optional(stack, v0);
            bool hasH1 = optional(stack, h1);
            bool hasV1 = optional(stack, v1);
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window) {
                return;
            }
            window::winHAlign(window) = halign;
            window::winVAlign(window) = valign;
            if (hasH0) {
                window::hMargin0(window) = h0;
                window::hMargin1(window) = hasH1 ? h1 : h0;
            } else if (hasH1) {
                window::hMargin1(window) = h1;
            }
            if (hasV0) {
                window::vMargin0(window) = v0;
                window::vMargin1(window) = hasV1 ? v1 : v0;
            } else if (hasV1) {
                window::vMargin1(window) = v1;
            }
            window::bNeedsReconfigure(window) = 1;
            window->askParentForReconfigure();
        }

        void nativeSetAcceleratorText(UObject* object, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window) {
                return;
            }
            int32_t key = 0;
            size_t pos = text.find("|&");
            if (pos != std::string::npos && pos + 2 < text.size()) {
                key = std::toupper(static_cast<unsigned char>(text[pos + 2]));
            }
            window::acceleratorKey(window) = key;
            if (UWindow* modal = window->modalWindow()) {
                modalwindow::bDirtyAccelerators(modal) = 1;
            }
        }

        void nativeSetFocusWindow(UObject* object, Frame& stack, void* result)
        {
            UObject* target = vm::get<UObject*>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            vm::retBool(result, window && window->setFocusWindow(windowOf(target)));
        }

        void nativeGetFocusWindow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            URootWindow* rootWindow = window ? window->root() : nullptr;
            vm::ret<UObject*>(result, rootWindow ? rootWindow->focusWindow() : nullptr);
        }

        void nativeIsFocusWindow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            URootWindow* rootWindow = window ? window->root() : nullptr;
            vm::retBool(result, rootWindow && rootWindow->focusWindow() == window);
        }

        void nativeMoveFocusLeft(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveFocus(0) : nullptr);
        }

        void nativeMoveFocusRight(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveFocus(1) : nullptr);
        }

        void nativeMoveFocusUp(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveFocus(2) : nullptr);
        }

        void nativeMoveFocusDown(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveFocus(3) : nullptr);
        }

        void nativeMoveTabGroupNext(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveTabGroup(3) : nullptr);
        }

        void nativeMoveTabGroupPrev(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window && window->root() ? window->root()->moveTabGroup(2) : nullptr);
        }

        void convertCoordinates(UWindow* from, float fromX, float fromY, UWindow* to, float& toX, float& toY)
        {
            float ax = 0;
            float ay = 0;
            if (from) {
                from->absolutePosition(ax, ay);
            }
            float bx = 0;
            float by = 0;
            if (to) {
                to->absolutePosition(bx, by);
            }
            toX = fromX + ax - bx;
            toY = fromY + ay - by;
        }

        void nativeConvertCoordinates(UObject*, Frame& stack, void*)
        {
            UObject* from = vm::get<UObject*>(stack);
            float fromX = vm::get<float>(stack);
            float fromY = vm::get<float>(stack);
            UObject* to = vm::get<UObject*>(stack);
            float* toX = vm::getRef<float>(stack);
            float* toY = vm::getRef<float>(stack);
            vm::finish(stack);
            convertCoordinates(windowOf(from), fromX, fromY, windowOf(to), *toX, *toY);
        }

        void nativeGrabMouse(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->grabMouse();
            }
        }

        void nativeUngrabMouse(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->ungrabMouse();
            }
        }

        void nativeGetCursorPos(UObject* object, Frame& stack, void*)
        {
            float* outX = vm::getRef<float>(stack);
            float* outY = vm::getRef<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            URootWindow* rootWindow = window ? window->root() : nullptr;
            if (rootWindow) {
                convertCoordinates(rootWindow, rootWindow->mouseX(), rootWindow->mouseY(), window, *outX, *outY);
            } else {
                *outX = 0;
                *outY = 0;
            }
        }

        void nativeSetCursorPos(UObject* object, Frame& stack, void*)
        {
            float newX = vm::get<float>(stack);
            float newY = vm::get<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            URootWindow* rootWindow = window ? window->root() : nullptr;
            if (rootWindow) {
                float rx = 0;
                float ry = 0;
                convertCoordinates(window, newX, newY, rootWindow, rx, ry);
                rootWindow->setRootCursorPos(rx, ry);
            }
        }

        void nativeSetDefaultCursor(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            UObject* shadow = nullptr;
            float hotX = -1;
            float hotY = -1;
            Color color {};
            bool hasShadow = optional(stack, shadow);
            optional(stack, hotX);
            optional(stack, hotY);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window) {
                return;
            }
            window::defaultCursor(window) = texture;
            if (hasShadow) {
                window::defaultCursorShadow(window) = shadow;
            }
            window::defaultHotX(window) = hotX;
            window::defaultHotY(window) = hotY;
            if (hasColor) {
                window::defaultCursorColor(window) = color;
            }
        }

        void nativeChildSibling(UObject* object, Frame& stack, void* result, int mode)
        {
            bool visibleOnly = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UWindow* window = self(object);
            UWindow* found = nullptr;
            if (window) {
                UWindow* current = nullptr;
                if (mode == 0) {
                    current = window->lastChild();
                } else if (mode == 1) {
                    current = window->firstChild();
                } else if (mode == 2) {
                    current = window->nextSibling();
                } else {
                    current = window->prevSibling();
                }
                bool forward = mode == 1 || mode == 2;
                while (current) {
                    if (!visibleOnly || current->visible()) {
                        found = current;
                        break;
                    }
                    current = forward ? current->nextSibling() : current->prevSibling();
                }
            }
            vm::ret<UObject*>(result, found);
        }

        void nativeGetTopChild(UObject* object, Frame& stack, void* result)
        {
            nativeChildSibling(object, stack, result, 0);
        }

        void nativeGetBottomChild(UObject* object, Frame& stack, void* result)
        {
            nativeChildSibling(object, stack, result, 1);
        }

        void nativeGetHigherSibling(UObject* object, Frame& stack, void* result)
        {
            nativeChildSibling(object, stack, result, 2);
        }

        void nativeGetLowerSibling(UObject* object, Frame& stack, void* result)
        {
            nativeChildSibling(object, stack, result, 3);
        }

        void nativeDestroyAllChildren(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window) {
                return;
            }
            while (UWindow* child = window->firstChild()) {
                child->destroyWindowTree();
                if (window->firstChild() == child) {
                    child->detachFromParent();
                }
            }
        }

        void nativeAskParentForReconfigure(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->askParentForReconfigure();
            }
        }

        void nativeConfigureChild(UObject* object, Frame& stack, void*)
        {
            float newX = vm::get<float>(stack);
            float newY = vm::get<float>(stack);
            float newWidth = vm::get<float>(stack);
            float newHeight = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->configureChild(newX, newY, newWidth, newHeight);
            }
        }

        void nativeResizeChild(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::bNeedsQuery(window) = 1;
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeQueryPreferredWidth(UObject* object, Frame& stack, void* result)
        {
            float queryHeight = vm::get<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<float>(result, window ? window->queryPreferredWidth(queryHeight) : 0.0f);
        }

        void nativeQueryPreferredHeight(UObject* object, Frame& stack, void* result)
        {
            float queryWidth = vm::get<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<float>(result, window ? window->queryPreferredHeight(queryWidth) : 0.0f);
        }

        void nativeQueryPreferredSize(UObject* object, Frame& stack, void*)
        {
            float* outWidth = vm::getRef<float>(stack);
            float* outHeight = vm::getRef<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            float w = 0;
            float h = 0;
            if (window) {
                window->queryPreferredSize(w, h);
            }
            *outWidth = w;
            *outHeight = h;
        }

        void nativeQueryGranularity(UObject* object, Frame& stack, void*)
        {
            float* outH = vm::getRef<float>(stack);
            float* outV = vm::getRef<float>(stack);
            vm::finish(stack);
            float h = 1;
            float v = 1;
            if (UWindow* window = self(object)) {
                window->parentRequestedGranularity(h, v);
            }
            *outH = h;
            *outV = v;
        }

        void nativeSetChildVisibility(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setChildVisibility(value);
            }
        }

        void nativeAskParentToShowArea(UObject* object, Frame& stack, void*)
        {
            float areaX = 0;
            float areaY = 0;
            float areaWidth = 0;
            float areaHeight = 0;
            UWindow* window = self(object);
            if (window) {
                areaWidth = window->width();
                areaHeight = window->height();
            }
            optional(stack, areaX);
            optional(stack, areaY);
            optional(stack, areaWidth);
            optional(stack, areaHeight);
            vm::finish(stack);
            if (window) {
                window->askParentToShowArea(areaX, areaY, areaWidth, areaHeight);
            }
        }

        void nativeConvertScriptString(UObject*, Frame& stack, void* result)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            vm::ret<std::string>(result, text);
        }

        void nativeIsKeyDown(UObject*, Frame& stack, void* result)
        {
            uint8_t key = vm::get<uint8_t>(stack);
            vm::finish(stack);
            vm::retBool(result, state().keys[key]);
        }

        void nativeIsPointInWindow(UObject* object, Frame& stack, void* result)
        {
            float px = vm::get<float>(stack);
            float py = vm::get<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            vm::retBool(result, window && px >= 0 && py >= 0 && px < window->width() && py < window->height());
        }

        void nativeFindWindow(UObject* object, Frame& stack, void* result)
        {
            float px = vm::get<float>(stack);
            float py = vm::get<float>(stack);
            float* outX = vm::getRef<float>(stack);
            float* outY = vm::getRef<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            UWindow* found = nullptr;
            if (window) {
                float ax = 0;
                float ay = 0;
                window->absolutePosition(ax, ay);
                URootWindow* rootWindow = window->root();
                UWindow* current = rootWindow ? static_cast<UWindow*>(rootWindow) : window;
                float cx = 0;
                float cy = 0;
                current->absolutePosition(cx, cy);
                float x = px + ax - cx;
                float y = py + ay - cy;
                while (true) {
                    UWindow* hit = nullptr;
                    for (UWindow* child = current->lastChild(); child; child = child->prevSibling()) {
                        if (child->visible() && child->usedX <= x && child->usedY <= y && child->usedX + child->width() > x && child->usedY + child->height() > y) {
                            hit = child;
                            break;
                        }
                    }
                    if (!hit) {
                        break;
                    }
                    current = hit;
                    x -= hit->usedX;
                    y -= hit->usedY;
                }
                found = current;
                *outX = x;
                *outY = y;
            }
            vm::ret<UObject*>(result, found);
        }

        void nativePlaySound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            float volume = 1;
            float pitch = 1;
            float posX = 0;
            float posY = 0;
            optional(stack, volume);
            optional(stack, pitch);
            optional(stack, posX);
            optional(stack, posY);
            vm::finish(stack);
            playSound(self(object), sound, volume, pitch);
        }

        void nativeSetSoundVolume(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::SoundVolume(window) = value;
            }
        }

        void nativeSetTileColor(UObject* object, Frame& stack, void*)
        {
            Color value = vm::get<Color>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::tileColor(window) = value;
            }
        }

        void nativeSetTextColor(UObject* object, Frame& stack, void*)
        {
            Color value = vm::get<Color>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::TextColor(window) = value;
            }
        }

        void nativeSetFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->setFont(font);
                window->askParentForReconfigure();
            }
        }

        void nativeSetFonts(UObject* object, Frame& stack, void*)
        {
            UObject* normal = vm::get<UObject*>(stack);
            UObject* bold = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::normalFont(window) = normal;
                window::boldFont(window) = bold;
                window->askParentForReconfigure();
            }
        }

        void nativeSetNormalFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::normalFont(window) = font;
                window->askParentForReconfigure();
            }
        }

        void nativeSetBoldFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::boldFont(window) = font;
            }
        }

        void nativeEnableSpecialText(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::bSpecialText(window) = value ? 1 : 0;
            }
        }

        void nativeEnableTranslucentText(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::bTextTranslucent(window) = value ? 1 : 0;
            }
        }

        void nativeCarriageReturn(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<std::string>(result, "\n");
        }

        void nativeSetBaselineData(UObject* object, Frame& stack, void*)
        {
            float baseline = 0;
            float underline = 0;
            bool hasBaseline = optional(stack, baseline);
            bool hasUnderline = optional(stack, underline);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                if (hasBaseline) {
                    window->baselineOffset = baseline;
                }
                if (hasUnderline) {
                    window->underlineHeight = underline;
                }
            }
        }

        void nativeGetGC(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            State& s = state();
            if (!s.windowGc) {
                UClass* cls = extensionClass("Extension.GC");
                if (cls) {
                    s.windowGc = cast<UGC>(objects::newObject(cls, objects::packageObject(Name("Transient")), Name("WindowGC"), RF_Transient));
                }
            }
            UWindow* window = self(object);
            if (s.windowGc && window) {
                float x = 0;
                float y = 0;
                for (UWindow* current = window; current; current = current->parent()) {
                    x += current->usedX;
                    y += current->usedY;
                }
                s.windowGc->resetState(window, x, y);
            }
            vm::ret<UObject*>(result, s.windowGc);
        }

        void nativeReleaseGC(UObject*, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            vm::finish(stack);
        }

        void nativeSetClientObject(UObject* object, Frame& stack, void*)
        {
            UObject* client = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window::clientObject(window) = client;
            }
        }

        void nativeGetClientObject(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            vm::ret<UObject*>(result, window ? window::clientObject(window) : nullptr);
        }

        void nativeConvertVectorToCoordinates(UObject* object, Frame& stack, void* result)
        {
            Vector location = vm::get<Vector>(stack);
            float* outX = vm::getRef<float>(stack);
            float* outY = vm::getRef<float>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            URootWindow* rootWindow = window ? window->root() : nullptr;
            float pixelX = 0;
            float pixelY = 0;
            if (!window || !rootWindow || !state().projector) {
                vm::retBool(result, false);
                return;
            }
            bool inFront = state().projector(location, pixelX, pixelY);
            float rootX = 0;
            float rootY = 0;
            if (inFront) {
                float scale = virtualScale();
                rootX = std::trunc(pixelX / scale - rootWindow->usedX);
                rootY = std::trunc(pixelY / scale - rootWindow->usedY);
            } else {
                rootX = std::trunc(state().viewportWidth * 0.5f);
                rootY = std::trunc(state().viewportHeight * 0.5f);
            }
            float ax = 0;
            float ay = 0;
            window->absolutePosition(ax, ay);
            if (outX) {
                *outX = rootX - (ax - rootWindow->usedX);
            }
            if (outY) {
                *outY = rootY - (ay - rootWindow->usedY);
            }
            vm::retBool(result, inFront);
        }

        void nativeAddTimer(UObject* object, Frame& stack, void* result)
        {
            float timeout = vm::get<float>(stack);
            bool loop = optionalBoolDefault(stack, false);
            int32_t clientData = 0;
            Name function;
            optional(stack, clientData);
            optional(stack, function);
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window || function.none()) {
                vm::ret<int32_t>(result, -1);
                return;
            }
            int32_t id = window->nextTimerId++;
            UWindow::Timer timer;
            timer.function = function;
            timer.timeout = timeout;
            timer.loop = loop;
            timer.clientData = clientData;
            timer.left = timeout;
            window->timers[id] = timer;
            vm::ret<int32_t>(result, id);
        }

        void nativeRemoveTimer(UObject* object, Frame& stack, void*)
        {
            int32_t id = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                window->timers.erase(id);
            }
        }

        void nativeGetTickOffset(UObject*, Frame& stack, void* result)
        {
            vm::finish(stack);
            vm::ret<float>(result, 0.0f);
        }

        void nativeChangeStyle(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UWindow* window = self(object);
            if (!window) {
                return;
            }
            UWindow* top = window;
            while (top->parent()) {
                top = top->parent();
            }
            std::vector<UWindow*> stackWindows { top };
            while (!stackWindows.empty()) {
                UWindow* current = stackWindows.back();
                stackWindows.pop_back();
                static const Name eventName("StyleChanged");
                vm::Call call(current, eventName);
                if (call.valid()) {
                    call.run();
                }
                for (UWindow* child = current->firstChild(); child; child = child->nextSibling()) {
                    stackWindows.push_back(child);
                }
            }
        }

        void nativeSetFocusSounds(UObject* object, Frame& stack, void*)
        {
            UObject* focus = nullptr;
            UObject* unfocus = nullptr;
            bool hasFocus = optional(stack, focus);
            bool hasUnfocus = optional(stack, unfocus);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                if (hasFocus) {
                    window::focusSound(window) = focus;
                }
                if (hasUnfocus) {
                    window::unfocusSound(window) = unfocus;
                }
            }
        }

        void nativeSetVisibilitySounds(UObject* object, Frame& stack, void*)
        {
            UObject* visibleSound = nullptr;
            UObject* invisibleSound = nullptr;
            bool hasVisible = optional(stack, visibleSound);
            bool hasInvisible = optional(stack, invisibleSound);
            vm::finish(stack);
            if (UWindow* window = self(object)) {
                if (hasVisible) {
                    window::visibleSound(window) = visibleSound;
                }
                if (hasInvisible) {
                    window::invisibleSound(window) = invisibleSound;
                }
            }
        }

        struct ActorRefLayout {
            UProperty* list = nullptr;
            size_t actorOffset = 0;
            size_t countOffset = 0;
            size_t stride = 0;
            int32_t capacity = 0;
        };

        ActorRefLayout actorRefLayout(UObject* player)
        {
            ActorRefLayout layout;
            if (!player || !player->cls) {
                return layout;
            }
            layout.list = player->cls->findProperty(Name("actorList"));
            if (!layout.list || !layout.list->structType) {
                layout.list = nullptr;
                return layout;
            }
            UProperty* actor = layout.list->structType->findProperty(Name("Actor"));
            UProperty* count = layout.list->structType->findProperty(Name("RefCount"));
            if (!actor || !count) {
                layout.list = nullptr;
                return layout;
            }
            layout.actorOffset = actor->offset;
            layout.countOffset = count->offset;
            layout.stride = layout.list->elementSize();
            layout.capacity = layout.list->arrayDim;
            return layout;
        }

        void nativeAddActorRef(UObject* object, Frame& stack, void*)
        {
            UObject* actor = vm::get<UObject*>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            UObject* player = window ? window->playerPawn() : nullptr;
            ActorRefLayout layout = actorRefLayout(player);
            if (!layout.list || !actor) {
                return;
            }
            uint8_t* base = player->data + layout.list->offset;
            int32_t& count = playerpawnext::actorCount(player);
            for (int32_t i = 0; i < count; i++) {
                uint8_t* entry = base + static_cast<size_t>(i) * layout.stride;
                if (*reinterpret_cast<UObject**>(entry + layout.actorOffset) == actor) {
                    (*reinterpret_cast<int32_t*>(entry + layout.countOffset))++;
                    return;
                }
            }
            if (count >= layout.capacity) {
                for (int32_t i = 1; i < count; i++) {
                    std::copy_n(base + static_cast<size_t>(i) * layout.stride, layout.stride, base + static_cast<size_t>(i - 1) * layout.stride);
                }
                count--;
            }
            uint8_t* entry = base + static_cast<size_t>(count) * layout.stride;
            *reinterpret_cast<UObject**>(entry + layout.actorOffset) = actor;
            *reinterpret_cast<int32_t*>(entry + layout.countOffset) = 1;
            count++;
        }

        void nativeRemoveActorRef(UObject* object, Frame& stack, void*)
        {
            UObject* actor = vm::get<UObject*>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            UObject* player = window ? window->playerPawn() : nullptr;
            ActorRefLayout layout = actorRefLayout(player);
            if (!layout.list || !actor) {
                return;
            }
            uint8_t* base = player->data + layout.list->offset;
            int32_t& count = playerpawnext::actorCount(player);
            for (int32_t i = 0; i < count; i++) {
                uint8_t* entry = base + static_cast<size_t>(i) * layout.stride;
                if (*reinterpret_cast<UObject**>(entry + layout.actorOffset) != actor) {
                    continue;
                }
                int32_t& refs = *reinterpret_cast<int32_t*>(entry + layout.countOffset);
                refs--;
                if (refs <= 0) {
                    for (int32_t j = i + 1; j < count; j++) {
                        std::copy_n(base + static_cast<size_t>(j) * layout.stride, layout.stride, base + static_cast<size_t>(j - 1) * layout.stride);
                    }
                    uint8_t* last = base + static_cast<size_t>(count - 1) * layout.stride;
                    *reinterpret_cast<UObject**>(last + layout.actorOffset) = nullptr;
                    *reinterpret_cast<int32_t*>(last + layout.countOffset) = 0;
                    count--;
                }
                return;
            }
        }

        void nativeIsActorValid(UObject* object, Frame& stack, void* result)
        {
            UObject* actor = vm::get<UObject*>(stack);
            vm::finish(stack);
            UWindow* window = self(object);
            UObject* player = window ? window->playerPawn() : nullptr;
            ActorRefLayout layout = actorRefLayout(player);
            bool valid = false;
            if (layout.list && actor && !actor->destroyed) {
                uint8_t* base = player->data + layout.list->offset;
                int32_t count = playerpawnext::actorCount(player);
                for (int32_t i = 0; i < count; i++) {
                    uint8_t* entry = base + static_cast<size_t>(i) * layout.stride;
                    if (*reinterpret_cast<UObject**>(entry + layout.actorOffset) == actor) {
                        valid = true;
                    }
                }
            }
            vm::retBool(result, valid);
        }

        URootWindow* rootSelf(UObject* object)
        {
            return cast<URootWindow>(object);
        }

        void nativeRootOptionalBool(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                field(rootWindow) = value ? 1 : 0;
            }
        }

        void nativeEnablePositionalSound(UObject* object, Frame& stack, void*)
        {
            nativeRootOptionalBool(object, stack, rootwindow::bPositionalSound);
        }

        void nativeEnableRendering(UObject* object, Frame& stack, void*)
        {
            nativeRootOptionalBool(object, stack, rootwindow::bRender);
        }

        void nativeShowCursor(UObject* object, Frame& stack, void*)
        {
            nativeRootOptionalBool(object, stack, rootwindow::bCursorVisible);
        }

        void nativeStretchRawBackground(UObject* object, Frame& stack, void*)
        {
            nativeRootOptionalBool(object, stack, rootwindow::bStretchRawBackground);
        }

        void nativeIsPositionalSoundEnabled(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            URootWindow* rootWindow = rootSelf(object);
            vm::retBool(result, rootWindow && rootwindow::bPositionalSound(rootWindow));
        }

        void nativeIsRenderingEnabled(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            URootWindow* rootWindow = rootSelf(object);
            vm::retBool(result, rootWindow && rootwindow::bRender(rootWindow));
        }

        void nativeLockMouse(UObject* object, Frame& stack, void*)
        {
            bool lockMove = optionalBoolDefault(stack, true);
            bool lockButton = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                rootwindow::bMouseMoveLocked(rootWindow) = lockMove ? 1 : 0;
                rootwindow::bMouseButtonLocked(rootWindow) = lockButton ? 1 : 0;
            }
        }

        void nativeSetDefaultEditCursor(UObject* object, Frame& stack, void*)
        {
            UObject* cursor = nullptr;
            bool has = optional(stack, cursor);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                if (has) {
                    rootwindow::defaultEditCursor(rootWindow) = cursor;
                }
            }
        }

        void nativeSetDefaultMovementCursors(UObject* object, Frame& stack, void*)
        {
            UObject* values[5] = {};
            bool present[5] = {};
            for (int i = 0; i < 5; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            URootWindow* rootWindow = rootSelf(object);
            if (!rootWindow) {
                return;
            }
            UObject*& (*fields[5])(UObject*) = { rootwindow::DefaultMoveCursor, rootwindow::defaultHorizontalMoveCursor, rootwindow::defaultVerticalMoveCursor, rootwindow::defaultTopLeftMoveCursor, rootwindow::defaultTopRightMoveCursor };
            for (int i = 0; i < 5; i++) {
                if (present[i]) {
                    fields[i](rootWindow) = values[i];
                }
            }
        }

        void nativeSetRawBackground(UObject* object, Frame& stack, void*)
        {
            UObject* texture = nullptr;
            Color color {};
            bool hasTexture = optional(stack, texture);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                if (hasTexture) {
                    rootwindow::rawBackground(rootWindow) = texture;
                }
                if (hasColor) {
                    rootwindow::rawColor(rootWindow) = color;
                }
            }
        }

        void nativeSetRawBackgroundSize(UObject* object, Frame& stack, void*)
        {
            float w = vm::get<float>(stack);
            float h = vm::get<float>(stack);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                rootwindow::rawBackgroundWidth(rootWindow) = w;
                rootwindow::rawBackgroundHeight(rootWindow) = h;
            }
        }

        void nativeSetRenderViewport(UObject* object, Frame& stack, void*)
        {
            float newX = vm::get<float>(stack);
            float newY = vm::get<float>(stack);
            float newWidth = vm::get<float>(stack);
            float newHeight = vm::get<float>(stack);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                rootwindow::renderX(rootWindow) = newX;
                rootwindow::renderY(rootWindow) = newY;
                rootwindow::renderWidth(rootWindow) = newWidth;
                rootwindow::renderHeight(rootWindow) = newHeight;
                rootWindow->renderViewportSet = true;
            }
        }

        void nativeResetRenderViewport(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                rootWindow->renderViewportSet = false;
            }
        }

        void nativeSetSnapshotSize(UObject* object, Frame& stack, void*)
        {
            float w = vm::get<float>(stack);
            float h = vm::get<float>(stack);
            vm::finish(stack);
            if (URootWindow* rootWindow = rootSelf(object)) {
                rootwindow::snapshotWidth(rootWindow) = static_cast<int32_t>(w);
                rootwindow::snapshotHeight(rootWindow) = static_cast<int32_t>(h);
            }
        }

        void nativeGenerateSnapshot(UObject*, Frame& stack, void* result)
        {
            optionalBoolDefault(stack, false);
            vm::finish(stack);
            vm::ret<UObject*>(result, nullptr);
        }

        void nativeIsCurrentModal(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UModalWindow* modal = cast<UModalWindow>(object);
            vm::retBool(result, modal && modal->isCurrentModal());
        }

        void nativeSetMouseFocusMode(UObject* object, Frame& stack, void*)
        {
            uint8_t mode = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (cast<UModalWindow>(object)) {
                modalwindow::focusMode(object) = mode;
            }
        }

        void nativeInitRootWindow(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (!object) {
                return;
            }
            std::string className = config::system().get("Engine.Engine", "Root").value_or("DeusEx.DeusExRootWindow");
            UClass* cls = extensionClass(className.c_str());
            if (!cls) {
                cls = extensionClass("Extension.RootWindow");
            }
            if (!cls) {
                return;
            }
            UObject* created = objects::newObject(cls, objects::packageObject(Name("Transient")), Name("RootWindow"), RF_Transient);
            objects::flushLoads();
            URootWindow* rootWindow = cast<URootWindow>(created);
            if (!rootWindow) {
                std::fprintf(stderr, "extension: root window class %s is not a RootWindow\n", className.c_str());
                return;
            }
            State& s = state();
            if (s.root && s.root != rootWindow && !s.root->destroyed) {
                s.root->destroyWindowTree();
            }
            s.root = rootWindow;
            playerpawnext::RootWindow(object) = rootWindow;
            rootwindow::parentPawn(rootWindow) = object;
            window::bIsVisible(rootWindow) = 1;
            window::bIsSensitive(rootWindow) = 1;
            rootwindow::bRender(rootWindow) = 1;
            rootwindow::bCursorVisible(rootWindow) = 1;
            rootwindow::bPositionalSound(rootWindow) = 1;
            rootWindow->width() = virtualWidth();
            rootWindow->height() = virtualHeight();
            window::bConfigured(rootWindow) = 1;
            rootWindow->initWindow();
            window::bIsInitialized(rootWindow) = 1;
        }

        void nativePreRenderWindows(UObject*, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            vm::finish(stack);
            State& s = state();
            if (s.root && !s.root->destroyed) {
                s.root->updateLayout();
            }
        }

        void nativePostRenderWindows(UObject*, Frame& stack, void*)
        {
            vm::get<UObject*>(stack);
            vm::finish(stack);
            State& s = state();
            if (s.frameCanvas) {
                drawRoot(*s.frameCanvas);
            }
        }

        void nativeStringToName(UObject*, Frame& stack, void* result)
        {
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            vm::ret<Name>(result, Name(text));
        }
    }

    void registerWindowNatives()
    {
        vm::registerNative(1409, nativeDestroy);
        vm::registerNative(1410, nativeNewChild);
        vm::registerNative(1411, nativeRaise);
        vm::registerNative(1412, nativeLower);
        vm::registerNative(1413, nativeShow);
        vm::registerNative(1414, nativeHide);
        vm::registerNative(1415, nativeIsVisible);
        vm::registerNative(1416, nativeSetSensitivity);
        vm::registerNative(1417, nativeEnableWindow);
        vm::registerNative(1418, nativeDisableWindow);
        vm::registerNative(1419, nativeIsSensitive);
        vm::registerNative(1420, nativeSetSelectability);
        vm::registerNative(1421, nativeSetBackground);
        vm::registerNative(1422, nativeSetBackgroundStyle);
        vm::registerNative(1423, nativeSetBackgroundSmoothing);
        vm::registerNative(1424, nativeSetBackgroundStretching);
        vm::registerNative(1425, nativeGetRootWindow);
        vm::registerNative(1426, nativeGetModalWindow);
        vm::registerNative(1427, nativeGetTabGroupWindow);
        vm::registerNative(1428, nativeGetParent);
        vm::registerNative(1429, nativeGetPlayerPawn);
        vm::registerNative(1430, nativeSetConfiguration);
        vm::registerNative(1431, nativeSetSize);
        vm::registerNative(1432, nativeSetPos);
        vm::registerNative(1433, nativeSetWidth);
        vm::registerNative(1434, nativeSetHeight);
        vm::registerNative(1435, nativeResetSize);
        vm::registerNative(1436, nativeResetWidth);
        vm::registerNative(1437, nativeResetHeight);
        vm::registerNative(1438, nativeSetWindowAlignments);
        vm::registerNative(1439, nativeSetAcceleratorText);
        vm::registerNative(1440, nativeSetFocusWindow);
        vm::registerNative(1441, nativeGetFocusWindow);
        vm::registerNative(1442, nativeMoveFocusLeft);
        vm::registerNative(1443, nativeMoveFocusRight);
        vm::registerNative(1444, nativeMoveFocusUp);
        vm::registerNative(1445, nativeMoveFocusDown);
        vm::registerNative(1446, nativeMoveTabGroupNext);
        vm::registerNative(1447, nativeMoveTabGroupPrev);
        vm::registerNative(1448, nativeIsFocusWindow);
        vm::registerNative(1449, nativeConvertCoordinates);
        vm::registerNative(1450, nativeGrabMouse);
        vm::registerNative(1451, nativeUngrabMouse);
        vm::registerNative(1452, nativeGetCursorPos);
        vm::registerNative(1453, nativeSetCursorPos);
        vm::registerNative(1454, nativeSetDefaultCursor);
        vm::registerNative(1455, nativeGetTopChild);
        vm::registerNative(1456, nativeGetBottomChild);
        vm::registerNative(1457, nativeGetHigherSibling);
        vm::registerNative(1458, nativeGetLowerSibling);
        vm::registerNative(1459, nativeDestroyAllChildren);
        vm::registerNative(1460, nativeAskParentForReconfigure);
        vm::registerNative(1461, nativeConfigureChild);
        vm::registerNative(1462, nativeResizeChild);
        vm::registerNative(1463, nativeQueryPreferredWidth);
        vm::registerNative(1464, nativeQueryPreferredHeight);
        vm::registerNative(1465, nativeQueryPreferredSize);
        vm::registerNative(1466, nativeQueryGranularity);
        vm::registerNative(1467, nativeSetChildVisibility);
        vm::registerNative(1468, nativeAskParentToShowArea);
        vm::registerNative(1469, nativeConvertScriptString);
        vm::registerNative(1470, nativeIsKeyDown);
        vm::registerNative(1471, nativeIsPointInWindow);
        vm::registerNative(1472, nativeFindWindow);
        vm::registerNative(1473, nativePlaySound);
        vm::registerNative(1474, nativeSetSoundVolume);
        vm::registerNative(1475, nativeSetTileColor);
        vm::registerNative(1476, nativeSetTextColor);
        vm::registerNative(1477, nativeSetFont);
        vm::registerNative(1478, nativeSetFonts);
        vm::registerNative(1479, nativeSetNormalFont);
        vm::registerNative(1480, nativeSetBoldFont);
        vm::registerNative(1481, nativeEnableSpecialText);
        vm::registerNative(1482, nativeCarriageReturn);
        vm::registerNative(1483, nativeEnableTranslucentText);
        vm::registerNative(1484, nativeSetBaselineData);
        vm::registerNative(1485, nativeGetGC);
        vm::registerNative(1486, nativeReleaseGC);
        vm::registerNative(1487, nativeSetClientObject);
        vm::registerNative(1488, nativeGetClientObject);
        vm::registerNative(1489, nativeConvertVectorToCoordinates);
        vm::registerNative(1490, nativeAddTimer);
        vm::registerNative(1491, nativeRemoveTimer);
        vm::registerNative(1492, nativeGetTickOffset);
        vm::registerNative(1493, nativeChangeStyle);
        vm::registerNative(1495, nativeSetFocusSounds);
        vm::registerNative(1496, nativeSetVisibilitySounds);
        vm::registerNative(1497, nativeAddActorRef);
        vm::registerNative(1498, nativeRemoveActorRef);
        vm::registerNative(1499, nativeIsActorValid);
        vm::registerNative(1500, nativeSetMouseFocusMode);
        vm::registerNative(1501, nativeIsCurrentModal);
        vm::registerNative(1510, nativeSetDefaultEditCursor);
        vm::registerNative(1511, nativeSetDefaultMovementCursors);
        vm::registerNative(1512, nativeEnableRendering);
        vm::registerNative(1513, nativeIsRenderingEnabled);
        vm::registerNative(1514, nativeSetRenderViewport);
        vm::registerNative(1515, nativeResetRenderViewport);
        vm::registerNative(1516, nativeSetRawBackground);
        vm::registerNative(1517, nativeSetRawBackgroundSize);
        vm::registerNative(1518, nativeStretchRawBackground);
        vm::registerNative(1519, nativeEnablePositionalSound);
        vm::registerNative(1520, nativeIsPositionalSoundEnabled);
        vm::registerNative(1521, nativeLockMouse);
        vm::registerNative(1522, nativeShowCursor);
        vm::registerNative(1523, nativeSetSnapshotSize);
        vm::registerNative(1524, nativeGenerateSnapshot);
        vm::registerNative(1050, nativePreRenderWindows);
        vm::registerNative(1051, nativePostRenderWindows);
        vm::registerNative(1052, nativeInitRootWindow);
        vm::registerNative(1024, nativeStringToName);
    }

    void registerNatives()
    {
        registerFactories();
        registerWindowNatives();
        registerGCNatives();
        registerTextWindowNatives();
        registerContainerNatives();
        registerListNatives();
        registerScaleNatives();
        registerComputerNatives();
        registerFlagNatives();
        registerConSysNatives();
        registerTextParserNatives();
    }

    void tick(float deltaSeconds)
    {
        State& s = state();
        s.time += static_cast<double>(deltaSeconds);
        if (s.root && !s.root->destroyed) {
            s.root->tick(deltaSeconds);
        }
    }

    void render(Canvas2D& canvas)
    {
        drawRoot(canvas);
    }

    bool keyEvent(uint8_t key, uint8_t action, float delta)
    {
        State& s = state();
        if (action == IST_Axis) {
            if (!s.root || s.root->destroyed) {
                return false;
            }
            if (key == IK_MouseX) {
                return s.root->onRawMouseMove(delta, 0);
            }
            if (key == IK_MouseY) {
                return s.root->onRawMouseMove(0, -delta);
            }
            return s.root->isModalOpen();
        }
        bool pressed = action == IST_Press || action == IST_Hold;
        bool wasDown = s.keys[key];
        if (action == IST_Press) {
            s.keys[key] = true;
        } else if (action == IST_Release) {
            s.keys[key] = false;
            if (!wasDown) {
                return s.root && !s.root->destroyed && s.root->isModalOpen();
            }
        }
        if (key == IK_LeftMouse || key == IK_RightMouse || key == IK_MiddleMouse || key == IK_MouseWheelUp || key == IK_MouseWheelDown) {
            if (action == IST_Hold) {
                return s.root && !s.root->destroyed && s.root->isModalOpen();
            }
            return mouseButton(key, pressed);
        }
        if (!s.root || s.root->destroyed || s.keyboardGrabs <= 0) {
            return false;
        }
        return s.root->onKey(key, pressed, action == IST_Hold);
    }

    bool charEvent(char character)
    {
        State& s = state();
        if (!s.root || s.root->destroyed || s.keyboardGrabs <= 0) {
            return false;
        }
        return s.root->onChar(std::string(1, character));
    }

    void mouseMove(float x, float y)
    {
        State& s = state();
        if (!s.root || s.root->destroyed || !s.root->isCursorVisible()) {
            return;
        }
        float scale = virtualScale();
        s.root->setRootCursorPos(x / scale - s.root->usedX, y / scale - s.root->usedY);
    }

    bool cursorPosition(float& x, float& y)
    {
        State& s = state();
        if (!s.root || s.root->destroyed) {
            return false;
        }
        float scale = virtualScale();
        x = (s.root->mouseX() + s.root->usedX) * scale;
        y = (s.root->mouseY() + s.root->usedY) * scale;
        return true;
    }

    bool mouseButton(uint8_t key, bool pressed)
    {
        State& s = state();
        if (pressed) {
            s.keys[key] = true;
        } else {
            s.keys[key] = false;
        }
        if (!s.root || s.root->destroyed) {
            return false;
        }
        if (key == IK_MouseWheelUp || key == IK_MouseWheelDown) {
            if (!pressed) {
                return s.root->isModalOpen();
            }
            bool handled = s.root->onMouseButton(key, true);
            s.root->onMouseButton(key, false);
            return handled;
        }
        return s.root->onMouseButton(key, pressed);
    }

    UObject* rootWindow()
    {
        State& s = state();
        if (s.root && s.root->destroyed) {
            s.root = nullptr;
        }
        return s.root;
    }

    bool wantsMouse()
    {
        State& s = state();
        return s.root && !s.root->destroyed && s.root->isCursorVisible();
    }

    void setViewportSize(float width, float height)
    {
        state().viewportWidth = width;
        state().viewportHeight = height;
    }

    void setFrameCanvas(Canvas2D* canvas)
    {
        state().frameCanvas = canvas;
        if (canvas) {
            setViewportSize(canvas->width(), canvas->height());
        }
    }

    void setViewportDrawer(ViewportDrawer drawer)
    {
        state().viewportDrawer = std::move(drawer);
    }

    void setActorDrawer(ActorDrawer drawer)
    {
        state().actorDrawer = std::move(drawer);
    }

    void setProjector(Projector projector)
    {
        state().projector = std::move(projector);
    }

    void setSoundPlayer(SoundPlayer player)
    {
        state().soundPlayer = std::move(player);
    }

    void reset()
    {
        State& s = state();
        if (s.root && !s.root->destroyed) {
            s.root->destroyWindowTree();
        }
        s.root = nullptr;
        s.keys.fill(false);
        s.keyboardGrabs = 0;
    }
}
