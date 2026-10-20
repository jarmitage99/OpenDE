#include "extension/windows.h"

#include "engine/input.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace dx::extension {
    namespace {
        bool vertical(UScaleWindow* scale)
        {
            return scalewindow::orientation(scale) == 1;
        }

        void computeTextureSize(UObject* texture, float w, float h, float border, float& outW, float& outH)
        {
            outW = w > 0 ? w : (texture ? textureWidth(texture) : 0.0f);
            outH = h > 0 ? h : (texture ? textureHeight(texture) : 0.0f);
            outW += border * 2.0f;
            outH += border * 2.0f;
        }

        float truncate(float value)
        {
            return std::trunc(value);
        }

        float tickToPixel(UScaleWindow* scale, int32_t tick)
        {
            float start = scalewindow::absStartScale(scale);
            float end = scalewindow::absEndScale(scale);
            float range = end - start;
            int32_t div = scalewindow::numPositions(scale) - 1;
            if (scalewindow::bSpanThumb(scale)) {
                range = end - start + 1.0f - (vertical(scale) ? scalewindow::thumbH(scale) : scalewindow::thumbW(scale));
                div = scalewindow::numPositions(scale) - scalewindow::spanRange(scale);
            }
            div = std::max(div, 1);
            return truncate(static_cast<float>(tick) * range / static_cast<float>(div) + start + 0.5f);
        }

        int32_t pixelToTick(UScaleWindow* scale, float pixel)
        {
            float start = scalewindow::absStartScale(scale);
            float end = scalewindow::absEndScale(scale);
            float range = end - start;
            int32_t div = scalewindow::numPositions(scale) - 1;
            if (scalewindow::bSpanThumb(scale)) {
                range = end - start + 1.0f - (vertical(scale) ? scalewindow::thumbH(scale) : scalewindow::thumbW(scale));
                div = scalewindow::numPositions(scale) - scalewindow::spanRange(scale);
            }
            range = std::max(range, 1.0f);
            return static_cast<int32_t>(truncate((pixel - start) * static_cast<float>(div) / range + 0.5f));
        }

        void computeThumbConfig(UScaleWindow* scale)
        {
            bool vert = vertical(scale);
            float border = scalewindow::thumbBorderSize(scale);
            computeTextureSize(scalewindow::thumbTexture(scale), scalewindow::ThumbWidth(scale), scalewindow::ThumbHeight(scale), border, scalewindow::thumbW(scale), scalewindow::thumbH(scale));
            scalewindow::preCapXOff(scale) = 0;
            scalewindow::preCapYOff(scale) = 0;
            scalewindow::postCapXOff(scale) = 0;
            scalewindow::postCapYOff(scale) = 0;
            computeTextureSize(scalewindow::preCapTexture(scale), scalewindow::preCapWidth(scale), scalewindow::preCapHeight(scale), 0, scalewindow::preCapW(scale), scalewindow::preCapH(scale));
            computeTextureSize(scalewindow::postCapTexture(scale), scalewindow::postCapWidth(scale), scalewindow::postCapHeight(scale), 0, scalewindow::postCapW(scale), scalewindow::postCapH(scale));
            if (vert) {
                scalewindow::thumbH(scale) += scalewindow::preCapH(scale) + scalewindow::postCapH(scale);
                scalewindow::preCapW(scale) = scalewindow::thumbW(scale) - border * 2.0f;
                scalewindow::postCapW(scale) = scalewindow::preCapW(scale);
            } else {
                scalewindow::thumbW(scale) += scalewindow::preCapW(scale) + scalewindow::postCapW(scale);
                scalewindow::preCapH(scale) = scalewindow::thumbH(scale) - border * 2.0f;
                scalewindow::postCapH(scale) = scalewindow::preCapH(scale);
            }
            scalewindow::thumbX(scale) = truncate((scale->width() - scalewindow::thumbW(scale)) * 0.5f);
            scalewindow::thumbY(scale) = truncate((scale->height() - scalewindow::thumbH(scale)) * 0.5f);
            int32_t current = scalewindow::currentPos(scale);
            if (scalewindow::bSpanThumb(scale)) {
                float start = scalewindow::absStartScale(scale);
                float end = scalewindow::absEndScale(scale);
                int32_t n = std::max(scalewindow::numPositions(scale), 1);
                float length = truncate((end + 1.0f - start) * static_cast<float>(scalewindow::spanRange(scale)) / static_cast<float>(n) + 0.5f);
                length = std::max(length, 6.0f);
                length = std::min(length, end - start + 1.0f);
                (vert ? scalewindow::thumbH(scale) : scalewindow::thumbW(scale)) = length;
                (vert ? scalewindow::thumbY(scale) : scalewindow::thumbX(scale)) = tickToPixel(scale, current);
            } else if (vert) {
                scalewindow::thumbY(scale) = truncate(tickToPixel(scale, current) - scalewindow::thumbH(scale) * 0.5f + 0.5f);
            } else {
                scalewindow::thumbX(scale) = truncate(tickToPixel(scale, current) - scalewindow::thumbW(scale) * 0.5f + 0.5f);
            }
        }

        void drawIconPattern(UGC* gc, float x, float y, float w, float h, float orgX, float orgY, float srcW, float srcH, UObject* texture)
        {
            if (!texture) {
                return;
            }
            if (srcW != 0) {
                w = std::min(w, textureWidth(texture) - orgX);
            }
            if (srcH != 0) {
                h = std::min(h, textureHeight(texture) - orgY);
            }
            if (w <= 0 || h <= 0) {
                return;
            }
            gc->drawPattern(x, y, w, h, orgX, orgY, texture);
        }

        void drawScaleTexture(UScaleWindow* scale, UGC* gc, UObject* texture, float x, float y, float w, float h, bool repeat, uint8_t style, Color color, float border, Color borderColor, UObject* preTexture, UObject* postTexture, const float* caps)
        {
            bool vert = vertical(scale);
            gc->setStyle(style);
            if (border > 0) {
                if (UObject* pattern = scalewindow::borderPattern(scale)) {
                    gc::tileColor(gc) = borderColor;
                    gc->drawBox(x, y, w, h, border, pattern);
                }
                x += border;
                y += border;
                w -= border * 2.0f;
                h -= border * 2.0f;
            }
            gc::tileColor(gc) = color;
            float preW = caps[2];
            float preH = caps[3];
            float postW = caps[6];
            float postH = caps[7];
            if (preTexture && preW > 0 && preH > 0 && w > 0 && h > 0) {
                if (vert) {
                    drawIconPattern(gc, x, y, preW, preH, caps[0], caps[1], 0, preH, preTexture);
                    y += preH;
                    h -= preH;
                } else {
                    drawIconPattern(gc, x, y, preW, preH, caps[0], caps[1], preW, 0, preTexture);
                    x += preW;
                    w -= preW;
                }
            }
            if (postTexture && postW > 0 && postH > 0 && w > 0 && h > 0) {
                if (vert) {
                    drawIconPattern(gc, x, y + h - postH, postW, postH, caps[4], caps[5], 0, postH, postTexture);
                    h -= postH;
                } else {
                    drawIconPattern(gc, x + w - postW, y, postW, postH, caps[4], caps[5], postW, 0, postTexture);
                    w -= postW;
                }
            }
            if (texture && w > 0 && h > 0) {
                if (vert) {
                    drawIconPattern(gc, x, y, w, h, 0, 0, 0, repeat ? 0 : h, texture);
                } else {
                    drawIconPattern(gc, x, y, w, h, 0, 0, repeat ? 0 : w, 0, texture);
                }
            }
            gc->resetState(scale, gc->offsetX, gc->offsetY);
        }
    }

    void UScaleWindow::initWindow()
    {
        Color white { 255, 255, 255, 255 };
        scalewindow::bDrawEndTicks(this) = 1;
        scalewindow::scaleBorderColor(this) = white;
        scalewindow::thumbBorderColor(this) = white;
        scalewindow::scaleStyle(this) = DSTY_Masked;
        scalewindow::thumbStyle(this) = DSTY_Masked;
        scalewindow::tickStyle(this) = DSTY_Masked;
        scalewindow::scaleColor(this) = white;
        scalewindow::thumbColor(this) = white;
        scalewindow::tickColor(this) = white;
        scalewindow::numPositions(this) = 10;
        scalewindow::spanRange(this) = 1;
        scalewindow::thumbStep(this) = 1;
        scalewindow::toValue(this) = 1.0f;
        scalewindow::initialDelay(this) = 0.5f;
        scalewindow::repeatRate(this) = 0.1f;
        scalewindow::initialPos(this) = -1;
        window::bIsSelectable(this) = 1;
        UWindow::initWindow();
    }

    int32_t UScaleWindow::maxPosition()
    {
        if (scalewindow::bSpanThumb(this)) {
            return std::max(scalewindow::numPositions(this) - scalewindow::spanRange(this), 0);
        }
        return std::max(scalewindow::numPositions(this) - 1, 0);
    }

    float UScaleWindow::positionValue(int32_t position)
    {
        float from = scalewindow::fromValue(this);
        float to = scalewindow::toValue(this);
        int32_t last = maxPosition();
        if (last <= 0) {
            return from;
        }
        return from + (to - from) * static_cast<float>(position) / static_cast<float>(last);
    }

    void UScaleWindow::notifyPosition(bool final)
    {
        int32_t position = scalewindow::currentPos(this);
        if (scalewindow::bSpanThumb(this)) {
            int32_t last = position + scalewindow::spanRange(this);
            for (UWindow* current = this; current; current = current->parent()) {
                if (current->scaleRangeChanged(this, position, last, positionValue(position), positionValue(last), final)) {
                    return;
                }
            }
            return;
        }
        float value = positionValue(position);
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->scalePositionChanged(this, position, value, final)) {
                return;
            }
        }
    }

    void UScaleWindow::changeScaleAttributes()
    {
        for (UWindow* current = this; current; current = current->parent()) {
            if (!current->visible()) {
                return;
            }
        }
        for (UWindow* current = this; current; current = current->parent()) {
            if (current->scaleAttributesChanged(this, scalewindow::currentPos(this), scalewindow::spanRange(this), scalewindow::numPositions(this))) {
                return;
            }
        }
    }

    void UScaleWindow::changeThumbPosition(int32_t position, bool force, bool final)
    {
        position = std::clamp(position, 0, maxPosition());
        if (scalewindow::currentPos(this) != position) {
            scalewindow::currentPos(this) = position;
            computeThumbConfig(this);
            notifyPosition(final);
            force = true;
        }
        if (force) {
            changeScaleAttributes();
            computeThumbConfig(this);
        }
    }

    void UScaleWindow::setRanges(int32_t span, int32_t positions)
    {
        span = std::max(span, 1);
        if (scalewindow::spanRange(this) != span || scalewindow::numPositions(this) != positions) {
            scalewindow::spanRange(this) = span;
            scalewindow::numPositions(this) = positions;
            changeThumbPosition(scalewindow::currentPos(this), true, true);
        }
    }

    void UScaleWindow::setPosition(int32_t position, bool final, bool notify)
    {
        position = std::clamp(position, 0, maxPosition());
        bool changed = position != scalewindow::currentPos(this);
        scalewindow::currentPos(this) = position;
        computeThumbConfig(this);
        if (changed) {
            changeScaleAttributes();
        }
        if (!notify || (!changed && !final)) {
            return;
        }
        if (changed) {
            playWindowSound(scalewindow::dragSound(this));
        }
        notifyPosition(final);
    }

    int32_t UScaleWindow::positionFromPoint(float pointX, float pointY)
    {
        float along = (vertical(this) ? pointY : pointX) - dragOffset;
        if (scalewindow::bSpanThumb(this)) {
            along -= (vertical(this) ? scalewindow::thumbH(this) : scalewindow::thumbW(this)) * 0.5f;
        }
        return std::clamp(pixelToTick(this, along), 0, maxPosition());
    }

    void UScaleWindow::moveThumb(uint8_t move)
    {
        int32_t position = scalewindow::currentPos(this);
        int32_t step = std::max(scalewindow::thumbStep(this), 1);
        int32_t page = scalewindow::bSpanThumb(this) ? scalewindow::spanRange(this) : 4;
        switch (move) {
        case 0:
            position = 0;
            break;
        case 1:
            position = maxPosition();
            break;
        case 2:
            position -= 1;
            break;
        case 3:
            position += 1;
            break;
        case 4:
            position -= step;
            break;
        case 5:
            position += step;
            break;
        case 6:
            position -= page;
            break;
        case 7:
            position += page;
            break;
        default:
            break;
        }
        playWindowSound(scalewindow::clickSound(this));
        setPosition(position, true, true);
    }

    std::string UScaleWindow::valueString()
    {
        int32_t position = scalewindow::currentPos(this);
        if (position >= 0 && static_cast<size_t>(position) < enumerations.size() && !enumerations[static_cast<size_t>(position)].empty()) {
            return enumerations[static_cast<size_t>(position)];
        }
        float value = positionValue(position);
        std::string format = scalewindow::valueFmt(this);
        if (format.empty()) {
            format = "%d";
        }
        return formatNumber(format, static_cast<double>(value));
    }

    void UScaleWindow::computeGeometry()
    {
        bool vert = vertical(this);
        float border = scalewindow::scaleBorderSize(this);
        computeTextureSize(scalewindow::scaleTexture(this), scalewindow::scaleWidth(this), scalewindow::scaleHeight(this), border, scalewindow::scaleW(this), scalewindow::scaleH(this));
        if (scalewindow::bStretchScale(this)) {
            if (vert) {
                scalewindow::scaleH(this) = height() - scalewindow::marginHeight(this) * 2.0f;
            } else {
                scalewindow::scaleW(this) = width() - scalewindow::marginWidth(this) * 2.0f;
            }
        }
        scalewindow::scaleX(this) = truncate((width() - scalewindow::scaleW(this)) * 0.5f);
        scalewindow::scaleY(this) = truncate((height() - scalewindow::scaleH(this)) * 0.5f);
        float start = vert ? scalewindow::scaleY(this) : scalewindow::scaleX(this);
        float end = start + (vert ? scalewindow::scaleH(this) : scalewindow::scaleW(this));
        float absStart = start + scalewindow::startOffset(this) + border;
        float absEnd = end - (scalewindow::endOffset(this) + 1.0f) - border;
        if (absEnd < absStart) {
            absStart = absStart + (absEnd - absStart) * 0.5f;
            absEnd = absStart;
        }
        scalewindow::absStartScale(this) = absStart;
        scalewindow::absEndScale(this) = absEnd;
        computeThumbConfig(this);
        computeTextureSize(scalewindow::tickTexture(this), scalewindow::tickWidth(this), scalewindow::tickHeight(this), 0, scalewindow::tickW(this), scalewindow::tickH(this));
        scalewindow::tickX(this) = truncate((width() - scalewindow::tickW(this)) * 0.5f);
        scalewindow::tickY(this) = truncate((height() - scalewindow::tickH(this)) * 0.5f);
    }

    void UScaleWindow::parentRequestedPreferredSize(bool, float& preferredWidth, bool, float& preferredHeight)
    {
        float w = 0;
        float h = 0;
        float tw = 0;
        float th = 0;
        computeTextureSize(scalewindow::scaleTexture(this), scalewindow::scaleWidth(this), scalewindow::scaleHeight(this), scalewindow::scaleBorderSize(this), w, h);
        computeTextureSize(scalewindow::thumbTexture(this), scalewindow::ThumbWidth(this), scalewindow::ThumbHeight(this), scalewindow::thumbBorderSize(this), tw, th);
        w = std::max(w, tw);
        h = std::max(h, th);
        computeTextureSize(scalewindow::tickTexture(this), scalewindow::tickWidth(this), scalewindow::tickHeight(this), 0, tw, th);
        w = std::max(w, tw);
        h = std::max(h, th);
        preferredWidth = w + scalewindow::marginWidth(this) * 2.0f;
        preferredHeight = h + scalewindow::marginHeight(this) * 2.0f;
    }

    void UScaleWindow::configurationChanged()
    {
        computeGeometry();
        UWindow::configurationChanged();
    }

    void UScaleWindow::drawWindow(UGC* gc)
    {
        drawBackground(gc);
        computeGeometry();
        const float noCaps[8] = {};
        drawScaleTexture(this, gc, scalewindow::scaleTexture(this), scalewindow::scaleX(this), scalewindow::scaleY(this), scalewindow::scaleW(this), scalewindow::scaleH(this), scalewindow::bRepeatScaleTexture(this) != 0, scalewindow::scaleStyle(this), scalewindow::scaleColor(this), scalewindow::scaleBorderSize(this), scalewindow::scaleBorderColor(this), nullptr, nullptr, noCaps);
        if (UObject* tick = scalewindow::tickTexture(this)) {
            int32_t count = scalewindow::numPositions(this) + (scalewindow::bSpanThumb(this) ? 1 : 0);
            int32_t first = 0;
            if (!scalewindow::bDrawEndTicks(this)) {
                first = 1;
                count--;
            }
            float tw = scalewindow::tickW(this);
            float th = scalewindow::tickH(this);
            for (int32_t i = first; i < count; i++) {
                float pixel = tickToPixel(this, i);
                if (vertical(this)) {
                    gc->setStyle(scalewindow::tickStyle(this));
                    gc::tileColor(gc) = scalewindow::tickColor(this);
                    drawIconPattern(gc, scalewindow::tickX(this), pixel - truncate(th * 0.5f), tw, th, 0, 0, 0, th, tick);
                } else {
                    gc->setStyle(scalewindow::tickStyle(this));
                    gc::tileColor(gc) = scalewindow::tickColor(this);
                    drawIconPattern(gc, pixel - truncate(tw * 0.5f), scalewindow::tickY(this), tw, th, 0, 0, tw, 0, tick);
                }
                gc->resetState(this, gc->offsetX, gc->offsetY);
            }
        }
        const float caps[8] = { scalewindow::preCapXOff(this), scalewindow::preCapYOff(this), scalewindow::preCapW(this), scalewindow::preCapH(this), scalewindow::postCapXOff(this), scalewindow::postCapYOff(this), scalewindow::postCapW(this), scalewindow::postCapH(this) };
        drawScaleTexture(this, gc, scalewindow::thumbTexture(this), scalewindow::thumbX(this), scalewindow::thumbY(this), scalewindow::thumbW(this), scalewindow::thumbH(this), scalewindow::bRepeatThumbTexture(this) != 0, scalewindow::thumbStyle(this), scalewindow::thumbColor(this), scalewindow::thumbBorderSize(this), scalewindow::thumbBorderColor(this), scalewindow::preCapTexture(this), scalewindow::postCapTexture(this), caps);
        scriptDrawWindow(gc);
    }

    bool UScaleWindow::mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        if (UWindow::mouseButtonPressed(pointX, pointY, button, numClicks)) {
            return true;
        }
        if (button == IK_MouseWheelUp || button == IK_MouseWheelDown) {
            moveThumb(button == IK_MouseWheelUp ? 4 : 5);
            return true;
        }
        if (button != IK_LeftMouse) {
            return false;
        }
        bool vert = vertical(this);
        float along = vert ? pointY : pointX;
        float thumbStart = vert ? scalewindow::thumbY(this) : scalewindow::thumbX(this);
        float thumbLength = vert ? scalewindow::thumbH(this) : scalewindow::thumbW(this);
        if (along >= thumbStart && along < thumbStart + thumbLength) {
            scalewindow::bDraggingThumb(this) = 1;
            dragOffset = along - thumbStart - thumbLength * 0.5f;
            grabMouse();
            playWindowSound(scalewindow::clickSound(this));
            return true;
        }
        if (scalewindow::bSpanThumb(this)) {
            moveThumb(along < thumbStart ? 6 : 7);
            return true;
        }
        dragOffset = 0;
        scalewindow::bDraggingThumb(this) = 1;
        grabMouse();
        setPosition(positionFromPoint(pointX, pointY), false, true);
        return true;
    }

    void UScaleWindow::mouseMoved(float newX, float newY)
    {
        if (scalewindow::bDraggingThumb(this)) {
            setPosition(positionFromPoint(newX, newY), false, true);
        }
        UWindow::mouseMoved(newX, newY);
    }

    bool UScaleWindow::mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks)
    {
        if (scalewindow::bDraggingThumb(this)) {
            scalewindow::bDraggingThumb(this) = 0;
            ungrabMouse();
            setPosition(scalewindow::currentPos(this), true, true);
            playWindowSound(scalewindow::setSound(this));
            return true;
        }
        return UWindow::mouseButtonReleased(pointX, pointY, button, numClicks);
    }

    bool UScaleWindow::virtualKeyPressed(uint8_t key, bool repeat)
    {
        if (UWindow::virtualKeyPressed(key, repeat)) {
            return true;
        }
        bool horizontal = scalewindow::orientation(this) == 0;
        if ((horizontal && key == IK_Left) || (!horizontal && key == IK_Up)) {
            moveThumb(4);
            return true;
        }
        if ((horizontal && key == IK_Right) || (!horizontal && key == IK_Down)) {
            moveThumb(5);
            return true;
        }
        if (key == IK_Home) {
            moveThumb(0);
            return true;
        }
        if (key == IK_End) {
            moveThumb(1);
            return true;
        }
        return false;
    }

    void UScaleManagerWindow::updateValueField()
    {
        UTextWindow* field = cast<UTextWindow>(scalemanagerwindow::valueField(this));
        UScaleWindow* scale = cast<UScaleWindow>(scalemanagerwindow::Scale(this));
        if (field && scale) {
            std::string value = scale->valueString();
            if (field->text() != value) {
                field->text() = value;
                field->askParentForReconfigure();
            }
        }
    }

    void UScaleManagerWindow::initWindow()
    {
        scalemanagerwindow::bStretchValueField(this) = 1;
        scalemanagerwindow::childHAlign(this) = 1;
        scalemanagerwindow::childVAlign(this) = 1;
        UWindow::initWindow();
    }

    void UScaleManagerWindow::parentRequestedPreferredSize(bool, float& preferredWidth, bool, float& preferredHeight)
    {
        bool vert = scalemanagerwindow::orientation(this) == 1;
        float w = 0;
        float h = 0;
        float spacing = 0;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (child->destroyed || !child->visible()) {
                continue;
            }
            float cw = 0;
            float ch = 0;
            child->queryPreferredSize(cw, ch);
            if (vert) {
                h += ch + spacing;
                w = std::max(w, cw);
            } else {
                w += cw + spacing;
                h = std::max(h, ch);
            }
            spacing = scalemanagerwindow::Spacing(this);
        }
        preferredWidth = w + scalemanagerwindow::marginWidth(this) * 2.0f;
        preferredHeight = h + scalemanagerwindow::marginHeight(this) * 2.0f;
    }

    void UScaleManagerWindow::configurationChanged()
    {
        bool vert = scalemanagerwindow::orientation(this) == 1;
        float marginW = scalemanagerwindow::marginWidth(this);
        float marginH = scalemanagerwindow::marginHeight(this);
        float spacing = vert ? marginH : marginW;
        float end = vert ? height() - marginH : width() - marginW;
        float position = 0;

        struct Hold {
            UWindow* window;
            float x;
            float y;
            float w;
            float h;
        };

        std::vector<Hold> holds;
        for (UWindow* child = firstChild(); child; child = child->nextSibling()) {
            if (child->destroyed || !child->visible()) {
                continue;
            }
            Hold hold { child, 0, 0, 0, 0 };
            child->queryPreferredSize(hold.w, hold.h);
            if (vert) {
                hold.y = position + spacing;
                switch (scalemanagerwindow::childHAlign(this)) {
                case 0:
                    hold.x = marginW;
                    break;
                case 2:
                    hold.x = width() - hold.w - marginW;
                    break;
                case 3:
                    hold.x = marginW;
                    hold.w = width() - marginW * 2.0f;
                    break;
                default:
                    hold.x = (width() - hold.w) * 0.5f;
                    break;
                }
                position = hold.y + hold.h;
            } else {
                hold.x = position + spacing;
                switch (scalemanagerwindow::childVAlign(this)) {
                case 0:
                    hold.y = marginH;
                    break;
                case 2:
                    hold.y = height() - hold.h - marginH;
                    break;
                case 3:
                    hold.y = marginH;
                    hold.h = height() - marginH * 2.0f;
                    break;
                default:
                    hold.y = (height() - hold.h) * 0.5f;
                    break;
                }
                position = hold.x + hold.w;
            }
            holds.push_back(hold);
            spacing = scalemanagerwindow::Spacing(this);
        }
        UWindow* scale = cast<UWindow>(scalemanagerwindow::Scale(this));
        UWindow* field = cast<UWindow>(scalemanagerwindow::valueField(this));
        bool stretchScale = scale && scale->parent() == this && scale->visible() && scalemanagerwindow::bStretchScaleField(this);
        bool stretchField = field && field->parent() == this && field->visible() && scalemanagerwindow::bStretchValueField(this);
        if (position != end && (stretchScale || stretchField)) {
            float extra = end - position;
            float scaleExtra = 0;
            float fieldExtra = 0;
            if (stretchScale && stretchField) {
                scaleExtra = extra * 0.5f;
                fieldExtra = extra - scaleExtra;
            } else if (stretchScale) {
                scaleExtra = extra;
            } else {
                fieldExtra = extra;
            }
            float offset = 0;
            for (Hold& hold : holds) {
                float grow = hold.window == scale ? scaleExtra : (hold.window == field ? fieldExtra : 0.0f);
                if (vert) {
                    hold.y += offset;
                    hold.h += grow;
                } else {
                    hold.x += offset;
                    hold.w += grow;
                }
                offset += grow;
            }
        }
        for (Hold& hold : holds) {
            hold.window->configureChild(hold.x, hold.y, hold.w, hold.h);
        }
        updateValueField();
        UWindow::configurationChanged();
    }

    bool UScaleManagerWindow::scaleAttributesChanged(UWindow* scale, int32_t tickPosition, int32_t tickSpan, int32_t numTicks)
    {
        bool handled = UWindow::scaleAttributesChanged(scale, tickPosition, tickSpan, numTicks);
        if (scale == scalemanagerwindow::Scale(this)) {
            if (UWindow* dec = cast<UWindow>(scalemanagerwindow::decButton(this))) {
                dec->setSensitivity(tickPosition > 0);
            }
            if (UWindow* inc = cast<UWindow>(scalemanagerwindow::incButton(this))) {
                inc->setSensitivity(tickPosition + tickSpan < numTicks);
            }
        }
        return handled;
    }

    bool UScaleManagerWindow::buttonActivated(UWindow* button)
    {
        UScaleWindow* scale = cast<UScaleWindow>(scalemanagerwindow::Scale(this));
        if (scale && button && button == scalemanagerwindow::decButton(this)) {
            scale->moveThumb(4);
            return true;
        }
        if (scale && button && button == scalemanagerwindow::incButton(this)) {
            scale->moveThumb(5);
            return true;
        }
        return UWindow::buttonActivated(button);
    }

    bool UScaleManagerWindow::scalePositionChanged(UWindow* scale, int32_t newTickPosition, float newValue, bool final)
    {
        if (scale == scalemanagerwindow::Scale(this)) {
            updateValueField();
        }
        return UWindow::scalePositionChanged(scale, newTickPosition, newValue, final);
    }

    namespace {
        UScaleWindow* self(UObject* object)
        {
            return cast<UScaleWindow>(object);
        }

        UScaleManagerWindow* managerOf(UScaleWindow* scale)
        {
            return scale ? cast<UScaleManagerWindow>(scale->parent()) : nullptr;
        }

        void refreshManager(UScaleWindow* scale)
        {
            if (UScaleManagerWindow* manager = managerOf(scale)) {
                manager->updateValueField();
            }
        }

        void nativeSetScaleOrientation(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scalewindow::orientation(scale) = value;
                scale->askParentForReconfigure();
            }
        }

        void nativeSetScaleTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            float values[4] = {};
            bool present[4] = {};
            for (int i = 0; i < 4; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            scalewindow::scaleTexture(scale) = texture;
            scalewindow::scaleWidth(scale) = present[0] ? std::max(values[0], 0.0f) : 0.0f;
            scalewindow::scaleHeight(scale) = present[1] ? std::max(values[1], 0.0f) : 0.0f;
            scalewindow::startOffset(scale) = present[2] ? values[2] : 0.0f;
            scalewindow::endOffset(scale) = present[3] ? values[3] : 0.0f;
            scale->askParentForReconfigure();
        }

        void nativeSetThumbTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            float w = 0;
            float h = 0;
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            scalewindow::thumbTexture(scale) = texture;
            scalewindow::ThumbWidth(scale) = hasW ? std::max(w, 0.0f) : 0.0f;
            scalewindow::ThumbHeight(scale) = hasH ? std::max(h, 0.0f) : 0.0f;
            scale->askParentForReconfigure();
        }

        void nativeSetTickTexture(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            bool ends = optionalBoolDefault(stack, true);
            float w = 0;
            float h = 0;
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            scalewindow::tickTexture(scale) = texture;
            scalewindow::bDrawEndTicks(scale) = ends ? 1 : 0;
            scalewindow::tickWidth(scale) = hasW ? std::max(w, 0.0f) : 0.0f;
            scalewindow::tickHeight(scale) = hasH ? std::max(h, 0.0f) : 0.0f;
            scale->askParentForReconfigure();
        }

        void nativeSetThumbCaps(UObject* object, Frame& stack, void*)
        {
            UObject* pre = vm::get<UObject*>(stack);
            UObject* post = vm::get<UObject*>(stack);
            float values[4] = {};
            bool present[4] = {};
            for (int i = 0; i < 4; i++) {
                present[i] = optional(stack, values[i]);
            }
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            scalewindow::preCapTexture(scale) = pre;
            scalewindow::postCapTexture(scale) = post;
            scalewindow::preCapWidth(scale) = present[0] ? std::max(values[0], 0.0f) : 0.0f;
            scalewindow::preCapHeight(scale) = present[1] ? std::max(values[1], 0.0f) : 0.0f;
            scalewindow::postCapWidth(scale) = present[2] ? std::max(values[2], 0.0f) : 0.0f;
            scalewindow::postCapHeight(scale) = present[3] ? std::max(values[3], 0.0f) : 0.0f;
            scale->askParentForReconfigure();
        }

        void nativeEnableStretchedScale(UObject* object, Frame& stack, void*)
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                if ((scalewindow::bStretchScale(scale) != 0) != value) {
                    scalewindow::bStretchScale(scale) = value ? 1 : 0;
                    scalewindow::bRepeatScaleTexture(scale) = value ? 1 : 0;
                    scale->askParentForReconfigure();
                }
            }
        }

        void nativeSetBorderPattern(UObject* object, Frame& stack, void*)
        {
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scalewindow::borderPattern(scale) = texture;
            }
        }

        void nativeSetScaleBorder(UObject* object, Frame& stack, void*)
        {
            float size = 0;
            Color color {};
            bool hasSize = optional(stack, size);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                if (hasSize && scalewindow::scaleBorderSize(scale) != size) {
                    scalewindow::scaleBorderSize(scale) = size;
                    scale->askParentForReconfigure();
                }
                if (hasColor) {
                    scalewindow::scaleBorderColor(scale) = color;
                }
            }
        }

        void nativeSetThumbBorder(UObject* object, Frame& stack, void*)
        {
            float size = 0;
            Color color {};
            bool hasSize = optional(stack, size);
            bool hasColor = optional(stack, color);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                if (hasSize && scalewindow::thumbBorderSize(scale) != size) {
                    scalewindow::thumbBorderSize(scale) = size;
                    scale->askParentForReconfigure();
                }
                if (hasColor) {
                    scalewindow::thumbBorderColor(scale) = color;
                }
            }
        }

        void byteSetter(UObject* object, Frame& stack, uint8_t& (*field)(UObject*))
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                field(scale) = value;
            }
        }

        void colorSetter(UObject* object, Frame& stack, Color& (*field)(UObject*))
        {
            Color value = vm::get<Color>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                field(scale) = value;
            }
        }

        void nativeSetScaleStyle(UObject* object, Frame& stack, void*)
        {
            byteSetter(object, stack, scalewindow::scaleStyle);
        }

        void nativeSetThumbStyle(UObject* object, Frame& stack, void*)
        {
            byteSetter(object, stack, scalewindow::thumbStyle);
        }

        void nativeSetTickStyle(UObject* object, Frame& stack, void*)
        {
            byteSetter(object, stack, scalewindow::tickStyle);
        }

        void nativeSetScaleColor(UObject* object, Frame& stack, void*)
        {
            colorSetter(object, stack, scalewindow::scaleColor);
        }

        void nativeSetThumbColor(UObject* object, Frame& stack, void*)
        {
            colorSetter(object, stack, scalewindow::thumbColor);
        }

        void nativeSetTickColor(UObject* object, Frame& stack, void*)
        {
            colorSetter(object, stack, scalewindow::tickColor);
        }

        void nativeSetScaleMargins(UObject* object, Frame& stack, void*)
        {
            float w = 0;
            float h = 0;
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                if (hasW) {
                    scalewindow::marginWidth(scale) = std::max(w, 0.0f);
                }
                if (hasH) {
                    scalewindow::marginHeight(scale) = std::max(h, 0.0f);
                }
                scale->askParentForReconfigure();
            }
        }

        void nativeSetNumTicks(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                int32_t positions = std::max(value, 1);
                if (scalewindow::numPositions(scale) != positions) {
                    scalewindow::numPositions(scale) = positions;
                    scale->changeThumbPosition(scalewindow::currentPos(scale), true, true);
                }
                refreshManager(scale);
            }
        }

        void nativeGetNumTicks(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            vm::ret<int32_t>(result, scale ? scalewindow::numPositions(scale) : 0);
        }

        void nativeSetThumbSpan(UObject* object, Frame& stack, void*)
        {
            int32_t value = 0;
            optional(stack, value);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                bool span = value >= 1;
                if (scalewindow::spanRange(scale) != value || (scalewindow::bSpanThumb(scale) != 0) != span) {
                    scalewindow::bSpanThumb(scale) = span ? 1 : 0;
                    scalewindow::bRepeatThumbTexture(scale) = span ? 1 : 0;
                    if (span) {
                        scalewindow::spanRange(scale) = value;
                    }
                    scale->changeThumbPosition(scalewindow::currentPos(scale), true, true);
                }
            }
        }

        void nativeGetThumbSpan(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            vm::ret<int32_t>(result, scale && scalewindow::bSpanThumb(scale) ? scalewindow::spanRange(scale) : 0);
        }

        void nativeSetTickPosition(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scale->changeThumbPosition(value, false, true);
                refreshManager(scale);
            }
        }

        void nativeGetTickPosition(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            vm::ret<int32_t>(result, scale ? scalewindow::currentPos(scale) : 0);
        }

        void nativeSetValueRange(UObject* object, Frame& stack, void*)
        {
            float from = vm::get<float>(stack);
            float to = vm::get<float>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scalewindow::fromValue(scale) = from;
                scalewindow::toValue(scale) = to;
                refreshManager(scale);
            }
        }

        void nativeSetValue(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            float from = scalewindow::fromValue(scale);
            float to = scalewindow::toValue(scale);
            int32_t position = 0;
            if (to != from) {
                position = static_cast<int32_t>(std::round((value - from) / (to - from) * static_cast<float>(scale->maxPosition())));
            }
            scale->setPosition(position, true, true);
            refreshManager(scale);
        }

        void nativeGetValue(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            vm::ret<float>(result, scale ? scale->positionValue(scalewindow::currentPos(scale)) : 0.0f);
        }

        void nativeGetValues(UObject* object, Frame& stack, void*)
        {
            float* from = vm::getRef<float>(stack);
            float* to = vm::getRef<float>(stack);
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (scale) {
                *from = scale->positionValue(scalewindow::currentPos(scale));
                *to = scale->positionValue(std::min(scalewindow::currentPos(scale) + std::max(scalewindow::spanRange(scale), 0), scale->maxPosition()));
            }
        }

        void nativeSetValueFormat(UObject* object, Frame& stack, void*)
        {
            std::string format = vm::get<std::string>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scalewindow::valueFmt(scale) = format;
                refreshManager(scale);
            }
        }

        void nativeGetValueString(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            vm::ret<std::string>(result, scale ? scale->valueString() : std::string());
        }

        void nativeSetEnumeration(UObject* object, Frame& stack, void*)
        {
            int32_t position = vm::get<int32_t>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale || position < 0) {
                return;
            }
            if (scale->enumerations.size() <= static_cast<size_t>(position)) {
                scale->enumerations.resize(static_cast<size_t>(position) + 1);
            }
            scale->enumerations[static_cast<size_t>(position)] = text;
            refreshManager(scale);
        }

        void nativeClearAllEnumerations(UObject* object, Frame& stack, void*)
        {
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scale->enumerations.clear();
                refreshManager(scale);
            }
        }

        void nativeMoveThumb(UObject* object, Frame& stack, void*)
        {
            uint8_t move = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scale->moveThumb(move);
                refreshManager(scale);
            }
        }

        void nativeSetThumbStep(UObject* object, Frame& stack, void*)
        {
            int32_t value = vm::get<int32_t>(stack);
            vm::finish(stack);
            if (UScaleWindow* scale = self(object)) {
                scalewindow::thumbStep(scale) = value;
            }
        }

        void nativeSetScaleSounds(UObject* object, Frame& stack, void*)
        {
            UObject* sounds[3] = {};
            bool present[3] = {};
            for (int i = 0; i < 3; i++) {
                present[i] = optional(stack, sounds[i]);
            }
            vm::finish(stack);
            UScaleWindow* scale = self(object);
            if (!scale) {
                return;
            }
            UObject*& (*fields[3])(UObject*) = { scalewindow::setSound, scalewindow::clickSound, scalewindow::dragSound };
            for (int i = 0; i < 3; i++) {
                if (present[i]) {
                    fields[i](scale) = sounds[i];
                }
            }
        }

        void nativePlayScaleSound(UObject* object, Frame& stack, void*)
        {
            UObject* sound = vm::get<UObject*>(stack);
            float volume = 1;
            float pitch = 1;
            optional(stack, volume);
            optional(stack, pitch);
            vm::finish(stack);
            playSound(self(object), sound, volume, pitch);
        }

        UScaleManagerWindow* manager(UObject* object)
        {
            return cast<UScaleManagerWindow>(object);
        }

        void nativeSetScaleButtons(UObject* object, Frame& stack, void*)
        {
            UObject* dec = vm::get<UObject*>(stack);
            UObject* inc = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                scalemanagerwindow::decButton(window) = dec;
                scalemanagerwindow::incButton(window) = inc;
                window->askParentForReconfigure();
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeSetValueField(UObject* object, Frame& stack, void*)
        {
            UObject* field = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                scalemanagerwindow::valueField(window) = field;
                window->updateValueField();
                window->askParentForReconfigure();
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeSetScale(UObject* object, Frame& stack, void*)
        {
            UObject* scale = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                scalemanagerwindow::Scale(window) = scale;
                window->updateValueField();
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeSetManagerOrientation(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                scalemanagerwindow::orientation(window) = value;
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void managerFlag(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = optionalBoolDefault(stack, true);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                field(window) = value ? 1 : 0;
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeStretchScaleField(UObject* object, Frame& stack, void*)
        {
            managerFlag(object, stack, scalemanagerwindow::bStretchScaleField);
        }

        void nativeStretchValueField(UObject* object, Frame& stack, void*)
        {
            managerFlag(object, stack, scalemanagerwindow::bStretchValueField);
        }

        void nativeSetManagerMargins(UObject* object, Frame& stack, void*)
        {
            float w = 0;
            float h = 0;
            bool hasW = optional(stack, w);
            bool hasH = optional(stack, h);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                if (hasW) {
                    scalemanagerwindow::marginWidth(window) = w;
                }
                if (hasH) {
                    scalemanagerwindow::marginHeight(window) = h;
                }
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeSetMarginSpacing(UObject* object, Frame& stack, void*)
        {
            float value = 0;
            bool has = optional(stack, value);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                if (has) {
                    scalemanagerwindow::Spacing(window) = value;
                }
                window::bNeedsReconfigure(window) = 1;
            }
        }

        void nativeSetManagerAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t h = vm::get<uint8_t>(stack);
            uint8_t v = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UScaleManagerWindow* window = manager(object)) {
                scalemanagerwindow::childHAlign(window) = h;
                scalemanagerwindow::childVAlign(window) = v;
            }
        }
    }

    void registerScaleNatives()
    {
        vm::registerNative(1620, nativeSetScaleOrientation);
        vm::registerNative(1621, nativeSetScaleTexture);
        vm::registerNative(1622, nativeSetThumbTexture);
        vm::registerNative(1623, nativeSetTickTexture);
        vm::registerNative(1624, nativeSetThumbCaps);
        vm::registerNative(1625, nativeEnableStretchedScale);
        vm::registerNative(1626, nativeSetBorderPattern);
        vm::registerNative(1627, nativeSetScaleBorder);
        vm::registerNative(1628, nativeSetThumbBorder);
        vm::registerNative(1629, nativeSetScaleStyle);
        vm::registerNative(1630, nativeSetThumbStyle);
        vm::registerNative(1631, nativeSetTickStyle);
        vm::registerNative(1632, nativeSetScaleColor);
        vm::registerNative(1633, nativeSetThumbColor);
        vm::registerNative(1634, nativeSetTickColor);
        vm::registerNative(1635, nativeSetScaleMargins);
        vm::registerNative(1636, nativeSetNumTicks);
        vm::registerNative(1637, nativeGetNumTicks);
        vm::registerNative(1638, nativeSetThumbSpan);
        vm::registerNative(1639, nativeGetThumbSpan);
        vm::registerNative(1640, nativeSetTickPosition);
        vm::registerNative(1641, nativeGetTickPosition);
        vm::registerNative(1642, nativeSetValueRange);
        vm::registerNative(1643, nativeSetValue);
        vm::registerNative(1644, nativeGetValue);
        vm::registerNative(1645, nativeGetValues);
        vm::registerNative(1646, nativeSetValueFormat);
        vm::registerNative(1647, nativeGetValueString);
        vm::registerNative(1648, nativeSetEnumeration);
        vm::registerNative(1649, nativeClearAllEnumerations);
        vm::registerNative(1650, nativeMoveThumb);
        vm::registerNative(1651, nativeSetThumbStep);
        vm::registerNative(1652, nativeSetScaleSounds);
        vm::registerNative(1653, nativePlayScaleSound);
        vm::registerNative(1660, nativeSetScaleButtons);
        vm::registerNative(1661, nativeSetValueField);
        vm::registerNative(1662, nativeSetScale);
        vm::registerNative(1663, nativeSetManagerOrientation);
        vm::registerNative(1664, nativeStretchScaleField);
        vm::registerNative(1665, nativeStretchValueField);
        vm::registerNative(1666, nativeSetManagerMargins);
        vm::registerNative(1667, nativeSetMarginSpacing);
        vm::registerNative(1668, nativeSetManagerAlignments);
    }
}
