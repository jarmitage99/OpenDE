#include "extension/windows.h"

#include "engine/input.h"
#include "engine/props_engine.h"
#include "engine/resources.h"

#include <algorithm>
#include <cmath>

namespace dx::extension {
    namespace {
        void finishEvent(UComputerWindow* window, const std::string& key, const std::string& value)
        {
            static const Name eventName("ComputerInputFinished");
            vm::Call call(window, eventName);
            if (call.valid()) {
                call.s(key).s(value);
                call.run();
            }
        }

        Color scaled(Color color, float factor)
        {
            color.r = static_cast<uint8_t>(std::trunc(static_cast<float>(color.r) * factor));
            color.g = static_cast<uint8_t>(std::trunc(static_cast<float>(color.g) * factor));
            color.b = static_cast<uint8_t>(std::trunc(static_cast<float>(color.b) * factor));
            return color;
        }
    }

    void UComputerWindow::initWindow()
    {
        UWindow::initWindow();
        computerwindow::cursorNextEvent(this) = computerwindow::cursorBlinkSpeed(this);
    }

    void UComputerWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        if (!widthSpecified) {
            preferredWidth = static_cast<float>(computerwindow::textCols(this) * computerwindow::fontWidth(this));
        }
        if (!heightSpecified) {
            preferredHeight = static_cast<float>(computerwindow::textRows(this) * computerwindow::fontHeight(this));
        }
    }

    void UComputerWindow::visibilityChanged(bool visible)
    {
        UWindow::visibilityChanged(visible);
        if (visible && !computerwindow::bComputerStartInvoked(this)) {
            computerwindow::bInvokeComputerStart(this) = 1;
        }
    }

    void UComputerWindow::addEvent(Event event, bool advance)
    {
        event.time = computerwindow::timeNextEvent(this);
        computerwindow::timeLastEvent(this) = computerwindow::timeCurrent(this);
        queued.push_back(event);
        if (advance) {
            computerwindow::timeNextEvent(this) += computerwindow::eventTimeInterval(this);
        }
    }

    void UComputerWindow::echoChar(char c)
    {
        Event event;
        event.type = EventCharacter;
        event.ch = c;
        event.noFade = true;
        addEvent(event, false);
    }

    void UComputerWindow::scroll()
    {
        for (Event& element : display) {
            if (element.type == EventCharacter) {
                element.row--;
            } else {
                element.y--;
            }
        }
        display.erase(std::remove_if(display.begin(), display.end(), [](const Event& element) {
            return (element.type == EventCharacter ? element.row : element.y) < 0;
        }),
            display.end());
    }

    bool UComputerWindow::calculateCharDisplayPosition(size_t index, int32_t& col, int32_t& row)
    {
        const Event& event = queued[index];
        if (event.col != -1) {
            return true;
        }
        int32_t textCols = computerwindow::textCols(this);
        int32_t& textX = computerwindow::TextX(this);
        int32_t& textY = computerwindow::TextY(this);
        bool wrap = false;
        if (computerwindow::bWordWrap(this)) {
            if (textX == 0 && event.ch == ' ') {
                return false;
            }
            computerwindow::bLastLineWrapped(this) = 0;
            wrap = true;
            size_t i = index;
            for (int32_t x = textX; x < textCols; x++, i++) {
                if (i == queued.size() || queued[i].type != EventCharacter || queued[i].col != -1 || queued[i].ch == ' ') {
                    wrap = false;
                    break;
                }
            }
        }
        if (!wrap && textX + 1 != textCols) {
            col = textX;
            row = textY;
            computerwindow::bLastLineWrapped(this) = 0;
            textX++;
            return true;
        }
        textX = 0;
        col = 0;
        row = ++textY;
        if (textY == computerwindow::textRows(this)) {
            textY--;
            row = textY;
            scroll();
        }
        computerwindow::bLastLineWrapped(this) = 1;
        if (event.ch == ' ') {
            return false;
        }
        textX++;
        return true;
    }

    void UComputerWindow::calculateGraphicDisplayPosition(Event& graphic)
    {
        int32_t fh = std::max(computerwindow::fontHeight(this), 1);
        if (graphic.y == -1) {
            graphic.y = computerwindow::TextY(this);
            computerwindow::TextY(this) += graphic.height / fh;
        } else {
            computerwindow::TextY(this) = (graphic.y + graphic.height) / fh + 1;
        }
        if (graphic.x == -1) {
            graphic.x = computerwindow::TextX(this);
        }
    }

    void UComputerWindow::processEvent(size_t index)
    {
        Event event = queued[index];
        switch (event.type) {
        case EventCharacter: {
            computerwindow::bCursorVisible(this) = 1;
            computerwindow::cursorNextEvent(this) = computerwindow::cursorBlinkSpeed(this);
            int32_t col = 0;
            int32_t row = 0;
            if (calculateCharDisplayPosition(index, col, row)) {
                Event element = event;
                element.color = computerwindow::FontColor(this);
                element.time = event.noFade ? 0.0f : computerwindow::fadeSpeed(this);
                element.col = col;
                element.row = row;
                display.push_back(element);
            }
            if (!event.noFade && computerwindow::textSound(this)) {
                playSound(this, computerwindow::textSound(this), window::SoundVolume(this), 1.0f);
            }
            break;
        }
        case EventTextColor:
            computerwindow::FontColor(this) = event.color;
            break;
        case EventTextSound:
            computerwindow::textSound(this) = event.object;
            break;
        case EventTextPosition:
            computerwindow::TextX(this) = event.x;
            computerwindow::TextY(this) = event.y;
            break;
        case EventNewLine:
            computerwindow::TextX(this) = 0;
            if (++computerwindow::TextY(this) == computerwindow::textRows(this)) {
                computerwindow::TextY(this)--;
                scroll();
            }
            break;
        case EventGraphic:
            computerwindow::bCursorVisible(this) = 1;
            computerwindow::cursorNextEvent(this) = computerwindow::cursorBlinkSpeed(this);
            display.push_back(event);
            calculateGraphicDisplayPosition(display.back());
            break;
        case EventClearScreen:
            computerwindow::bWaitingForKey(this) = 0;
            display.clear();
            computerwindow::TextX(this) = 0;
            computerwindow::TextY(this) = 0;
            break;
        case EventSound:
            playSound(this, event.object, window::SoundVolume(this), 1.0f);
            break;
        case EventGetInput:
            computerwindow::inputKey(this) = event.key;
            computerwindow::inputMask(this) = event.mask;
            inputMax = event.maxLength;
            input = event.defaultValue;
            inputActive = true;
            computerwindow::bWaitingForKey(this) = 1;
            for (char c : event.defaultValue) {
                echoChar(event.mask.empty() ? c : event.mask[0]);
            }
            setFocusWindow(this);
            break;
        case EventGetChar:
            computerwindow::inputKey(this) = event.key;
            computerwindow::bEchoKey(this) = event.echo ? 1 : 0;
            charActive = true;
            computerwindow::bWaitingForKey(this) = 1;
            setFocusWindow(this);
            break;
        case EventFadeOut:
            computerwindow::fadeOutStart(this) = event.duration;
            computerwindow::fadeOutTimer(this) = event.duration;
            break;
        default:
            break;
        }
    }

    void UComputerWindow::processQueuedBuffer()
    {
        while (queuedStart < queued.size() && computerwindow::timeCurrent(this) > queued[queuedStart].time) {
            processEvent(queuedStart);
            queuedStart++;
        }
        if (queuedStart >= 0x1000) {
            queued.erase(queued.begin(), queued.begin() + 0x1000);
            queuedStart -= 0x1000;
        }
    }

    void UComputerWindow::invokeParents(Name eventName)
    {
        for (UWindow* w = this; w; w = w->parent()) {
            vm::Call call(w, eventName);
            if (!call.valid()) {
                continue;
            }
            call.run();
            if (call.returnValue() && *reinterpret_cast<uint32_t*>(call.returnValue())) {
                break;
            }
        }
    }

    void UComputerWindow::tick(float deltaSeconds)
    {
        if (!computerwindow::bIgnoreTick(this)) {
            if (computerwindow::bFirstTick(this)) {
                computerwindow::bFirstTick(this) = 0;
            } else {
                if (!computerwindow::bIgnoreGamePaused(this)) {
                    UObject* player = playerPawn();
                    UObject* level = player && player->data ? actor::Level(player) : nullptr;
                    bool pauser = level && !levelinfo::Pauser(level).empty();
                    if (pauser && !computerwindow::bGamePaused(this)) {
                        computerwindow::bGamePaused(this) = 1;
                        computerwindow::bPauseProcessing(this) = 1;
                    } else if (!pauser && computerwindow::bGamePaused(this)) {
                        computerwindow::bGamePaused(this) = 0;
                        computerwindow::bPauseProcessing(this) = 0;
                    }
                }
                if (computerwindow::bInvokeComputerStart(this)) {
                    computerwindow::bInvokeComputerStart(this) = 0;
                    computerwindow::bComputerStartInvoked(this) = 1;
                    static const Name startName("ComputerStart");
                    invokeParents(startName);
                }
                if (!destroyed && !computerwindow::bGamePaused(this)) {
                    if (computerwindow::cursorBlinkSpeed(this) > 0) {
                        computerwindow::cursorNextEvent(this) -= deltaSeconds;
                        if (computerwindow::cursorNextEvent(this) < 0) {
                            computerwindow::cursorNextEvent(this) += computerwindow::cursorBlinkSpeed(this);
                            computerwindow::bCursorVisible(this) = computerwindow::bCursorVisible(this) ? 0 : 1;
                        }
                    }
                    if (!computerwindow::bPauseProcessing(this)) {
                        float interval = computerwindow::eventTimeInterval(this);
                        computerwindow::timeCurrent(this) += deltaSeconds * computerwindow::throttle(this);
                        if (computerwindow::timeCurrent(this) - computerwindow::timeLastEvent(this) > interval) {
                            computerwindow::timeNextEvent(this) = computerwindow::timeCurrent(this) + interval;
                        }
                        processQueuedBuffer();
                        float& fadeOutTimer = computerwindow::fadeOutTimer(this);
                        if (fadeOutTimer > 0) {
                            fadeOutTimer = std::max(fadeOutTimer - deltaSeconds, 0.0f);
                            if (fadeOutTimer == 0) {
                                static const Name fadeName("ComputerFadeOutCompleted");
                                for (UWindow* w = this; w && !destroyed; w = w->parent()) {
                                    vm::Call call(w, fadeName);
                                    if (call.valid()) {
                                        call.run();
                                    }
                                }
                            }
                        }
                    }
                    if (computerwindow::fadeSpeed(this) > 0) {
                        for (Event& element : display) {
                            element.time = std::max(element.time - deltaSeconds, 0.0f);
                        }
                    }
                }
            }
        }
        if (!destroyed) {
            UWindow::tick(deltaSeconds);
        }
    }

    void UComputerWindow::finishInput(bool cancel)
    {
        std::string key = computerwindow::inputKey(this);
        std::string value = cancel ? std::string() : input;
        inputActive = false;
        charActive = false;
        computerwindow::bWaitingForKey(this) = 0;
        finishEvent(this, key, value);
    }

    bool UComputerWindow::keyPressed(const std::string& key)
    {
        if (UWindow::keyPressed(key)) {
            return true;
        }
        if (key.empty() || static_cast<unsigned char>(key[0]) < 32) {
            return false;
        }
        if (charActive) {
            if (computerwindow::bEchoKey(this)) {
                echoChar(key[0]);
                playSound(this, computerwindow::typingSound(this), window::SoundVolume(this), 1.0f);
            }
            std::string inputName = computerwindow::inputKey(this);
            charActive = false;
            computerwindow::bWaitingForKey(this) = 0;
            finishEvent(this, inputName, key.substr(0, 1));
            return true;
        }
        if (inputActive) {
            if (inputMax <= 0 || static_cast<int32_t>(input.size()) < inputMax) {
                input.push_back(key[0]);
                const std::string& mask = computerwindow::inputMask(this);
                echoChar(mask.empty() ? key[0] : mask[0]);
                playSound(this, computerwindow::typingSound(this), window::SoundVolume(this), 1.0f);
            }
            return true;
        }
        return false;
    }

    bool UComputerWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        if (UWindow::virtualKeyPressed(key, repeat)) {
            return true;
        }
        if (inputActive) {
            if (key == IK_Enter) {
                finishInput(false);
                return true;
            }
            if (key == IK_Backspace && !input.empty()) {
                input.pop_back();
                if (computerwindow::TextX(this) > 0) {
                    computerwindow::TextX(this)--;
                    int32_t col = computerwindow::TextX(this);
                    int32_t row = computerwindow::TextY(this);
                    display.erase(std::remove_if(display.begin(), display.end(), [&](const Event& element) {
                        return element.type == EventCharacter && element.col == col && element.row == row;
                    }),
                        display.end());
                }
                return true;
            }
            if (key == IK_Escape) {
                finishInput(true);
                return true;
            }
            return true;
        }
        if (charActive && key == IK_Enter) {
            std::string inputName = computerwindow::inputKey(this);
            charActive = false;
            computerwindow::bWaitingForKey(this) = 0;
            finishEvent(this, inputName, std::string("\n"));
            return true;
        }
        return false;
    }

    void UComputerWindow::drawWindow(UGC* gc)
    {
        UWindow::drawWindow(gc);
        gc->setStyle(DSTY_Translucent);
        if (backgrounds[0]) {
            for (size_t i = 0; i < backgrounds.size(); i++) {
                if (backgrounds[i]) {
                    gc->drawIcon(static_cast<float>((i % 3) * 256), static_cast<float>((i / 3) * 256), backgrounds[i]);
                }
            }
        }
        UObject* font = computerwindow::textFont(this);
        gc->setFonts(font, font);
        float fw = static_cast<float>(computerwindow::fontWidth(this));
        float fh = static_cast<float>(computerwindow::fontHeight(this));
        float fadeSpeed = computerwindow::fadeSpeed(this);
        float fadeOutTimer = computerwindow::fadeOutTimer(this);
        for (const Event& element : display) {
            if (element.type == EventCharacter) {
                Color color = element.color;
                if (fadeOutTimer > 0) {
                    color = scaled(color, fadeOutTimer / computerwindow::fadeOutStart(this));
                } else if (fadeSpeed > 0 && element.time > 0) {
                    color = scaled(color, (fadeSpeed - element.time) / fadeSpeed);
                }
                gc::TextColor(gc) = color;
                gc->drawText(static_cast<float>(element.col) * fw, static_cast<float>(element.row) * fh, fw, fh, std::string(1, element.ch));
            } else if (element.type == EventGraphic) {
                gc::tileColor(gc) = computerwindow::colGraphicTile(this);
                if (element.pixelPos) {
                    gc->drawIcon(static_cast<float>(element.x), static_cast<float>(element.y), element.object);
                } else {
                    gc->drawIcon(static_cast<float>(element.x) * fw, static_cast<float>(element.y) * fh, element.object);
                }
            }
        }
        UObject* cursor = computerwindow::cursorTexture(this);
        if (cursor && !inputActive && computerwindow::bCursorVisible(this) && computerwindow::bShowCursor(this)) {
            gc->setStyle(DSTY_Masked);
            gc::tileColor(gc) = computerwindow::cursorColor(this);
            float cursorHeight = static_cast<float>(computerwindow::cursorHeight(this));
            gc->drawPattern(static_cast<float>(computerwindow::TextX(this)) * fw, static_cast<float>(computerwindow::TextY(this) + 1) * fh - cursorHeight, static_cast<float>(computerwindow::cursorWidth(this)), cursorHeight, 0, 0, cursor);
        }
    }

    namespace {
        UComputerWindow* self(UObject* object)
        {
            return cast<UComputerWindow>(object);
        }

        void nativeSetBackgroundTextures(UObject* object, Frame& stack, void*)
        {
            UObject* textures[6] = {};
            for (int i = 0; i < 6; i++) {
                textures[i] = vm::get<UObject*>(stack);
            }
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                for (size_t i = 0; i < 6; i++) {
                    window->backgrounds[i] = textures[i];
                }
            }
        }

        void nativeSetTextSize(UObject* object, Frame& stack, void*)
        {
            int32_t newCols = vm::get<int32_t>(stack);
            int32_t newRows = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::textCols(window) = newCols;
                computerwindow::textRows(window) = newRows;
            }
        }

        void nativeSetTextWindowPosition(UObject*, Frame& stack, void*)
        {
            vm::get<int32_t>(stack);
            vm::get<int32_t>(stack);
            vm::finish(stack);
        }

        void nativeSetTextFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            int32_t w = vm::get<int32_t>(stack);
            int32_t h = vm::get<int32_t>(stack);
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::textFont(window) = font;
                computerwindow::fontWidth(window) = w;
                computerwindow::fontHeight(window) = h;
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventTextColor;
                event.color = color;
                window->addEvent(event, false);
            }
        }

        void nativeSetFontColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventTextColor;
                event.color = color;
                window->addEvent(event, false);
            }
        }

        void nativeSetTextTiming(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::eventTimeInterval(window) = value;
            }
        }

        void nativeSetFadeSpeed(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::fadeSpeed(window) = value;
            }
        }

        void nativeSetCursorTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            UComputerWindow* window = self(object);
            int32_t w = window ? computerwindow::fontWidth(window) : 0;
            int32_t h = 2;
            optional(stack, w);
            optional(stack, h);
            vm::finish(stack);
            if (window) {
                computerwindow::cursorTexture(window) = texture;
                computerwindow::cursorWidth(window) = w;
                computerwindow::cursorHeight(window) = h;
            }
        }

        void nativeSetCursorColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::cursorColor(window) = color;
            }
        }

        void nativeSetCursorBlinkSpeed(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::cursorBlinkSpeed(window) = value;
            }
        }

        void nativeShowTextCursor(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::bShowCursor(window) = value ? 1 : 0;
            }
        }

        void nativeSetTextSound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventTextSound;
                event.object = sound;
                window->addEvent(event, false);
            }
        }

        void nativeSetTypingSound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::typingSound(window) = sound;
            }
        }

        void nativeSetComputerSoundVolume(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::computerSoundVolume(window) = value;
            }
        }

        void nativeSetTypingSoundVolume(UObject*, Frame& stack, void*)
        {
            vm::get<float>(stack);
            vm::finish(stack);
        }

        void nativeClearScreen(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventClearScreen;
                window->addEvent(event, false);
            }
        }

        void nativeClearLine(UObject* object, Frame& stack, void*)
        {
            int32_t row = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                std::vector<UComputerWindow::Event>& display = window->display;
                display.erase(std::remove_if(display.begin(), display.end(), [&](const UComputerWindow::Event& element) {
                    return element.type == UComputerWindow::EventCharacter && element.row == row;
                }),
                    display.end());
            }
        }

        void nativePrint(UObject* object, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            bool newLine = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UComputerWindow* window = self(object);
            if (!window) {
                return;
            }
            for (char c : text) {
                UComputerWindow::Event event;
                if (c == '^') {
                    event.type = UComputerWindow::EventNewLine;
                } else {
                    event.type = UComputerWindow::EventCharacter;
                    event.ch = c;
                }
                window->addEvent(event, true);
            }
            if (newLine) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventNewLine;
                window->addEvent(event, true);
            }
        }

        void nativePrintLn(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventNewLine;
                window->addEvent(event, true);
            }
        }

        void nativeGetInput(UObject* object, Frame& stack, void*)
        {
            UComputerWindow::Event event;
            event.type = UComputerWindow::EventGetInput;
            event.maxLength = vm::get<int32_t>(stack);
            event.key = vm::get<std::string>(stack);
            optional(stack, event.defaultValue);
            optional(stack, event.mask);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                window->addEvent(event, false);
            }
        }

        void nativeGetChar(UObject* object, Frame& stack, void*)
        {
            UComputerWindow::Event event;
            event.type = UComputerWindow::EventGetChar;
            event.key = vm::get<std::string>(stack);
            event.echo = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                window->addEvent(event, false);
            }
        }

        void nativePrintGraphic(UObject* object, Frame& stack, void*)
        {
            UComputerWindow::Event event;
            event.type = UComputerWindow::EventGraphic;
            event.object = vm::get<UObject*>(stack);
            event.width = vm::get<int32_t>(stack);
            event.height = vm::get<int32_t>(stack);
            optional(stack, event.x);
            optional(stack, event.y);
            optionalBoolDefault(stack, false);
            event.pixelPos = optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                window->addEvent(event, true);
            }
        }

        void nativePlaySoundLater(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventSound;
                event.object = sound;
                window->addEvent(event, false);
            }
        }

        void nativeSetTextPosition(UObject* object, Frame& stack, void*)
        {
            int32_t newX = vm::get<int32_t>(stack);
            int32_t newY = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                UComputerWindow::Event event;
                event.type = UComputerWindow::EventTextPosition;
                event.x = newX;
                event.y = newY;
                window->addEvent(event, false);
            }
        }

        void nativeIsBufferFlushed(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UComputerWindow* window = self(object);
            vm::retBool(result, !window || window->queued.empty());
        }

        void nativePause(UObject* object, Frame& stack, void*)
        {
            float length = -1;
            optional(stack, length);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                if (length == -1) {
                    computerwindow::bPauseProcessing(window) = 1;
                } else {
                    computerwindow::timeNextEvent(window) += length;
                }
            }
        }

        void nativeResume(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::timeNextEvent(window) = computerwindow::timeCurrent(window);
                computerwindow::bPauseProcessing(window) = 0;
            }
        }

        void nativeIsPaused(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UComputerWindow* window = self(object);
            vm::retBool(result, window && computerwindow::bPauseProcessing(window));
        }

        void nativeSetThrottle(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::throttle(window) = value;
            }
        }

        void nativeGetThrottle(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UComputerWindow* window = self(object);
            vm::ret<float>(result, window ? computerwindow::throttle(window) : 1.0f);
        }

        void nativeResetThrottle(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::throttle(window) = 1;
            }
        }

        void nativeEnableWordWrap(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                computerwindow::bWordWrap(window) = value ? 1 : 0;
            }
        }

        void nativeFadeOutText(UObject* object, Frame& stack, void*)
        {
            UComputerWindow::Event event;
            event.type = UComputerWindow::EventFadeOut;
            event.duration = 2.0f;
            optional(stack, event.duration);
            vm::finish(stack);
            if (UComputerWindow* window = self(object)) {
                window->addEvent(event, false);
            }
        }
    }

    void registerComputerNatives()
    {
        vm::registerNative(1970, nativeSetBackgroundTextures);
        vm::registerNative(1971, nativeSetTextSize);
        vm::registerNative(1972, nativeSetTextWindowPosition);
        vm::registerNative(1973, nativeSetTextFont);
        vm::registerNative(1974, nativeSetFontColor);
        vm::registerNative(1975, nativeSetTextTiming);
        vm::registerNative(1976, nativeSetFadeSpeed);
        vm::registerNative(1977, nativeSetCursorTexture);
        vm::registerNative(1978, nativeSetCursorColor);
        vm::registerNative(1979, nativeSetCursorBlinkSpeed);
        vm::registerNative(1980, nativeShowTextCursor);
        vm::registerNative(1981, nativeSetTextSound);
        vm::registerNative(1982, nativeSetTypingSound);
        vm::registerNative(1983, nativeSetComputerSoundVolume);
        vm::registerNative(1984, nativeSetTypingSoundVolume);
        vm::registerNative(1985, nativeClearScreen);
        vm::registerNative(1986, nativeClearLine);
        vm::registerNative(1987, nativePrint);
        vm::registerNative(1988, nativePrintLn);
        vm::registerNative(1989, nativeGetInput);
        vm::registerNative(1990, nativeGetChar);
        vm::registerNative(1991, nativePrintGraphic);
        vm::registerNative(1992, nativePlaySoundLater);
        vm::registerNative(1993, nativeSetTextPosition);
        vm::registerNative(1994, nativeIsBufferFlushed);
        vm::registerNative(1995, nativePause);
        vm::registerNative(1996, nativeResume);
        vm::registerNative(1997, nativeIsPaused);
        vm::registerNative(1998, nativeSetThrottle);
        vm::registerNative(1999, nativeGetThrottle);
        vm::registerNative(2000, nativeResetThrottle);
        vm::registerNative(2001, nativeEnableWordWrap);
        vm::registerNative(2002, nativeFadeOutText);
    }
}
