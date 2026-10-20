#include "extension/windows.h"

#include "engine/input.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace dx::extension {
    namespace {
        bool startsWithNoCase(const std::string& text, const std::string& prefix)
        {
            if (prefix.size() > text.size()) {
                return false;
            }
            for (size_t i = 0; i < prefix.size(); i++) {
                if (std::tolower(static_cast<unsigned char>(text[i])) != std::tolower(static_cast<unsigned char>(prefix[i]))) {
                    return false;
                }
            }
            return true;
        }

        int compareText(const std::string& a, const std::string& b, bool caseSensitive)
        {
            size_t count = std::min(a.size(), b.size());
            for (size_t i = 0; i < count; i++) {
                int ca = static_cast<unsigned char>(a[i]);
                int cb = static_cast<unsigned char>(b[i]);
                if (!caseSensitive) {
                    ca = std::tolower(ca);
                    cb = std::tolower(cb);
                }
                if (ca != cb) {
                    return ca < cb ? -1 : 1;
                }
            }
            if (a.size() == b.size()) {
                return 0;
            }
            return a.size() < b.size() ? -1 : 1;
        }

        double timeValue(const std::string& text)
        {
            double total = 0;
            double part = 0;
            for (char c : text) {
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    part = part * 10 + (c - '0');
                } else if (c == ':') {
                    total = total * 60 + part;
                    part = 0;
                }
            }
            return total * 60 + part;
        }
    }

    void UListWindow::initWindow()
    {
        listwindow::focusLine(this) = -1;
        listwindow::anchorLine(this) = -1;
        listwindow::lastIndex(this) = -1;
        listwindow::focusThickness(this) = 1.0f;
        listwindow::highlightColor(this) = { 255, 255, 255, 255 };
        listwindow::focusColor(this) = { 128, 128, 128, 255 };
        listwindow::hotKeyCol(this) = 0;
        window::bIsSelectable(this) = 1;
        UWindow::initWindow();
        if (!window::normalFont(this)) {
            setFont(defaultFont());
        }
    }

    void UListWindow::parentRequestedGranularity(float& hGranularity, float& vGranularity)
    {
        hGranularity = 1.0f;
        vGranularity = lineHeight();
        UWindow::parentRequestedGranularity(hGranularity, vGranularity);
    }

    float UListWindow::lineHeight()
    {
        float height = fontHeight(window::normalFont(this));
        for (const Column& column : columns) {
            if (column.font) {
                height = std::max(height, fontHeight(column.font));
            }
        }
        height += fieldMarginHeight * 2.0f + listwindow::rowMargin(this);
        return std::max(height, 1.0f);
    }

    int32_t UListWindow::indexOf(int32_t rowId)
    {
        for (size_t i = 0; i < rows.size(); i++) {
            if (rows[i].id == rowId) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    UListWindow::Row* UListWindow::row(int32_t rowId)
    {
        int32_t index = indexOf(rowId);
        return index >= 0 ? &rows[static_cast<size_t>(index)] : nullptr;
    }

    std::vector<std::string> UListWindow::splitRow(const std::string& text)
    {
        std::string delimiter = listwindow::Delimiter(this);
        if (delimiter.empty()) {
            delimiter = ";";
        }
        std::vector<std::string> fields;
        size_t start = 0;
        while (true) {
            size_t pos = text.find(delimiter, start);
            if (pos == std::string::npos) {
                fields.push_back(text.substr(start));
                break;
            }
            fields.push_back(text.substr(start, pos - start));
            start = pos + delimiter.size();
        }
        return fields;
    }

    int32_t UListWindow::selectedCount()
    {
        int32_t count = 0;
        for (const Row& entry : rows) {
            if (entry.selected) {
                count++;
            }
        }
        listwindow::numSelected(this) = count;
        return count;
    }

    void UListWindow::selectRow(int32_t rowId, bool select)
    {
        if (Row* entry = row(rowId)) {
            entry->selected = select;
        }
        selectedCount();
    }

    void UListWindow::setFocusLine(int32_t index, bool show)
    {
        if (rows.empty()) {
            listwindow::focusLine(this) = -1;
            return;
        }
        index = std::clamp(index, 0, static_cast<int32_t>(rows.size()) - 1);
        listwindow::focusLine(this) = index;
        if (show) {
            float lh = lineHeight();
            askParentToShowArea(0, static_cast<float>(index) * lh, width(), lh);
        }
    }

    void UListWindow::setRow(int32_t rowId, bool select, bool clearRows)
    {
        int32_t index = indexOf(rowId);
        if (index < 0) {
            return;
        }
        bool changed = false;
        if (clearRows) {
            for (size_t i = 0; i < rows.size(); i++) {
                if (rows[i].selected && !(select && i == static_cast<size_t>(index))) {
                    rows[i].selected = false;
                    changed = true;
                }
            }
        }
        if (select && !rows[static_cast<size_t>(index)].selected) {
            rows[static_cast<size_t>(index)].selected = true;
            changed = true;
        }
        listwindow::anchorLine(this) = index;
        setFocusLine(index, true);
        if (changed) {
            notifySelection();
        }
    }

    void UListWindow::visibilityChanged(bool value)
    {
        UWindow::visibilityChanged(value);
        if (value && listwindow::focusLine(this) < 0 && !rows.empty()) {
            setRow(rows.front().id, true, true);
        }
    }

    void UListWindow::notifySelection()
    {
        if (notifying) {
            return;
        }
        int32_t focus = listwindow::focusLine(this);
        int32_t focusId = focus >= 0 && focus < static_cast<int32_t>(rows.size()) ? rows[static_cast<size_t>(focus)].id : 0;
        int32_t count = selectedCount();
        notifying = true;
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->listSelectionChanged(this, count, focusId)) {
                break;
            }
        }
        notifying = false;
    }

    void UListWindow::notifyActivated(int32_t rowId)
    {
        playWindowSound(listwindow::ActivateSound(this));
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->listRowActivated(this, rowId)) {
                return;
            }
        }
    }

    void UListWindow::moveRow(uint8_t move, bool select, bool clearRows)
    {
        if (rows.empty()) {
            return;
        }
        int32_t focus = listwindow::focusLine(this);
        int32_t last = static_cast<int32_t>(rows.size()) - 1;
        float lh = lineHeight();
        float visibleHeight = height();
        if (UClipWindow* clip = cast<UClipWindow>(parent())) {
            visibleHeight = clip->height();
        }
        int32_t page = std::max(static_cast<int32_t>(visibleHeight / lh) - 1, 1);
        switch (move) {
        case 0:
            focus = focus < 0 ? 0 : focus - 1;
            break;
        case 1:
            focus = focus < 0 ? 0 : focus + 1;
            break;
        case 2:
            focus -= page;
            break;
        case 3:
            focus += page;
            break;
        case 4:
            focus = 0;
            break;
        case 5:
            focus = last;
            break;
        default:
            break;
        }
        focus = std::clamp(focus, 0, last);
        if (select) {
            if (clearRows) {
                for (Row& entry : rows) {
                    entry.selected = false;
                }
            }
            rows[static_cast<size_t>(focus)].selected = true;
        }
        playWindowSound(listwindow::moveSound(this));
        setFocusLine(focus, true);
    }

    void UListWindow::sortRows()
    {
        if (sortKeys.empty()) {
            return;
        }
        int32_t focus = listwindow::focusLine(this);
        int32_t focusId = focus >= 0 && focus < static_cast<int32_t>(rows.size()) ? rows[static_cast<size_t>(focus)].id : 0;
        std::stable_sort(rows.begin(), rows.end(), [this](const Row& a, const Row& b) {
            for (const SortKey& key : sortKeys) {
                size_t column = static_cast<size_t>(std::max(key.column, 0));
                std::string fa = column < a.fields.size() ? a.fields[column] : std::string();
                std::string fb = column < b.fields.size() ? b.fields[column] : std::string();
                uint8_t type = column < columns.size() ? columns[column].type : 0;
                int order = 0;
                if (type == 1) {
                    double va = std::atof(fa.c_str());
                    double vb = std::atof(fb.c_str());
                    order = va < vb ? -1 : (va > vb ? 1 : 0);
                } else if (type == 2) {
                    double va = timeValue(fa);
                    double vb = timeValue(fb);
                    order = va < vb ? -1 : (va > vb ? 1 : 0);
                } else {
                    order = compareText(fa, fb, key.caseSensitive);
                }
                if (key.reverse) {
                    order = -order;
                }
                if (order != 0) {
                    return order < 0;
                }
            }
            return false;
        });
        if (focusId != 0) {
            listwindow::focusLine(this) = indexOf(focusId);
        }
    }

    void UListWindow::autoSize()
    {
        UObject* font = window::normalFont(this);
        for (size_t c = 0; c < columns.size(); c++) {
            float w = 0;
            float h = 0;
            measureString(columns[c].font ? columns[c].font : font, columns[c].title, w, h);
            float widest = w;
            for (const Row& entry : rows) {
                if (c < entry.fields.size()) {
                    measureString(columns[c].font ? columns[c].font : font, entry.fields[c], w, h);
                    widest = std::max(widest, w);
                }
            }
            columns[c].width = std::max(columns[c].width, widest + fieldMarginWidth * 2.0f);
        }
    }

    void UListWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        if (!widthSpecified) {
            float total = 0;
            for (const Column& column : columns) {
                if (!column.hidden) {
                    total += column.width;
                }
            }
            preferredWidth = total;
        }
        if (!heightSpecified) {
            preferredHeight = static_cast<float>(rows.size()) * lineHeight();
        }
        UWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UListWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        UObject* font = window::normalFont(this);
        float lh = lineHeight();
        listwindow::lineSize(this) = lh;
        std::vector<float> widths;
        float fixed = 0;
        int32_t flexible = 0;
        for (const Column& column : columns) {
            if (column.hidden) {
                widths.push_back(0);
                continue;
            }
            widths.push_back(column.width);
            if (column.width <= 0) {
                flexible++;
            } else {
                fixed += column.width;
            }
        }
        float remaining = std::max(width() - fixed, 0.0f);
        for (size_t c = 0; c < columns.size(); c++) {
            if (!columns[c].hidden && columns[c].width <= 0) {
                widths[c] = flexible > 0 ? remaining / static_cast<float>(flexible) : 0.0f;
            }
        }
        if (listwindow::bAutoExpandColumns(this) && fixed > 0 && flexible == 0 && fixed < width()) {
            float scale = width() / fixed;
            for (float& w : widths) {
                w *= scale;
            }
        }
        if (columns.empty()) {
            widths.push_back(width());
        }
        int32_t focus = listwindow::focusLine(this);
        float y = 0;
        for (size_t index = 0; index < rows.size(); index++) {
            const Row& entry = rows[index];
            if (gc->offsetY + y + lh < gc->clipTop || gc->offsetY + y > gc->clipBottom) {
                y += lh;
                continue;
            }
            if (entry.selected) {
                gc->setStyle(DSTY_Masked);
                gc::tileColor(gc) = listwindow::highlightColor(this);
                gc->drawPattern(0, y, width(), lh, 0, 0, listwindow::highlightTexture(this));
            }
            gc->resetState(this, gc->offsetX, gc->offsetY);
            float x = 0;
            for (size_t c = 0; c < widths.size(); c++) {
                float w = widths[c];
                if (w <= 0) {
                    continue;
                }
                if (c < entry.fields.size() && !entry.fields[c].empty()) {
                    const Column* column = c < columns.size() ? &columns[c] : nullptr;
                    gc::normalFont(gc) = column && column->font ? column->font : font;
                    gc::boldFont(gc) = gc::normalFont(gc);
                    gc::HAlign(gc) = HALIGN_Left;
                    gc::VAlign(gc) = VALIGN_Top;
                    gc::bWordWrap(gc) = 0;
                    Color color = window::TextColor(this);
                    if (column && column->colorSet) {
                        color = column->color;
                    }
                    if (entry.selected) {
                        color = highlightTextColor;
                    }
                    gc::TextColor(gc) = color;
                    float textWidth = 0;
                    float textHeight = 0;
                    gc->textExtent(0, entry.fields[c], textWidth, textHeight);
                    uint8_t align = column ? column->align : HALIGN_Left;
                    float textX = x + fieldMarginWidth;
                    if (align == HALIGN_Right) {
                        textX = x + w - textWidth - fieldMarginWidth;
                    } else if (align == HALIGN_Center) {
                        textX = x + (w - textWidth) * 0.5f;
                    }
                    float textY = y + lh - textHeight - fieldMarginHeight + 1.0f;
                    gc->pushClip(gc->offsetX + x + fieldMarginWidth, gc->offsetY + y + fieldMarginHeight, w - fieldMarginWidth * 2.0f, lh - fieldMarginHeight * 2.0f);
                    gc->drawText(textX, textY, textWidth, textHeight, entry.fields[c]);
                    gc->popClip();
                }
                x += w;
            }
            gc->resetState(this, gc->offsetX, gc->offsetY);
            y += lh;
        }
        URootWindow* rootWindow = root();
        if (rootWindow && rootWindow->focusWindow() == this && focus >= 0 && focus < static_cast<int32_t>(rows.size())) {
            gc->setStyle(DSTY_Masked);
            gc::tileColor(gc) = listwindow::focusColor(this);
            gc->drawBox(0, static_cast<float>(focus) * lh, width(), lh, listwindow::focusThickness(this), listwindow::focusTexture(this));
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        scriptDrawWindow(gc);
    }

    bool UListWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        if (UWindow::mouseButtonPressed(pointX, pointY, button, numClicks)) {
            return true;
        }
        if (button != IK_LeftMouse && button != IK_RightMouse) {
            return false;
        }
        setFocusWindow(this);
        int32_t index = static_cast<int32_t>(std::floor(pointY / lineHeight()));
        if (index < 0 || index >= static_cast<int32_t>(rows.size())) {
            return true;
        }
        bool ctrl = state().keys[IK_Ctrl] || state().keys[IK_LControl] || state().keys[IK_RControl];
        bool shift = state().keys[IK_Shift] || state().keys[IK_LShift] || state().keys[IK_RShift];
        bool multi = listwindow::bMultiSelect(this) != 0;
        Row& entry = rows[static_cast<size_t>(index)];
        if (multi && ctrl) {
            entry.selected = !entry.selected;
            listwindow::anchorLine(this) = index;
        } else if (multi && shift && listwindow::anchorLine(this) >= 0) {
            int32_t anchor = std::min(listwindow::anchorLine(this), static_cast<int32_t>(rows.size()) - 1);
            for (Row& other : rows) {
                other.selected = false;
            }
            for (int32_t i = std::min(anchor, index); i <= std::max(anchor, index); i++) {
                rows[static_cast<size_t>(i)].selected = true;
            }
        } else {
            for (Row& other : rows) {
                other.selected = false;
            }
            entry.selected = true;
            listwindow::anchorLine(this) = index;
        }
        setFocusLine(index, true);
        int32_t rowId = entry.id;
        notifySelection();
        if (numClicks >= 2 && button == IK_LeftMouse) {
            notifyActivated(rowId);
        }
        return true;
    }

    bool UListWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        if (UWindow::virtualKeyPressed(key, repeat)) {
            return true;
        }
        bool multi = listwindow::bMultiSelect(this) != 0;
        bool shift = state().keys[IK_Shift] || state().keys[IK_LShift] || state().keys[IK_RShift];
        int move = -1;
        switch (key) {
        case IK_Up:
            move = 0;
            break;
        case IK_Down:
            move = 1;
            break;
        case IK_PageUp:
            move = 2;
            break;
        case IK_PageDown:
            move = 3;
            break;
        case IK_Home:
            move = 4;
            break;
        case IK_End:
            move = 5;
            break;
        case IK_Enter: {
            int32_t focus = listwindow::focusLine(this);
            if (focus >= 0 && focus < static_cast<int32_t>(rows.size())) {
                notifyActivated(rows[static_cast<size_t>(focus)].id);
                return true;
            }
            return false;
        }
        default:
            return false;
        }
        moveRow(static_cast<uint8_t>(move), true, !(multi && shift));
        notifySelection();
        return true;
    }

    bool UListWindow::keyPressed(const std::string& key)
    {
        if (UWindow::keyPressed(key)) {
            return true;
        }
        if (!listwindow::bHotKeys(this) || key.empty() || static_cast<unsigned char>(key[0]) < 32) {
            return false;
        }
        listwindow::hotKeyString(this) += key;
        listwindow::hotKeyTimer(this) = 1.0f;
        size_t column = static_cast<size_t>(std::max(listwindow::hotKeyCol(this), 0));
        for (size_t i = 0; i < rows.size(); i++) {
            if (column < rows[i].fields.size() && startsWithNoCase(rows[i].fields[column], listwindow::hotKeyString(this))) {
                for (Row& other : rows) {
                    other.selected = false;
                }
                rows[i].selected = true;
                setFocusLine(static_cast<int32_t>(i), true);
                notifySelection();
                return true;
            }
        }
        return false;
    }

    void UListWindow::tick(float deltaSeconds)
    {
        if (listwindow::hotKeyTimer(this) > 0) {
            listwindow::hotKeyTimer(this) -= deltaSeconds;
            if (listwindow::hotKeyTimer(this) <= 0) {
                listwindow::hotKeyString(this).clear();
            }
        }
        UWindow::tick(deltaSeconds);
    }

    namespace {
        UListWindow* self(UObject* object)
        {
            return cast<UListWindow>(object);
        }

        UListWindow::Column* columnOf(UListWindow* list, int32_t index)
        {
            if (!list || index < 0 || static_cast<size_t>(index) >= list->columns.size()) {
                return nullptr;
            }
            return &list->columns[static_cast<size_t>(index)];
        }

        void nativeIndexToRowId(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            int32_t id = 0;
            if (list && index >= 0 && static_cast<size_t>(index) < list->rows.size()) {
                id = list->rows[static_cast<size_t>(index)].id;
            }
            vm::ret<int32_t>(result, id);
        }

        void nativeRowIdToIndex(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            vm::ret<int32_t>(result, list ? list->indexOf(rowId) : -1);
        }

        void nativeSetRowClientInt(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                if (UListWindow::Row* entry = list->row(rowId)) {
                    entry->clientInt = value;
                }
            }
        }

        void nativeGetRowClientInt(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            vm::ret<int32_t>(result, entry ? entry->clientInt : 0);
        }

        void nativeSetRowClientObject(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            UObject* value = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                if (UListWindow::Row* entry = list->row(rowId)) {
                    entry->clientObject = value;
                }
            }
        }

        void nativeGetRowClientObject(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            vm::ret<UObject*>(result, entry ? entry->clientObject : nullptr);
        }

        void nativeAddRow(UObject* object, Frame& stack, void* result)
        {
            std::string text = vm::get<std::string>(stack);
            int32_t clientData = 0;
            optional(stack, clientData);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                vm::ret<int32_t>(result, 0);
                return;
            }
            UListWindow::Row entry;
            entry.id = list->nextRowId++;
            entry.fields = list->splitRow(text);
            entry.clientInt = clientData;
            list->rows.push_back(entry);
            if (listwindow::bAutoSort(list)) {
                list->sortRows();
            }
            list->askParentForReconfigure();
            vm::ret<int32_t>(result, entry.id);
        }

        void nativeDeleteRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                return;
            }
            int32_t index = list->indexOf(rowId);
            if (index < 0) {
                return;
            }
            list->rows.erase(list->rows.begin() + index);
            int32_t& focus = listwindow::focusLine(list);
            if (focus >= static_cast<int32_t>(list->rows.size())) {
                focus = static_cast<int32_t>(list->rows.size()) - 1;
            }
            list->selectedCount();
            list->askParentForReconfigure();
        }

        void nativeModifyRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                return;
            }
            if (UListWindow::Row* entry = list->row(rowId)) {
                entry->fields = list->splitRow(text);
                if (listwindow::bAutoSort(list)) {
                    list->sortRows();
                }
            }
        }

        void nativeDeleteAllRows(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->rows.clear();
                listwindow::focusLine(list) = -1;
                listwindow::anchorLine(list) = -1;
                listwindow::numSelected(list) = 0;
                list->askParentForReconfigure();
            }
        }

        void nativeSetField(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            int32_t column = vm::get<int32_t>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            if (entry && column >= 0) {
                if (entry->fields.size() <= static_cast<size_t>(column)) {
                    entry->fields.resize(static_cast<size_t>(column) + 1);
                }
                entry->fields[static_cast<size_t>(column)] = text;
                if (listwindow::bAutoSort(list)) {
                    list->sortRows();
                }
            }
        }

        void nativeGetField(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            int32_t column = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            std::string text;
            if (entry && column >= 0 && static_cast<size_t>(column) < entry->fields.size()) {
                text = entry->fields[static_cast<size_t>(column)];
            }
            vm::ret<std::string>(result, text);
        }

        void nativeSetFieldValue(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            int32_t column = vm::get<int32_t>(stack);
            float value = vm::get<float>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            if (!entry || column < 0) {
                return;
            }
            std::string format = "%g";
            if (static_cast<size_t>(column) < list->columns.size() && !list->columns[static_cast<size_t>(column)].format.empty()) {
                format = list->columns[static_cast<size_t>(column)].format;
            }
            if (entry->fields.size() <= static_cast<size_t>(column)) {
                entry->fields.resize(static_cast<size_t>(column) + 1);
            }
            entry->fields[static_cast<size_t>(column)] = formatNumber(format, static_cast<double>(value));
        }

        void nativeGetFieldValue(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            int32_t column = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            float value = 0;
            if (entry && column >= 0 && static_cast<size_t>(column) < entry->fields.size()) {
                value = static_cast<float>(std::atof(entry->fields[static_cast<size_t>(column)].c_str()));
            }
            vm::ret<float>(result, value);
        }

        void nativeGetNumRows(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            vm::ret<int32_t>(result, list ? static_cast<int32_t>(list->rows.size()) : 0);
        }

        void nativeGetNumSelectedRows(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            vm::ret<int32_t>(result, list ? list->selectedCount() : 0);
        }

        void nativeSelectRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            bool select = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->selectRow(rowId, select);
            }
        }

        void nativeSelectAllRows(UObject* object, Frame& stack, void*)
        {
            bool select = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                for (UListWindow::Row& entry : list->rows) {
                    entry.selected = select;
                }
                list->selectedCount();
            }
        }

        void nativeSelectToRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            bool clearRows = optionalBoolDefault(stack, true);
            bool invert = optionalBoolDefault(stack, false);
            optionalBoolDefault(stack, false);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                return;
            }
            int32_t target = list->indexOf(rowId);
            if (target < 0) {
                return;
            }
            int32_t anchor = listwindow::anchorLine(list);
            if (anchor < 0 || anchor >= static_cast<int32_t>(list->rows.size())) {
                anchor = target;
            }
            if (clearRows) {
                for (UListWindow::Row& entry : list->rows) {
                    entry.selected = false;
                }
            }
            for (int32_t i = std::min(anchor, target); i <= std::max(anchor, target); i++) {
                UListWindow::Row& entry = list->rows[static_cast<size_t>(i)];
                entry.selected = invert ? !entry.selected : true;
            }
            list->setFocusLine(target, false);
            list->selectedCount();
        }

        void nativeToggleRowSelection(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (UListWindow::Row* entry = list ? list->row(rowId) : nullptr) {
                entry->selected = !entry->selected;
                list->selectedCount();
            }
        }

        void nativeIsRowSelected(UObject* object, Frame& stack, void* result)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            UListWindow::Row* entry = list ? list->row(rowId) : nullptr;
            vm::retBool(result, entry && entry->selected);
        }

        void nativeGetSelectedRow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            int32_t id = 0;
            if (list) {
                int32_t focus = listwindow::focusLine(list);
                if (focus >= 0 && focus < static_cast<int32_t>(list->rows.size()) && list->rows[static_cast<size_t>(focus)].selected) {
                    id = list->rows[static_cast<size_t>(focus)].id;
                } else {
                    for (const UListWindow::Row& entry : list->rows) {
                        if (entry.selected) {
                            id = entry.id;
                            break;
                        }
                    }
                }
            }
            vm::ret<int32_t>(result, id);
        }

        void nativeMoveRow(UObject* object, Frame& stack, void*)
        {
            uint8_t move = vm::get<uint8_t>(stack);
            bool select = optionalBoolDefault(stack, true);
            bool clearRows = optionalBoolDefault(stack, true);
            optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->moveRow(move, select, clearRows);
                list->notifySelection();
            }
        }

        void nativeSetRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            bool select = optionalBoolDefault(stack, true);
            bool clearRows = optionalBoolDefault(stack, true);
            optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->setRow(rowId, select, clearRows);
            }
        }

        void nativeSetFocusRow(UObject* object, Frame& stack, void*)
        {
            int32_t rowId = vm::get<int32_t>(stack);
            bool moveTo = optionalBoolDefault(stack, true);
            bool anchor = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                return;
            }
            int32_t index = list->indexOf(rowId);
            if (index < 0) {
                return;
            }
            list->setFocusLine(index, moveTo);
            if (anchor) {
                listwindow::anchorLine(list) = index;
            }
        }

        void nativeGetFocusRow(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            int32_t id = 0;
            if (list) {
                int32_t focus = listwindow::focusLine(list);
                if (focus >= 0 && focus < static_cast<int32_t>(list->rows.size())) {
                    id = list->rows[static_cast<size_t>(focus)].id;
                }
            }
            vm::ret<int32_t>(result, id);
        }

        void nativeShowFocusRow(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->setFocusLine(listwindow::focusLine(list), true);
            }
        }

        void nativeSetNumColumns(UObject* object, Frame& stack, void*)
        {
            int32_t count = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->columns.resize(static_cast<size_t>(std::max(count, 0)));
                list->askParentForReconfigure();
            }
        }

        void nativeGetNumColumns(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            vm::ret<int32_t>(result, list ? static_cast<int32_t>(list->columns.size()) : 0);
        }

        void nativeResizeColumns(UObject* object, Frame& stack, void*)
        {
            bool expandOnly = optionalBoolDefault(stack, false);
            vm::finish(stack);
            UListWindow* list = self(object);
            if (!list) {
                return;
            }
            if (!expandOnly) {
                for (UListWindow::Column& column : list->columns) {
                    column.width = 0;
                }
            }
            list->autoSize();
            list->askParentForReconfigure();
        }

        void nativeSetColumnTitle(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            std::string title = vm::get<std::string>(stack);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->title = title;
            }
        }

        void nativeGetColumnTitle(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::ret<std::string>(result, column ? column->title : std::string());
        }

        void nativeSetColumnWidth(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            float w = vm::get<float>(stack);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->width = w;
                self(object)->askParentForReconfigure();
            }
        }

        void nativeGetColumnWidth(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::ret<float>(result, column ? column->width : 0.0f);
        }

        void nativeSetColumnAlignment(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            uint8_t align = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->align = align;
            }
        }

        void nativeGetColumnAlignment(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::ret<uint8_t>(result, column ? column->align : static_cast<uint8_t>(0));
        }

        void nativeSetColumnColor(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->color = color;
                column->colorSet = true;
            }
        }

        void nativeGetColumnColor(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            Color* out = vm::getRef<Color>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            *out = column ? column->color : Color { 255, 255, 255, 255 };
        }

        void nativeSetColumnFont(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->font = font;
            }
        }

        void nativeGetColumnFont(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::ret<UObject*>(result, column ? column->font : nullptr);
        }

        void nativeSetColumnType(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            uint8_t type = vm::get<uint8_t>(stack);
            std::string format;
            optional(stack, format);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->type = type;
                column->format = format;
            }
        }

        void nativeGetColumnType(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::ret<uint8_t>(result, column ? column->type : static_cast<uint8_t>(0));
        }

        void nativeHideColumn(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            bool hide = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow::Column* column = columnOf(self(object), index)) {
                column->hidden = hide;
            }
        }

        void nativeIsColumnHidden(UObject* object, Frame& stack, void* result)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            UListWindow::Column* column = columnOf(self(object), index);
            vm::retBool(result, column && column->hidden);
        }

        void readSortKey(Frame& stack, UListWindow::SortKey& key)
        {
            key.column = vm::get<int32_t>(stack);
            key.reverse = optionalBoolDefault(stack, false);
            key.caseSensitive = optionalBoolDefault(stack, false);
            vm::finish(stack);
        }

        void nativeSetSortColumn(UObject* object, Frame& stack, void*)
        {
            UListWindow::SortKey key;
            readSortKey(stack, key);
            if (UListWindow* list = self(object)) {
                list->sortKeys.assign(1, key);
                if (listwindow::bAutoSort(list)) {
                    list->sortRows();
                }
            }
        }

        void nativeAddSortColumn(UObject* object, Frame& stack, void*)
        {
            UListWindow::SortKey key;
            readSortKey(stack, key);
            if (UListWindow* list = self(object)) {
                list->sortKeys.push_back(key);
                if (listwindow::bAutoSort(list)) {
                    list->sortRows();
                }
            }
        }

        void nativeRemoveSortColumn(UObject* object, Frame& stack, void*)
        {
            int32_t index = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->sortKeys.erase(std::remove_if(list->sortKeys.begin(), list->sortKeys.end(), [index](const UListWindow::SortKey& key) {
                    return key.column == index;
                }),
                    list->sortKeys.end());
            }
        }

        void nativeResetSortColumns(UObject* object, Frame& stack, void*)
        {
            bool sort = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->sortKeys.clear();
                if (sort) {
                    std::sort(list->rows.begin(), list->rows.end(), [](const UListWindow::Row& a, const UListWindow::Row& b) {
                        return a.id < b.id;
                    });
                }
            }
        }

        void nativeSort(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->sortRows();
            }
        }

        void nativeEnableHotKeys(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::bHotKeys(list) = value ? 1 : 0;
            }
        }

        void nativeSetHotKeyColumn(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::hotKeyCol(list) = value;
            }
        }

        void listFlag(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                field(list) = value ? 1 : 0;
                if (field == listwindow::bAutoSort && value) {
                    list->sortRows();
                }
            }
        }

        void listFlagGet(UObject* object, Frame& stack, void* result, uint32_t& (*field)(UObject*))
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            vm::retBool(result, list && field(list));
        }

        void nativeEnableAutoSort(UObject* object, Frame& stack, void*)
        {
            listFlag(object, stack, listwindow::bAutoSort);
        }

        void nativeIsAutoSortEnabled(UObject* object, Frame& stack, void* result)
        {
            listFlagGet(object, stack, result, listwindow::bAutoSort);
        }

        void nativeEnableAutoExpandColumns(UObject* object, Frame& stack, void*)
        {
            listFlag(object, stack, listwindow::bAutoExpandColumns);
        }

        void nativeIsAutoExpandColumnsEnabled(UObject* object, Frame& stack, void* result)
        {
            listFlagGet(object, stack, result, listwindow::bAutoExpandColumns);
        }

        void nativeEnableMultiSelect(UObject* object, Frame& stack, void*)
        {
            listFlag(object, stack, listwindow::bMultiSelect);
        }

        void nativeIsMultiSelectEnabled(UObject* object, Frame& stack, void* result)
        {
            listFlagGet(object, stack, result, listwindow::bMultiSelect);
        }

        void nativeSetFieldMargins(UObject* object, Frame& stack, void*)
        {
            float w = vm::get<float>(stack);
            float h = vm::get<float>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->fieldMarginWidth = w;
                list->fieldMarginHeight = h;
                list->askParentForReconfigure();
            }
        }

        void nativeGetFieldMargins(UObject* object, Frame& stack, void*)
        {
            float* w = vm::getRef<float>(stack);
            float* h = vm::getRef<float>(stack);
            vm::finish(stack);
            UListWindow* list = self(object);
            *w = list ? list->fieldMarginWidth : 0.0f;
            *h = list ? list->fieldMarginHeight : 0.0f;
        }

        void nativeGetPageSize(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UListWindow* list = self(object);
            int32_t size = 0;
            if (list) {
                float visible = list->height();
                if (UClipWindow* clip = cast<UClipWindow>(list->parent())) {
                    visible = clip->height();
                }
                size = static_cast<int32_t>(visible / list->lineHeight());
            }
            vm::ret<int32_t>(result, size);
        }

        void nativeSetDelimiter(UObject* object, Frame& stack, void*)
        {
            std::string value = vm::get<std::string>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::Delimiter(list) = value;
            }
        }

        void nativeSetHighlightTextColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                list->highlightTextColor = color;
            }
        }

        void nativeSetHighlightTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::highlightTexture(list) = texture;
            }
        }

        void nativeSetHighlightColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::highlightColor(list) = color;
            }
        }

        void nativeSetFocusTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::focusTexture(list) = texture;
            }
        }

        void nativeSetFocusColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::focusColor(list) = color;
            }
        }

        void nativeSetFocusThickness(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                listwindow::focusThickness(list) = value;
            }
        }

        void nativeSetListSounds(UObject* object, Frame& stack, void*)
        {
            UObject* activate = nullptr;
            UObject* move = nullptr;
            bool hasActivate = optional(stack, activate);
            bool hasMove = optional(stack, move);
            vm::finish(stack);
            if (UListWindow* list = self(object)) {
                if (hasActivate) {
                    listwindow::ActivateSound(list) = activate;
                }
                if (hasMove) {
                    listwindow::moveSound(list) = move;
                }
            }
        }

        void nativePlayListSound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            float volume = 1;
            float pitch = 1;
            optional(stack, volume);
            optional(stack, pitch);
            vm::finish(stack);
            playSound(self(object), sound, volume, pitch);
        }
    }

    void registerListNatives()
    {
        vm::registerNative(1720, nativeIndexToRowId);
        vm::registerNative(1721, nativeRowIdToIndex);
        vm::registerNative(1722, nativeSetRowClientInt);
        vm::registerNative(1723, nativeGetRowClientInt);
        vm::registerNative(1724, nativeSetRowClientObject);
        vm::registerNative(1725, nativeGetRowClientObject);
        vm::registerNative(1730, nativeAddRow);
        vm::registerNative(1731, nativeDeleteRow);
        vm::registerNative(1732, nativeModifyRow);
        vm::registerNative(1733, nativeDeleteAllRows);
        vm::registerNative(1734, nativeSetField);
        vm::registerNative(1735, nativeGetField);
        vm::registerNative(1736, nativeSetFieldValue);
        vm::registerNative(1737, nativeGetFieldValue);
        vm::registerNative(1740, nativeGetNumRows);
        vm::registerNative(1741, nativeGetNumSelectedRows);
        vm::registerNative(1742, nativeSelectRow);
        vm::registerNative(1743, nativeSelectAllRows);
        vm::registerNative(1744, nativeSelectToRow);
        vm::registerNative(1745, nativeToggleRowSelection);
        vm::registerNative(1746, nativeIsRowSelected);
        vm::registerNative(1747, nativeGetSelectedRow);
        vm::registerNative(1750, nativeMoveRow);
        vm::registerNative(1751, nativeSetRow);
        vm::registerNative(1755, nativeSetFocusRow);
        vm::registerNative(1756, nativeGetFocusRow);
        vm::registerNative(1760, nativeSetNumColumns);
        vm::registerNative(1761, nativeGetNumColumns);
        vm::registerNative(1762, nativeResizeColumns);
        vm::registerNative(1765, nativeSetColumnTitle);
        vm::registerNative(1766, nativeGetColumnTitle);
        vm::registerNative(1767, nativeSetColumnWidth);
        vm::registerNative(1768, nativeGetColumnWidth);
        vm::registerNative(1769, nativeSetColumnAlignment);
        vm::registerNative(1770, nativeGetColumnAlignment);
        vm::registerNative(1771, nativeSetColumnColor);
        vm::registerNative(1772, nativeGetColumnColor);
        vm::registerNative(1773, nativeSetColumnFont);
        vm::registerNative(1774, nativeGetColumnFont);
        vm::registerNative(1775, nativeSetColumnType);
        vm::registerNative(1776, nativeGetColumnType);
        vm::registerNative(1777, nativeHideColumn);
        vm::registerNative(1778, nativeIsColumnHidden);
        vm::registerNative(1780, nativeSetSortColumn);
        vm::registerNative(1781, nativeAddSortColumn);
        vm::registerNative(1782, nativeRemoveSortColumn);
        vm::registerNative(1783, nativeResetSortColumns);
        vm::registerNative(1784, nativeSort);
        vm::registerNative(1785, nativeEnableHotKeys);
        vm::registerNative(1786, nativeSetHotKeyColumn);
        vm::registerNative(1790, nativeEnableAutoSort);
        vm::registerNative(1791, nativeIsAutoSortEnabled);
        vm::registerNative(1792, nativeEnableAutoExpandColumns);
        vm::registerNative(1793, nativeIsAutoExpandColumnsEnabled);
        vm::registerNative(1794, nativeEnableMultiSelect);
        vm::registerNative(1795, nativeIsMultiSelectEnabled);
        vm::registerNative(1800, nativeSetFieldMargins);
        vm::registerNative(1801, nativeGetFieldMargins);
        vm::registerNative(1802, nativeGetPageSize);
        vm::registerNative(1803, nativeSetDelimiter);
        vm::registerNative(1804, nativeSetHighlightTextColor);
        vm::registerNative(1805, nativeSetHighlightTexture);
        vm::registerNative(1806, nativeSetHighlightColor);
        vm::registerNative(1807, nativeSetFocusTexture);
        vm::registerNative(1808, nativeSetFocusColor);
        vm::registerNative(1809, nativeSetFocusThickness);
        vm::registerNative(1810, nativeShowFocusRow);
        vm::registerNative(1811, nativeSetListSounds);
        vm::registerNative(1812, nativePlayListSound);
    }
}
