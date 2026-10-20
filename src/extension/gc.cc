#include "extension/windows.h"

#include "core/objects.h"
#include "engine/resources.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace dx::extension {
    namespace {
        const Color paletteColors[] = {
            { 0, 0, 0, 255 },
            { 255, 255, 255, 255 },
            { 255, 0, 0, 255 },
            { 0, 255, 0, 255 },
            { 255, 255, 0, 255 },
            { 0, 0, 255, 255 },
            { 255, 0, 255, 255 },
            { 0, 255, 255, 255 },
        };

        int hexValue(char c)
        {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            return -1;
        }

        UFont* fontOf(UObject* font)
        {
            UFont* value = cast<UFont>(font);
            if (value) {
                objects::ensureLoaded(value);
            }
            return value;
        }

        UGC* self(UObject* object)
        {
            return cast<UGC>(object);
        }
    }

    namespace {
        struct TextState {
            bool bold = false;
            bool useColor = false;
            bool afterNewline = false;
            bool underline = false;
            Color color {};
        };

        struct Glyph {
            UTexture* texture = nullptr;
            float u = 0;
            float v = 0;
            float width = 0;
            float height = 0;
        };

        Glyph glyphOf(UObject* fontObject, char c)
        {
            Glyph result;
            UFont* font = fontOf(fontObject);
            if (!font) {
                return result;
            }
            const FontCharacter* glyph = font->glyph(static_cast<uint8_t>(c), &result.texture);
            if (glyph) {
                result.u = static_cast<float>(glyph->u);
                result.v = static_cast<float>(glyph->v);
                result.width = static_cast<float>(glyph->width);
                result.height = static_cast<float>(glyph->height);
            }
            return result;
        }

        UObject* stateFont(const TextContext& context, const TextState& state)
        {
            return state.bold ? context.boldFont : context.normalFont;
        }

        char lowerAscii(char c)
        {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        }

        bool colorByte(const std::string& text, size_t& pos, size_t end, uint8_t& value)
        {
            value = 0;
            if (pos >= end || hexValue(text[pos]) < 0) {
                return false;
            }
            value = static_cast<uint8_t>(hexValue(text[pos]) << 4);
            pos++;
            if (pos >= end || hexValue(text[pos]) < 0) {
                return false;
            }
            value = static_cast<uint8_t>(value | hexValue(text[pos]));
            pos++;
            return true;
        }

        char nextChar(const TextContext& context, const std::string& text, size_t& pos, size_t end, TextState& state)
        {
            state.underline = false;
            while (pos < end) {
                if (!context.parse || text[pos] != '|') {
                    return text[pos++];
                }
                pos++;
                char code = 0;
                bool inverse = false;
                if (pos < end) {
                    code = lowerAscii(text[pos]);
                    if (code == '!') {
                        inverse = true;
                        pos++;
                        if (pos < end) {
                            code = lowerAscii(text[pos]);
                        }
                    }
                }
                if (pos >= end) {
                    break;
                }
                if (code == 'b') {
                    pos++;
                    state.bold = !inverse;
                    continue;
                }
                if (code == 'c') {
                    pos++;
                    if (inverse) {
                        state.color = context.textColor;
                        state.useColor = false;
                    } else {
                        uint8_t r = 0;
                        uint8_t g = 0;
                        uint8_t b = 0;
                        if (colorByte(text, pos, end, r) && colorByte(text, pos, end, g)) {
                            colorByte(text, pos, end, b);
                        }
                        state.color = { r, g, b, 255 };
                        state.useColor = true;
                    }
                    continue;
                }
                if (code == 'p') {
                    pos++;
                    if (pos >= end) {
                        continue;
                    }
                    if (inverse) {
                        state.color = context.textColor;
                        state.useColor = false;
                        continue;
                    }
                    char digit = text[pos];
                    if (digit >= '0' && digit < '8') {
                        pos++;
                        state.color = paletteColors[digit - '0'];
                        state.useColor = true;
                    }
                    continue;
                }
                if (code == '&') {
                    pos++;
                    state.underline = true;
                    continue;
                }
                return text[pos++];
            }
            if (pos > end) {
                pos = end;
            }
            return 0;
        }

        struct ParsedLine {
            bool valid = false;
            size_t start = 0;
            size_t end = 0;
            size_t next = 0;
            float width = 0;
            TextState state;
        };

        bool isWhitespace(char c)
        {
            return (c >= 9 && c <= 13) || c == ' ';
        }

        ParsedLine parseLine(const TextContext& context, const std::string& text, size_t start, size_t end, float width, TextState state)
        {
            if (width <= 0) {
                width = 500000.0f;
            }
            UObject* font = stateFont(context, state);
            bool hadNewline = state.afterNewline;
            state.afterNewline = false;
            float current = 0;
            float widthAtBreak = 0;
            bool inSpace = true;
            bool first = true;
            bool wrapPending = false;
            bool hasBreak = false;
            size_t breakPos = start;
            size_t wordStart = start;
            TextState wordState;
            ParsedLine line;
            line.start = start;
            size_t pos = start;
            while (true) {
                TextState previous = state;
                size_t before = pos;
                char c = nextChar(context, text, pos, end, state);
                if (c == 0 || c == '\n') {
                    if (wrapPending) {
                        line.end = breakPos;
                        line.width = widthAtBreak;
                    } else {
                        line.end = before;
                        line.width = current;
                    }
                    if (c == '\n') {
                        state.afterNewline = true;
                    }
                    line.next = pos;
                    break;
                }
                if (isWhitespace(c)) {
                    if (!inSpace) {
                        inSpace = true;
                        hasBreak = true;
                        breakPos = before;
                        widthAtBreak = current;
                    }
                } else if (inSpace) {
                    if (wrapPending) {
                        line.end = breakPos;
                        line.width = widthAtBreak;
                        state = previous;
                        line.next = before;
                        break;
                    }
                    inSpace = false;
                    wordStart = before;
                    wordState = previous;
                }
                if (state.bold != previous.bold) {
                    font = stateFont(context, state);
                }
                if (font) {
                    float w = current + glyphOf(font, c).width;
                    if (w > width && !first) {
                        if (!hasBreak) {
                            line.end = before;
                            line.width = current;
                            state = previous;
                            line.next = before;
                            break;
                        }
                        if (wordStart > breakPos) {
                            line.end = breakPos;
                            line.width = widthAtBreak;
                            state = wordState;
                            line.next = wordStart;
                            break;
                        }
                        wrapPending = true;
                    }
                    current = w;
                }
                first = false;
            }
            line.state = state;
            line.valid = line.next > start || hadNewline;
            return line;
        }

        template <typename Visit>
        void forEachLine(const TextContext& context, const std::string& text, float width, Visit visit)
        {
            TextState state;
            size_t pos = 0;
            size_t end = text.size();
            while (true) {
                TextState lineState = state;
                ParsedLine line = parseLine(context, text, pos, end, width, lineState);
                if (!line.valid) {
                    break;
                }
                visit(line, state);
                state = line.state;
                pos = line.next;
            }
        }

        float lineGlyphHeight(const TextContext& context, const std::string& text, const ParsedLine& line, TextState state)
        {
            float height = 0;
            UObject* font = stateFont(context, state);
            size_t pos = line.start;
            while (char c = nextChar(context, text, pos, line.end, state)) {
                font = stateFont(context, state);
                if (c == '\n' || !font) {
                    continue;
                }
                height = std::max(height, glyphOf(font, c).height);
            }
            if (height < 1.0f && font) {
                height = glyphOf(font, ' ').height;
            }
            return height;
        }
    }

    TextContext textContextOf(UWindow* window)
    {
        TextContext context;
        context.normalFont = window::normalFont(window);
        context.boldFont = window::boldFont(window);
        context.parse = window::bSpecialText(window) != 0;
        context.vSpacing = window::textVSpacing(window);
        context.textColor = window::TextColor(window);
        return context;
    }

    void measureText(const TextContext& context, float wrapWidth, const std::string& text, float& outWidth, float& outHeight)
    {
        outWidth = 0;
        outHeight = 0;
        forEachLine(context, text, wrapWidth, [&](const ParsedLine& line, const TextState& state) {
            outWidth = std::max(outWidth, line.width);
            outHeight += lineGlyphHeight(context, text, line, state) + context.vSpacing;
        });
    }

    std::vector<TextRow> textRows(const TextContext& context, float wrapWidth, const std::string& text)
    {
        std::vector<TextRow> rows;
        forEachLine(context, text, wrapWidth, [&](const ParsedLine& line, const TextState& state) {
            TextRow row;
            row.start = line.start;
            row.end = line.end;
            row.width = line.width;
            row.height = lineGlyphHeight(context, text, line, state);
            row.bold = state.bold;
            row.useColor = state.useColor;
            row.color = state.color;
            rows.push_back(row);
        });
        return rows;
    }

    void measureString(UObject* font, const std::string& text, float& width, float& height)
    {
        width = 0;
        height = 0;
        UFont* value = fontOf(font);
        if (!value) {
            return;
        }
        for (char c : text) {
            const FontCharacter* glyph = value->glyph(static_cast<uint8_t>(c), nullptr);
            if (glyph) {
                width += static_cast<float>(glyph->width);
                height = std::max(height, static_cast<float>(glyph->height));
            }
        }
    }

    void UGC::resetState(UWindow* window, float newOffsetX, float newOffsetY)
    {
        gc::HAlign(this) = HALIGN_Left;
        gc::VAlign(this) = VALIGN_Top;
        gc::PolyFlags(this) = 0;
        gc::textPolyFlags(this) = 0;
        gc::Style(this) = DSTY_Normal;
        gc::bDrawEnabled(this) = 1;
        gc::bMasked(this) = 0;
        gc::bModulated(this) = 0;
        gc::bTranslucent(this) = 0;
        gc::bSmoothed(this) = 0;
        gc::bTextTranslucent(this) = 0;
        gc::bWordWrap(this) = 1;
        gc::bParseMetachars(this) = 1;
        gc::baselineOffset(this) = 2.0f;
        gc::underlineHeight(this) = 1.0f;
        gc::textVSpacing(this) = 1.0f;
        gc::TextColor(this) = { 0, 255, 0, 255 };
        gc::tileColor(this) = { 255, 255, 255, 255 };
        gc::normalFont(this) = nullptr;
        gc::boldFont(this) = nullptr;
        specialText = false;
        if (window) {
            specialText = window::bSpecialText(window) != 0;
            gc::bParseMetachars(this) = specialText ? 1 : 0;
            gc::bTextTranslucent(this) = window::bTextTranslucent(window);
            gc::baselineOffset(this) = window->baselineOffset;
            gc::underlineHeight(this) = window->underlineHeight;
            gc::normalFont(this) = window::normalFont(window);
            gc::boldFont(this) = window::boldFont(window);
            gc::TextColor(this) = window::TextColor(window);
            gc::textVSpacing(this) = window::textVSpacing(window);
            gc::tileColor(this) = window::tileColor(window);
        }
        offsetX = newOffsetX;
        offsetY = newOffsetY;
    }

    void UGC::resetClip(float clipX, float clipY, float clipWidth, float clipHeight)
    {
        clipStack.clear();
        clipLeft = clipX;
        clipTop = clipY;
        clipRight = clipX + clipWidth;
        clipBottom = clipY + clipHeight;
    }

    void UGC::pushClip(float clipX, float clipY, float clipWidth, float clipHeight)
    {
        clipStack.push_back({ clipLeft, clipTop, clipRight, clipBottom });
        clipLeft = std::max(clipLeft, clipX);
        clipTop = std::max(clipTop, clipY);
        clipRight = std::max(std::min(clipRight, clipX + clipWidth), clipLeft);
        clipBottom = std::max(std::min(clipBottom, clipY + clipHeight), clipTop);
    }

    void UGC::popClip()
    {
        if (clipStack.empty()) {
            return;
        }
        clipLeft = clipStack.back()[0];
        clipTop = clipStack.back()[1];
        clipRight = clipStack.back()[2];
        clipBottom = clipStack.back()[3];
        clipStack.pop_back();
    }

    BlendStyle UGC::tileStyle()
    {
        if (gc::bTranslucent(this)) {
            return BlendStyle::Translucent;
        }
        if (gc::bModulated(this)) {
            return BlendStyle::Modulated;
        }
        if (gc::bMasked(this)) {
            return BlendStyle::Masked;
        }
        return BlendStyle::Normal;
    }

    BlendStyle UGC::textStyle()
    {
        return gc::bTextTranslucent(this) ? BlendStyle::Translucent : BlendStyle::Masked;
    }

    bool UGC::smoothed()
    {
        return gc::bSmoothed(this) != 0;
    }

    void UGC::drawTile(UObject* texture, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, const Color& color, BlendStyle style, bool smooth)
    {
        Canvas2D* canvas = state().canvas;
        UTexture* tex = cast<UTexture>(texture);
        if (!canvas || !tex || destWidth <= 0 || destHeight <= 0 || !gc::bDrawEnabled(this)) {
            return;
        }
        objects::ensureLoaded(tex);
        float left = destX;
        float top = destY;
        float right = destX + destWidth;
        float bottom = destY + destHeight;
        float scaleU = srcWidth / destWidth;
        float scaleV = srcHeight / destHeight;
        if (left < clipLeft) {
            srcX += (clipLeft - left) * scaleU;
            left = clipLeft;
        }
        if (top < clipTop) {
            srcY += (clipTop - top) * scaleV;
            top = clipTop;
        }
        if (right > clipRight) {
            right = clipRight;
        }
        if (bottom > clipBottom) {
            bottom = clipBottom;
        }
        if (right <= left || bottom <= top) {
            return;
        }
        float scale = virtualScale();
        canvas->drawTile(tex, left * scale, top * scale, (right - left) * scale, (bottom - top) * scale, srcX, srcY, (right - left) * scaleU, (bottom - top) * scaleV, toLinear(color), style, smooth);
    }

    void UGC::drawRect(float destX, float destY, float destWidth, float destHeight, const Color& color, BlendStyle style)
    {
        Canvas2D* canvas = state().canvas;
        if (!canvas || !gc::bDrawEnabled(this)) {
            return;
        }
        float left = std::max(destX, clipLeft);
        float top = std::max(destY, clipTop);
        float right = std::min(destX + destWidth, clipRight);
        float bottom = std::min(destY + destHeight, clipBottom);
        if (right <= left || bottom <= top) {
            return;
        }
        float scale = virtualScale();
        canvas->drawRect(left * scale, top * scale, (right - left) * scale, (bottom - top) * scale, toLinear(color), style);
    }

    TextContext UGC::textContext()
    {
        TextContext context;
        context.normalFont = gc::normalFont(this);
        context.boldFont = gc::boldFont(this);
        context.parse = gc::bParseMetachars(this) != 0;
        context.vSpacing = gc::textVSpacing(this);
        context.textColor = gc::TextColor(this);
        return context;
    }

    void UGC::textExtent(float destWidth, const std::string& text, float& outWidth, float& outHeight)
    {
        measureText(textContext(), destWidth, text, outWidth, outHeight);
    }

    float UGC::drawChar(UObject* font, char c, const Color& color, bool underline, float x, float y)
    {
        Glyph glyph = glyphOf(font, c);
        BlendStyle style = textStyle();
        if (glyph.texture && glyph.width > 0 && glyph.height > 0) {
            drawTile(glyph.texture, std::trunc(x + 0.1f), std::trunc(y + 0.1f), glyph.width, glyph.height, glyph.u, glyph.v, glyph.width, glyph.height, color, style, false);
            if (underline && gc::underlineHeight(this) > 0) {
                static UObject* solid = nullptr;
                if (!solid) {
                    solid = objects::loadObject("Extension.Solid", nullptr);
                    objects::ensureLoaded(solid);
                }
                float width = glyph.width - 1.0f;
                float height = gc::underlineHeight(this);
                if (solid && width > 0) {
                    drawTile(solid, std::trunc(x + 0.1f), std::trunc(y + glyph.height - gc::baselineOffset(this) + 0.1f), width, height, 0, 0, width, height, color, style, false);
                }
            }
        }
        return glyph.height;
    }

    void UGC::drawText(float destX, float destY, float destWidth, float destHeight, const std::string& text)
    {
        if (!gc::bDrawEnabled(this)) {
            return;
        }
        TextContext context = textContext();
        float wrap = gc::bWordWrap(this) ? std::trunc(destWidth) : 0.0f;
        pushClip(offsetX + destX, offsetY + destY, destWidth, destHeight);
        float y = offsetY + destY;
        uint8_t valign = gc::VAlign(this);
        if (valign == VALIGN_Center || valign == VALIGN_Bottom) {
            float extentWidth = 0;
            float extentHeight = 0;
            measureText(context, wrap, text, extentWidth, extentHeight);
            if (valign == VALIGN_Center) {
                y += static_cast<float>(static_cast<int32_t>(std::trunc(destHeight - extentHeight)) / 2);
            } else {
                y = y + destHeight - extentHeight;
            }
        }
        uint8_t halign = gc::HAlign(this);
        forEachLine(context, text, wrap, [&](const ParsedLine& line, TextState state) {
            float x = offsetX + destX;
            if (halign == HALIGN_Center) {
                x += static_cast<float>(static_cast<int32_t>(std::trunc(destWidth - line.width)) / 2);
            } else if (halign == HALIGN_Right) {
                x += destWidth - line.width;
            }
            UObject* font = stateFont(context, state);
            float lineHeight = 0;
            size_t pos = line.start;
            while (char c = nextChar(context, text, pos, line.end, state)) {
                font = stateFont(context, state);
                if (c == '\n' || !font) {
                    continue;
                }
                lineHeight = std::max(lineHeight, drawChar(font, c, state.useColor ? state.color : context.textColor, state.underline, x, y));
                x += glyphOf(font, c).width;
            }
            if (lineHeight < 1.0f && font) {
                lineHeight = glyphOf(font, ' ').height;
            }
            y += lineHeight + context.vSpacing;
        });
        popClip();
    }

    void UGC::drawTextRow(float destX, float destY, const std::string& text, const TextRow& row)
    {
        if (!gc::bDrawEnabled(this)) {
            return;
        }
        TextContext context = textContext();
        TextState state;
        state.bold = row.bold;
        state.useColor = row.useColor;
        state.color = row.color;
        float x = offsetX + destX;
        float y = offsetY + destY;
        size_t pos = row.start;
        while (char c = nextChar(context, text, pos, row.end, state)) {
            UObject* font = stateFont(context, state);
            if (c == '\n' || !font) {
                continue;
            }
            drawChar(font, c, state.useColor ? state.color : context.textColor, state.underline, x, y);
            x += glyphOf(font, c).width;
        }
    }

    void UGC::drawPattern(float destX, float destY, float destWidth, float destHeight, float orgX, float orgY, UObject* texture)
    {
        drawTile(texture, offsetX + destX, offsetY + destY, destWidth, destHeight, orgX, orgY, destWidth, destHeight, gc::tileColor(this), tileStyle(), smoothed());
    }

    void UGC::drawStretched(float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, UObject* texture)
    {
        drawTile(texture, offsetX + destX, offsetY + destY, destWidth, destHeight, srcX, srcY, srcWidth, srcHeight, gc::tileColor(this), tileStyle(), smoothed());
    }

    void UGC::drawIcon(float destX, float destY, UObject* texture)
    {
        float w = textureWidth(texture);
        float h = textureHeight(texture);
        drawTile(texture, offsetX + destX, offsetY + destY, w, h, 0, 0, w, h, gc::tileColor(this), tileStyle(), smoothed());
    }

    void UGC::drawBox(float destX, float destY, float destWidth, float destHeight, float thickness, UObject* texture)
    {
        Color color = gc::tileColor(this);
        BlendStyle style = tileStyle();
        float x = offsetX + destX;
        float y = offsetY + destY;
        if (!texture) {
            drawRect(x, y, destWidth, thickness, color, style);
            drawRect(x, y + destHeight - thickness, destWidth, thickness, color, style);
            drawRect(x, y + thickness, thickness, destHeight - 2 * thickness, color, style);
            drawRect(x + destWidth - thickness, y + thickness, thickness, destHeight - 2 * thickness, color, style);
            return;
        }
        float w = textureWidth(texture);
        float h = textureHeight(texture);
        drawTile(texture, x, y, destWidth, thickness, 0, 0, w, h, color, style, smoothed());
        drawTile(texture, x, y + destHeight - thickness, destWidth, thickness, 0, 0, w, h, color, style, smoothed());
        drawTile(texture, x, y + thickness, thickness, destHeight - 2 * thickness, 0, 0, w, h, color, style, smoothed());
        drawTile(texture, x + destWidth - thickness, y + thickness, thickness, destHeight - 2 * thickness, 0, 0, w, h, color, style, smoothed());
    }

    void UGC::drawBorders(float destX, float destY, float destWidth, float destHeight, UObject* const* borders)
    {
        UObject* topLeft = borders[0];
        UObject* topRight = borders[1];
        UObject* bottomLeft = borders[2];
        UObject* bottomRight = borders[3];
        UObject* left = borders[4];
        UObject* right = borders[5];
        UObject* top = borders[6];
        UObject* bottom = borders[7];
        UObject* center = borders[8];
        Color color = gc::tileColor(this);
        BlendStyle style = tileStyle();
        bool smooth = smoothed();
        float x = offsetX + destX;
        float y = offsetY + destY;
        float leftWidth = std::max({ textureWidth(topLeft), textureWidth(bottomLeft), textureWidth(left) });
        float rightWidth = std::max({ textureWidth(topRight), textureWidth(bottomRight), textureWidth(right) });
        float topHeight = std::max({ textureHeight(topLeft), textureHeight(topRight), textureHeight(top) });
        float bottomHeight = std::max({ textureHeight(bottomLeft), textureHeight(bottomRight), textureHeight(bottom) });
        float innerWidth = std::max(destWidth - leftWidth - rightWidth, 0.0f);
        float innerHeight = std::max(destHeight - topHeight - bottomHeight, 0.0f);
        auto stretched = [&](UObject* texture, float tx, float ty, float tw, float th) {
            if (texture) {
                drawTile(texture, tx, ty, tw, th, 0, 0, textureWidth(texture), textureHeight(texture), color, style, smooth);
            }
        };
        auto tiled = [&](UObject* texture, float tx, float ty, float tw, float th) {
            if (texture) {
                drawTile(texture, tx, ty, tw, th, 0, 0, tw, th, color, style, smooth);
            }
        };
        stretched(center, x + leftWidth, y + topHeight, innerWidth, innerHeight);
        tiled(top, x + leftWidth, y, innerWidth, textureHeight(top));
        tiled(bottom, x + leftWidth, y + destHeight - textureHeight(bottom), innerWidth, textureHeight(bottom));
        tiled(left, x, y + topHeight, textureWidth(left), innerHeight);
        tiled(right, x + destWidth - textureWidth(right), y + topHeight, textureWidth(right), innerHeight);
        stretched(topLeft, x, y, textureWidth(topLeft), textureHeight(topLeft));
        stretched(topRight, x + destWidth - textureWidth(topRight), y, textureWidth(topRight), textureHeight(topRight));
        stretched(bottomLeft, x, y + destHeight - textureHeight(bottomLeft), textureWidth(bottomLeft), textureHeight(bottomLeft));
        stretched(bottomRight, x + destWidth - textureWidth(bottomRight), y + destHeight - textureHeight(bottomRight), textureWidth(bottomRight), textureHeight(bottomRight));
    }

    void UGC::setStyle(uint8_t style)
    {
        gc::Style(this) = style;
        gc::PolyFlags(this) = 0;
        gc::textPolyFlags(this) = 0;
        gc::bDrawEnabled(this) = style != DSTY_None ? 1 : 0;
        gc::bMasked(this) = style == DSTY_Masked ? 1 : 0;
        gc::bTranslucent(this) = style == DSTY_Translucent ? 1 : 0;
        gc::bModulated(this) = style == DSTY_Modulated ? 1 : 0;
    }

    void UGC::setFonts(UObject* normal, UObject* bold)
    {
        gc::normalFont(this) = normal;
        gc::boldFont(this) = bold;
    }

    namespace {
        void boolSetter(UObject* object, Frame& stack, uint32_t& (*field)(UObject*))
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                field(gc) = value ? 1 : 0;
            }
        }

        void boolGetter(UObject* object, Frame& stack, void* result, uint32_t& (*field)(UObject*))
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::retBool(result, gc && field(gc));
        }

        void nativeEnableSmoothing(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bSmoothed);
        }

        void nativeIsSmoothingEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bSmoothed);
        }

        void nativeSetStyle(UObject* object, Frame& stack, void*)
        {
            uint8_t style = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->setStyle(style);
            }
        }

        void nativeGetStyle(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::ret<uint8_t>(result, gc ? gc::Style(gc) : static_cast<uint8_t>(0));
        }

        void nativeEnableDrawing(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bDrawEnabled);
        }

        void nativeIsDrawingEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bDrawEnabled);
        }

        void nativeEnableMasking(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bMasked);
        }

        void nativeIsMaskingEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bMasked);
        }

        void nativeEnableTranslucency(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bTranslucent);
        }

        void nativeIsTranslucencyEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bTranslucent);
        }

        void nativeEnableModulation(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bModulated);
        }

        void nativeIsModulationEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bModulated);
        }

        void nativeEnableTranslucentText(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bTextTranslucent);
        }

        void nativeIsTranslucentTextEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bTextTranslucent);
        }

        void nativeEnableWordWrap(UObject* object, Frame& stack, void*)
        {
            boolSetter(object, stack, gc::bWordWrap);
        }

        void nativeIsWordWrapEnabled(UObject* object, Frame& stack, void* result)
        {
            boolGetter(object, stack, result, gc::bWordWrap);
        }

        void nativeEnableSpecialText(UObject* object, Frame& stack, void*)
        {
            bool value = vm::getBool(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->specialText = value;
            }
        }

        void nativeIsSpecialTextEnabled(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::retBool(result, gc && gc->specialText);
        }

        void nativeSetTileColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::tileColor(gc) = color;
            }
        }

        void nativeGetTileColor(UObject* object, Frame& stack, void*)
        {
            Color* out = vm::getRef<Color>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                *out = gc::tileColor(gc);
            }
        }

        void nativeSetTextColor(UObject* object, Frame& stack, void*)
        {
            Color color = vm::get<Color>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::TextColor(gc) = color;
            }
        }

        void nativeGetTextColor(UObject* object, Frame& stack, void*)
        {
            Color* out = vm::getRef<Color>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                *out = gc::TextColor(gc);
            }
        }

        void nativeSetFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->setFonts(font, font);
            }
        }

        void nativeSetNormalFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::normalFont(gc) = font;
            }
        }

        void nativeSetBoldFont(UObject* object, Frame& stack, void*)
        {
            UObject* font = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::boldFont(gc) = font;
            }
        }

        void nativeSetFonts(UObject* object, Frame& stack, void*)
        {
            UObject* normal = vm::get<UObject*>(stack);
            UObject* bold = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->setFonts(normal, bold);
            }
        }

        void nativeGetFonts(UObject* object, Frame& stack, void*)
        {
            UObject** normal = vm::getRef<UObject*>(stack);
            UObject** bold = vm::getRef<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                *normal = gc::normalFont(gc);
                *bold = gc::boldFont(gc);
            }
        }

        void nativeSetTextVSpacing(UObject* object, Frame& stack, void*)
        {
            float value = vm::get<float>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::textVSpacing(gc) = value;
            }
        }

        void nativeGetTextVSpacing(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::ret<float>(result, gc ? gc::textVSpacing(gc) : 0.0f);
        }

        void nativeSetHorizontalAlignment(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::HAlign(gc) = value;
            }
        }

        void nativeGetHorizontalAlignment(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::ret<uint8_t>(result, gc ? gc::HAlign(gc) : static_cast<uint8_t>(0));
        }

        void nativeSetVerticalAlignment(UObject* object, Frame& stack, void*)
        {
            uint8_t value = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::VAlign(gc) = value;
            }
        }

        void nativeGetVerticalAlignment(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            vm::ret<uint8_t>(result, gc ? gc::VAlign(gc) : static_cast<uint8_t>(0));
        }

        void nativeSetAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t halign = vm::get<uint8_t>(stack);
            uint8_t valign = vm::get<uint8_t>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc::HAlign(gc) = halign;
                gc::VAlign(gc) = valign;
            }
        }

        void nativeGetAlignments(UObject* object, Frame& stack, void*)
        {
            uint8_t* halign = vm::getRef<uint8_t>(stack);
            uint8_t* valign = vm::getRef<uint8_t>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                *halign = gc::HAlign(gc);
                *valign = gc::VAlign(gc);
            }
        }

        void nativeSetBaselineData(UObject* object, Frame& stack, void*)
        {
            float baseline = 0;
            float underline = 0;
            bool hasBaseline = optional(stack, baseline);
            bool hasUnderline = optional(stack, underline);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                if (hasBaseline) {
                    gc::baselineOffset(gc) = baseline;
                }
                if (hasUnderline) {
                    gc::underlineHeight(gc) = underline;
                }
            }
        }

        void nativeCopyGC(UObject* object, Frame& stack, void*)
        {
            UObject* other = vm::get<UObject*>(stack);
            vm::finish(stack);
            UGC* gc = self(object);
            UGC* source = cast<UGC>(other);
            if (gc && source && gc != source) {
                gc->cls->copyValue(gc->data, source->data);
                gc->offsetX = source->offsetX;
                gc->offsetY = source->offsetY;
                gc->specialText = source->specialText;
            }
        }

        void nativePushGC(UObject* object, Frame& stack, void* result)
        {
            vm::finish(stack);
            UGC* gc = self(object);
            if (!gc) {
                vm::ret<int32_t>(result, 0);
                return;
            }
            UGC::Saved saved;
            saved.data.assign(gc->data, gc->data + gc->cls->size);
            saved.offsetX = gc->offsetX;
            saved.offsetY = gc->offsetY;
            saved.specialText = gc->specialText;
            gc->savedStates.push_back(std::move(saved));
            gc->clipStack.push_back({ gc->clipLeft, gc->clipTop, gc->clipRight, gc->clipBottom });
            vm::ret<int32_t>(result, static_cast<int32_t>(gc->savedStates.size()));
        }

        void nativePopGC(UObject* object, Frame& stack, void*)
        {
            int32_t number = -1;
            optional(stack, number);
            vm::finish(stack);
            UGC* gc = self(object);
            if (!gc || gc->savedStates.empty()) {
                return;
            }
            size_t target = number > 0 ? static_cast<size_t>(number) : gc->savedStates.size();
            while (gc->savedStates.size() > target) {
                gc->savedStates.pop_back();
                gc->popClip();
            }
            UGC::Saved& saved = gc->savedStates.back();
            std::memcpy(gc->data, saved.data.data(), std::min(saved.data.size(), gc->cls->size));
            gc->offsetX = saved.offsetX;
            gc->offsetY = saved.offsetY;
            gc->specialText = saved.specialText;
            gc->savedStates.pop_back();
            gc->popClip();
        }

        void nativeIntersect(UObject* object, Frame& stack, void*)
        {
            float clipX = vm::get<float>(stack);
            float clipY = vm::get<float>(stack);
            float clipWidth = vm::get<float>(stack);
            float clipHeight = vm::get<float>(stack);
            vm::finish(stack);
            UGC* gc = self(object);
            if (!gc) {
                return;
            }
            float left = std::max(gc->clipLeft, gc->offsetX + clipX);
            float top = std::max(gc->clipTop, gc->offsetY + clipY);
            gc->clipRight = std::max(std::min(gc->clipRight, gc->offsetX + clipX + clipWidth), left);
            gc->clipBottom = std::max(std::min(gc->clipBottom, gc->offsetY + clipY + clipHeight), top);
            gc->clipLeft = left;
            gc->clipTop = top;
        }

        void nativeGetTextExtent(UObject* object, Frame& stack, void*)
        {
            float destWidth = vm::get<float>(stack);
            float* outWidth = vm::getRef<float>(stack);
            float* outHeight = vm::getRef<float>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            float w = 0;
            float h = 0;
            if (UGC* gc = self(object)) {
                gc->textExtent(destWidth, text, w, h);
            }
            *outWidth = w;
            *outHeight = h;
        }

        void nativeGetFontHeight(UObject* object, Frame& stack, void* result)
        {
            bool includeSpace = optionalBoolDefault(stack, true);
            vm::finish(stack);
            UGC* gc = self(object);
            float height = gc ? fontHeight(gc::normalFont(gc)) : 0.0f;
            if (gc && includeSpace) {
                height += gc::textVSpacing(gc);
            }
            vm::ret<float>(result, height);
        }

        void nativeDrawText(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            std::string text = vm::get<std::string>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawText(destX, destY, destWidth, destHeight, text);
            }
        }

        void nativeDrawIcon(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawIcon(destX, destY, texture);
            }
        }

        void nativeDrawTexture(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            float srcX = vm::get<float>(stack);
            float srcY = vm::get<float>(stack);
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawStretched(destX, destY, destWidth, destHeight, srcX, srcY, destWidth, destHeight, texture);
            }
        }

        void nativeDrawPattern(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            float orgX = vm::get<float>(stack);
            float orgY = vm::get<float>(stack);
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawPattern(destX, destY, destWidth, destHeight, orgX, orgY, texture);
            }
        }

        void nativeDrawBox(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            vm::get<float>(stack);
            vm::get<float>(stack);
            float thickness = vm::get<float>(stack);
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawBox(destX, destY, destWidth, destHeight, thickness, texture);
            }
        }

        void nativeDrawStretchedTexture(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            float srcX = vm::get<float>(stack);
            float srcY = vm::get<float>(stack);
            float srcWidth = vm::get<float>(stack);
            float srcHeight = vm::get<float>(stack);
            UObject* texture = vm::get<UObject*>(stack);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawStretched(destX, destY, destWidth, destHeight, srcX, srcY, srcWidth, srcHeight, texture);
            }
        }

        void nativeDrawActor(UObject* object, Frame& stack, void*)
        {
            ActorDrawRequest request;
            request.actor = vm::get<UObject*>(stack);
            request.clearZ = optionalBoolDefault(stack, false);
            request.constrain = optionalBoolDefault(stack, false);
            request.unlit = optionalBoolDefault(stack, false);
            optional(stack, request.drawScale);
            optional(stack, request.scaleGlow);
            optional(stack, request.skin);
            vm::finish(stack);
            UGC* gc = self(object);
            State& s = state();
            if (!gc || !s.canvas || !s.actorDrawer || !request.actor) {
                return;
            }
            float scale = virtualScale();
            request.x = gc->clipLeft * scale;
            request.y = gc->clipTop * scale;
            request.width = (gc->clipRight - gc->clipLeft) * scale;
            request.height = (gc->clipBottom - gc->clipTop) * scale;
            s.actorDrawer(*s.canvas, request);
        }

        void nativeDrawBorders(UObject* object, Frame& stack, void*)
        {
            float destX = vm::get<float>(stack);
            float destY = vm::get<float>(stack);
            float destWidth = vm::get<float>(stack);
            float destHeight = vm::get<float>(stack);
            vm::get<float>(stack);
            vm::get<float>(stack);
            vm::get<float>(stack);
            vm::get<float>(stack);
            stack.propAddr = nullptr;
            UObject* first = nullptr;
            stack.step(stack.object, &first);
            UObject* borders[9] = {};
            if (stack.propAddr && stack.prop && stack.prop->arrayDim >= 9) {
                std::memcpy(borders, stack.propAddr, sizeof(borders));
            } else {
                borders[0] = first;
            }
            optionalBoolDefault(stack, false);
            optionalBoolDefault(stack, false);
            vm::finish(stack);
            if (UGC* gc = self(object)) {
                gc->drawBorders(destX, destY, destWidth, destHeight, borders);
            }
        }

        void nativeClearZ(UObject*, Frame& stack, void*)
        {
            vm::finish(stack);
        }
    }

    void registerGCNatives()
    {
        vm::registerNative(1200, nativeIntersect);
        vm::registerNative(1210, nativeEnableSmoothing);
        vm::registerNative(1211, nativeIsSmoothingEnabled);
        vm::registerNative(1212, nativeSetStyle);
        vm::registerNative(1213, nativeGetStyle);
        vm::registerNative(1214, nativeEnableDrawing);
        vm::registerNative(1215, nativeIsDrawingEnabled);
        vm::registerNative(1216, nativeEnableMasking);
        vm::registerNative(1217, nativeIsMaskingEnabled);
        vm::registerNative(1218, nativeEnableTranslucency);
        vm::registerNative(1219, nativeIsTranslucencyEnabled);
        vm::registerNative(1220, nativeEnableModulation);
        vm::registerNative(1221, nativeIsModulationEnabled);
        vm::registerNative(1230, nativeSetTileColor);
        vm::registerNative(1231, nativeGetTileColor);
        vm::registerNative(1240, nativeSetTextColor);
        vm::registerNative(1241, nativeGetTextColor);
        vm::registerNative(1242, nativeSetFont);
        vm::registerNative(1243, nativeSetNormalFont);
        vm::registerNative(1244, nativeSetBoldFont);
        vm::registerNative(1245, nativeSetFonts);
        vm::registerNative(1246, nativeGetFonts);
        vm::registerNative(1247, nativeEnableTranslucentText);
        vm::registerNative(1248, nativeIsTranslucentTextEnabled);
        vm::registerNative(1250, nativeSetTextVSpacing);
        vm::registerNative(1251, nativeGetTextVSpacing);
        vm::registerNative(1252, nativeSetHorizontalAlignment);
        vm::registerNative(1253, nativeGetHorizontalAlignment);
        vm::registerNative(1254, nativeSetVerticalAlignment);
        vm::registerNative(1255, nativeGetVerticalAlignment);
        vm::registerNative(1256, nativeSetAlignments);
        vm::registerNative(1257, nativeGetAlignments);
        vm::registerNative(1258, nativeEnableWordWrap);
        vm::registerNative(1259, nativeIsWordWrapEnabled);
        vm::registerNative(1260, nativeEnableSpecialText);
        vm::registerNative(1261, nativeIsSpecialTextEnabled);
        vm::registerNative(1262, nativeSetBaselineData);
        vm::registerNative(1270, nativeCopyGC);
        vm::registerNative(1271, nativePushGC);
        vm::registerNative(1272, nativePopGC);
        vm::registerNative(1280, nativeGetTextExtent);
        vm::registerNative(1281, nativeGetFontHeight);
        vm::registerNative(1282, nativeDrawText);
        vm::registerNative(1283, nativeDrawIcon);
        vm::registerNative(1284, nativeDrawTexture);
        vm::registerNative(1285, nativeDrawPattern);
        vm::registerNative(1286, nativeDrawBox);
        vm::registerNative(1287, nativeDrawStretchedTexture);
        vm::registerNative(1288, nativeDrawActor);
        vm::registerNative(1289, nativeDrawBorders);
        vm::registerNative(1295, nativeClearZ);
    }
}
