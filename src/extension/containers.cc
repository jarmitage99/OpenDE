#include "extension/windows.h"

#include "core/objects.h"
#include "engine/input.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>

namespace dx::extension {
    namespace {
        struct TileRow {
            int32_t minor = 0;
            int32_t major = 0;
            int32_t count = 0;
        };

        int32_t ftol(float value)
        {
            return static_cast<int32_t>(value);
        }
    }

    void UTileWindow::initWindow()
    {
        tilewindow::hMargin(this) = 3.0f;
        tilewindow::vMargin(this) = 3.0f;
        tilewindow::minorSpacing(this) = 1.0f;
        tilewindow::majorSpacing(this) = 1.0f;
        tilewindow::bWrap(this) = 1;
        tilewindow::bFillParent(this) = 1;
        tilewindow::bEqualWidth(this) = 0;
        tilewindow::bEqualHeight(this) = 1;
        tilewindow::orientation(this) = 0;
        tilewindow::hDirection(this) = 0;
        tilewindow::vDirection(this) = 0;
        tilewindow::hChildAlign(this) = 3;
        tilewindow::vChildAlign(this) = 3;
        UWindow::initWindow();
    }

    void UTileWindow::computeChildSizes(bool widthSpecified, float& w, bool heightSpecified, float& h)
    {
        bool vertical = tilewindow::orientation(this) == 1;
        float hMargin = tilewindow::hMargin(this);
        float vMargin = tilewindow::vMargin(this);
        float minorSpacing = tilewindow::minorSpacing(this);
        float majorSpacing = tilewindow::majorSpacing(this);
        bool wrap = tilewindow::bWrap(this) != 0;
        bool equalWidth = tilewindow::bEqualWidth(this) != 0;
        bool equalHeight = tilewindow::bEqualHeight(this) != 0;

        float constraint = 0;
        bool useWidth = false;
        bool useHeight = false;
        if (!wrap && tilewindow::bFillParent(this)) {
            if (vertical && widthSpecified) {
                useWidth = true;
                constraint = std::max(w - 2.0f * hMargin, 0.0f);
            } else if (!vertical && heightSpecified) {
                useHeight = true;
                constraint = std::max(h - 2.0f * vMargin, 0.0f);
            }
        }

        holds.clear();
        float maxWidth = 0;
        float maxHeight = 0;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (!child->visible()) {
                continue;
            }
            Hold hold;
            hold.window = child;
            if (useWidth) {
                hold.width = constraint;
                hold.height = child->queryPreferredHeight(constraint);
            } else if (useHeight) {
                hold.height = constraint;
                hold.width = child->queryPreferredWidth(constraint);
            } else {
                child->queryPreferredSize(hold.width, hold.height);
            }
            if (equalWidth) {
                maxWidth = std::max(maxWidth, hold.width);
            }
            if (equalHeight) {
                maxHeight = std::max(maxHeight, hold.height);
            }
            holds.push_back(hold);
        }
        for (Hold& hold : holds) {
            if (equalWidth) {
                hold.width = maxWidth;
            }
            if (equalHeight) {
                hold.height = maxHeight;
            }
        }

        float limit = 1000000.0f;
        if (wrap) {
            if (widthSpecified && !vertical) {
                limit = w - 2.0f * hMargin;
            }
            if (heightSpecified && vertical) {
                limit = h - 2.0f * vMargin;
            }
        }

        std::vector<TileRow> rows;
        TileRow row;
        row.minor = ftol(constraint);
        for (const Hold& hold : holds) {
            float major = vertical ? hold.height : hold.width;
            float minor = vertical ? hold.width : hold.height;
            if (row.count > 0 && !(static_cast<float>(row.major) + major <= limit)) {
                row.major = ftol(static_cast<float>(row.major) - minorSpacing);
                rows.push_back(row);
                row = TileRow();
                row.minor = ftol(constraint);
            }
            row.major = ftol(static_cast<float>(row.major) + minorSpacing + major);
            if (static_cast<float>(row.minor) < minor) {
                row.minor = ftol(minor);
            }
            row.count++;
        }
        if (row.count > 0) {
            row.major = ftol(static_cast<float>(row.major) - minorSpacing);
            rows.push_back(row);
        }

        if (widthSpecified && heightSpecified) {
            uint8_t hAlign = tilewindow::hChildAlign(this);
            uint8_t vAlign = tilewindow::vChildAlign(this);
            bool hReverse = tilewindow::hDirection(this) == 1;
            bool vReverse = tilewindow::vDirection(this) == 1;
            float x = hMargin;
            float y = vMargin;
            size_t r = 0;
            for (Hold& hold : holds) {
                float rowMinor = static_cast<float>(rows[r].minor);
                if (vertical) {
                    hold.y = vReverse ? h - (y + hold.height) : y;
                    hold.x = hReverse ? w - (rowMinor + x) : x;
                    if (hAlign == HALIGN_Center) {
                        hold.x += static_cast<float>(ftol(rowMinor - hold.width) / 2);
                    } else if (hAlign == HALIGN_Right) {
                        hold.x += rowMinor - hold.width;
                    } else if (hAlign == HALIGN_Full) {
                        hold.width = rowMinor;
                    }
                    y = minorSpacing + hold.height + y;
                    if (--rows[r].count < 1) {
                        x = rowMinor + majorSpacing + x;
                        y = vMargin;
                        r++;
                    }
                } else {
                    hold.y = vReverse ? h - (rowMinor + y) : y;
                    hold.x = hReverse ? w - (x + hold.width) : x;
                    if (vAlign == VALIGN_Center) {
                        hold.y += static_cast<float>(ftol(rowMinor - hold.height) / 2);
                    } else if (vAlign == VALIGN_Bottom) {
                        hold.y += rowMinor - hold.height;
                    } else if (vAlign == VALIGN_Full) {
                        hold.height = rowMinor;
                    }
                    x = minorSpacing + hold.width + x;
                    if (--rows[r].count < 1) {
                        y = rowMinor + majorSpacing + y;
                        x = hMargin;
                        r++;
                    }
                }
            }
            return;
        }

