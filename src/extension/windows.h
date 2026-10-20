#pragma once

#include "extension/internal.h"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace dx::extension {
    struct TextRow;

    class UWindow : public UObject {
    public:
        UWindow* parent();
        UWindow* firstChild();
        UWindow* lastChild();
        UWindow* nextSibling();
        UWindow* prevSibling();
        bool visible();
        bool sensitive();
        bool selectable();
        float& x();
        float& y();
        float& width();
        float& height();

        URootWindow* root();
        UObject* playerPawn();
        UWindow* newChild(UClass* childClass, bool show);
        void destroyWindowTree();
        void detachFromParent();
        void attachLast(UWindow* owner);
        void attachFirst(UWindow* owner);
        void updateLayout();
        void show(bool value);
        bool effectivelyVisible();
        void setChildVisibility(bool value);
        void invokeVisibilityChange(bool value);
        void setSensitivity(bool value);
        void setWidth(float value);
        void setHeight(float value);
        void configureChild(float newX, float newY, float newWidth, float newHeight);
        void queryPreferredSize(float& preferredWidth, float& preferredHeight);
        float queryPreferredWidth(float queryHeight);
        float queryPreferredHeight(float queryWidth);
        void askParentForReconfigure();
        void askParentToShowArea(float areaX, float areaY, float areaWidth, float areaHeight);
        void queryGranularity(float& hGranularity, float& vGranularity);
        bool setFocusWindow(UWindow* window);
        void grabMouse();
        void ungrabMouse();
        void setFont(UObject* font);
        UWindow* modalWindow();
        void absolutePosition(float& outX, float& outY);
        bool sendButtonActivated(UWindow* button);
        void playWindowSound(UObject* sound);
        void drawBackground(UGC* gc);
        void scriptDrawWindow(UGC* gc);

        virtual void initWindow();
        virtual void destroyWindow();
        virtual void windowReady();
        virtual void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight);
        virtual void parentRequestedGranularity(float& hGranularity, float& vGranularity);
        virtual bool childRequestedReconfiguration(UWindow* child);
        virtual void childRequestedShowArea(UWindow* child, float showX, float showY, float showWidth, float showHeight);
        virtual void childRequestedVisibilityChange(UWindow* child, bool visible);
        virtual void configurationChanged();
        virtual void visibilityChanged(bool visible);
        virtual void sensitivityChanged(bool sensitive);
        virtual void mouseMoved(float newX, float newY);
        virtual void mouseEnteredWindow();
        virtual void mouseLeftWindow();
        virtual bool rawMouseButtonPressed(float pointX, float pointY, uint8_t button, uint8_t inputState);
        virtual bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks);
        virtual bool mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks);
        virtual bool rawKeyPressed(uint8_t key, uint8_t inputState, bool repeat);
        virtual bool keyPressed(const std::string& key);
        virtual bool virtualKeyPressed(uint8_t key, bool repeat);
        virtual bool acceleratorKeyPressed(const std::string& key);
        virtual void focusEnteredWindow();
        virtual void focusLeftWindow();
        virtual void focusEnteredDescendant(UWindow* window);
        virtual void focusLeftDescendant(UWindow* window);
        virtual bool buttonActivated(UWindow* button);
        virtual bool toggleChanged(UWindow* button, bool newToggle);
        virtual bool boxOptionSelected(UWindow* box, int32_t buttonNumber);
        virtual bool scalePositionChanged(UWindow* scale, int32_t newTickPosition, float newValue, bool final);
        virtual bool scaleRangeChanged(UWindow* scale, int32_t fromTick, int32_t toTick, float fromValue, float toValue, bool final);
        virtual bool scaleAttributesChanged(UWindow* scale, int32_t tickPosition, int32_t tickSpan, int32_t numTicks);
        virtual bool clipAttributesChanged(UWindow* clip, int32_t newClipWidth, int32_t newClipHeight, int32_t newChildWidth, int32_t newChildHeight);
        virtual bool clipPositionChanged(UWindow* clip, int32_t newCol, int32_t newRow);
        virtual bool listRowActivated(UWindow* list, int32_t rowId);
        virtual bool listSelectionChanged(UWindow* list, int32_t numSelections, int32_t focusRowId);
        virtual bool textChanged(UWindow* edit, bool modified);
        virtual bool editActivated(UWindow* edit, bool modified);
        virtual void drawWindow(UGC* gc);
        virtual void postDrawWindow(UGC* gc);
        virtual void childAdded(UWindow* child);
        virtual void childRemoved(UWindow* child);
        virtual void descendantAdded(UWindow* descendant);
        virtual void descendantRemoved(UWindow* descendant);
        virtual void tick(float deltaSeconds);

        struct Timer {
            Name function;
            float timeout = 0;
            bool loop = false;
            int32_t clientData = 0;
            float left = 0;
        };

        bool firstDraw = true;
        bool fixedWidth = false;
        bool fixedHeight = false;
        float baselineOffset = 0;
        float underlineHeight = 0;
        float usedX = 0;
        float usedY = 0;
        std::map<int32_t, Timer> timers;
        int32_t nextTimerId = 1;
    };

    class UTabGroupWindow : public UWindow {
    };

    class UModalWindow : public UTabGroupWindow {
    public:
        void initWindow() override;
        bool keyPressed(const std::string& key) override;
        void visibilityChanged(bool visible) override;
        bool isCurrentModal();
    };

    class URootWindow : public UModalWindow {
    public:
        void tick(float deltaSeconds) override;
        UWindow* topModal();
        bool isTraversable(UWindow* window, bool checkModal);
        UWindow* moveFocus(int32_t move);
        UWindow* moveTabGroup(int32_t move);
        void checkFocusWindow();
        void postDrawWindow(UGC* gc) override;
        bool setRootFocusWindow(UWindow* window);
        void setRootCursorPos(float newMouseX, float newMouseY);
        UWindow* cursorFocus(float& relativeX, float& relativeY);
        bool isModalOpen();
        bool isCursorVisible();
        bool onMouseButton(uint8_t key, bool pressed);
        bool onKey(uint8_t key, bool pressed, bool repeat);
        bool onChar(const std::string& chars);
        bool onRawMouseMove(float dx, float dy);
        void forgetWindow(UWindow* window);

        float& mouseX();
        float& mouseY();
        UWindow* focusWindow();

        bool renderViewportSet = false;
        double lastClickTime = -1;
        uint8_t lastClickButton = 0;
        UWindow* lastClickWindow = nullptr;
        int32_t clicks = 0;
    };

    class UClipWindow : public UTabGroupWindow {
    public:
        void initWindow() override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void configurationChanged() override;
        void childRequestedShowArea(UWindow* child, float showX, float showY, float showWidth, float showHeight) override;
        bool childRequestedReconfiguration(UWindow* child) override;
        UWindow* child();
        void childUnits(UWindow* content, float& hUnit, float& vUnit);
        void clipPreferredSize(UWindow* content, bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight);
        void queryClipPreferredSize(bool widthSpecified, float w, float& outWidth, bool& needH, bool heightSpecified, float h, float& outHeight, bool& needV);
        void reconfigureChild(UWindow* content, int32_t col, int32_t row, float w, float h);
        void setChildPosition(int32_t col, int32_t row);
    };

    class URadioBoxWindow : public UTabGroupWindow {
    public:
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void configurationChanged() override;
        bool toggleChanged(UWindow* button, bool newToggle) override;
    };

    class UTextWindow : public UWindow {
    public:
        void initWindow() override;
        void parentRequestedGranularity(float& hGranularity, float& vGranularity) override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void drawWindow(UGC* gc) override;
        virtual void textModified();
        void textExtent(float maxWidth, float& outWidth, float& outHeight);
        std::string& text();
    };

    class UButtonWindow : public UTextWindow {
    public:
        void initWindow() override;
        void drawWindow(UGC* gc) override;
        void mouseMoved(float newX, float newY) override;
        bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        bool mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        bool virtualKeyPressed(uint8_t key, bool repeat) override;
        void tick(float deltaSeconds) override;
        virtual void activate(uint8_t key);
        size_t stateIndex();

        std::array<Color, 6> buttonColors {};
        std::array<Color, 6> textColors {};
        std::array<UObject*, 6> buttonTextures {};
        bool colorsSet = false;
        bool textColorsSet = false;
        float repeatCountdown = 0;
    };

    class UToggleWindow : public UButtonWindow {
    public:
        void activate(uint8_t key) override;
        void setToggle(bool value);

        bool toggled = false;
    };

    class UCheckboxWindow : public UToggleWindow {
    public:
        void drawWindow(UGC* gc) override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
    };

    class ULargeTextWindow : public UTextWindow {
    public:
        void initWindow() override;
        void drawWindow(UGC* gc) override;
        void parentRequestedGranularity(float& hGranularity, float& vGranularity) override;
    };

    class UEditWindow : public ULargeTextWindow {
    public:
        void initWindow() override;
        void drawWindow(UGC* gc) override;
        bool keyPressed(const std::string& key) override;
        bool virtualKeyPressed(uint8_t key, bool repeat) override;
        bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        void textModified() override;
        bool filterChar(char& c);
        bool insertText(const std::string& value, bool undo, bool select);
        void deleteChar(bool before, bool undo);
        std::vector<TextRow> editRows();
        int32_t rowOf(const std::vector<TextRow>& rows, int32_t position);
        int32_t rowEnd(const std::vector<TextRow>& rows, int32_t row);
        float columnX(const std::vector<TextRow>& rows, int32_t position);
        int32_t positionAtX(const std::vector<TextRow>& rows, int32_t row, float x);
        void moveInsertion(uint8_t move, bool drag);
        void setInsertion(int32_t position, bool drag);
        void deleteSelection();
        void pushUndo();
        bool hasTextChanged();
        void setTextChangedFlag(bool value);
        void clearUndo();
        void notifyChanged();
        void clampPositions();

        std::vector<std::string> undoStack;
        size_t undoPosition = 0;
        int32_t unchangedUndo = 0;
        std::string clipboard;
        double blinkTime = 0;
    };

    class UTextLogWindow : public UTextWindow {
    public:
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void drawWindow(UGC* gc) override;
        void tick(float deltaSeconds) override;

        struct Entry {
            std::string text;
            Color color;
            double expires = 0;
        };

        std::deque<Entry> entries;
        double totalTime = 0;
    };

    class UTileWindow : public UWindow {
    public:
        void initWindow() override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void parentRequestedGranularity(float& hGranularity, float& vGranularity) override;
        void configurationChanged() override;
        bool childRequestedReconfiguration(UWindow* child) override;
        void computeChildSizes(bool widthSpecified, float& w, bool heightSpecified, float& h);

        struct Hold {
            UWindow* window = nullptr;
            float x = 0;
            float y = 0;
            float width = 0;
            float height = 0;
        };

        std::vector<Hold> holds;
    };

    class UBorderWindow : public UWindow {
    public:
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void configurationChanged() override;
        void drawWindow(UGC* gc) override;
        bool childRequestedReconfiguration(UWindow* child) override;
    };

    class UScaleWindow : public UWindow {
    public:
        void initWindow() override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void configurationChanged() override;
        void drawWindow(UGC* gc) override;
        bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        bool mouseButtonReleased(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        void mouseMoved(float newX, float newY) override;
        bool virtualKeyPressed(uint8_t key, bool repeat) override;
        void computeGeometry();
        int32_t maxPosition();
        void setPosition(int32_t position, bool final, bool notify);
        float positionValue(int32_t position);
        int32_t positionFromPoint(float pointX, float pointY);
        void moveThumb(uint8_t move);
        void notifyPosition(bool final);
        void changeScaleAttributes();
        void changeThumbPosition(int32_t position, bool force, bool final);
        void setRanges(int32_t span, int32_t positions);
        std::string valueString();

        std::vector<std::string> enumerations;
        float dragOffset = 0;
    };

    class UScaleManagerWindow : public UWindow {
    public:
        void initWindow() override;
        bool scaleAttributesChanged(UWindow* scale, int32_t tickPosition, int32_t tickSpan, int32_t numTicks) override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void configurationChanged() override;
        bool buttonActivated(UWindow* button) override;
        bool scalePositionChanged(UWindow* scale, int32_t newTickPosition, float newValue, bool final) override;
        void updateValueField();
    };

    class UScrollAreaWindow : public UWindow {
    public:
        void initWindow() override;
        void configurationChanged() override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        bool childRequestedReconfiguration(UWindow* child) override;
        bool scaleRangeChanged(UWindow* scale, int32_t fromTick, int32_t toTick, float fromValue, float toValue, bool final) override;
        bool clipAttributesChanged(UWindow* clip, int32_t newClipWidth, int32_t newClipHeight, int32_t newChildWidth, int32_t newChildHeight) override;
        bool clipPositionChanged(UWindow* clip, int32_t newCol, int32_t newRow) override;
        bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        void enableScrolling(bool horizontal, bool vertical);
        void computeChildSizes(bool widthSpecified, float w, float* outWidth, bool heightSpecified, float h, float* outHeight, bool configure);
    };

    class UListWindow : public UWindow {
    public:
        void initWindow() override;
        void parentRequestedGranularity(float& hGranularity, float& vGranularity) override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void drawWindow(UGC* gc) override;
        bool mouseButtonPressed(float pointX, float pointY, uint8_t button, int32_t numClicks) override;
        bool virtualKeyPressed(uint8_t key, bool repeat) override;
        bool keyPressed(const std::string& key) override;
        void tick(float deltaSeconds) override;
        void visibilityChanged(bool visible) override;
        void setRow(int32_t rowId, bool select, bool clearRows);

        struct Column {
            uint8_t align = HALIGN_Left;
            Color color { 255, 255, 255, 255 };
            bool colorSet = false;
            UObject* font = nullptr;
            std::string title;
            uint8_t type = 0;
            std::string format;
            float width = 0;
            bool hidden = false;
        };

        struct Row {
            int32_t id = 0;
            std::vector<std::string> fields;
            int32_t clientInt = 0;
            UObject* clientObject = nullptr;
            bool selected = false;
        };

        struct SortKey {
            int32_t column = 0;
            bool reverse = false;
            bool caseSensitive = false;
        };

        float lineHeight();
        int32_t indexOf(int32_t rowId);
        Row* row(int32_t rowId);
        std::vector<std::string> splitRow(const std::string& text);
        void selectRow(int32_t rowId, bool select);
        void setFocusLine(int32_t index, bool show);
        void moveRow(uint8_t move, bool select, bool clearRows);
        void notifySelection();
        void notifyActivated(int32_t rowId);
        void sortRows();
        void autoSize();
        int32_t selectedCount();

        std::vector<Column> columns;
        std::vector<Row> rows;
        std::vector<SortKey> sortKeys;
        int32_t nextRowId = 1;
        bool notifying = false;
        Color highlightTextColor { 0, 0, 0, 255 };
        float fieldMarginWidth = 3;
        float fieldMarginHeight = 1;
    };

    class UComputerWindow : public UWindow {
    public:
        void initWindow() override;
        void parentRequestedPreferredSize(bool widthSpecified, float& preferredWidth, bool heightSpecified, float& preferredHeight) override;
        void visibilityChanged(bool visible) override;
        void drawWindow(UGC* gc) override;
        void tick(float deltaSeconds) override;
        bool keyPressed(const std::string& key) override;
        bool virtualKeyPressed(uint8_t key, bool repeat) override;

        enum EventType {
            EventCharacter = 0,
            EventTextColor = 1,
            EventTextSound = 2,
            EventTextPosition = 3,
            EventNewLine = 4,
            EventGraphic = 5,
            EventClearScreen = 6,
            EventSound = 7,
            EventGetInput = 9,
            EventGetChar = 10,
            EventFadeOut = 11,
        };

        struct Event {
            int32_t type = EventCharacter;
            float time = 0;
            char ch = 0;
            Color color { 255, 255, 255, 255 };
            int32_t col = -1;
            int32_t row = -1;
            bool noFade = false;
            UObject* object = nullptr;
            int32_t x = -1;
            int32_t y = -1;
            int32_t width = 0;
            int32_t height = 0;
            bool pixelPos = false;
            float duration = 0;
            int32_t maxLength = 0;
            std::string key;
            std::string defaultValue;
            std::string mask;
            bool echo = false;
        };

        void addEvent(Event event, bool advance);
        void echoChar(char c);
        void processQueuedBuffer();
        void processEvent(size_t index);
        bool calculateCharDisplayPosition(size_t index, int32_t& col, int32_t& row);
        void calculateGraphicDisplayPosition(Event& graphic);
        void scroll();
        void invokeParents(Name eventName);
        void finishInput(bool cancel);

        std::vector<Event> queued;
        size_t queuedStart = 0;
        std::vector<Event> display;
        std::array<UObject*, 6> backgrounds {};
        std::string input;
        int32_t inputMax = 0;
        bool inputActive = false;
        bool charActive = false;
    };

    class UViewportWindow : public UWindow {
    public:
        void drawWindow(UGC* gc) override;
    };

    struct TextContext {
        UObject* normalFont = nullptr;
        UObject* boldFont = nullptr;
        bool parse = true;
        float vSpacing = 1.0f;
        Color textColor { 0, 255, 0, 255 };
    };

    struct TextRow {
        size_t start = 0;
        size_t end = 0;
        float width = 0;
        float height = 0;
        bool bold = false;
        bool useColor = false;
        Color color {};
    };

    class UGC : public UObject {
    public:
        void resetState(UWindow* window, float offsetX, float offsetY);
        void resetClip(float clipX, float clipY, float clipWidth, float clipHeight);
        void pushClip(float clipX, float clipY, float clipWidth, float clipHeight);
        void popClip();
        BlendStyle tileStyle();
        BlendStyle textStyle();
        bool smoothed();
        void drawTile(UObject* texture, float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, const Color& color, BlendStyle style, bool smooth);
        void drawRect(float destX, float destY, float destWidth, float destHeight, const Color& color, BlendStyle style);
        void drawText(float destX, float destY, float destWidth, float destHeight, const std::string& text);
        void drawTextRow(float destX, float destY, const std::string& text, const TextRow& row);
        float drawChar(UObject* font, char c, const Color& color, bool underline, float x, float y);
        TextContext textContext();
        void textExtent(float destWidth, const std::string& text, float& outWidth, float& outHeight);
        void drawPattern(float destX, float destY, float destWidth, float destHeight, float orgX, float orgY, UObject* texture);
        void drawStretched(float destX, float destY, float destWidth, float destHeight, float srcX, float srcY, float srcWidth, float srcHeight, UObject* texture);
        void drawIcon(float destX, float destY, UObject* texture);
        void drawBox(float destX, float destY, float destWidth, float destHeight, float thickness, UObject* texture);
        void drawBorders(float destX, float destY, float destWidth, float destHeight, UObject* const* borders);
        void setStyle(uint8_t style);
        void setFonts(UObject* normal, UObject* bold);

        float offsetX = 0;
        float offsetY = 0;
        float clipLeft = 0;
        float clipTop = 0;
        float clipRight = 0;
        float clipBottom = 0;
        std::vector<std::array<float, 4>> clipStack;
        bool specialText = false;

        struct Saved {
            std::vector<uint8_t> data;
            float offsetX = 0;
            float offsetY = 0;
            bool specialText = false;
        };

        std::vector<Saved> savedStates;
    };

    TextContext textContextOf(UWindow* window);
    void measureText(const TextContext& context, float wrapWidth, const std::string& text, float& outWidth, float& outHeight);
    std::vector<TextRow> textRows(const TextContext& context, float wrapWidth, const std::string& text);
    void measureString(UObject* font, const std::string& text, float& width, float& height);
}
