#include "extension/windows.h"

#include "core/objects.h"
#include "engine/input.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace dx::extension {
    namespace {
        void applyLineLimits(UWindow* window, bool heightSpecified, float& textHeight)
        {
            int32_t minLines = textwindow::minLines(window);
            int32_t maxLines = textwindow::MaxLines(window);
            if (heightSpecified || (minLines < 0 && maxLines < 0)) {
                return;
            }
            float lineHeight = fontHeight(window::normalFont(window)) + window::textVSpacing(window);
            float extent = textHeight;
            if (minLines >= 0 && static_cast<float>(minLines) * lineHeight > extent) {
                textHeight = static_cast<float>(minLines) * lineHeight;
            }
            if (maxLines >= 0 && extent > static_cast<float>(maxLines) * lineHeight) {
                textHeight = static_cast<float>(maxLines) * lineHeight;
            }
        }

        std::string convertScriptString(const std::string& text)
        {
            std::string result;
            result.reserve(text.size());
            for (size_t i = 0; i < text.size(); i++) {
                if (text[i] == '|' && i + 1 < text.size() && text[i + 1] == 'n') {
                    result.push_back('\n');
                    i++;
                } else {
                    result.push_back(text[i]);
                }
            }
            return result;
        }

        int32_t acceleratorFor(const std::string& text)
        {
            size_t pos = text.find("|&");
            if (pos != std::string::npos && pos + 2 < text.size()) {
                return std::toupper(static_cast<unsigned char>(text[pos + 2]));
            }
            return 0;
        }

        void updateAccelerator(UTextWindow* window)
        {
            if (textwindow::bTextIsAccelerator(window)) {
                window::acceleratorKey(window) = acceleratorFor(window->text());
                if (UWindow* modal = window->modalWindow()) {
                    modalwindow::bDirtyAccelerators(modal) = 1;
                }
            }
        }

        float largeLineHeight(UWindow* window)
        {
            return std::max(fontHeight(window::normalFont(window)), 1.0f);
        }

        std::vector<TextRow> largeRows(UWindow* window, float wrapWidth)
        {
            TextContext context = textContextOf(window);
            return textRows(context, textwindow::bWordWrap(window) ? wrapWidth : 0.0f, textwindow::Text(window));
        }

        void bubbleToggle(UWindow* button, bool value)
        {
            for (UWindow* current = button; current; current = current->parent()) {
                if (current->toggleChanged(button, value)) {
                    return;
                }
            }
        }

        void bubbleText(UWindow* edit, bool modified)
        {
            for (UWindow* current = edit; current; current = current->parent()) {
                if (current->textChanged(edit, modified)) {
                    return;
                }
            }
        }

        void bubbleEditActivated(UWindow* edit, bool modified)
        {
            for (UWindow* current = edit; current; current = current->parent()) {
                if (current->editActivated(edit, modified)) {
                    return;
                }
            }
        }

        bool isWordChar(char c)
        {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }
    }

    std::string& UTextWindow::text()
    {
        return textwindow::Text(this);
    }

    void UTextWindow::initWindow()
    {
        setFont(defaultFont());
        textwindow::hMargin(this) = 3.0f;
        textwindow::vMargin(this) = 3.0f;
        textwindow::HAlign(this) = HALIGN_Center;
        textwindow::VAlign(this) = VALIGN_Center;
        textwindow::bWordWrap(this) = 1;
        textwindow::minLines(this) = -1;
        textwindow::MaxLines(this) = -1;
        textwindow::bTextIsAccelerator(this) = 1;
        UWindow::initWindow();
    }

    void UTextWindow::textModified()
    {
    }

    void UTextWindow::textExtent(float maxWidth, float& outWidth, float& outHeight)
    {
        outWidth = 0;
        outHeight = 0;
        if (cast<ULargeTextWindow>(this)) {
            std::vector<TextRow> rows = largeRows(this, maxWidth);
            for (const TextRow& row : rows) {
                outWidth = std::max(outWidth, row.width);
            }
            if (!rows.empty()) {
                outHeight = static_cast<float>(rows.size()) * largeLineHeight(this) + static_cast<float>(rows.size() - 1) * largetextwindow::vSpace(this);
            }
            return;
        }
        measureText(textContextOf(this), maxWidth, text(), outWidth, outHeight);
    }

    void UTextWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        if (!text().empty()) {
            float w = 0;
            float h = 0;
            textExtent((widthSpecified && textwindow::bWordWrap(this)) ? preferredWidth - hMargin * 2.0f : 0.0f, w, h);
            applyLineLimits(this, heightSpecified, h);
            preferredWidth = w + hMargin * 2.0f;
            preferredHeight = h + vMargin * 2.0f;
        } else if (UObject* background = window::Background(this)) {
            float lineHeight = fontHeight(window::normalFont(this)) + window::textVSpacing(this);
            preferredHeight = textureHeight(background);
            if (!heightSpecified) {
                if (textwindow::minLines(this) >= 0) {
                    preferredHeight = static_cast<float>(textwindow::minLines(this)) * lineHeight;
                } else if (textwindow::MaxLines(this) >= 0) {
                    preferredHeight = static_cast<float>(textwindow::MaxLines(this)) * lineHeight;
                }
            }
            preferredWidth = textureWidth(background);
        } else {
            UWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
        }
        if (!widthSpecified && preferredWidth < textwindow::MinWidth(this)) {
            preferredWidth = textwindow::MinWidth(this);
        }
    }

    void UTextWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        if (!text().empty()) {
            float hMargin = textwindow::hMargin(this);
            float vMargin = textwindow::vMargin(this);
            gc::HAlign(gc) = textwindow::HAlign(this);
            gc::VAlign(gc) = textwindow::VAlign(this);
            gc::bWordWrap(gc) = textwindow::bWordWrap(this);
            gc->drawText(hMargin, vMargin, width() - hMargin * 2.0f, height() - vMargin * 2.0f, text());
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        scriptDrawWindow(gc);
    }

    void ULargeTextWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        std::vector<TextRow> rows = largeRows(this, width() - hMargin * 2.0f);
        if (!rows.empty()) {
            float lineHeight = largeLineHeight(this);
            float space = largetextwindow::vSpace(this);
            float total = static_cast<float>(rows.size()) * lineHeight + static_cast<float>(rows.size() - 1) * space;
            float y = vMargin;
            uint8_t valign = textwindow::VAlign(this);
            if (valign == VALIGN_Bottom) {
                y = height() - total - vMargin;
            } else if (valign != VALIGN_Top) {
                y = std::trunc((height() - total) * 0.5f);
            }
            gc->pushClip(gc->offsetX + hMargin, gc->offsetY + vMargin, width() - hMargin * 2.0f, height() - vMargin * 2.0f);
            uint8_t halign = textwindow::HAlign(this);
            for (const TextRow& row : rows) {
                float x = hMargin;
                if (halign == HALIGN_Right) {
                    x = width() - row.width - hMargin;
                } else if (halign == HALIGN_Center) {
                    x = std::trunc((width() - row.width) * 0.5f);
                }
                gc->drawTextRow(x, y, text(), row);
                y += lineHeight + space;
            }
            gc->popClip();
        }
        scriptDrawWindow(gc);
    }

    void UButtonWindow::initWindow()
    {
        window::bIsSelectable(this) = 1;
        UTextWindow::initWindow();
    }

    size_t UButtonWindow::stateIndex()
    {
        URootWindow* rootWindow = root();
        bool focus = rootWindow && rootWindow->focusWindow() == this;
        bool pressed = buttonwindow::bButtonPressed(this) != 0;
        if (UToggleWindow* toggle = cast<UToggleWindow>(this)) {
            pressed = pressed || toggle->toggled;
        }
        if (!sensitive()) {
            return pressed ? 5 : 4;
        }
        if (focus) {
            return pressed ? 3 : 2;
        }
        return pressed ? 1 : 0;
    }

    void UButtonWindow::drawWindow(UGC* gc)
    {
        size_t index = stateIndex();
        UObject* texture = buttonTextures[index];
        if (!texture) {
            texture = buttonTextures[index & 1];
        }
        if (!texture) {
            texture = buttonTextures[0];
        }
        if (texture) {
            gc->setStyle(window::backgroundStyle(this));
            gc::tileColor(gc) = colorsSet ? buttonColors[index] : window::tileColor(this);
            gc->drawPattern(0, 0, width(), height(), 0, 0, texture);
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        drawBackground(gc);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        float w = width() - hMargin * 2.0f;
        float h = height() - vMargin * 2.0f;
        if (w > 0 && h > 0 && !text().empty()) {
            if (textColorsSet) {
                gc::TextColor(gc) = textColors[index];
            }
            gc::HAlign(gc) = textwindow::HAlign(this);
            gc::VAlign(gc) = textwindow::VAlign(this);
            gc::bWordWrap(gc) = textwindow::bWordWrap(this);
            gc->drawText(hMargin, vMargin, w, h, text());
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        buttonwindow::curTexture(this) = texture;
        buttonwindow::curTileColor(this) = colorsSet ? buttonColors[index] : window::tileColor(this);
        buttonwindow::curTextColor(this) = textColorsSet ? textColors[index] : window::TextColor(this);
        scriptDrawWindow(gc);
    }

    void UButtonWindow::activate(uint8_t key)
    {
        buttonwindow::lastInputKey(this) = key;
        playWindowSound(buttonwindow::clickSound(this));
        sendButtonActivated(this);
    }

    void UButtonWindow::mouseMoved(float newX, float newY)
    {
        if (buttonwindow::bMousePressed(this)) {
            bool inside = newX >= 0 && newY >= 0 && newX < width() && newY < height();
            buttonwindow::bButtonPressed(this) = inside ? 1 : 0;
        }
        UTextWindow::mouseMoved(newX, newY);
    }

    bool UButtonWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        bool handled = UTextWindow::mouseButtonPressed(pointX, pointY, button, numClicks);
        if (destroyed) {
            return true;
        }
        if (button == IK_RightMouse && !buttonwindow::bEnableRightMouseClick(this)) {
            return handled;
        }
        if (button != IK_LeftMouse && button != IK_RightMouse) {
            return handled;
        }
        if (selectable()) {
            setFocusWindow(this);
        }
        buttonwindow::bMousePressed(this) = 1;
        buttonwindow::bButtonPressed(this) = 1;
        buttonwindow::lastInputKey(this) = button;
        playWindowSound(buttonwindow::pressSound(this));
        repeatCountdown = buttonwindow::initialDelay(this);
        grabMouse();
        return true;
    }

    bool UButtonWindow::mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        bool wasPressed = buttonwindow::bMousePressed(this) != 0;
        if (wasPressed) {
            ungrabMouse();
            buttonwindow::bMousePressed(this) = 0;
            buttonwindow::bButtonPressed(this) = 0;
            bool inside = pointX >= 0 && pointY >= 0 && pointX < width() && pointY < height();
            if (inside && !buttonwindow::bAutoRepeat(this)) {
                activate(button);
            }
            if (destroyed) {
                return true;
            }
        }
        bool handled = UTextWindow::mouseButtonReleased(pointX, pointY, button, numClicks);
        return wasPressed || handled;
    }

    bool UButtonWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        if (UTextWindow::virtualKeyPressed(key, repeat)) {
            return true;
        }
        if ((key == IK_Enter || key == IK_Space) && !repeat) {
            URootWindow* rootWindow = root();
            if (rootWindow && rootWindow->focusWindow() == this && sensitive()) {
                activate(key);
                return true;
            }
        }
        return false;
    }

    void UButtonWindow::tick(float deltaSeconds)
    {
        if (buttonwindow::bMousePressed(this) && buttonwindow::bAutoRepeat(this) && buttonwindow::bButtonPressed(this)) {
            repeatCountdown -= deltaSeconds;
            if (repeatCountdown <= 0) {
                float rate = buttonwindow::repeatRate(this);
                repeatCountdown += rate > 0.01f ? rate : 0.1f;
                activate(buttonwindow::lastInputKey(this));
            }
        }
        if (!destroyed) {
            UTextWindow::tick(deltaSeconds);
        }
    }

    void UToggleWindow::setToggle(bool value)
    {
        if (toggled == value) {
            return;
        }
        toggled = value;
        playWindowSound(value ? togglewindow::enableSound(this) : togglewindow::disableSound(this));
        bubbleToggle(this, value);
    }

    void UToggleWindow::activate(uint8_t key)
    {
        buttonwindow::lastInputKey(this) = key;
        playWindowSound(buttonwindow::clickSound(this));
        setToggle(!toggled);
    }

    void UCheckboxWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        float w = width() - hMargin * 2.0f;
        float h = height() - vMargin * 2.0f;
        UObject* texture = toggled ? checkboxwindow::toggleOn(this) : checkboxwindow::toggleOff(this);
        if (w > 0 && h > 0) {
            float boxWidth = texture ? textureWidth(texture) : 0.0f;
            float boxHeight = texture ? textureHeight(texture) : 0.0f;
            if (checkboxwindow::textureWidth(this) > 0) {
                boxWidth = checkboxwindow::textureWidth(this);
            }
            if (checkboxwindow::textureHeight(this) > 0) {
                boxHeight = checkboxwindow::textureHeight(this);
            }
            float spacing = checkboxwindow::checkboxSpacing(this);
            bool right = checkboxwindow::bRightSide(this) != 0;
            float textX = right ? hMargin : hMargin + boxWidth + spacing;
            float textWidth = std::max(w - boxWidth - spacing, 0.0f);
            if (!text().empty()) {
                if (textColorsSet) {
                    gc::TextColor(gc) = textColors[stateIndex()];
                }
                gc::HAlign(gc) = textwindow::HAlign(this);
                gc::VAlign(gc) = textwindow::VAlign(this);
                gc->drawText(textX, vMargin, textWidth, h, text());
                gc->resetState(this, gc->offsetX, gc->offsetY);
            }
            if (texture) {
                gc->setStyle(checkboxwindow::checkboxStyle(this) ? checkboxwindow::checkboxStyle(this) : DSTY_Masked);
                Color color = checkboxwindow::checkboxColor(this);
                gc::tileColor(gc) = (color.r || color.g || color.b) ? color : Color { 255, 255, 255, 255 };
                float boxX = right ? hMargin + w - boxWidth : hMargin;
                float boxY = vMargin + std::floor((h - boxHeight) * 0.5f);
                gc->drawStretched(boxX, boxY, boxWidth, boxHeight, 0, 0, textureWidth(texture), textureHeight(texture), texture);
                gc->resetState(this, gc->offsetX, gc->offsetY);
            }
        }
        scriptDrawWindow(gc);
    }

    void UCheckboxWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        UObject* texture = toggled ? checkboxwindow::toggleOn(this) : checkboxwindow::toggleOff(this);
        float boxWidth = checkboxwindow::textureWidth(this) > 0 ? checkboxwindow::textureWidth(this) : textureWidth(texture);
        float boxHeight = checkboxwindow::textureHeight(this) > 0 ? checkboxwindow::textureHeight(this) : textureHeight(texture);
        float spacing = checkboxwindow::checkboxSpacing(this);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        float available = widthSpecified ? std::max(preferredWidth - hMargin * 2.0f - boxWidth - spacing, 0.0f) : 1e9f;
        float w = 0;
        float h = 0;
        textExtent(available, w, h);
        applyLineLimits(this, heightSpecified, h);
        if (!widthSpecified) {
            preferredWidth = std::max(w, textwindow::MinWidth(this)) + boxWidth + spacing + hMargin * 2.0f;
        }
        if (!heightSpecified) {
            preferredHeight = std::max(h, boxHeight) + vMargin * 2.0f;
        }
        UWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UTextWindow::parentRequestedGranularity(float& hGranularity, float& vGranularity)
    {
        hGranularity = 1.0f;
        vGranularity = fontHeight(window::normalFont(this)) + window::textVSpacing(this);
        UWindow::parentRequestedGranularity(hGranularity, vGranularity);
    }

    void ULargeTextWindow::parentRequestedGranularity(float& hGranularity, float& vGranularity)
    {
        hGranularity = 1.0f;
        vGranularity = std::max(fontHeight(window::normalFont(this)), 1.0f) + largetextwindow::vSpace(this);
        UWindow::parentRequestedGranularity(hGranularity, vGranularity);
    }

    void ULargeTextWindow::initWindow()
    {
        largetextwindow::vSpace(this) = 1.0f;
        UTextWindow::initWindow();
        textwindow::bTextIsAccelerator(this) = 0;
    }

    void UEditWindow::initWindow()
    {
        largetextwindow::vSpace(this) = 1.0f;
        setFont(defaultFont());
        editwindow::bEditable(this) = 1;
        textwindow::bWordWrap(this) = 1;
        textwindow::HAlign(this) = HALIGN_Left;
        textwindow::VAlign(this) = VALIGN_Top;
        textwindow::minLines(this) = -1;
        textwindow::MaxLines(this) = -1;
        textwindow::bTextIsAccelerator(this) = 0;
        textwindow::hMargin(this) = 3.0f;
        textwindow::vMargin(this) = 0.0f;
        editwindow::insertColor(this) = { 255, 255, 255, 255 };
        editwindow::selectColor(this) = { 196, 196, 196, 255 };
        editwindow::inverseColor(this) = { 0, 0, 0, 255 };
        editwindow::maxSize(this) = 200000;
        editwindow::maxUndos(this) = 64;
        editwindow::blinkStart(this) = 0.75f;
        editwindow::blinkPeriod(this) = 1.0f;
        window::bIsSelectable(this) = 1;
        UWindow::initWindow();
        undoStack.clear();
        undoStack.push_back(text());
        undoPosition = 0;
    }

    void UEditWindow::clampPositions()
    {
        int32_t length = static_cast<int32_t>(text().size());
        editwindow::insertPos(this) = std::clamp(editwindow::insertPos(this), 0, length);
        editwindow::selectStart(this) = std::clamp(editwindow::selectStart(this), 0, length);
        editwindow::selectEnd(this) = std::clamp(editwindow::selectEnd(this), 0, length);
    }

    void UEditWindow::textModified()
    {
        clampPositions();
    }

    void UEditWindow::pushUndo()
    {
        if (undoPosition + 1 < undoStack.size()) {
            undoStack.resize(undoPosition + 1);
        }
        undoStack.push_back(text());
        int32_t maxUndos = editwindow::maxUndos(this);
        size_t limit = maxUndos > 0 ? static_cast<size_t>(maxUndos) + 1 : 64;
        while (undoStack.size() > limit) {
            undoStack.erase(undoStack.begin());
            unchangedUndo = unchangedUndo > 0 ? unchangedUndo - 1 : -1;
        }
        undoPosition = undoStack.size() - 1;
    }

    bool UEditWindow::hasTextChanged()
    {
        return static_cast<int32_t>(undoPosition) != unchangedUndo;
    }

    void UEditWindow::setTextChangedFlag(bool value)
    {
        unchangedUndo = value ? -1 : static_cast<int32_t>(undoPosition);
    }

    void UEditWindow::clearUndo()
    {
        unchangedUndo = hasTextChanged() ? -1 : 0;
        undoStack.assign(1, text());
        undoPosition = 0;
    }

    void UEditWindow::notifyChanged()
    {
        askParentForReconfigure();
        bubbleText(this, hasTextChanged());
    }

    void UEditWindow::deleteSelection()
    {
        int32_t start = std::min(editwindow::selectStart(this), editwindow::selectEnd(this));
        int32_t end = std::max(editwindow::selectStart(this), editwindow::selectEnd(this));
        if (end > start) {
            text().erase(static_cast<size_t>(start), static_cast<size_t>(end - start));
            editwindow::insertPos(this) = start;
        }
        editwindow::selectStart(this) = editwindow::insertPos(this);
        editwindow::selectEnd(this) = editwindow::insertPos(this);
    }

    bool UEditWindow::insertText(const std::string& value, bool undo, bool select)
    {
        std::string source = value;
        for (size_t at = source.find("|n"); at != std::string::npos; at = source.find("|n", at)) {
            source.replace(at, 2, "\n");
        }
        std::string insert;
        for (char c : source) {
            if (editwindow::bSingleLine(this) && c == '\n') {
                continue;
            }
            if (filterChar(c) && c != '\0') {
                insert.push_back(c);
            }
        }
        if (!source.empty() && insert.empty()) {
            return false;
        }
        clampPositions();
        size_t lengthBefore = text().size();
        deleteSelection();
        int32_t maxSize = editwindow::maxSize(this);
        if (maxSize > 0) {
            int32_t room = std::max(maxSize - static_cast<int32_t>(text().size()), 0);
            if (static_cast<int32_t>(insert.size()) > room) {
                insert.resize(static_cast<size_t>(room));
            }
        }
        if (insert.empty() && text().size() == lengthBefore) {
            return true;
        }
        int32_t position = editwindow::insertPos(this);
        text().insert(static_cast<size_t>(position), insert);
        editwindow::insertPos(this) = position + static_cast<int32_t>(insert.size());
        if (select) {
            editwindow::selectStart(this) = position;
            editwindow::selectEnd(this) = editwindow::insertPos(this);
        } else {
            editwindow::selectStart(this) = editwindow::insertPos(this);
            editwindow::selectEnd(this) = editwindow::insertPos(this);
        }
        if (undo) {
            pushUndo();
        } else {
            clearUndo();
            setTextChangedFlag(true);
        }
        blinkTime = state().time;
        notifyChanged();
        return true;
    }

    void UEditWindow::deleteChar(bool before, bool undo)
    {
        if (!editwindow::bEditable(this)) {
            return;
        }
        clampPositions();
        if (editwindow::selectStart(this) != editwindow::selectEnd(this)) {
            deleteSelection();
        } else {
            int32_t position = editwindow::insertPos(this);
            if (before) {
                if (position <= 0) {
                    return;
                }
                text().erase(static_cast<size_t>(position - 1), 1);
                editwindow::insertPos(this) = position - 1;
            } else {
                if (position >= static_cast<int32_t>(text().size())) {
                    return;
                }
                text().erase(static_cast<size_t>(position), 1);
            }
            editwindow::selectStart(this) = editwindow::insertPos(this);
            editwindow::selectEnd(this) = editwindow::insertPos(this);
        }
        if (undo) {
            pushUndo();
        }
        blinkTime = state().time;
        notifyChanged();
    }

    void UEditWindow::setInsertion(int32_t position, bool drag)
    {
        int32_t length = static_cast<int32_t>(text().size());
        position = std::clamp(position, 0, length);
        int32_t anchor = editwindow::selectStart(this) == editwindow::insertPos(this) ? editwindow::selectEnd(this) : editwindow::selectStart(this);
        editwindow::insertPos(this) = position;
        editwindow::insertPreferredCol(this) = columnX(editRows(), position);
        if (drag) {
            editwindow::selectStart(this) = anchor;
            editwindow::selectEnd(this) = position;
        } else {
            editwindow::selectStart(this) = position;
            editwindow::selectEnd(this) = position;
        }
        blinkTime = state().time;
    }

    std::vector<TextRow> UEditWindow::editRows()
    {
        float wrapWidth = width() - textwindow::hMargin(this) * 2.0f;
        if (!textwindow::bWordWrap(this) || editwindow::bSingleLine(this) || wrapWidth <= 0) {
            wrapWidth = 0;
        }
        return textRows(textContextOf(this), wrapWidth, text());
    }

    int32_t UEditWindow::rowOf(const std::vector<TextRow>& rows, int32_t position)
    {
        int32_t row = static_cast<int32_t>(rows.size()) - 1;
        while (row > 0 && static_cast<int32_t>(rows[static_cast<size_t>(row)].start) > position) {
            row--;
        }
        return std::max(row, 0);
    }

    int32_t UEditWindow::rowEnd(const std::vector<TextRow>& rows, int32_t row)
    {
        if (static_cast<size_t>(row) + 1 < rows.size()) {
            return static_cast<int32_t>(rows[static_cast<size_t>(row) + 1].start) - 1;
        }
        return static_cast<int32_t>(text().size());
    }

    float UEditWindow::columnX(const std::vector<TextRow>& rows, int32_t position)
    {
        if (rows.empty()) {
            return 0;
        }
        size_t start = rows[static_cast<size_t>(rowOf(rows, position))].start;
        float w = 0;
        float h = 0;
        if (static_cast<size_t>(position) > start) {
            measureString(window::normalFont(this), text().substr(start, static_cast<size_t>(position) - start), w, h);
        }
        return w;
    }

    int32_t UEditWindow::positionAtX(const std::vector<TextRow>& rows, int32_t row, float x)
    {
        if (rows.empty()) {
            return 0;
        }
        const std::string& value = text();
        int32_t position = static_cast<int32_t>(rows[static_cast<size_t>(row)].start);
        int32_t end = rowEnd(rows, row);
        UObject* font = window::normalFont(this);
        float cursor = 0;
        while (position < end && value[static_cast<size_t>(position)] != '\n') {
            float w = 0;
            float h = 0;
            measureString(font, std::string(1, value[static_cast<size_t>(position)]), w, h);
            if (cursor + w * 0.5f > x) {
                break;
            }
            cursor += w;
            position++;
            if (x <= cursor) {
                break;
            }
        }
        return position;
    }

    void UEditWindow::moveInsertion(uint8_t move, bool drag)
    {
        const std::string& value = text();
        int32_t position = editwindow::insertPos(this);
        int32_t length = static_cast<int32_t>(value.size());
        std::vector<TextRow> rows = editRows();
        int32_t row = rowOf(rows, position);
        float lineHeight = largeLineHeight(this) + largetextwindow::vSpace(this);
        int32_t page = std::max(static_cast<int32_t>(height() / lineHeight), 1);
        int32_t targetRow = row;
        switch (move) {
        case 0:
            targetRow = row - 1;
            break;
        case 1:
            targetRow = row + 1;
            break;
        case 8:
            targetRow = row - page;
            break;
        case 9:
            targetRow = row + page;
            break;
        case 2:
            position--;
            break;
        case 3:
            position++;
            break;
        case 4:
            while (position > 0 && !isWordChar(value[static_cast<size_t>(position - 1)])) {
                position--;
            }
            while (position > 0 && isWordChar(value[static_cast<size_t>(position - 1)])) {
                position--;
            }
            break;
        case 5:
            while (position < length && isWordChar(value[static_cast<size_t>(position)])) {
                position++;
            }
            while (position < length && !isWordChar(value[static_cast<size_t>(position)])) {
                position++;
            }
            break;
        case 6:
            if (!rows.empty()) {
                position = static_cast<int32_t>(rows[static_cast<size_t>(row)].start);
            }
            break;
        case 7:
            if (!rows.empty()) {
                position = rowEnd(rows, row);
            }
            break;
        case 10:
            position = 0;
            break;
        case 11:
            position = length;
            break;
        default:
            break;
        }
        if (move == 0 || move == 1 || move == 8 || move == 9) {
            if (rows.empty()) {
                return;
            }
            float column = editwindow::insertPreferredCol(this);
            targetRow = std::clamp(targetRow, 0, static_cast<int32_t>(rows.size()) - 1);
            setInsertion(positionAtX(rows, targetRow, column), drag);
            editwindow::insertPreferredCol(this) = column;
            return;
        }
        setInsertion(position, drag);
    }

    bool UEditWindow::filterChar(char& c)
    {
        if (editwindow::bUppercaseOnly(this) && c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 32);
        }
        static const Name functionName("FilterChar");
        vm::Call call(this, functionName);
        if (!call.valid()) {
            return true;
        }
        call.s(std::string(1, c));
        uint8_t* result = call.returnValue();
        *reinterpret_cast<uint32_t*>(result) = 1;
        call.run();
        if (*reinterpret_cast<uint32_t*>(result) == 0) {
            return false;
        }
        const std::string& filtered = *reinterpret_cast<std::string*>(call.parameter(0));
        c = filtered.empty() ? '\0' : filtered[0];
        return true;
    }

    bool UEditWindow::keyPressed(const std::string& key)
    {
        bool handled = UTextWindow::keyPressed(key);
        if (!editwindow::bEditable(this) || key.empty()) {
            return handled;
        }
        unsigned char c = static_cast<unsigned char>(key[0]);
        if (c < 32 || c == '`' || c == '~') {
            return handled;
        }
        if (!insertText(key, true, false)) {
            return handled;
        }
        playWindowSound(editwindow::typeSound(this));
        return true;
    }

    bool UEditWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        if (key == IK_Enter) {
            playWindowSound(editwindow::enterSound(this));
            if (editwindow::bSingleLine(this) && editwindow::bEditable(this)) {
                bubbleEditActivated(this, hasTextChanged());
                return true;
            }
        }
        return UTextWindow::virtualKeyPressed(key, repeat);
    }

    bool UEditWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        if (UTextWindow::mouseButtonPressed(pointX, pointY, button, numClicks)) {
            return true;
        }
        setFocusWindow(this);
        std::vector<TextRow> rows = editRows();
        float step = largeLineHeight(this) + largetextwindow::vSpace(this);
        int32_t row = static_cast<int32_t>(std::floor((pointY - textwindow::vMargin(this)) / step));
        row = std::clamp(row, 0, std::max(static_cast<int32_t>(rows.size()) - 1, 0));
        setInsertion(positionAtX(rows, row, pointX - textwindow::hMargin(this)), false);
        return true;
    }

    void UEditWindow::drawWindow(UGC* gc)
    {
        clampPositions();
        drawBackground(gc);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        const std::string& value = text();
        URootWindow* rootWindow = root();
        bool focused = rootWindow && rootWindow->focusWindow() == this;
        int32_t start = std::min(editwindow::selectStart(this), editwindow::selectEnd(this));
        int32_t end = focused ? std::max(editwindow::selectStart(this), editwindow::selectEnd(this)) : start;
        std::vector<TextRow> rows = editRows();
        float lineHeight = largeLineHeight(this);
        float step = lineHeight + largetextwindow::vSpace(this);
        gc->pushClip(gc->offsetX + hMargin, gc->offsetY + vMargin, std::max(width() - hMargin * 2.0f, 0.0f), std::max(height() - vMargin * 2.0f, 0.0f));
        for (size_t i = 0; i < rows.size(); i++) {
            int32_t a = std::max(start, static_cast<int32_t>(rows[i].start));
            int32_t b = std::min(end, rowEnd(rows, static_cast<int32_t>(i)));
            if (b <= a) {
                continue;
            }
            float x0 = columnX(rows, a);
            float x1 = columnX(rows, b);
            float y = vMargin + static_cast<float>(i) * step;
            UObject* texture = editwindow::selectTexture(this);
            gc->setStyle(DSTY_Normal);
            gc::tileColor(gc) = editwindow::selectColor(this);
            if (texture) {
                gc->drawStretched(hMargin + x0, y, x1 - x0, lineHeight, 0, 0, textureWidth(texture), textureHeight(texture), texture);
            } else {
                gc->drawRect(gc->offsetX + hMargin + x0, gc->offsetY + y, x1 - x0, lineHeight, gc::tileColor(gc), BlendStyle::Normal);
            }
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        for (size_t i = 0; i < rows.size(); i++) {
            gc->drawTextRow(hMargin, vMargin + static_cast<float>(i) * step, value, rows[i]);
        }
        for (size_t i = 0; i < rows.size(); i++) {
            int32_t a = std::max(start, static_cast<int32_t>(rows[i].start));
            int32_t b = std::min(end, rowEnd(rows, static_cast<int32_t>(i)));
            if (b <= a) {
                continue;
            }
            float x0 = columnX(rows, a);
            gc::HAlign(gc) = HALIGN_Left;
            gc::VAlign(gc) = VALIGN_Top;
            gc::bWordWrap(gc) = 0;
            gc::TextColor(gc) = editwindow::inverseColor(this);
            gc->drawText(hMargin + x0, vMargin + static_cast<float>(i) * step, std::max(width() - hMargin * 2.0f - x0, 0.0f), lineHeight, value.substr(static_cast<size_t>(a), static_cast<size_t>(b - a)));
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        gc->popClip();
        double elapsed = state().time - blinkTime;
        double blinkStart = static_cast<double>(editwindow::blinkStart(this));
        double halfPeriod = static_cast<double>(editwindow::blinkPeriod(this)) * 0.5;
        bool blinkOn = elapsed < blinkStart || halfPeriod <= 0 || static_cast<int64_t>(std::floor((elapsed - blinkStart) / halfPeriod)) % 2 == 1;
        bool showCursor = focused && editwindow::bEditable(this) && blinkOn;
        editwindow::bCursorShowing(this) = showCursor ? 1 : 0;
        if (showCursor) {
            int32_t position = editwindow::insertPos(this);
            float caretX = columnX(rows, position);
            float caretY = vMargin + static_cast<float>(rows.empty() ? 0 : rowOf(rows, position)) * step;
            Color color = editwindow::insertColor(this);
            UObject* texture = editwindow::insertTexture(this);
            float caretWidth = editwindow::insertPrefWidth(this) > 0 ? editwindow::insertPrefWidth(this) : 1.0f;
            gc->setStyle(DSTY_Normal);
            gc::tileColor(gc) = color;
            if (texture) {
                gc->drawStretched(hMargin + caretX, caretY, caretWidth, lineHeight, 0, 0, textureWidth(texture), textureHeight(texture), texture);
            } else {
                gc->drawRect(gc->offsetX + hMargin + caretX, gc->offsetY + caretY, caretWidth, lineHeight, color, BlendStyle::Normal);
            }
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        scriptDrawWindow(gc);
    }

    void UTextLogWindow::tick(float deltaSeconds)
    {
        if (!textlogwindow::bPaused(this)) {
            totalTime += static_cast<double>(deltaSeconds);
            bool removed = false;
            while (!entries.empty() && entries.front().expires > 0 && entries.front().expires <= totalTime) {
                entries.pop_front();
                removed = true;
            }
            if (removed) {
                askParentForReconfigure();
            }
        }
        UTextWindow::tick(deltaSeconds);
    }

    void UTextLogWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        float available = widthSpecified ? std::max(preferredWidth - hMargin * 2.0f, 0.0f) : 1e9f;
        float w = 0;
        float h = 0;
        TextContext context = textContextOf(this);
        for (const Entry& entry : entries) {
            float lw = 0;
            float lh = 0;
            measureText(context, available, entry.text, lw, lh);
            w = std::max(w, lw);
            h += lh;
        }
        applyLineLimits(this, heightSpecified, h);
        if (!widthSpecified) {
            preferredWidth = std::max(w, textwindow::MinWidth(this)) + hMargin * 2.0f;
        }
        if (!heightSpecified) {
            preferredHeight = h + vMargin * 2.0f;
        }
        UWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UTextLogWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        float hMargin = textwindow::hMargin(this);
        float vMargin = textwindow::vMargin(this);
        float w = std::max(width() - hMargin * 2.0f, 0.0f);
        float h = std::max(height() - vMargin * 2.0f, 0.0f);
        std::vector<float> heights;
        float total = 0;
        for (const Entry& entry : entries) {
            float lw = 0;
            float lh = 0;
            measureText(textContextOf(this), w, entry.text, lw, lh);
            heights.push_back(lh);
            total += lh;
        }
        textlogwindow::bTooTall(this) = total > h ? 1 : 0;
        float y = vMargin + std::min(h - total, 0.0f);
        size_t index = 0;
        for (const Entry& entry : entries) {
            if (y + heights[index] > vMargin) {
                gc::HAlign(gc) = textwindow::HAlign(this);
                gc::VAlign(gc) = VALIGN_Top;
                gc::bWordWrap(gc) = 1;
                gc::TextColor(gc) = entry.color;
                gc->drawText(hMargin, y, w, heights[index], entry.text);
            }
            y += heights[index];
            index++;
        }
        gc->resetState(this, gc->offsetX, gc->offsetY);
        scriptDrawWindow(gc);
    }

    namespace {
        UTextWindow* textSelf(UObject* object)
        {
            return cast<UTextWindow>(object);
        }

        void nativeSetText(UObject* object, Frame& stack, void*)
        {
            std::string value = vm::get<std::string>(stack);
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            value = convertScriptString(value);
            if (UEditWindow* edit = cast<UEditWindow>(window)) {
                editwindow::selectStart(edit) = 0;
                editwindow::selectEnd(edit) = static_cast<int32_t>(edit->text().size());
                editwindow::insertPos(edit) = static_cast<int32_t>(edit->text().size());
                edit->insertText(value, false, false);
                edit->setInsertion(0, false);
                return;
            }
            if (window && window->text() != value) {
                window->text() = value;
                updateAccelerator(window);
                window->textModified();
                window->askParentForReconfigure();
            }
        }

        void nativeAppendText(UObject* object, Frame& stack, void*)
        {
            std::string value = vm::get<std::string>(stack);
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            value = convertScriptString(value);
            if (window && !value.empty()) {
                window->text() += value;
                updateAccelerator(window);
                window->textModified();
                window->askParentForReconfigure();
            }
        }

        void nativeGetText(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            vm::ret<std::string>(result, window ? window->text() : std::string());
        }

        void nativeGetTextLength(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            vm::ret<int32_t>(result, window ? static_cast<int32_t>(window->text().size()) : 0);
        }

        void nativeGetTextPart(UObject* object, Frame& stack, void* result)
        {
            int32_t start = vm::get<int32_t>(stack);
            int32_t count = vm::get<int32_t>(stack);
            std::string* out = vm::getRef<std::string>(stack);
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            std::string part;
            if (window) {
                int32_t length = static_cast<int32_t>(window->text().size());
                int32_t first = std::clamp(start, 0, length);
                int32_t last = std::clamp(start + count, first, length);
                part = window->text().substr(static_cast<size_t>(first), static_cast<size_t>(last - first));
            }
            *out = part;
            vm::ret<int32_t>(result, static_cast<int32_t>(part.size()));
        }

        void nativeSetWordWrap(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::bWordWrap(window) = value ? 1 : 0;
                window->askParentForReconfigure();
            }
        }

        void nativeSetTextAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t halign = vm::get<uint8_t>(stack);
            uint8_t valign = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::HAlign(window) = halign;
                textwindow::VAlign(window) = valign;
            }
        }

        void nativeSetTextMargins(UObject* object, Frame& stack, void*)
        {
            float h = vm::get<float>(stack);
            float v = vm::get<float>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::hMargin(window) = h;
                textwindow::vMargin(window) = v;
                window->askParentForReconfigure();
            }
        }

        void nativeSetLines(UObject* object, Frame& stack, void*)
        {
            int32_t minLines = vm::get<int32_t>(stack);
            int32_t maxLines = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::minLines(window) = minLines;
                textwindow::MaxLines(window) = maxLines;
                window->askParentForReconfigure();
            }
        }

        void nativeSetMinLines(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::minLines(window) = value;
                window->askParentForReconfigure();
            }
        }

        void nativeSetMaxLines(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::MaxLines(window) = value;
                window->askParentForReconfigure();
            }
        }

        void nativeResetLines(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::minLines(window) = 0;
                textwindow::MaxLines(window) = 0;
                window->askParentForReconfigure();
            }
        }

        void nativeSetMinWidth(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::MinWidth(window) = value;
                window->askParentForReconfigure();
            }
        }

        void nativeResetMinWidth(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UTextWindow* window = textSelf(object)) {
                textwindow::MinWidth(window) = 0;
                window->askParentForReconfigure();
            }
        }

        void nativeEnableTextAsAccelerator(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UTextWindow* window = textSelf(object);
            if (!window) {
                return;
            }
            textwindow::bTextIsAccelerator(window) = value ? 1 : 0;
            int32_t key = 0;
            if (value) {
                const std::string& text = window->text();
                size_t pos = text.find("|&");
                if (pos != std::string::npos && pos + 2 < text.size()) {
                    key = std::toupper(static_cast<unsigned char>(text[pos + 2]));
                }
            }
            window::acceleratorKey(window) = key;
        }

        UButtonWindow* buttonSelf(UObject* object)
        {
            return cast<UButtonWindow>(object);
        }

        void nativeActivateButton(UObject* object, Frame& stack, void*)
        {
            uint8_t key = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                button->activate(key);
            }
        }

        void nativePressButton(UObject* object, Frame& stack, void*)
        {
            uint8_t key = IK_LeftMouse;
            optional(stack, key);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                button->playWindowSound(buttonwindow::pressSound(button));
                button->activate(key);
            }
        }

        void nativeSetActivateDelay(UObject* object, Frame& stack, void*)
        {
            float value = 0;
            bool has = optional(stack, value);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                buttonwindow::activateDelay(button) = has ? value : 0.0f;
            }
        }

        void readColors(Frame& stack, std::array<Color, 6>& colors, bool& set)
        {
            for (size_t i = 0; i < 6; i++) {
                Color value {};
                if (optional(stack, value)) {
                    colors[i] = value;
                    set = true;
                }
            }
            vm::finish(stack);
        }

        void nativeSetButtonColors(UObject* object, Frame& stack, void*)
        {
            UButtonWindow* button = buttonSelf(object);
            std::array<Color, 6> scratch {};
            bool scratchSet = false;
            if (button) {
                readColors(stack, button->buttonColors, button->colorsSet);
            } else {
                readColors(stack, scratch, scratchSet);
            }
        }

        void nativeSetTextColors(UObject* object, Frame& stack, void*)
        {
            UButtonWindow* button = buttonSelf(object);
            std::array<Color, 6> scratch {};
            bool scratchSet = false;
            if (button) {
                readColors(stack, button->textColors, button->textColorsSet);
            } else {
                readColors(stack, scratch, scratchSet);
            }
        }

        void nativeSetButtonTextures(UObject* object, Frame& stack, void*)
        {
            UButtonWindow* button = buttonSelf(object);
            for (size_t i = 0; i < 6; i++) {
                UObject* value = nullptr;
                if (optional(stack, value) && button) {
                    button->buttonTextures[i] = value;
                }
            }
            vm::finish(stack);
        }

        void nativeSetButtonSounds(UObject* object, Frame& stack, void*)
        {
            UObject* press = nullptr;
            UObject* click = nullptr;
            bool hasPress = optional(stack, press);
            bool hasClick = optional(stack, click);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                if (hasPress) {
                    buttonwindow::pressSound(button) = press;
                }
                if (hasClick) {
                    buttonwindow::clickSound(button) = click;
                }
            }
        }

        void nativeEnableAutoRepeat(UObject* object, Frame& stack, void*)
        {
            bool enable = optionalBoolDefault(stack, true);
            float delay = 0.5f;
            float rate = 0.1f;
            bool hasDelay = optional(stack, delay);
            bool hasRate = optional(stack, rate);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                buttonwindow::bAutoRepeat(button) = enable ? 1 : 0;
                if (hasDelay || buttonwindow::initialDelay(button) <= 0) {
                    buttonwindow::initialDelay(button) = delay;
                }
                if (hasRate || buttonwindow::repeatRate(button) <= 0) {
                    buttonwindow::repeatRate(button) = rate;
                }
            }
        }

        void nativeEnableRightMouseClick(UObject* object, Frame& stack, void*)
        {
            bool enable = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UButtonWindow* button = buttonSelf(object)) {
                buttonwindow::bEnableRightMouseClick(button) = enable ? 1 : 0;
            }
        }

        void nativeChangeToggle(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UToggleWindow* toggle = cast<UToggleWindow>(object)) {
                toggle->setToggle(!toggle->toggled);
            }
        }

        void nativeSetToggle(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UToggleWindow* toggle = cast<UToggleWindow>(object)) {
                toggle->setToggle(value);
            }
        }

        void nativeGetToggle(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UToggleWindow* toggle = cast<UToggleWindow>(object);
            vm::retBool(result, toggle && toggle->toggled);
        }

        void nativeSetToggleSounds(UObject* object, Frame& stack, void*)
        {
            UObject* enable = nullptr;
            UObject* disable = nullptr;
            bool hasEnable = optional(stack, enable);
            bool hasDisable = optional(stack, disable);
            vm::finish(stack);
            if (UToggleWindow* toggle = cast<UToggleWindow>(object)) {
                if (hasEnable) {
                    togglewindow::enableSound(toggle) = enable;
                }
                if (hasDisable) {
                    togglewindow::disableSound(toggle) = disable;
                }
            }
        }

        void nativeSetCheckboxTextures(UObject* object, Frame& stack, void*)
        {
            UObject* off = nullptr;
            UObject* on = nullptr;
            float w = 0;
            float h = 0;
            bool hasOff = optional(stack, off);
            bool hasOn = optional(stack, on);
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            UCheckboxWindow* box = cast<UCheckboxWindow>(object);
            if (!box) {
                return;
            }
            if (hasOff) {
                checkboxwindow::toggleOff(box) = off;
            }
            if (hasOn) {
                checkboxwindow::toggleOn(box) = on;
            }
            if (hasW) {
                checkboxwindow::textureWidth(box) = w;
            }
            if (hasH) {
                checkboxwindow::textureHeight(box) = h;
            }
            box->askParentForReconfigure();
        }

        void nativeSetCheckboxSpacing(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UCheckboxWindow* box = cast<UCheckboxWindow>(object)) {
                checkboxwindow::checkboxSpacing(box) = value;
            }
        }

        void nativeShowCheckboxOnRightSide(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UCheckboxWindow* box = cast<UCheckboxWindow>(object)) {
                checkboxwindow::bRightSide(box) = value ? 1 : 0;
            }
        }

        void nativeSetCheckboxStyle(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UCheckboxWindow* box = cast<UCheckboxWindow>(object)) {
                checkboxwindow::checkboxStyle(box) = value;
            }
        }

        void nativeSetCheckboxColor(UObject* object, Frame& stack, void*)
        {
            Color value = vm::get<Color>(stack);
            vm::finish(stack);
            if (UCheckboxWindow* box = cast<UCheckboxWindow>(object)) {
                checkboxwindow::checkboxColor(box) = value;
            }
        }

        void nativeSetVerticalSpacing(UObject* object, Frame& stack, void*)
        {
            float value = 0;
            optional(stack, value);
            vm::finish(stack);
            if (ULargeTextWindow* window = cast<ULargeTextWindow>(object)) {
                largetextwindow::vSpace(window) = value;
                window->askParentForReconfigure();
            }
        }

        void nativeAddLog(UObject* object, Frame& stack, void*)
        {
            std::string text = vm::get<std::string>(stack);
            Color color {};
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            UTextLogWindow* log = cast<UTextLogWindow>(object);
            if (!log) {
                return;
            }
            UTextLogWindow::Entry entry;
            entry.text = text;
            entry.color = hasColor ? color : window::TextColor(log);
            float timeout = textlogwindow::textTimeout(log);
            entry.expires = timeout > 0 ? log->totalTime + static_cast<double>(timeout) : 0.0;
            log->entries.push_back(entry);
            log->askParentForReconfigure();
        }

        void nativeClearLog(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UTextLogWindow* log = cast<UTextLogWindow>(object)) {
                log->entries.clear();
                log->askParentForReconfigure();
            }
        }

        void nativeSetTextTimeout(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UTextLogWindow* log = cast<UTextLogWindow>(object)) {
                textlogwindow::textTimeout(log) = value;
            }
        }

        void nativePauseLog(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UTextLogWindow* log = cast<UTextLogWindow>(object)) {
                textlogwindow::bPaused(log) = value ? 1 : 0;
            }
        }

        UEditWindow* editSelf(UObject* object)
        {
            return cast<UEditWindow>(object);
        }

        void nativeMoveInsertionPoint(UObject* object, Frame& stack, void*)
        {
            uint8_t move = vm::get<uint8_t>(stack);
            bool drag = optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->moveInsertion(move, drag);
            }
        }

        void nativeSetInsertionPoint(UObject* object, Frame& stack, void*)
        {
            int32_t position = vm::get<int32_t>(stack);
            bool drag = optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->setInsertion(position, drag);
            }
        }

        void nativeGetInsertionPoint(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            vm::ret<int32_t>(result, edit ? editwindow::insertPos(edit) : 0);
        }

        void nativeSetSelectedArea(UObject* object, Frame& stack, void*)
        {
            int32_t start = vm::get<int32_t>(stack);
            int32_t count = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                editwindow::selectStart(edit) = start;
                editwindow::selectEnd(edit) = start + count;
                editwindow::insertPos(edit) = start + count;
                edit->clampPositions();
            }
        }

        void nativeGetSelectedArea(UObject* object, Frame& stack, void*)
        {
            int32_t* start = vm::getRef<int32_t>(stack);
            int32_t* count = vm::getRef<int32_t>(stack);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                int32_t a = std::min(editwindow::selectStart(edit), editwindow::selectEnd(edit));
                int32_t b = std::max(editwindow::selectStart(edit), editwindow::selectEnd(edit));
                *start = a;
                *count = b - a;
            }
        }

        void editFlag(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                field(edit) = value ? 1 : 0;
            }
        }

        void nativeEnableEditing(UObject* object, Frame& stack, void*)
        {
            editFlag(object, stack, editwindow::bEditable);
        }

        void nativeIsEditingEnabled(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            vm::retBool(result, edit && editwindow::bEditable(edit));
        }

        void nativeEnableSingleLineEditing(UObject* object, Frame& stack, void*)
        {
            editFlag(object, stack, editwindow::bSingleLine);
        }

        void nativeIsSingleLineEditingEnabled(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            vm::retBool(result, edit && editwindow::bSingleLine(edit));
        }

        void nativeEnableUppercaseOnly(UObject* object, Frame& stack, void*)
        {
            editFlag(object, stack, editwindow::bUppercaseOnly);
        }

        void nativeClearTextChangedFlag(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->setTextChangedFlag(false);
            }
        }

        void nativeSetTextChangedFlag(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->setTextChangedFlag(value);
            }
        }

        void nativeHasTextChanged(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            vm::retBool(result, edit && edit->hasTextChanged());
        }

        void nativeSetMaxSize(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                editwindow::maxSize(edit) = value;
            }
        }

        void nativeSetMaxUndos(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                editwindow::maxUndos(edit) = value;
            }
        }

        void nativeInsertText(UObject* object, Frame& stack, void* result)
        {
            std::string text;
            optional(stack, text);
            bool undo = optionalBoolDefault(stack, true);
            bool select = optionalBoolDefault(stack, false);
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            vm::retBool(result, edit && edit->insertText(text, undo, select));
        }

        void nativeDeleteChar(UObject* object, Frame& stack, void*)
        {
            bool before = optionalBoolDefault(stack, true);
            bool undo = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->deleteChar(before, undo);
            }
        }

        void nativeSetInsertionPointBlinkRate(UObject* object, Frame& stack, void*)
        {
            float start = 0;
            float period = 0;
            bool hasStart = optional(stack, start);
            bool hasPeriod = optional(stack, period);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                if (hasStart) {
                    editwindow::blinkStart(edit) = start;
                }
                if (hasPeriod) {
                    editwindow::blinkPeriod(edit) = period;
                }
            }
        }

        void nativeSetInsertionPointTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = nullptr;
            Color color {};
            bool hasTexture = optional(stack, texture);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                if (hasTexture) {
                    editwindow::insertTexture(edit) = texture;
                }
                if (hasColor) {
                    editwindow::insertColor(edit) = color;
                }
            }
        }

        void nativeSetInsertionPointType(UObject* object, Frame& stack, void*)
        {
            uint8_t type = vm::get<uint8_t>(stack);
            float w = 0;
            float h = 0;
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                editwindow::insertType(edit) = type;
                if (hasW) {
                    editwindow::insertPrefWidth(edit) = w;
                }
                if (hasH) {
                    editwindow::insertPrefHeight(edit) = h;
                }
            }
        }

        void nativeSetSelectedAreaTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = nullptr;
            Color color {};
            bool hasTexture = optional(stack, texture);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                if (hasTexture) {
                    editwindow::selectTexture(edit) = texture;
                }
                if (hasColor) {
                    editwindow::selectColor(edit) = color;
                }
            }
        }

        void nativeSetSelectedAreaTextColor(UObject* object, Frame& stack, void*)
        {
            Color color {};
            bool has = optional(stack, color);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                if (has) {
                    editwindow::inverseColor(edit) = color;
                }
            }
        }

        void nativeSetEditCursor(UObject* object, Frame& stack, void*)
        {
            UObject* cursor = nullptr;
            UObject* shadow = nullptr;
            Color color {};
            bool hasCursor = optional(stack, cursor);
            bool hasShadow = optional(stack, shadow);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                if (hasCursor) {
                    editwindow::editCursor(edit) = cursor;
                    window::defaultCursor(edit) = cursor;
                }
                if (hasShadow) {
                    editwindow::editCursorShadow(edit) = shadow;
                    window::defaultCursorShadow(edit) = shadow;
                }
                if (hasColor) {
                    editwindow::editCursorColor(edit) = color;
                }
            }
        }

        void nativeUndo(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            if (edit && edit->undoPosition > 0) {
                edit->undoPosition--;
                edit->text() = edit->undoStack[edit->undoPosition];
                edit->clampPositions();
                edit->notifyChanged();
            }
        }

        void nativeRedo(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            if (edit && edit->undoPosition + 1 < edit->undoStack.size()) {
                edit->undoPosition++;
                edit->text() = edit->undoStack[edit->undoPosition];
                edit->clampPositions();
                edit->notifyChanged();
            }
        }

        void nativeClearUndo(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->clearUndo();
            }
        }

        std::string selectionOf(UEditWindow* edit)
        {
            int32_t a = std::min(editwindow::selectStart(edit), editwindow::selectEnd(edit));
            int32_t b = std::max(editwindow::selectStart(edit), editwindow::selectEnd(edit));
            return edit->text().substr(static_cast<size_t>(a), static_cast<size_t>(b - a));
        }

        void nativeCopy(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->clampPositions();
                edit->clipboard = selectionOf(edit);
            }
        }

        void nativeCut(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->clampPositions();
                edit->clipboard = selectionOf(edit);
                if (!edit->clipboard.empty()) {
                    edit->deleteChar(true, true);
                }
            }
        }

        void nativePaste(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UEditWindow* edit = editSelf(object)) {
                edit->insertText(edit->clipboard, true, false);
            }
        }

        void nativeSetEditSounds(UObject* object, Frame& stack, void*)
        {
            UObject* sounds[4] = {};
            bool present[4] = {};
            for (int i = 0; i < 4; i++) {
                present[i] = optional(stack, sounds[i]);
            }
            vm::finish(stack);
            UEditWindow* edit = editSelf(object);
            if (!edit) {
                return;
            }
            UObject*& (*fields[4])(UObject*) = { editwindow::typeSound, editwindow::deleteSound, editwindow::enterSound, editwindow::moveSound };
            for (int i = 0; i < 4; i++) {
                if (present[i]) {
                    fields[i](edit) = sounds[i];
                }
            }
        }

        void nativePlayEditSound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            float volume = 1;
            float pitch = 1;
            optional(stack, volume);
            optional(stack, pitch);
            vm::finish(stack);
            playSound(cast<UWindow>(object), sound, volume, pitch);
        }
    }

    void registerTextWindowNatives()
    {
        vm::registerNative(1550, nativeSetText);
        vm::registerNative(1551, nativeAppendText);
        vm::registerNative(1552, nativeGetText);
        vm::registerNative(1553, nativeGetTextLength);
        vm::registerNative(1554, nativeGetTextPart);
        vm::registerNative(1555, nativeSetWordWrap);
        vm::registerNative(1556, nativeSetTextAlignments);
        vm::registerNative(1557, nativeSetTextMargins);
        vm::registerNative(1558, nativeSetLines);
        vm::registerNative(1559, nativeSetMinLines);
        vm::registerNative(1560, nativeSetMaxLines);
        vm::registerNative(1561, nativeResetLines);
        vm::registerNative(1562, nativeSetMinWidth);
        vm::registerNative(1563, nativeResetMinWidth);
        vm::registerNative(1564, nativeEnableTextAsAccelerator);
        vm::registerNative(1570, nativeAddLog);
        vm::registerNative(1571, nativeClearLog);
        vm::registerNative(1572, nativeSetTextTimeout);
        vm::registerNative(1573, nativePauseLog);
        vm::registerNative(1590, nativeActivateButton);
        vm::registerNative(1591, nativeSetActivateDelay);
        vm::registerNative(1592, nativeSetButtonTextures);
        vm::registerNative(1593, nativeSetButtonColors);
        vm::registerNative(1594, nativeSetTextColors);
        vm::registerNative(1595, nativeEnableAutoRepeat);
        vm::registerNative(1596, nativeEnableRightMouseClick);
        vm::registerNative(1597, nativeSetButtonSounds);
        vm::registerNative(1598, nativePressButton);
        vm::registerNative(1610, nativeChangeToggle);
        vm::registerNative(1611, nativeSetToggle);
        vm::registerNative(1612, nativeGetToggle);
        vm::registerNative(1613, nativeSetToggleSounds);
        vm::registerNative(1840, nativeSetCheckboxTextures);
        vm::registerNative(1841, nativeSetCheckboxSpacing);
        vm::registerNative(1842, nativeShowCheckboxOnRightSide);
        vm::registerNative(1843, nativeSetCheckboxStyle);
        vm::registerNative(1844, nativeSetCheckboxColor);
        vm::registerNative(1860, nativeSetVerticalSpacing);
        vm::registerNative(1880, nativeMoveInsertionPoint);
        vm::registerNative(1881, nativeSetInsertionPoint);
        vm::registerNative(1882, nativeGetInsertionPoint);
        vm::registerNative(1883, nativeSetSelectedArea);
        vm::registerNative(1884, nativeGetSelectedArea);
        vm::registerNative(1885, nativeEnableEditing);
        vm::registerNative(1886, nativeIsEditingEnabled);
        vm::registerNative(1887, nativeEnableSingleLineEditing);
        vm::registerNative(1888, nativeIsSingleLineEditingEnabled);
        vm::registerNative(1889, nativeEnableUppercaseOnly);
        vm::registerNative(1890, nativeClearTextChangedFlag);
        vm::registerNative(1891, nativeSetTextChangedFlag);
        vm::registerNative(1892, nativeHasTextChanged);
        vm::registerNative(1895, nativeSetMaxSize);
        vm::registerNative(1896, nativeSetMaxUndos);
        vm::registerNative(1900, nativeInsertText);
        vm::registerNative(1901, nativeDeleteChar);
        vm::registerNative(1902, nativeSetInsertionPointBlinkRate);
        vm::registerNative(1905, nativeSetInsertionPointTexture);
        vm::registerNative(1906, nativeSetInsertionPointType);
        vm::registerNative(1907, nativeSetSelectedAreaTexture);
        vm::registerNative(1908, nativeSetSelectedAreaTextColor);
        vm::registerNative(1909, nativeSetEditCursor);
        vm::registerNative(1910, nativeUndo);
        vm::registerNative(1911, nativeRedo);
        vm::registerNative(1912, nativeClearUndo);
        vm::registerNative(1915, nativeCopy);
        vm::registerNative(1916, nativeCut);
        vm::registerNative(1917, nativePaste);
        vm::registerNative(1920, nativeSetEditSounds);
        vm::registerNative(1921, nativePlayEditSound);
    }
}