        float maxMajor = 0;
        float sumMinor = 0;
        for (const TileRow& entry : rows) {
            maxMajor = std::max(maxMajor, static_cast<float>(entry.major));
            sumMinor += static_cast<float>(entry.minor);
        }
        if (!rows.empty()) {
            sumMinor += majorSpacing * static_cast<float>(rows.size() - 1);
        }
        if (!widthSpecified) {
            w = 2.0f * hMargin + (vertical ? sumMinor : maxMajor);
        }
        if (!heightSpecified) {
            h = 2.0f * vMargin + (vertical ? maxMajor : sumMinor);
        }
    }

    void UTileWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        computeChildSizes(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UTileWindow::parentRequestedGranularity(float& hGranularity, float& vGranularity)
    {
        UWindow::parentRequestedGranularity(hGranularity, vGranularity);
        UWindow* last = nullptr;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (child->visible()) {
                last = child;
            }
        }
        if (!last) {
            return;
        }
        bool vertical = tilewindow::orientation(this) == 1;
        if (tilewindow::bEqualWidth(this)) {
            hGranularity = (vertical ? tilewindow::majorSpacing(this) : tilewindow::minorSpacing(this)) + last->width();
        }
        if (tilewindow::bEqualHeight(this)) {
            vGranularity = (vertical ? tilewindow::minorSpacing(this) : tilewindow::majorSpacing(this)) + last->height();
        }
    }

    void UTileWindow::configurationChanged()
    {
        float w = width();
        float h = height();
        computeChildSizes(true, w, true, h);
        std::vector<Hold> placed = holds;
        for (const Hold& hold : placed) {
            hold.window->configureChild(hold.x, hold.y, hold.width, hold.height);
        }
    }

    bool UTileWindow::childRequestedReconfiguration(UWindow*)
    {
        return false;
    }

    void UBorderWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        float left = borderwindow::childLeftMargin(this);
        float right = borderwindow::childRightMargin(this);
        float top = borderwindow::childTopMargin(this);
        float bottom = borderwindow::childBottomMargin(this);
        float w = 0;
        float h = 0;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (!child->visible()) {
                continue;
            }
            float cw = 0;
            float ch = 0;
            if (widthSpecified) {
                cw = std::max(preferredWidth - left - right, 0.0f);
                ch = child->queryPreferredHeight(cw);
            } else {
                child->queryPreferredSize(cw, ch);
            }
            w = std::max(w, cw);
            h = std::max(h, ch);
        }
        if (!widthSpecified) {
            preferredWidth = w + left + right;
        }
        if (!heightSpecified) {
            preferredHeight = h + top + bottom;
        }
        UWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UBorderWindow::configurationChanged()
    {
        float left = borderwindow::childLeftMargin(this);
        float right = borderwindow::childRightMargin(this);
        float top = borderwindow::childTopMargin(this);
        float bottom = borderwindow::childBottomMargin(this);
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (child->visible()) {
                child->configureChild(left, top, std::max(width() - left - right, 0.0f), std::max(height() - top - bottom, 0.0f));
            }
        }
        UWindow::configurationChanged();
    }

    bool UBorderWindow::childRequestedReconfiguration(UWindow* child)
    {
        window::bNeedsReconfigure(this) = 1;
        return UWindow::childRequestedReconfiguration(child);
    }

    void UBorderWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        UObject* borders[9] = {
            borderwindow::borderTopLeft(this),
            borderwindow::borderTopRight(this),
            borderwindow::borderBottomLeft(this),
            borderwindow::borderBottomRight(this),
            borderwindow::borderLeft(this),
            borderwindow::borderRight(this),
            borderwindow::borderTop(this),
            borderwindow::borderBottom(this),
            borderwindow::center(this),
        };
        bool any = false;
        for (UObject* border : borders) {
            any = any || border != nullptr;
        }
        if (any) {
            gc->setStyle(borderwindow::borderStyle(this) ? borderwindow::borderStyle(this) : DSTY_Normal);
            gc::bSmoothed(gc) = borderwindow::bSmoothBorder(this);
            gc::tileColor(gc) = window::tileColor(this);
            gc->drawBorders(0, 0, width(), height(), borders);
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        scriptDrawWindow(gc);
    }

    namespace {
        struct ClipHold {
            float x = 0;
            float y = 0;
            float w = 0;
            float h = 0;
        };

        void clampChildPosition(UClipWindow* clip, float& x, float& y, float w, float h)
        {
            x = std::max(x, clip->width() - w);
            if (x > 0) {
                x = 0;
            }
            y = std::max(y, clip->height() - h);
            if (y > 0) {
                y = 0;
            }
        }

        ClipHold childPreferred(UClipWindow* clip, UWindow* content, bool widthSpecified, float w, bool heightSpecified, float h)
        {
            ClipHold hold;
            bool forceW = widthSpecified && clipwindow::bForceChildWidth(clip);
            bool forceH = heightSpecified && clipwindow::bForceChildHeight(clip);
            if (forceW) {
                hold.w = w;
                if (forceH) {
                    hold.h = h;
                } else {
                    hold.y = static_cast<float>(clipwindow::childV(clip));
                    hold.h = content->queryPreferredHeight(w);
                }
            } else {
                hold.x = static_cast<float>(clipwindow::childH(clip));
                if (forceH) {
                    hold.w = content->queryPreferredWidth(h);
                    hold.h = h;
                } else {
                    hold.y = static_cast<float>(clipwindow::childV(clip));
                    content->queryPreferredSize(hold.w, hold.h);
                }
            }
            return hold;
        }
    }

    void UClipWindow::initWindow()
    {
        clipwindow::prefHUnits(this) = -1;
        clipwindow::prefVUnits(this) = -1;
        clipwindow::hMult(this) = 1.0f;
        clipwindow::vMult(this) = 1.0f;
        clipwindow::bSnapToUnits(this) = 1;
        clipwindow::bFillWindow(this) = 1;
        UTabGroupWindow::initWindow();
    }

    UWindow* UClipWindow::child()
    {
        for (UWindow* current = lastChild(); current; current = current->prevSibling()) {
            if (!current->destroyed && current->visible()) {
                return current;
            }
        }
        return nullptr;
    }

    void UClipWindow::childUnits(UWindow* content, float& hUnit, float& vUnit)
    {
        hUnit = 1.0f;
        vUnit = 1.0f;
        if (clipwindow::bSnapToUnits(this) && content) {
            content->queryGranularity(hUnit, vUnit);
        }
    }

    void UClipWindow::clipPreferredSize(UWindow* content, bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        if (!content) {
            UTabGroupWindow::parentRequestedPreferredSize(widthSpecified, preferredWidth, heightSpecified, preferredHeight);
            return;
        }
        float hUnit = 1;
        float vUnit = 1;
        childUnits(content, hUnit, vUnit);
        if (clipwindow::prefHUnits(this) >= 0 && !widthSpecified) {
            preferredWidth = static_cast<float>(clipwindow::prefHUnits(this)) * hUnit;
            widthSpecified = true;
        }
        if (clipwindow::prefVUnits(this) >= 0 && !heightSpecified) {
            preferredHeight = static_cast<float>(clipwindow::prefVUnits(this)) * vUnit;
            heightSpecified = true;
        }
        ClipHold hold = childPreferred(this, content, widthSpecified, preferredWidth, heightSpecified, preferredHeight);
        if (!widthSpecified) {
            preferredWidth = hold.w;
        }
        if (!heightSpecified) {
            preferredHeight = hold.h;
        }
    }

    void UClipWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        clipPreferredSize(child(), widthSpecified, preferredWidth, heightSpecified, preferredHeight);
    }

    void UClipWindow::queryClipPreferredSize(bool widthSpecified, float w, float& outWidth, bool& needH, bool heightSpecified, float h, float& outHeight, bool& needV)
    {
        if (!widthSpecified && fixedWidth) {
            widthSpecified = true;
            w = window::hardcodedWidth(this);
        }
        if (!heightSpecified && fixedHeight) {
            heightSpecified = true;
            h = window::hardcodedHeight(this);
        }
        float tw = widthSpecified ? w : 10.0f;
        float th = heightSpecified ? h : 10.0f;
        UWindow* content = child();
        clipPreferredSize(content, widthSpecified, tw, heightSpecified, th);
        needH = false;
        needV = false;
        if (content) {
            ClipHold hold = childPreferred(this, content, widthSpecified, tw, heightSpecified, th);
            needH = tw < hold.w;
            needV = th < hold.h;
        }
        outWidth = widthSpecified ? w : tw;
        outHeight = heightSpecified ? h : th;
    }

    void UClipWindow::reconfigureChild(UWindow* content, int32_t col, int32_t row, float w, float h)
    {
        float hUnit = 1;
        float vUnit = 1;
        childUnits(content, hUnit, vUnit);
        clipwindow::hMult(this) = hUnit;
        clipwindow::vMult(this) = vUnit;
        float x = -hUnit * static_cast<float>(col);
        float y = -vUnit * static_cast<float>(row);
        int32_t childHUnits = static_cast<int32_t>(std::ceil(w / hUnit));
        int32_t childVUnits = static_cast<int32_t>(std::ceil(h / vUnit));
        if (clipwindow::bFillWindow(this)) {
            w = std::max(w, width());
            h = std::max(h, height());
        }
        int32_t areaHUnits = childHUnits - static_cast<int32_t>(std::ceil((w - width()) / hUnit));
        int32_t areaVUnits = childVUnits - static_cast<int32_t>(std::ceil((h - height()) / vUnit));
        clampChildPosition(this, x, y, w, h);
        bool changed = clipwindow::childHSize(this) != childHUnits || clipwindow::childVSize(this) != childVUnits || clipwindow::areaHSize(this) != areaHUnits || clipwindow::areaVSize(this) != areaVUnits;
        content->configureChild(x, y, w, h);
        clipwindow::childHSize(this) = childHUnits;
        clipwindow::childVSize(this) = childVUnits;
        clipwindow::areaHSize(this) = areaHUnits;
        clipwindow::areaVSize(this) = areaVUnits;
        if (changed) {
            for (UWindow* current = this; current; current = current->parent()) {
                if (current->clipAttributesChanged(this, areaHUnits, areaVUnits, childHUnits, childVUnits)) {
                    break;
                }
            }
        }
    }

    void UClipWindow::configurationChanged()
    {
        UWindow* content = child();
        for (UWindow* current = lastChild(); current; current = current->prevSibling()) {
            if (current->destroyed || !current->visible()) {
                continue;
            }
            if (current == content) {
                ClipHold hold = childPreferred(this, current, true, width(), true, height());
                reconfigureChild(current, static_cast<int32_t>(std::trunc(hold.x)), static_cast<int32_t>(std::trunc(hold.y)), hold.w, hold.h);
            } else {
                current->configureChild(0, 0, 0, 0);
            }
        }
    }

    bool UClipWindow::childRequestedReconfiguration(UWindow*)
    {
        window::bNeedsReconfigure(this) = 1;
        return false;
    }

    void UClipWindow::setChildPosition(int32_t col, int32_t row)
    {
        if (clipwindow::childH(this) == col && clipwindow::childV(this) == row) {
            return;
        }
        clipwindow::childH(this) = col;
        clipwindow::childV(this) = row;
        UWindow* content = child();
        if (!content) {
            return;
        }
        float x = clipwindow::bForceChildWidth(this) ? 0.0f : -clipwindow::hMult(this) * static_cast<float>(col);
        float y = clipwindow::bForceChildHeight(this) ? 0.0f : -clipwindow::vMult(this) * static_cast<float>(row);
        clampChildPosition(this, x, y, content->width(), content->height());
        content->configureChild(x, y, content->width(), content->height());
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->clipPositionChanged(this, col, row)) {
                break;
            }
        }
    }

    void UClipWindow::childRequestedShowArea(UWindow* content, float showX, float showY, float showWidth, float showHeight)
    {
        float hUnit = 1;
        float vUnit = 1;
        childUnits(content, hUnit, vUnit);
        if (hUnit <= 0 || vUnit <= 0) {
            return;
        }
        showX += 0.005f;
        showY += 0.005f;
        showWidth -= 0.01f;
        showHeight -= 0.01f;
        int32_t left = static_cast<int32_t>(std::nearbyint(showX / hUnit - 0.5f));
        int32_t right = static_cast<int32_t>(std::ceil((showX + showWidth) / hUnit));
        int32_t top = static_cast<int32_t>(std::nearbyint(showY / vUnit - 0.5f));
        int32_t bottom = static_cast<int32_t>(std::ceil((showY + showHeight) / vUnit));
        int32_t col = clipwindow::childH(this);
        int32_t row = clipwindow::childV(this);
        int32_t areaH = clipwindow::areaHSize(this);
        int32_t areaV = clipwindow::areaVSize(this);
        int32_t spanH = right - left;
        int32_t spanV = bottom - top;
        int32_t deltaL = left - col;
        int32_t deltaT = top - row;
        if (areaH >= spanH) {
            if (deltaL > areaH - spanH) {
                col = right - areaH;
            } else if (deltaL < 0) {
                col = left;
            }
        } else {
            if (deltaL < areaH - spanH) {
                col = right - areaH;
            } else if (deltaL > 0) {
                col = left;
            }
        }
        if (areaV >= spanV) {
            if (deltaT > areaV - spanV) {
                row = bottom - areaV;
            } else if (deltaT < 0) {
                row = top;
            }
        } else {
            if (deltaT < areaV - spanV) {
                row = bottom - areaV;
            } else if (deltaT > 0) {
                row = top;
            }
        }
        setChildPosition(col, row);
    }

    void URadioBoxWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        float maxWidth = 0;
        float maxHeight = 0;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (!child->visible()) {
                continue;
            }
            float w = preferredWidth;
            float h = preferredHeight;
            if (!widthSpecified && !heightSpecified) {
                child->queryPreferredSize(w, h);
            } else if (!heightSpecified) {
                h = child->queryPreferredHeight(w);
            } else if (!widthSpecified) {
                w = child->queryPreferredWidth(h);
            }
            maxWidth = std::max(maxWidth, w);
            maxHeight = std::max(maxHeight, h);
        }
        preferredWidth = maxWidth;
        preferredHeight = maxHeight;
    }

    void URadioBoxWindow::configurationChanged()
    {
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (child->visible()) {
                child->configureChild(0, 0, width(), height());
            }
        }
    }

    bool URadioBoxWindow::toggleChanged(UWindow* button, bool newToggle)
    {
        UToggleWindow* toggle = cast<UToggleWindow>(button);
        if (toggle && newToggle) {
            radioboxwindow::currentSelection(this) = toggle;
            std::vector<UWindow*> pending { this };
            while (!pending.empty()) {
                UWindow* current = pending.back();
                pending.pop_back();
                for (UWindow* child = current->firstChild(); child; child = child->nextSibling()) {
                    UToggleWindow* other = cast<UToggleWindow>(child);
                    if (other && other != toggle && other->toggled) {
                        other->toggled = false;
                    }
                    pending.push_back(child);
                }
            }
            UWindow::toggleChanged(button, newToggle);
            return true;
        }
        if (toggle && !newToggle && radioboxwindow::currentSelection(this) == toggle && radioboxwindow::bOneCheck(this)) {
            toggle->toggled = true;
            return true;
        }
        return UWindow::toggleChanged(button, newToggle);
    }

    void UScrollAreaWindow::initWindow()
    {
        UClass* clipClass = extensionClass("Extension.ClipWindow");
        UClass* managerClass = extensionClass("Extension.ScaleManagerWindow");
        UClass* scaleClass = extensionClass("Extension.ScaleWindow");
        UClass* buttonClass = extensionClass("Extension.ButtonWindow");
        scrollareawindow::marginWidth(this) = 3.0f;
        scrollareawindow::marginHeight(this) = 3.0f;
        scrollareawindow::scrollbarDistance(this) = 3.0f;
        scrollareawindow::bHideScrollbars(this) = 1;
        scrollareawindow::bHLastShow(this) = 0;
        scrollareawindow::bVLastShow(this) = 0;
        UWindow* hManager = newChild(managerClass, false);
        UWindow* vManager = newChild(managerClass, false);
        UWindow* clip = newChild(clipClass, true);
        UWindow* left = hManager ? hManager->newChild(buttonClass, true) : nullptr;
        UScaleWindow* hScale = hManager ? cast<UScaleWindow>(hManager->newChild(scaleClass, true)) : nullptr;
        UWindow* right = hManager ? hManager->newChild(buttonClass, true) : nullptr;
        UWindow* up = vManager ? vManager->newChild(buttonClass, true) : nullptr;
        UScaleWindow* vScale = vManager ? cast<UScaleWindow>(vManager->newChild(scaleClass, true)) : nullptr;
        UWindow* down = vManager ? vManager->newChild(buttonClass, true) : nullptr;
        scrollareawindow::ClipWindow(this) = clip;
        scrollareawindow::hScaleMgr(this) = hManager;
        scrollareawindow::vScaleMgr(this) = vManager;
        scrollareawindow::hScale(this) = hScale;
        scrollareawindow::vScale(this) = vScale;
        scrollareawindow::LeftButton(this) = left;
        scrollareawindow::RightButton(this) = right;
        scrollareawindow::UpButton(this) = up;
        scrollareawindow::DownButton(this) = down;
        if (hManager) {
            scalemanagerwindow::orientation(hManager) = 0;
            scalemanagerwindow::decButton(hManager) = left;
            scalemanagerwindow::incButton(hManager) = right;
            scalemanagerwindow::Scale(hManager) = hScale;
            scalemanagerwindow::bStretchScaleField(hManager) = 1;
        }
        if (vManager) {
            scalemanagerwindow::orientation(vManager) = 1;
            scalemanagerwindow::decButton(vManager) = up;
            scalemanagerwindow::incButton(vManager) = down;
            scalemanagerwindow::Scale(vManager) = vScale;
            scalemanagerwindow::bStretchScaleField(vManager) = 1;
        }
        uint8_t orientation = 0;
        for (UScaleWindow* scale : { hScale, vScale }) {
            if (scale) {
                scalewindow::orientation(scale) = orientation;
                window::bIsSelectable(scale) = 0;
                scale->setRanges(1, 1);
                scalewindow::bSpanThumb(scale) = 1;
                scalewindow::bRepeatThumbTexture(scale) = 1;
                scalewindow::bStretchScale(scale) = 1;
                scalewindow::bRepeatScaleTexture(scale) = 1;
            }
            orientation = 1;
        }
        for (UWindow* button : { left, right, up, down }) {
            if (button) {
                window::bIsSelectable(button) = 0;
                buttonwindow::bAutoRepeat(button) = 1;
                buttonwindow::initialDelay(button) = 0.5f;
                buttonwindow::repeatRate(button) = 0.1f;
            }
        }
        enableScrolling(false, true);
        window::bIsSelectable(this) = 0;
        UWindow::initWindow();
    }

    void UScrollAreaWindow::enableScrolling(bool horizontal, bool vertical)
    {
        if (UWindow* hManager = cast<UWindow>(scrollareawindow::hScaleMgr(this))) {
            hManager->show(horizontal);
        }
        if (UWindow* vManager = cast<UWindow>(scrollareawindow::vScaleMgr(this))) {
            vManager->show(vertical);
        }
        if (UClipWindow* clip = cast<UClipWindow>(scrollareawindow::ClipWindow(this))) {
            bool forceWidth = !horizontal;
            bool forceHeight = !vertical;
            if ((clipwindow::bForceChildWidth(clip) != 0) != forceWidth || (clipwindow::bForceChildHeight(clip) != 0) != forceHeight) {
                clipwindow::bForceChildWidth(clip) = forceWidth ? 1 : 0;
                clipwindow::bForceChildHeight(clip) = forceHeight ? 1 : 0;
                clip->askParentForReconfigure();
                window::bNeedsReconfigure(clip) = 1;
            }
        }
    }

    void UScrollAreaWindow::computeChildSizes(bool widthSpecified, float w, float* outWidth, bool heightSpecified, float h, float* outHeight, bool configure)
    {
        UWindow* hManager = cast<UWindow>(scrollareawindow::hScaleMgr(this));
        UWindow* vManager = cast<UWindow>(scrollareawindow::vScaleMgr(this));
        if (hManager && !hManager->visible()) {
            hManager = nullptr;
        }
        if (vManager && !vManager->visible()) {
            vManager = nullptr;
        }
        UClipWindow* clip = cast<UClipWindow>(scrollareawindow::ClipWindow(this));
        if (!clip || !clip->visible()) {
            if (configure && hManager) {
                hManager->configureChild(0, 0, 0, 0);
            }
            if (configure && vManager) {
                vManager->configureChild(0, 0, 0, 0);
            }
            if (outWidth) {
                *outWidth = 10.0f;
            }
            if (outHeight) {
                *outHeight = 10.0f;
            }
            return;
        }
        float distance = scrollareawindow::scrollbarDistance(this);
        float marginW = scrollareawindow::marginWidth(this);
        float marginH = scrollareawindow::marginHeight(this);
        float hBarHeight = 0;
        float vBarWidth = 0;
        float hExtra = 0;
        float vExtra = 0;
        bool showH = false;
        bool showV = false;
        bool autoH = false;
        bool autoV = false;
        if (hManager) {
            float unused = 0;
            hManager->queryPreferredSize(unused, hBarHeight);
            hExtra = hBarHeight + distance;
            showH = true;
        }
        if (vManager) {
            float unused = 0;
            vManager->queryPreferredSize(vBarWidth, unused);
            vExtra = vBarWidth + distance;
            showV = true;
        }
        if (scrollareawindow::bHideScrollbars(this)) {
            if (hManager) {
                autoH = true;
                showH = scrollareawindow::bHLastShow(this) != 0;
            }
            if (vManager) {
                autoV = true;
                showV = scrollareawindow::bVLastShow(this) != 0;
            }
        }
        float availW = widthSpecified ? w - marginW * 2.0f : 0.0f;
        float availH = heightSpecified ? h - marginH * 2.0f : 0.0f;
        float clipW = 0;
        float clipH = 0;
        int32_t maxIter = (autoH ? 2 : 1) * (autoV ? 2 : 1);
        for (int32_t iter = 1;; iter++) {
            if (iter > maxIter) {
                if (autoH) {
                    showH = true;
                }
                if (autoV) {
                    showV = true;
                }
            }
            float hSub = (showH && heightSpecified) ? hExtra : 0.0f;
            float vSub = (showV && widthSpecified) ? vExtra : 0.0f;
            bool needH = false;
            bool needV = false;
            clip->queryClipPreferredSize(widthSpecified, availW - vSub, clipW, needH, heightSpecified, availH - hSub, clipH, needV);
            if (iter > maxIter) {
                break;
            }
            if (autoH && needH && !showH) {
                showH = true;
                continue;
            }
            if (autoV && needV && !showV) {
                showV = true;
                continue;
            }
            if (autoH && !needH && showH) {
                showH = false;
                continue;
            }
            if (autoV && !needV && showV) {
                showV = false;
                continue;
            }
            break;
        }
        scrollareawindow::bHLastShow(this) = showH ? 1 : 0;
        scrollareawindow::bVLastShow(this) = showV ? 1 : 0;
        if (configure) {
            if (hManager) {
                hManager->configureChild(marginW, distance + clipH + marginH, clipW, showH ? hBarHeight : 0.0f);
            }
            if (vManager) {
                vManager->configureChild(distance + clipW + marginW, marginH, showV ? vBarWidth : 0.0f, clipH);
            }
            clip->configureChild(marginW, marginH, clipW, clipH);
        }
        if (outWidth) {
            *outWidth = marginW * 2.0f + clipW + (showV ? vExtra : 0.0f);
        }
        if (outHeight) {
            *outHeight = marginW * 2.0f + clipH + (showH ? hExtra : 0.0f);
        }
    }

    void UScrollAreaWindow::configurationChanged()
    {
        computeChildSizes(true, width(), nullptr, true, height(), nullptr, true);
    }

    void UScrollAreaWindow::parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight)
    {
        computeChildSizes(widthSpecified, preferredWidth, &preferredWidth, heightSpecified, preferredHeight, &preferredHeight, false);
    }

    bool UScrollAreaWindow::childRequestedReconfiguration(UWindow*)
    {
        window::bNeedsReconfigure(this) = 1;
        return false;
    }

    bool UScrollAreaWindow::scaleRangeChanged(UWindow* scale, int32_t fromTick, int32_t toTick, float fromValue, float toValue, bool final)
    {
        bool handled = UWindow::scaleRangeChanged(scale, fromTick, toTick, fromValue, toValue, final);
        if (UClipWindow* clip = cast<UClipWindow>(scrollareawindow::ClipWindow(this))) {
            int32_t col = clipwindow::childH(clip);
            int32_t row = clipwindow::childV(clip);
            if (scale == scrollareawindow::hScale(this)) {
                col = fromTick;
                handled = true;
            } else if (scale == scrollareawindow::vScale(this)) {
                row = fromTick;
                handled = true;
            }
            clip->setChildPosition(col, row);
        }
        return handled;
    }

    bool UScrollAreaWindow::clipAttributesChanged(UWindow* clip, int32_t newClipWidth, int32_t newClipHeight, int32_t newChildWidth, int32_t newChildHeight)
    {
        bool handled = UWindow::clipAttributesChanged(clip, newClipWidth, newClipHeight, newChildWidth, newChildHeight);
        if (clip != scrollareawindow::ClipWindow(this)) {
            return handled;
        }
        if (UScaleWindow* hScale = cast<UScaleWindow>(scrollareawindow::hScale(this))) {
            hScale->setRanges(newClipWidth, std::max(newChildWidth, 1));
        }
        if (UScaleWindow* vScale = cast<UScaleWindow>(scrollareawindow::vScale(this))) {
            vScale->setRanges(newClipHeight, std::max(newChildHeight, 1));
        }
        return true;
    }

    bool UScrollAreaWindow::clipPositionChanged(UWindow* clip, int32_t newCol, int32_t newRow)
    {
        bool handled = UWindow::clipPositionChanged(clip, newCol, newRow);
        if (clip != scrollareawindow::ClipWindow(this)) {
            return handled;
        }
        if (UScaleWindow* hScale = cast<UScaleWindow>(scrollareawindow::hScale(this))) {
            hScale->changeThumbPosition(newCol, false, true);
        }
        if (UScaleWindow* vScale = cast<UScaleWindow>(scrollareawindow::vScale(this))) {
            vScale->changeThumbPosition(newRow, false, true);
        }
        return true;
    }

    bool UScrollAreaWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        bool handled = UWindow::mouseButtonPressed(pointX, pointY, button, numClicks);
        if (button == IK_MouseWheelUp || button == IK_MouseWheelDown) {
            UScaleWindow* vScale = cast<UScaleWindow>(scrollareawindow::vScale(this));
            bool shown = vScale != nullptr;
            for (UWindow* current = vScale; current; current = current->parent()) {
                if (!current->visible()) {
                    shown = false;
                    break;
                }
            }
            if (shown) {
                vScale->moveThumb(button == IK_MouseWheelUp ? 4 : 5);
            }
            return true;
        }
        return handled;
    }

    void UViewportWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        State& s = state();
        bool drawn = false;
        if (viewportwindow::bEnableViewport(this) && s.viewportDrawer && s.canvas) {
            ViewportRequest request;
            float scale = virtualScale();
            request.x = gc->offsetX * scale;
            request.y = gc->offsetY * scale;
            request.width = width() * scale;
            request.height = height() * scale;
            request.fov = viewportwindow::FOV(this) > 0 ? viewportwindow::FOV(this) : 90.0f;
            request.originActor = viewportwindow::originActor(this);
            request.watchActor = viewportwindow::watchActor(this);
            request.showActor = viewportwindow::bShowActor(this) != 0;
            request.showWeapons = viewportwindow::bShowWeapons(this) != 0;
            request.clearZ = viewportwindow::bClearZ(this) != 0;
            Vector location = viewportwindow::Location(this);
            Rotator rotation = viewportwindow::Rotation(this);
            UObject* origin = request.originActor;
            if (origin && !origin->destroyed && origin->data) {
                location = actor::Location(origin);
                if (viewportwindow::bUseEyeHeight(this) && origin->isA(Name("Pawn"))) {
                    location.z += pawn::BaseEyeHeight(origin);
                }
                rotation = viewportwindow::bUseViewRotation(this) && origin->isA(Name("Pawn")) ? pawn::ViewRotation(origin) : actor::Rotation(origin);
                location = location + viewportwindow::relLocation(this);
                Rotator rel = viewportwindow::relRotation(this);
                rotation.pitch += rel.pitch;
                rotation.yaw += rel.yaw;
                rotation.roll += rel.roll;
            }
            UObject* watch = request.watchActor;
            if (watch && !watch->destroyed && watch->data) {
                Vector target = actor::Location(watch);
                rotation = vectorRotation(target - location);
            }
            request.location = location;
            request.rotation = rotation;
            viewportwindow::lastLocation(this) = location;
            viewportwindow::lastRotation(this) = rotation;
            s.viewportDrawer(*s.canvas, request);
            drawn = true;
        }
        UObject* fallback = viewportwindow::DefaultTexture(this);
        if (!drawn && fallback) {
            gc->setStyle(DSTY_Normal);
            Color color = viewportwindow::DefaultColor(this);
            gc::tileColor(gc) = (color.r || color.g || color.b) ? color : Color { 255, 255, 255, 255 };
            gc->drawStretched(0, 0, width(), height(), 0, 0, textureWidth(fallback), textureHeight(fallback), fallback);
            gc->resetState(this, gc->offsetX, gc->offsetY);
        }
        scriptDrawWindow(gc);
    }

    namespace {
        UTileWindow* tileSelf(UObject* object)
        {
            return cast<UTileWindow>(object);
        }

        void tileByte(UObject* object, Frame& stack, uint8_t& (*field)(UObject*))
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                field(tile) = value;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void tileFloat(UObject* object, Frame& stack, float& (*field)(UObject*))
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                field(tile) = value;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void tileBool(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                field(tile) = value ? 1 : 0;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void nativeTileSetMargins(UObject* object, Frame& stack, void*)
        {
            float h = vm::get<float>(stack);
            float v = vm::get<float>(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                tilewindow::hMargin(tile) = h;
                tilewindow::vMargin(tile) = v;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void nativeTileSetOrientation(UObject* object, Frame& stack, void*)
        {
            tileByte(object, stack, tilewindow::orientation);
        }

        void nativeTileSetDirections(UObject* object, Frame& stack, void*)
        {
            uint8_t h = vm::get<uint8_t>(stack);
            uint8_t v = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                tilewindow::hDirection(tile) = h;
                tilewindow::vDirection(tile) = v;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void nativeTileSetOrder(UObject* object, Frame& stack, void*)
        {
            uint8_t order = vm::get<uint8_t>(stack);
            vm::finish(stack);
            UTileWindow* tile = tileSelf(object);
            if (!tile) {
                return;
            }
            static const uint8_t table[12][4] = {
                { 0, 0, 0, 0 },
                { 0, 1, 0, 0 },
                { 1, 0, 0, 0 },
                { 1, 0, 1, 0 },
                { 0, 0, 0, 1 },
                { 0, 0, 1, 1 },
                { 0, 1, 0, 1 },
                { 0, 1, 1, 1 },
                { 1, 0, 0, 1 },
                { 1, 1, 0, 1 },
                { 1, 0, 1, 1 },
                { 1, 1, 1, 1 },
            };
            uint8_t index = order < 12 ? order : 4;
            tilewindow::orientation(tile) = table[index][0];
            tilewindow::hDirection(tile) = table[index][1];
            tilewindow::vDirection(tile) = table[index][2];
            tilewindow::bWrap(tile) = table[index][3];
            tile->askParentForReconfigure();
            window::bNeedsReconfigure(tile) = 1;
        }

        void nativeTileSetMinorSpacing(UObject* object, Frame& stack, void*)
        {
            tileFloat(object, stack, tilewindow::minorSpacing);
        }

        void nativeTileSetMajorSpacing(UObject* object, Frame& stack, void*)
        {
            tileFloat(object, stack, tilewindow::majorSpacing);
        }

        void nativeTileSetChildAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t h = vm::get<uint8_t>(stack);
            uint8_t v = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UTileWindow* tile = tileSelf(object)) {
                tilewindow::hChildAlign(tile) = h;
                tilewindow::vChildAlign(tile) = v;
                tile->askParentForReconfigure();
                window::bNeedsReconfigure(tile) = 1;
            }
        }

        void nativeTileEnableWrapping(UObject* object, Frame& stack, void*)
        {
            tileBool(object, stack, tilewindow::bWrap);
        }

        void nativeTileFillParent(UObject* object, Frame& stack, void*)
        {
            tileBool(object, stack, tilewindow::bFillParent);
        }

        void nativeTileMakeWidthsEqual(UObject* object, Frame& stack, void*)
        {
            tileBool(object, stack, tilewindow::bEqualWidth);
        }

        void nativeTileMakeHeightsEqual(UObject* object, Frame& stack, void*)
        {
            tileBool(object, stack, tilewindow::bEqualHeight);
        }

        UBorderWindow* borderSelf(UObject* object)
        {
            return cast<UBorderWindow>(object);
        }

        void nativeSetBorders(UObject* object, Frame& stack, void*)
        {
            UObject* values[9] = {};
            bool present[9] = {};
            for (int i = 0; i < 9; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            UBorderWindow* border = borderSelf(object);
            if (!border) {
                return;
            }
            UObject*& (*fields[9])(UObject*) = {
                borderwindow::borderTopLeft,
                borderwindow::borderTopRight,
                borderwindow::borderBottomLeft,
                borderwindow::borderBottomRight,
                borderwindow::borderLeft,
                borderwindow::borderRight,
                borderwindow::borderTop,
                borderwindow::borderBottom,
                borderwindow::center,
            };
            for (int i = 0; i < 9; i++) {
                if (present[i]) {
                    fields[i](border) = values[i];
                }
            }
            if (borderwindow::bMarginsFromBorder(border)) {
                borderwindow::childLeftMargin(border) = std::max(textureWidth(borderwindow::borderLeft(border)), textureWidth(borderwindow::borderTopLeft(border)));
                borderwindow::childRightMargin(border) = std::max(textureWidth(borderwindow::borderRight(border)), textureWidth(borderwindow::borderTopRight(border)));
                borderwindow::childTopMargin(border) = std::max(textureHeight(borderwindow::borderTop(border)), textureHeight(borderwindow::borderTopLeft(border)));
                borderwindow::childBottomMargin(border) = std::max(textureHeight(borderwindow::borderBottom(border)), textureHeight(borderwindow::borderBottomLeft(border)));
            }
            border->askParentForReconfigure();
        }

        void nativeSetBorderMargins(UObject* object, Frame& stack, void*)
        {
            float values[4] = {};
            bool present[4] = {};
            for (int i = 0; i < 4; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            UBorderWindow* border = borderSelf(object);
            if (!border) {
                return;
            }
            float& (*fields[4])(UObject*) = { borderwindow::childLeftMargin, borderwindow::childRightMargin, borderwindow::childTopMargin, borderwindow::childBottomMargin };
            for (int i = 0; i < 4; i++) {
                if (present[i]) {
                    fields[i](border) = values[i];
                }
            }
            border->askParentForReconfigure();
            window::bNeedsReconfigure(border) = 1;
        }

        void nativeBaseMarginsFromBorder(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UBorderWindow* border = borderSelf(object)) {
                borderwindow::bMarginsFromBorder(border) = value ? 1 : 0;
            }
        }

        void nativeEnableResizing(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UBorderWindow* border = borderSelf(object)) {
                borderwindow::bResizeable(border) = value ? 1 : 0;
            }
        }

        void nativeSetMoveCursors(UObject* object, Frame& stack, void*)
        {
            UObject* values[5] = {};
            bool present[5] = {};
            for (int i = 0; i < 5; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            UBorderWindow* border = borderSelf(object);
            if (!border) {
                return;
            }
            UObject*& (*fields[5])(UObject*) = { borderwindow::MoveCursor, borderwindow::hMoveCursor, borderwindow::vMoveCursor, borderwindow::tlMoveCursor, borderwindow::trMoveCursor };
            for (int i = 0; i < 5; i++) {
                if (present[i]) {
                    fields[i](border) = values[i];
                }
            }
        }

        UClipWindow* clipSelf(UObject* object)
        {
            return cast<UClipWindow>(object);
        }

        void nativeSetChildPosition(UObject* object, Frame& stack, void*)
        {
            int32_t newX = vm::get<int32_t>(stack);
            int32_t newY = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                clip->setChildPosition(newX, newY);
            }
        }

        void nativeGetChildPosition(UObject* object, Frame& stack, void*)
        {
            int32_t* outX = vm::getRef<int32_t>(stack);
            int32_t* outY = vm::getRef<int32_t>(stack);
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            *outX = clip ? clipwindow::childH(clip) : 0;
            *outY = clip ? clipwindow::childV(clip) : 0;
        }

        void setUnits(UClipWindow* clip, int32_t* h, int32_t* v)
        {
            bool changed = false;
            if (h) {
                int32_t value = std::max(*h, 1);
                changed = changed || clipwindow::prefHUnits(clip) != value;
                clipwindow::prefHUnits(clip) = value;
            }
            if (v) {
                int32_t value = std::max(*v, 1);
                changed = changed || clipwindow::prefVUnits(clip) != value;
                clipwindow::prefVUnits(clip) = value;
            }
            if (changed) {
                clip->askParentForReconfigure();
            }
        }

        void nativeUnitSize(UObject* object, Frame& stack, void*)
        {
            int32_t h = vm::get<int32_t>(stack);
            int32_t v = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                setUnits(clip, &h, &v);
            }
        }

        void nativeUnitWidth(UObject* object, Frame& stack, void*)
        {
            int32_t h = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                setUnits(clip, &h, nullptr);
            }
        }

        void nativeUnitHeight(UObject* object, Frame& stack, void*)
        {
            int32_t v = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                setUnits(clip, nullptr, &v);
            }
        }

        void nativeResetUnitSize(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            if (clip && (clipwindow::prefHUnits(clip) >= 0 || clipwindow::prefVUnits(clip) >= 0)) {
                clipwindow::prefHUnits(clip) = -1;
                clipwindow::prefVUnits(clip) = -1;
                clip->askParentForReconfigure();
            }
        }

        void nativeResetUnitWidth(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            if (clip && clipwindow::prefHUnits(clip) >= 0) {
                clipwindow::prefHUnits(clip) = -1;
                clip->askParentForReconfigure();
            }
        }

        void nativeResetUnitHeight(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            if (clip && clipwindow::prefVUnits(clip) >= 0) {
                clipwindow::prefHUnits(clip) = -1;
                clip->askParentForReconfigure();
            }
        }

        void nativeGetUnitSize(UObject* object, Frame& stack, void*)
        {
            int32_t* areaH = vm::getRef<int32_t>(stack);
            int32_t* areaV = vm::getRef<int32_t>(stack);
            int32_t* childH = vm::getRef<int32_t>(stack);
            int32_t* childV = vm::getRef<int32_t>(stack);
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            *areaH = clip ? clipwindow::areaHSize(clip) : 0;
            *areaV = clip ? clipwindow::areaVSize(clip) : 0;
            *childH = clip ? clipwindow::childHSize(clip) : 0;
            *childV = clip ? clipwindow::childVSize(clip) : 0;
        }

        void nativeForceChildSize(UObject* object, Frame& stack, void*)
        {
            bool forceWidth = optionalBoolDefault(stack, true);
            bool forceHeight = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                if ((clipwindow::bForceChildWidth(clip) != 0) != forceWidth || (clipwindow::bForceChildHeight(clip) != 0) != forceHeight) {
                    clipwindow::bForceChildWidth(clip) = forceWidth ? 1 : 0;
                    clipwindow::bForceChildHeight(clip) = forceHeight ? 1 : 0;
                    clip->askParentForReconfigure();
                    window::bNeedsReconfigure(clip) = 1;
                }
            }
        }

        void nativeEnableSnapToUnits(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UClipWindow* clip = clipSelf(object)) {
                if ((clipwindow::bSnapToUnits(clip) != 0) != value) {
                    clipwindow::bSnapToUnits(clip) = value ? 1 : 0;
                    clip->askParentForReconfigure();
                    window::bNeedsReconfigure(clip) = 1;
                }
            }
        }

        void nativeGetChild(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UClipWindow* clip = clipSelf(object);
            vm::ret<UObject*>(result, clip ? clip->child() : nullptr);
        }

        void nativeGetEnabledToggle(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UObject* found = nullptr;
            if (URadioBoxWindow* box = cast<URadioBoxWindow>(object)) {
                std::vector<UWindow*> pending { box };
                while (!pending.empty() && !found) {
                    UWindow* current = pending.back();
                    pending.pop_back();
                    UToggleWindow* toggle = cast<UToggleWindow>(current);
                    if (toggle && toggle->toggled) {
                        found = toggle;
                    }
                    for (UWindow* child = current->firstChild(); child; child = child->nextSibling()) {
                        pending.push_back(child);
                    }
                }
            }
            vm::ret<UObject*>(result, found);
        }

        UScrollAreaWindow* scrollSelf(UObject* object)
        {
            return cast<UScrollAreaWindow>(object);
        }

        void nativeEnableScrolling(UObject* object, Frame& stack, void*)
        {
            bool horizontal = optionalBoolDefault(stack, true);
            bool vertical = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UScrollAreaWindow* area = scrollSelf(object)) {
                area->enableScrolling(horizontal, vertical);
            }
        }

        void nativeSetScrollbarDistance(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            UScrollAreaWindow* area = scrollSelf(object);
            if (area && scrollareawindow::scrollbarDistance(area) != value) {
                scrollareawindow::scrollbarDistance(area) = value;
                area->askParentForReconfigure();
                window::bNeedsReconfigure(area) = 1;
            }
        }

        void nativeSetAreaMargins(UObject* object, Frame& stack, void*)
        {
            float w = vm::get<float>(stack);
            float h = vm::get<float>(stack);
            vm::finish(stack);
            UScrollAreaWindow* area = scrollSelf(object);
            if (area && (scrollareawindow::marginWidth(area) != w || scrollareawindow::marginHeight(area) != h)) {
                scrollareawindow::marginWidth(area) = w;
                scrollareawindow::marginHeight(area) = h;
                area->askParentForReconfigure();
                window::bNeedsReconfigure(area) = 1;
            }
        }

        void nativeAutoHideScrollbars(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UScrollAreaWindow* area = scrollSelf(object);
            if (area && (scrollareawindow::bHideScrollbars(area) != 0) != value) {
                scrollareawindow::bHideScrollbars(area) = value ? 1 : 0;
                area->askParentForReconfigure();
                window::bNeedsReconfigure(area) = 1;
            }
        }

        UViewportWindow* viewportSelf(UObject* object)
        {
            return cast<UViewportWindow>(object);
        }

        void nativeSetViewportActor(UObject* object, Frame& stack, void*)
        {
            UObject* origin = nullptr;
            optional(stack, origin);
            bool eyeLevel = optionalBoolDefault(stack, true);
            bool enable = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::originActor(view) = origin;
                viewportwindow::bUseEyeHeight(view) = eyeLevel ? 1 : 0;
                viewportwindow::bEnableViewport(view) = enable ? 1 : 0;
            }
        }

        void nativeSetViewportLocation(UObject* object, Frame& stack, void*)
        {
            Vector location = vm::get<Vector>(stack);
            bool enable = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::Location(view) = location;
                viewportwindow::originActor(view) = nullptr;
                viewportwindow::bEnableViewport(view) = enable ? 1 : 0;
            }
        }

        void nativeSetWatchActor(UObject* object, Frame& stack, void*)
        {
            UObject* watch = nullptr;
            optional(stack, watch);
            bool eyeLevel = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::watchActor(view) = watch;
                viewportwindow::bWatchEyeHeight(view) = eyeLevel ? 1 : 0;
            }
        }

        void nativeSetViewportRotation(UObject* object, Frame& stack, void*)
        {
            Rotator rotation;
            optional(stack, rotation);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::Rotation(view) = rotation;
            }
        }

        void nativeEnableViewport(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::bEnableViewport(view) = value ? 1 : 0;
            }
        }

        void nativeSetFOVAngle(UObject* object, Frame& stack, void*)
        {
            float value = 90;
            optional(stack, value);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::FOV(view) = value;
            }
        }

        void nativeShowViewportActor(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::bShowActor(view) = value ? 1 : 0;
            }
        }

        void nativeShowWeapons(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::bShowWeapons(view) = value ? 1 : 0;
            }
        }

        void nativeSetRelativeLocation(UObject* object, Frame& stack, void*)
        {
            Vector value;
            optional(stack, value);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::relLocation(view) = value;
            }
        }

        void nativeSetRelativeRotation(UObject* object, Frame& stack, void*)
        {
            Rotator value;
            optional(stack, value);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::relRotation(view) = value;
            }
        }

        void nativeSetDefaultTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = nullptr;
            Color color {};
            bool hasTexture = optional(stack, texture);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                if (hasTexture) {
                    viewportwindow::DefaultTexture(view) = texture;
                }
                if (hasColor) {
                    viewportwindow::DefaultColor(view) = color;
                }
            }
        }

        void nativeClearZBuffer(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UViewportWindow* view = viewportSelf(object)) {
                viewportwindow::bClearZ(view) = value ? 1 : 0;
            }
        }
    }

    void registerContainerNatives()
    {
        vm::registerNative(1535, nativeTileSetMargins);
        vm::registerNative(1536, nativeTileSetOrientation);
        vm::registerNative(1537, nativeTileSetDirections);
        vm::registerNative(1538, nativeTileSetOrder);
        vm::registerNative(1539, nativeTileSetMinorSpacing);
        vm::registerNative(1540, nativeTileSetMajorSpacing);
        vm::registerNative(1541, nativeTileSetChildAlignments);
        vm::registerNative(1542, nativeTileEnableWrapping);
        vm::registerNative(1543, nativeTileFillParent);
        vm::registerNative(1544, nativeTileMakeWidthsEqual);
        vm::registerNative(1545, nativeTileMakeHeightsEqual);
        vm::registerNative(1530, nativeSetBorders);
        vm::registerNative(1531, nativeSetBorderMargins);
        vm::registerNative(1532, nativeBaseMarginsFromBorder);
        vm::registerNative(1533, nativeEnableResizing);
        vm::registerNative(1534, nativeSetMoveCursors);
        vm::registerNative(1680, nativeSetChildPosition);
        vm::registerNative(1681, nativeGetChildPosition);
        vm::registerNative(1682, nativeUnitSize);
        vm::registerNative(1683, nativeUnitWidth);
        vm::registerNative(1684, nativeUnitHeight);
        vm::registerNative(1685, nativeResetUnitSize);
        vm::registerNative(1686, nativeResetUnitWidth);
        vm::registerNative(1687, nativeResetUnitHeight);
        vm::registerNative(1688, nativeGetUnitSize);
        vm::registerNative(1689, nativeForceChildSize);
        vm::registerNative(1690, nativeEnableSnapToUnits);
        vm::registerNative(1691, nativeGetChild);
        vm::registerNative(1820, nativeGetEnabledToggle);
        vm::registerNative(1700, nativeEnableScrolling);
        vm::registerNative(1701, nativeSetScrollbarDistance);
        vm::registerNative(1702, nativeSetAreaMargins);
        vm::registerNative(1703, nativeAutoHideScrollbars);
        vm::registerNative(1940, nativeSetViewportActor);
        vm::registerNative(1941, nativeSetViewportLocation);
        vm::registerNative(1942, nativeSetWatchActor);
        vm::registerNative(1943, nativeSetViewportRotation);
        vm::registerNative(1945, nativeEnableViewport);
        vm::registerNative(1946, nativeSetFOVAngle);
        vm::registerNative(1947, nativeShowViewportActor);
        vm::registerNative(1948, nativeShowWeapons);
        vm::registerNative(1950, nativeSetRelativeLocation);
        vm::registerNative(1951, nativeSetRelativeRotation);
        vm::registerNative(1955, nativeSetDefaultTexture);
        vm::registerNative(1956, nativeClearZBuffer);
    }
}
