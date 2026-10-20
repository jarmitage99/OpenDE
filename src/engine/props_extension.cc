#include "engine/props_extension.h"

#include "core/objects.h"

#include <cstdio>

namespace dx {
    window::Offsets offsets_window;
    flag::Offsets offsets_flag;
    gc::Offsets offsets_gc;
    buttonwindow::Offsets offsets_buttonwindow;
    extensionobject::Offsets offsets_extensionobject;
    textwindow::Offsets offsets_textwindow;
    tabgroupwindow::Offsets offsets_tabgroupwindow;
    scalewindow::Offsets offsets_scalewindow;
    rootwindow::Offsets offsets_rootwindow;
    togglewindow::Offsets offsets_togglewindow;
    modalwindow::Offsets offsets_modalwindow;
    playerpawnext::Offsets offsets_playerpawnext;
    flagbase::Offsets offsets_flagbase;
    largetextwindow::Offsets offsets_largetextwindow;
    scalemanagerwindow::Offsets offsets_scalemanagerwindow;
    editwindow::Offsets offsets_editwindow;
    clipwindow::Offsets offsets_clipwindow;
    borderwindow::Offsets offsets_borderwindow;
    checkboxwindow::Offsets offsets_checkboxwindow;
    computerwindow::Offsets offsets_computerwindow;
    listwindow::Offsets offsets_listwindow;
    radioboxwindow::Offsets offsets_radioboxwindow;
    scrollareawindow::Offsets offsets_scrollareawindow;
    extstring::Offsets offsets_extstring;
    textlogwindow::Offsets offsets_textlogwindow;
    tilewindow::Offsets offsets_tilewindow;
    viewportwindow::Offsets offsets_viewportwindow;

    void bindProps_extension()
    {
        {
            UClass* cls = objects::findClass("Extension.Window");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.Window\n");
            } else {
                offsets_window.windowType = cls->findProperty(Name("windowType"))->offset;
                offsets_window.parentOwner = cls->findProperty(Name("parentOwner"))->offset;
                offsets_window.bIsVisible = cls->findProperty(Name("bIsVisible"))->offset;
                offsets_window.bIsSensitive = cls->findProperty(Name("bIsSensitive"))->offset;
                offsets_window.bIsSelectable = cls->findProperty(Name("bIsSelectable"))->offset;
                offsets_window.bIsInitialized = cls->findProperty(Name("bIsInitialized"))->offset;
                offsets_window.bBeingDestroyed = cls->findProperty(Name("bBeingDestroyed"))->offset;
                offsets_window.lockCount = cls->findProperty(Name("lockCount"))->offset;
                offsets_window.bDrawRawBackground = cls->findProperty(Name("bDrawRawBackground"))->offset;
                offsets_window.bStretchBackground = cls->findProperty(Name("bStretchBackground"))->offset;
                offsets_window.bSmoothBackground = cls->findProperty(Name("bSmoothBackground"))->offset;
                offsets_window.backgroundStyle = cls->findProperty(Name("backgroundStyle"))->offset;
                offsets_window.TextColor = cls->findProperty(Name("TextColor"))->offset;
                offsets_window.textPlane = cls->findProperty(Name("textPlane"))->offset;
                offsets_window.normalFont = cls->findProperty(Name("normalFont"))->offset;
                offsets_window.boldFont = cls->findProperty(Name("boldFont"))->offset;
                offsets_window.textVSpacing = cls->findProperty(Name("textVSpacing"))->offset;
                offsets_window.tileColor = cls->findProperty(Name("tileColor"))->offset;
                offsets_window.tilePlane = cls->findProperty(Name("tilePlane"))->offset;
                offsets_window.bSpecialText = cls->findProperty(Name("bSpecialText"))->offset;
                offsets_window.bTextTranslucent = cls->findProperty(Name("bTextTranslucent"))->offset;
                offsets_window.X = cls->findProperty(Name("X"))->offset;
                offsets_window.Y = cls->findProperty(Name("Y"))->offset;
                offsets_window.Width = cls->findProperty(Name("Width"))->offset;
                offsets_window.Height = cls->findProperty(Name("Height"))->offset;
                offsets_window.winHAlign = cls->findProperty(Name("winHAlign"))->offset;
                offsets_window.winVAlign = cls->findProperty(Name("winVAlign"))->offset;
                offsets_window.hMargin0 = cls->findProperty(Name("hMargin0"))->offset;
                offsets_window.hMargin1 = cls->findProperty(Name("hMargin1"))->offset;
                offsets_window.vMargin0 = cls->findProperty(Name("vMargin0"))->offset;
                offsets_window.vMargin1 = cls->findProperty(Name("vMargin1"))->offset;
                offsets_window.hardcodedWidth = cls->findProperty(Name("hardcodedWidth"))->offset;
                offsets_window.hardcodedHeight = cls->findProperty(Name("hardcodedHeight"))->offset;
                offsets_window.holdX = cls->findProperty(Name("holdX"))->offset;
                offsets_window.holdY = cls->findProperty(Name("holdY"))->offset;
                offsets_window.holdWidth = cls->findProperty(Name("holdWidth"))->offset;
                offsets_window.holdHeight = cls->findProperty(Name("holdHeight"))->offset;
                offsets_window.acceleratorKey = cls->findProperty(Name("acceleratorKey"))->offset;
                offsets_window.bNeedsReconfigure = cls->findProperty(Name("bNeedsReconfigure"))->offset;
                offsets_window.bNeedsQuery = cls->findProperty(Name("bNeedsQuery"))->offset;
                offsets_window.bConfigured = cls->findProperty(Name("bConfigured"))->offset;
                offsets_window.bLastWidthSpecified = cls->findProperty(Name("bLastWidthSpecified"))->offset;
                offsets_window.bLastHeightSpecified = cls->findProperty(Name("bLastHeightSpecified"))->offset;
                offsets_window.lastSpecifiedWidth = cls->findProperty(Name("lastSpecifiedWidth"))->offset;
                offsets_window.lastSpecifiedHeight = cls->findProperty(Name("lastSpecifiedHeight"))->offset;
                offsets_window.lastQueryWidth = cls->findProperty(Name("lastQueryWidth"))->offset;
                offsets_window.lastQueryHeight = cls->findProperty(Name("lastQueryHeight"))->offset;
                offsets_window.bTickEnabled = cls->findProperty(Name("bTickEnabled"))->offset;
                offsets_window.maxClicks = cls->findProperty(Name("maxClicks"))->offset;
                offsets_window.winClipRect = cls->findProperty(Name("winClipRect"))->offset;
                offsets_window.Background = cls->findProperty(Name("Background"))->offset;
                offsets_window.defaultCursor = cls->findProperty(Name("defaultCursor"))->offset;
                offsets_window.defaultCursorShadow = cls->findProperty(Name("defaultCursorShadow"))->offset;
                offsets_window.defaultHotX = cls->findProperty(Name("defaultHotX"))->offset;
                offsets_window.defaultHotY = cls->findProperty(Name("defaultHotY"))->offset;
                offsets_window.defaultCursorColor = cls->findProperty(Name("defaultCursorColor"))->offset;
                offsets_window.focusSound = cls->findProperty(Name("focusSound"))->offset;
                offsets_window.unfocusSound = cls->findProperty(Name("unfocusSound"))->offset;
                offsets_window.visibleSound = cls->findProperty(Name("visibleSound"))->offset;
                offsets_window.invisibleSound = cls->findProperty(Name("invisibleSound"))->offset;
                offsets_window.SoundVolume = cls->findProperty(Name("SoundVolume"))->offset;
                offsets_window.clientObject = cls->findProperty(Name("clientObject"))->offset;
                offsets_window.rowMajorIndex = cls->findProperty(Name("rowMajorIndex"))->offset;
                offsets_window.colMajorIndex = cls->findProperty(Name("colMajorIndex"))->offset;
                offsets_window.att = cls->findProperty(Name("att"))->offset;
                offsets_window.firstTimer = cls->findProperty(Name("firstTimer"))->offset;
                offsets_window.freeTimer = cls->findProperty(Name("freeTimer"))->offset;
                offsets_window.wGc = cls->findProperty(Name("wGc"))->offset;
                offsets_window.gGc = cls->findProperty(Name("gGc"))->offset;
                offsets_window.winParent = cls->findProperty(Name("winParent"))->offset;
                offsets_window.firstChild = cls->findProperty(Name("firstChild"))->offset;
                offsets_window.lastChild = cls->findProperty(Name("lastChild"))->offset;
                offsets_window.prevSibling = cls->findProperty(Name("prevSibling"))->offset;
                offsets_window.nextSibling = cls->findProperty(Name("nextSibling"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.Flag");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.Flag\n");
            } else {
                offsets_flag.FlagName = cls->findProperty(Name("FlagName"))->offset;
                offsets_flag.flagHash = cls->findProperty(Name("flagHash"))->offset;
                offsets_flag.FlagBase = cls->findProperty(Name("FlagBase"))->offset;
                offsets_flag.nextFlag = cls->findProperty(Name("nextFlag"))->offset;
                offsets_flag.flagType = cls->findProperty(Name("flagType"))->offset;
                offsets_flag.expiration = cls->findProperty(Name("expiration"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.GC");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.GC\n");
            } else {
                offsets_gc.Canvas = cls->findProperty(Name("Canvas"))->offset;
                offsets_gc.gcClipRect = cls->findProperty(Name("gcClipRect"))->offset;
                offsets_gc.Style = cls->findProperty(Name("Style"))->offset;
                offsets_gc.bSmoothed = cls->findProperty(Name("bSmoothed"))->offset;
                offsets_gc.bDrawEnabled = cls->findProperty(Name("bDrawEnabled"))->offset;
                offsets_gc.bMasked = cls->findProperty(Name("bMasked"))->offset;
                offsets_gc.bTranslucent = cls->findProperty(Name("bTranslucent"))->offset;
                offsets_gc.bModulated = cls->findProperty(Name("bModulated"))->offset;
                offsets_gc.bTextTranslucent = cls->findProperty(Name("bTextTranslucent"))->offset;
                offsets_gc.PolyFlags = cls->findProperty(Name("PolyFlags"))->offset;
                offsets_gc.textPolyFlags = cls->findProperty(Name("textPolyFlags"))->offset;
                offsets_gc.tileColor = cls->findProperty(Name("tileColor"))->offset;
                offsets_gc.tilePlane = cls->findProperty(Name("tilePlane"))->offset;
                offsets_gc.TextColor = cls->findProperty(Name("TextColor"))->offset;
                offsets_gc.textPlane = cls->findProperty(Name("textPlane"))->offset;
                offsets_gc.normalFont = cls->findProperty(Name("normalFont"))->offset;
                offsets_gc.boldFont = cls->findProperty(Name("boldFont"))->offset;
                offsets_gc.underlineTexture = cls->findProperty(Name("underlineTexture"))->offset;
                offsets_gc.underlineHeight = cls->findProperty(Name("underlineHeight"))->offset;
                offsets_gc.baselineOffset = cls->findProperty(Name("baselineOffset"))->offset;
                offsets_gc.textVSpacing = cls->findProperty(Name("textVSpacing"))->offset;
                offsets_gc.HAlign = cls->findProperty(Name("HAlign"))->offset;
                offsets_gc.VAlign = cls->findProperty(Name("VAlign"))->offset;
                offsets_gc.bWordWrap = cls->findProperty(Name("bWordWrap"))->offset;
                offsets_gc.bParseMetachars = cls->findProperty(Name("bParseMetachars"))->offset;
                offsets_gc.hMultiplier = cls->findProperty(Name("hMultiplier"))->offset;
                offsets_gc.vMultiplier = cls->findProperty(Name("vMultiplier"))->offset;
                offsets_gc.bFree = cls->findProperty(Name("bFree"))->offset;
                offsets_gc.gcCount = cls->findProperty(Name("gcCount"))->offset;
                offsets_gc.gcStack = cls->findProperty(Name("gcStack"))->offset;
                offsets_gc.gcFree = cls->findProperty(Name("gcFree"))->offset;
                offsets_gc.gcOwner = cls->findProperty(Name("gcOwner"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ButtonWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ButtonWindow\n");
            } else {
                offsets_buttonwindow.bButtonPressed = cls->findProperty(Name("bButtonPressed"))->offset;
                offsets_buttonwindow.bMousePressed = cls->findProperty(Name("bMousePressed"))->offset;
                offsets_buttonwindow.bAutoRepeat = cls->findProperty(Name("bAutoRepeat"))->offset;
                offsets_buttonwindow.bEnableRightMouseClick = cls->findProperty(Name("bEnableRightMouseClick"))->offset;
                offsets_buttonwindow.activateDelay = cls->findProperty(Name("activateDelay"))->offset;
                offsets_buttonwindow.initialDelay = cls->findProperty(Name("initialDelay"))->offset;
                offsets_buttonwindow.repeatRate = cls->findProperty(Name("repeatRate"))->offset;
                offsets_buttonwindow.curTexture = cls->findProperty(Name("curTexture"))->offset;
                offsets_buttonwindow.curTileColor = cls->findProperty(Name("curTileColor"))->offset;
                offsets_buttonwindow.curTextColor = cls->findProperty(Name("curTextColor"))->offset;
                offsets_buttonwindow.pressSound = cls->findProperty(Name("pressSound"))->offset;
                offsets_buttonwindow.clickSound = cls->findProperty(Name("clickSound"))->offset;
                offsets_buttonwindow.activateTimer = cls->findProperty(Name("activateTimer"))->offset;
                offsets_buttonwindow.repeatTime = cls->findProperty(Name("repeatTime"))->offset;
                offsets_buttonwindow.lastInputKey = cls->findProperty(Name("lastInputKey"))->offset;
                offsets_buttonwindow.Info = cls->findProperty(Name("Info"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ExtensionObject");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ExtensionObject\n");
            } else {
            }
        }
        {
            UClass* cls = objects::findClass("Extension.TextWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.TextWindow\n");
            } else {
                offsets_textwindow.HAlign = cls->findProperty(Name("HAlign"))->offset;
                offsets_textwindow.VAlign = cls->findProperty(Name("VAlign"))->offset;
                offsets_textwindow.bWordWrap = cls->findProperty(Name("bWordWrap"))->offset;
                offsets_textwindow.bTextIsAccelerator = cls->findProperty(Name("bTextIsAccelerator"))->offset;
                offsets_textwindow.hMargin = cls->findProperty(Name("hMargin"))->offset;
                offsets_textwindow.vMargin = cls->findProperty(Name("vMargin"))->offset;
                offsets_textwindow.minLines = cls->findProperty(Name("minLines"))->offset;
                offsets_textwindow.MaxLines = cls->findProperty(Name("MaxLines"))->offset;
                offsets_textwindow.MinWidth = cls->findProperty(Name("MinWidth"))->offset;
                offsets_textwindow.Text = cls->findProperty(Name("Text"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.TabGroupWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.TabGroupWindow\n");
            } else {
                offsets_tabgroupwindow.rowMajorWindowList = cls->findProperty(Name("rowMajorWindowList"))->offset;
                offsets_tabgroupwindow.colMajorWindowList = cls->findProperty(Name("colMajorWindowList"))->offset;
                offsets_tabgroupwindow.bSizeParentToChildren = cls->findProperty(Name("bSizeParentToChildren"))->offset;
                offsets_tabgroupwindow.bSizeChildrenToParent = cls->findProperty(Name("bSizeChildrenToParent"))->offset;
                offsets_tabgroupwindow.tabGroupIndex = cls->findProperty(Name("tabGroupIndex"))->offset;
                offsets_tabgroupwindow.firstAbsX = cls->findProperty(Name("firstAbsX"))->offset;
                offsets_tabgroupwindow.firstAbsY = cls->findProperty(Name("firstAbsY"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ScaleWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ScaleWindow\n");
            } else {
                offsets_scalewindow.orientation = cls->findProperty(Name("orientation"))->offset;
                offsets_scalewindow.scaleTexture = cls->findProperty(Name("scaleTexture"))->offset;
                offsets_scalewindow.thumbTexture = cls->findProperty(Name("thumbTexture"))->offset;
                offsets_scalewindow.tickTexture = cls->findProperty(Name("tickTexture"))->offset;
                offsets_scalewindow.preCapTexture = cls->findProperty(Name("preCapTexture"))->offset;
                offsets_scalewindow.postCapTexture = cls->findProperty(Name("postCapTexture"))->offset;
                offsets_scalewindow.bRepeatScaleTexture = cls->findProperty(Name("bRepeatScaleTexture"))->offset;
                offsets_scalewindow.bRepeatThumbTexture = cls->findProperty(Name("bRepeatThumbTexture"))->offset;
                offsets_scalewindow.bDrawEndTicks = cls->findProperty(Name("bDrawEndTicks"))->offset;
                offsets_scalewindow.bStretchScale = cls->findProperty(Name("bStretchScale"))->offset;
                offsets_scalewindow.bSpanThumb = cls->findProperty(Name("bSpanThumb"))->offset;
                offsets_scalewindow.borderPattern = cls->findProperty(Name("borderPattern"))->offset;
                offsets_scalewindow.scaleBorderSize = cls->findProperty(Name("scaleBorderSize"))->offset;
                offsets_scalewindow.thumbBorderSize = cls->findProperty(Name("thumbBorderSize"))->offset;
                offsets_scalewindow.scaleBorderColor = cls->findProperty(Name("scaleBorderColor"))->offset;
                offsets_scalewindow.thumbBorderColor = cls->findProperty(Name("thumbBorderColor"))->offset;
                offsets_scalewindow.scaleStyle = cls->findProperty(Name("scaleStyle"))->offset;
                offsets_scalewindow.thumbStyle = cls->findProperty(Name("thumbStyle"))->offset;
                offsets_scalewindow.tickStyle = cls->findProperty(Name("tickStyle"))->offset;
                offsets_scalewindow.scaleColor = cls->findProperty(Name("scaleColor"))->offset;
                offsets_scalewindow.thumbColor = cls->findProperty(Name("thumbColor"))->offset;
                offsets_scalewindow.tickColor = cls->findProperty(Name("tickColor"))->offset;
                offsets_scalewindow.scaleWidth = cls->findProperty(Name("scaleWidth"))->offset;
                offsets_scalewindow.scaleHeight = cls->findProperty(Name("scaleHeight"))->offset;
                offsets_scalewindow.ThumbWidth = cls->findProperty(Name("ThumbWidth"))->offset;
                offsets_scalewindow.ThumbHeight = cls->findProperty(Name("ThumbHeight"))->offset;
                offsets_scalewindow.tickWidth = cls->findProperty(Name("tickWidth"))->offset;
                offsets_scalewindow.tickHeight = cls->findProperty(Name("tickHeight"))->offset;
                offsets_scalewindow.preCapWidth = cls->findProperty(Name("preCapWidth"))->offset;
                offsets_scalewindow.preCapHeight = cls->findProperty(Name("preCapHeight"))->offset;
                offsets_scalewindow.postCapWidth = cls->findProperty(Name("postCapWidth"))->offset;
                offsets_scalewindow.postCapHeight = cls->findProperty(Name("postCapHeight"))->offset;
                offsets_scalewindow.startOffset = cls->findProperty(Name("startOffset"))->offset;
                offsets_scalewindow.endOffset = cls->findProperty(Name("endOffset"))->offset;
                offsets_scalewindow.marginWidth = cls->findProperty(Name("marginWidth"))->offset;
                offsets_scalewindow.marginHeight = cls->findProperty(Name("marginHeight"))->offset;
                offsets_scalewindow.numPositions = cls->findProperty(Name("numPositions"))->offset;
                offsets_scalewindow.currentPos = cls->findProperty(Name("currentPos"))->offset;
                offsets_scalewindow.spanRange = cls->findProperty(Name("spanRange"))->offset;
                offsets_scalewindow.thumbStep = cls->findProperty(Name("thumbStep"))->offset;
                offsets_scalewindow.fromValue = cls->findProperty(Name("fromValue"))->offset;
                offsets_scalewindow.toValue = cls->findProperty(Name("toValue"))->offset;
                offsets_scalewindow.valueFmt = cls->findProperty(Name("valueFmt"))->offset;
                offsets_scalewindow.initialDelay = cls->findProperty(Name("initialDelay"))->offset;
                offsets_scalewindow.repeatRate = cls->findProperty(Name("repeatRate"))->offset;
                offsets_scalewindow.initialPos = cls->findProperty(Name("initialPos"))->offset;
                offsets_scalewindow.setSound = cls->findProperty(Name("setSound"))->offset;
                offsets_scalewindow.clickSound = cls->findProperty(Name("clickSound"))->offset;
                offsets_scalewindow.dragSound = cls->findProperty(Name("dragSound"))->offset;
                offsets_scalewindow.scaleX = cls->findProperty(Name("scaleX"))->offset;
                offsets_scalewindow.scaleY = cls->findProperty(Name("scaleY"))->offset;
                offsets_scalewindow.scaleW = cls->findProperty(Name("scaleW"))->offset;
                offsets_scalewindow.scaleH = cls->findProperty(Name("scaleH"))->offset;
                offsets_scalewindow.thumbX = cls->findProperty(Name("thumbX"))->offset;
                offsets_scalewindow.thumbY = cls->findProperty(Name("thumbY"))->offset;
                offsets_scalewindow.thumbW = cls->findProperty(Name("thumbW"))->offset;
                offsets_scalewindow.thumbH = cls->findProperty(Name("thumbH"))->offset;
                offsets_scalewindow.tickX = cls->findProperty(Name("tickX"))->offset;
                offsets_scalewindow.tickY = cls->findProperty(Name("tickY"))->offset;
                offsets_scalewindow.tickW = cls->findProperty(Name("tickW"))->offset;
                offsets_scalewindow.tickH = cls->findProperty(Name("tickH"))->offset;
                offsets_scalewindow.preCapXOff = cls->findProperty(Name("preCapXOff"))->offset;
                offsets_scalewindow.preCapYOff = cls->findProperty(Name("preCapYOff"))->offset;
                offsets_scalewindow.preCapW = cls->findProperty(Name("preCapW"))->offset;
                offsets_scalewindow.preCapH = cls->findProperty(Name("preCapH"))->offset;
                offsets_scalewindow.postCapXOff = cls->findProperty(Name("postCapXOff"))->offset;
                offsets_scalewindow.postCapYOff = cls->findProperty(Name("postCapYOff"))->offset;
                offsets_scalewindow.postCapW = cls->findProperty(Name("postCapW"))->offset;
                offsets_scalewindow.postCapH = cls->findProperty(Name("postCapH"))->offset;
                offsets_scalewindow.absStartScale = cls->findProperty(Name("absStartScale"))->offset;
                offsets_scalewindow.absEndScale = cls->findProperty(Name("absEndScale"))->offset;
                offsets_scalewindow.bDraggingThumb = cls->findProperty(Name("bDraggingThumb"))->offset;
                offsets_scalewindow.mousePos = cls->findProperty(Name("mousePos"))->offset;
                offsets_scalewindow.repeatDir = cls->findProperty(Name("repeatDir"))->offset;
                offsets_scalewindow.RemainingTime = cls->findProperty(Name("RemainingTime"))->offset;
                offsets_scalewindow.enumStrings = cls->findProperty(Name("enumStrings"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.RootWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.RootWindow\n");
            } else {
                offsets_rootwindow.parentPawn = cls->findProperty(Name("parentPawn"))->offset;
                offsets_rootwindow.nextRootWindow = cls->findProperty(Name("nextRootWindow"))->offset;
                offsets_rootwindow.MouseX = cls->findProperty(Name("MouseX"))->offset;
                offsets_rootwindow.MouseY = cls->findProperty(Name("MouseY"))->offset;
                offsets_rootwindow.prevMouseX = cls->findProperty(Name("prevMouseX"))->offset;
                offsets_rootwindow.prevMouseY = cls->findProperty(Name("prevMouseY"))->offset;
                offsets_rootwindow.lastMouseWindow = cls->findProperty(Name("lastMouseWindow"))->offset;
                offsets_rootwindow.bMouseMoved = cls->findProperty(Name("bMouseMoved"))->offset;
                offsets_rootwindow.bMouseMoveLocked = cls->findProperty(Name("bMouseMoveLocked"))->offset;
                offsets_rootwindow.bMouseButtonLocked = cls->findProperty(Name("bMouseButtonLocked"))->offset;
                offsets_rootwindow.bCursorVisible = cls->findProperty(Name("bCursorVisible"))->offset;
                offsets_rootwindow.defaultEditCursor = cls->findProperty(Name("defaultEditCursor"))->offset;
                offsets_rootwindow.DefaultMoveCursor = cls->findProperty(Name("DefaultMoveCursor"))->offset;
                offsets_rootwindow.defaultHorizontalMoveCursor = cls->findProperty(Name("defaultHorizontalMoveCursor"))->offset;
                offsets_rootwindow.defaultVerticalMoveCursor = cls->findProperty(Name("defaultVerticalMoveCursor"))->offset;
                offsets_rootwindow.defaultTopLeftMoveCursor = cls->findProperty(Name("defaultTopLeftMoveCursor"))->offset;
                offsets_rootwindow.defaultTopRightMoveCursor = cls->findProperty(Name("defaultTopRightMoveCursor"))->offset;
                offsets_rootwindow.bPositionalSound = cls->findProperty(Name("bPositionalSound"))->offset;
                offsets_rootwindow.grabbedWindow = cls->findProperty(Name("grabbedWindow"))->offset;
                offsets_rootwindow.FocusWindow = cls->findProperty(Name("FocusWindow"))->offset;
                offsets_rootwindow.handleMouseRef = cls->findProperty(Name("handleMouseRef"))->offset;
                offsets_rootwindow.handleKeyboardRef = cls->findProperty(Name("handleKeyboardRef"))->offset;
                offsets_rootwindow.initCount = cls->findProperty(Name("initCount"))->offset;
                offsets_rootwindow.bRender = cls->findProperty(Name("bRender"))->offset;
                offsets_rootwindow.bClipRender = cls->findProperty(Name("bClipRender"))->offset;
                offsets_rootwindow.bStretchRawBackground = cls->findProperty(Name("bStretchRawBackground"))->offset;
                offsets_rootwindow.renderX = cls->findProperty(Name("renderX"))->offset;
                offsets_rootwindow.renderY = cls->findProperty(Name("renderY"))->offset;
                offsets_rootwindow.renderWidth = cls->findProperty(Name("renderWidth"))->offset;
                offsets_rootwindow.renderHeight = cls->findProperty(Name("renderHeight"))->offset;
                offsets_rootwindow.rawBackground = cls->findProperty(Name("rawBackground"))->offset;
                offsets_rootwindow.rawBackgroundWidth = cls->findProperty(Name("rawBackgroundWidth"))->offset;
                offsets_rootwindow.rawBackgroundHeight = cls->findProperty(Name("rawBackgroundHeight"))->offset;
                offsets_rootwindow.rawColor = cls->findProperty(Name("rawColor"))->offset;
                offsets_rootwindow.TickCycles = cls->findProperty(Name("TickCycles"))->offset;
                offsets_rootwindow.paintCycles = cls->findProperty(Name("paintCycles"))->offset;
                offsets_rootwindow.bShowStats = cls->findProperty(Name("bShowStats"))->offset;
                offsets_rootwindow.bShowFrames = cls->findProperty(Name("bShowFrames"))->offset;
                offsets_rootwindow.debugTexture = cls->findProperty(Name("debugTexture"))->offset;
                offsets_rootwindow.frameTimer = cls->findProperty(Name("frameTimer"))->offset;
                offsets_rootwindow.multiClickTimeout = cls->findProperty(Name("multiClickTimeout"))->offset;
                offsets_rootwindow.maxMouseDist = cls->findProperty(Name("maxMouseDist"))->offset;
                offsets_rootwindow.clickCount = cls->findProperty(Name("clickCount"))->offset;
                offsets_rootwindow.lastButtonType = cls->findProperty(Name("lastButtonType"))->offset;
                offsets_rootwindow.lastButtonPress = cls->findProperty(Name("lastButtonPress"))->offset;
                offsets_rootwindow.lastButtonWindow = cls->findProperty(Name("lastButtonWindow"))->offset;
                offsets_rootwindow.firstButtonMouseX = cls->findProperty(Name("firstButtonMouseX"))->offset;
                offsets_rootwindow.firstButtonMouseY = cls->findProperty(Name("firstButtonMouseY"))->offset;
                offsets_rootwindow.keyDownMap = cls->findProperty(Name("keyDownMap"))->offset;
                offsets_rootwindow.hMultiplier = cls->findProperty(Name("hMultiplier"))->offset;
                offsets_rootwindow.vMultiplier = cls->findProperty(Name("vMultiplier"))->offset;
                offsets_rootwindow.snapshotWidth = cls->findProperty(Name("snapshotWidth"))->offset;
                offsets_rootwindow.snapshotHeight = cls->findProperty(Name("snapshotHeight"))->offset;
                offsets_rootwindow.rootFrame = cls->findProperty(Name("rootFrame"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ToggleWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ToggleWindow\n");
            } else {
                offsets_togglewindow.enableSound = cls->findProperty(Name("enableSound"))->offset;
                offsets_togglewindow.disableSound = cls->findProperty(Name("disableSound"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ModalWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ModalWindow\n");
            } else {
                offsets_modalwindow.focusMode = cls->findProperty(Name("focusMode"))->offset;
                offsets_modalwindow.preferredFocus = cls->findProperty(Name("preferredFocus"))->offset;
                offsets_modalwindow.tabGroupWindowList = cls->findProperty(Name("tabGroupWindowList"))->offset;
                offsets_modalwindow.acceleratorTable = cls->findProperty(Name("acceleratorTable"))->offset;
                offsets_modalwindow.bDirtyAccelerators = cls->findProperty(Name("bDirtyAccelerators"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.PlayerPawnExt");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.PlayerPawnExt\n");
            } else {
                offsets_playerpawnext.FlagBase = cls->findProperty(Name("FlagBase"))->offset;
                offsets_playerpawnext.RootWindow = cls->findProperty(Name("RootWindow"))->offset;
                offsets_playerpawnext.actorList = cls->findProperty(Name("actorList"))->offset;
                offsets_playerpawnext.actorCount = cls->findProperty(Name("actorCount"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.FlagBase");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.FlagBase\n");
            } else {
                offsets_flagbase.hashTable = cls->findProperty(Name("hashTable"))->offset;
                offsets_flagbase.defaultFlagExpiration = cls->findProperty(Name("defaultFlagExpiration"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.LargeTextWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.LargeTextWindow\n");
            } else {
                offsets_largetextwindow.vSpace = cls->findProperty(Name("vSpace"))->offset;
                offsets_largetextwindow.lineHeight = cls->findProperty(Name("lineHeight"))->offset;
                offsets_largetextwindow.textParams = cls->findProperty(Name("textParams"))->offset;
                offsets_largetextwindow.rowData = cls->findProperty(Name("rowData"))->offset;
                offsets_largetextwindow.queryTextParams = cls->findProperty(Name("queryTextParams"))->offset;
                offsets_largetextwindow.queryRowData = cls->findProperty(Name("queryRowData"))->offset;
                offsets_largetextwindow.tempRowData = cls->findProperty(Name("tempRowData"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ScaleManagerWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ScaleManagerWindow\n");
            } else {
                offsets_scalemanagerwindow.decButton = cls->findProperty(Name("decButton"))->offset;
                offsets_scalemanagerwindow.incButton = cls->findProperty(Name("incButton"))->offset;
                offsets_scalemanagerwindow.valueField = cls->findProperty(Name("valueField"))->offset;
                offsets_scalemanagerwindow.Scale = cls->findProperty(Name("Scale"))->offset;
                offsets_scalemanagerwindow.orientation = cls->findProperty(Name("orientation"))->offset;
                offsets_scalemanagerwindow.bStretchScaleField = cls->findProperty(Name("bStretchScaleField"))->offset;
                offsets_scalemanagerwindow.bStretchValueField = cls->findProperty(Name("bStretchValueField"))->offset;
                offsets_scalemanagerwindow.marginWidth = cls->findProperty(Name("marginWidth"))->offset;
                offsets_scalemanagerwindow.marginHeight = cls->findProperty(Name("marginHeight"))->offset;
                offsets_scalemanagerwindow.Spacing = cls->findProperty(Name("Spacing"))->offset;
                offsets_scalemanagerwindow.childHAlign = cls->findProperty(Name("childHAlign"))->offset;
                offsets_scalemanagerwindow.childVAlign = cls->findProperty(Name("childVAlign"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.EditWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.EditWindow\n");
            } else {
                offsets_editwindow.bEditable = cls->findProperty(Name("bEditable"))->offset;
                offsets_editwindow.bSingleLine = cls->findProperty(Name("bSingleLine"))->offset;
                offsets_editwindow.bUppercaseOnly = cls->findProperty(Name("bUppercaseOnly"))->offset;
                offsets_editwindow.insertPos = cls->findProperty(Name("insertPos"))->offset;
                offsets_editwindow.insertHookPos = cls->findProperty(Name("insertHookPos"))->offset;
                offsets_editwindow.insertType = cls->findProperty(Name("insertType"))->offset;
                offsets_editwindow.selectStart = cls->findProperty(Name("selectStart"))->offset;
                offsets_editwindow.selectEnd = cls->findProperty(Name("selectEnd"))->offset;
                offsets_editwindow.maxSize = cls->findProperty(Name("maxSize"))->offset;
                offsets_editwindow.insertTexture = cls->findProperty(Name("insertTexture"))->offset;
                offsets_editwindow.insertColor = cls->findProperty(Name("insertColor"))->offset;
                offsets_editwindow.selectTexture = cls->findProperty(Name("selectTexture"))->offset;
                offsets_editwindow.selectColor = cls->findProperty(Name("selectColor"))->offset;
                offsets_editwindow.inverseColor = cls->findProperty(Name("inverseColor"))->offset;
                offsets_editwindow.editCursor = cls->findProperty(Name("editCursor"))->offset;
                offsets_editwindow.editCursorShadow = cls->findProperty(Name("editCursorShadow"))->offset;
                offsets_editwindow.editCursorColor = cls->findProperty(Name("editCursorColor"))->offset;
                offsets_editwindow.typeSound = cls->findProperty(Name("typeSound"))->offset;
                offsets_editwindow.deleteSound = cls->findProperty(Name("deleteSound"))->offset;
                offsets_editwindow.enterSound = cls->findProperty(Name("enterSound"))->offset;
                offsets_editwindow.moveSound = cls->findProperty(Name("moveSound"))->offset;
                offsets_editwindow.bufferList = cls->findProperty(Name("bufferList"))->offset;
                offsets_editwindow.currentUndo = cls->findProperty(Name("currentUndo"))->offset;
                offsets_editwindow.maxUndos = cls->findProperty(Name("maxUndos"))->offset;
                offsets_editwindow.unchangedUndo = cls->findProperty(Name("unchangedUndo"))->offset;
                offsets_editwindow.dragDelay = cls->findProperty(Name("dragDelay"))->offset;
                offsets_editwindow.blinkDelay = cls->findProperty(Name("blinkDelay"))->offset;
                offsets_editwindow.blinkStart = cls->findProperty(Name("blinkStart"))->offset;
                offsets_editwindow.blinkPeriod = cls->findProperty(Name("blinkPeriod"))->offset;
                offsets_editwindow.bCursorShowing = cls->findProperty(Name("bCursorShowing"))->offset;
                offsets_editwindow.bDragging = cls->findProperty(Name("bDragging"))->offset;
                offsets_editwindow.bSelectWords = cls->findProperty(Name("bSelectWords"))->offset;
                offsets_editwindow.insertX = cls->findProperty(Name("insertX"))->offset;
                offsets_editwindow.insertY = cls->findProperty(Name("insertY"))->offset;
                offsets_editwindow.insertWidth = cls->findProperty(Name("insertWidth"))->offset;
                offsets_editwindow.insertHeight = cls->findProperty(Name("insertHeight"))->offset;
                offsets_editwindow.insertPrefWidth = cls->findProperty(Name("insertPrefWidth"))->offset;
                offsets_editwindow.insertPrefHeight = cls->findProperty(Name("insertPrefHeight"))->offset;
                offsets_editwindow.showAreaX = cls->findProperty(Name("showAreaX"))->offset;
                offsets_editwindow.showAreaY = cls->findProperty(Name("showAreaY"))->offset;
                offsets_editwindow.showAreaWidth = cls->findProperty(Name("showAreaWidth"))->offset;
                offsets_editwindow.showAreaHeight = cls->findProperty(Name("showAreaHeight"))->offset;
                offsets_editwindow.insertPreferredCol = cls->findProperty(Name("insertPreferredCol"))->offset;
                offsets_editwindow.lastConfigWidth = cls->findProperty(Name("lastConfigWidth"))->offset;
                offsets_editwindow.lastConfigHeight = cls->findProperty(Name("lastConfigHeight"))->offset;
                offsets_editwindow.selectStartRow = cls->findProperty(Name("selectStartRow"))->offset;
                offsets_editwindow.selectEndRow = cls->findProperty(Name("selectEndRow"))->offset;
                offsets_editwindow.selectStartX = cls->findProperty(Name("selectStartX"))->offset;
                offsets_editwindow.selectEndX = cls->findProperty(Name("selectEndX"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ClipWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ClipWindow\n");
            } else {
                offsets_clipwindow.childH = cls->findProperty(Name("childH"))->offset;
                offsets_clipwindow.childV = cls->findProperty(Name("childV"))->offset;
                offsets_clipwindow.prefHUnits = cls->findProperty(Name("prefHUnits"))->offset;
                offsets_clipwindow.prefVUnits = cls->findProperty(Name("prefVUnits"))->offset;
                offsets_clipwindow.bForceChildWidth = cls->findProperty(Name("bForceChildWidth"))->offset;
                offsets_clipwindow.bForceChildHeight = cls->findProperty(Name("bForceChildHeight"))->offset;
                offsets_clipwindow.bSnapToUnits = cls->findProperty(Name("bSnapToUnits"))->offset;
                offsets_clipwindow.bFillWindow = cls->findProperty(Name("bFillWindow"))->offset;
                offsets_clipwindow.areaHSize = cls->findProperty(Name("areaHSize"))->offset;
                offsets_clipwindow.areaVSize = cls->findProperty(Name("areaVSize"))->offset;
                offsets_clipwindow.childHSize = cls->findProperty(Name("childHSize"))->offset;
                offsets_clipwindow.childVSize = cls->findProperty(Name("childVSize"))->offset;
                offsets_clipwindow.hMult = cls->findProperty(Name("hMult"))->offset;
                offsets_clipwindow.vMult = cls->findProperty(Name("vMult"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.BorderWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.BorderWindow\n");
            } else {
                offsets_borderwindow.borderLeft = cls->findProperty(Name("borderLeft"))->offset;
                offsets_borderwindow.borderTopLeft = cls->findProperty(Name("borderTopLeft"))->offset;
                offsets_borderwindow.borderTop = cls->findProperty(Name("borderTop"))->offset;
                offsets_borderwindow.borderTopRight = cls->findProperty(Name("borderTopRight"))->offset;
                offsets_borderwindow.borderRight = cls->findProperty(Name("borderRight"))->offset;
                offsets_borderwindow.borderBottomRight = cls->findProperty(Name("borderBottomRight"))->offset;
                offsets_borderwindow.borderBottom = cls->findProperty(Name("borderBottom"))->offset;
                offsets_borderwindow.borderBottomLeft = cls->findProperty(Name("borderBottomLeft"))->offset;
                offsets_borderwindow.center = cls->findProperty(Name("center"))->offset;
                offsets_borderwindow.MoveCursor = cls->findProperty(Name("MoveCursor"))->offset;
                offsets_borderwindow.hMoveCursor = cls->findProperty(Name("hMoveCursor"))->offset;
                offsets_borderwindow.vMoveCursor = cls->findProperty(Name("vMoveCursor"))->offset;
                offsets_borderwindow.tlMoveCursor = cls->findProperty(Name("tlMoveCursor"))->offset;
                offsets_borderwindow.trMoveCursor = cls->findProperty(Name("trMoveCursor"))->offset;
                offsets_borderwindow.borderStyle = cls->findProperty(Name("borderStyle"))->offset;
                offsets_borderwindow.bSmoothBorder = cls->findProperty(Name("bSmoothBorder"))->offset;
                offsets_borderwindow.bStretchBorder = cls->findProperty(Name("bStretchBorder"))->offset;
                offsets_borderwindow.bResizeable = cls->findProperty(Name("bResizeable"))->offset;
                offsets_borderwindow.bMarginsFromBorder = cls->findProperty(Name("bMarginsFromBorder"))->offset;
                offsets_borderwindow.childLeftMargin = cls->findProperty(Name("childLeftMargin"))->offset;
                offsets_borderwindow.childRightMargin = cls->findProperty(Name("childRightMargin"))->offset;
                offsets_borderwindow.childTopMargin = cls->findProperty(Name("childTopMargin"))->offset;
                offsets_borderwindow.childBottomMargin = cls->findProperty(Name("childBottomMargin"))->offset;
                offsets_borderwindow.leftMargin = cls->findProperty(Name("leftMargin"))->offset;
                offsets_borderwindow.rightMargin = cls->findProperty(Name("rightMargin"))->offset;
                offsets_borderwindow.TopMargin = cls->findProperty(Name("TopMargin"))->offset;
                offsets_borderwindow.BottomMargin = cls->findProperty(Name("BottomMargin"))->offset;
                offsets_borderwindow.bLeftDrag = cls->findProperty(Name("bLeftDrag"))->offset;
                offsets_borderwindow.bRightDrag = cls->findProperty(Name("bRightDrag"))->offset;
                offsets_borderwindow.bUpDrag = cls->findProperty(Name("bUpDrag"))->offset;
                offsets_borderwindow.bDownDrag = cls->findProperty(Name("bDownDrag"))->offset;
                offsets_borderwindow.lastMouseX = cls->findProperty(Name("lastMouseX"))->offset;
                offsets_borderwindow.lastMouseY = cls->findProperty(Name("lastMouseY"))->offset;
                offsets_borderwindow.DragX = cls->findProperty(Name("DragX"))->offset;
                offsets_borderwindow.DragY = cls->findProperty(Name("DragY"))->offset;
                offsets_borderwindow.dragWidth = cls->findProperty(Name("dragWidth"))->offset;
                offsets_borderwindow.dragHeight = cls->findProperty(Name("dragHeight"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.CheckboxWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.CheckboxWindow\n");
            } else {
                offsets_checkboxwindow.toggleOff = cls->findProperty(Name("toggleOff"))->offset;
                offsets_checkboxwindow.toggleOn = cls->findProperty(Name("toggleOn"))->offset;
                offsets_checkboxwindow.textureWidth = cls->findProperty(Name("textureWidth"))->offset;
                offsets_checkboxwindow.textureHeight = cls->findProperty(Name("textureHeight"))->offset;
                offsets_checkboxwindow.checkboxSpacing = cls->findProperty(Name("checkboxSpacing"))->offset;
                offsets_checkboxwindow.bRightSide = cls->findProperty(Name("bRightSide"))->offset;
                offsets_checkboxwindow.checkboxStyle = cls->findProperty(Name("checkboxStyle"))->offset;
                offsets_checkboxwindow.checkboxColor = cls->findProperty(Name("checkboxColor"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ComputerWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ComputerWindow\n");
            } else {
                offsets_computerwindow.eventTimeInterval = cls->findProperty(Name("eventTimeInterval"))->offset;
                offsets_computerwindow.timeNextEvent = cls->findProperty(Name("timeNextEvent"))->offset;
                offsets_computerwindow.timeLastEvent = cls->findProperty(Name("timeLastEvent"))->offset;
                offsets_computerwindow.timeCurrent = cls->findProperty(Name("timeCurrent"))->offset;
                offsets_computerwindow.fadeSpeed = cls->findProperty(Name("fadeSpeed"))->offset;
                offsets_computerwindow.throttle = cls->findProperty(Name("throttle"))->offset;
                offsets_computerwindow.fadeOutTimer = cls->findProperty(Name("fadeOutTimer"))->offset;
                offsets_computerwindow.fadeOutStart = cls->findProperty(Name("fadeOutStart"))->offset;
                offsets_computerwindow.textCols = cls->findProperty(Name("textCols"))->offset;
                offsets_computerwindow.textRows = cls->findProperty(Name("textRows"))->offset;
                offsets_computerwindow.textFont = cls->findProperty(Name("textFont"))->offset;
                offsets_computerwindow.fontWidth = cls->findProperty(Name("fontWidth"))->offset;
                offsets_computerwindow.fontHeight = cls->findProperty(Name("fontHeight"))->offset;
                offsets_computerwindow.FontColor = cls->findProperty(Name("FontColor"))->offset;
                offsets_computerwindow.textSound = cls->findProperty(Name("textSound"))->offset;
                offsets_computerwindow.typingSound = cls->findProperty(Name("typingSound"))->offset;
                offsets_computerwindow.computerSoundVolume = cls->findProperty(Name("computerSoundVolume"))->offset;
                offsets_computerwindow.bWordWrap = cls->findProperty(Name("bWordWrap"))->offset;
                offsets_computerwindow.bLastLineWrapped = cls->findProperty(Name("bLastLineWrapped"))->offset;
                offsets_computerwindow.bInvokeComputerStart = cls->findProperty(Name("bInvokeComputerStart"))->offset;
                offsets_computerwindow.bComputerStartInvoked = cls->findProperty(Name("bComputerStartInvoked"))->offset;
                offsets_computerwindow.bFirstTick = cls->findProperty(Name("bFirstTick"))->offset;
                offsets_computerwindow.backgroundTextures = cls->findProperty(Name("backgroundTextures"))->offset;
                offsets_computerwindow.cursorTexture = cls->findProperty(Name("cursorTexture"))->offset;
                offsets_computerwindow.cursorColor = cls->findProperty(Name("cursorColor"))->offset;
                offsets_computerwindow.cursorWidth = cls->findProperty(Name("cursorWidth"))->offset;
                offsets_computerwindow.cursorHeight = cls->findProperty(Name("cursorHeight"))->offset;
                offsets_computerwindow.cursorBlinkSpeed = cls->findProperty(Name("cursorBlinkSpeed"))->offset;
                offsets_computerwindow.bCursorVisible = cls->findProperty(Name("bCursorVisible"))->offset;
                offsets_computerwindow.bShowCursor = cls->findProperty(Name("bShowCursor"))->offset;
                offsets_computerwindow.cursorNextEvent = cls->findProperty(Name("cursorNextEvent"))->offset;
                offsets_computerwindow.colGraphicTile = cls->findProperty(Name("colGraphicTile"))->offset;
                offsets_computerwindow.TextWindow = cls->findProperty(Name("TextWindow"))->offset;
                offsets_computerwindow.Player = cls->findProperty(Name("Player"))->offset;
                offsets_computerwindow.displayBuffer = cls->findProperty(Name("displayBuffer"))->offset;
                offsets_computerwindow.queuedBuffer = cls->findProperty(Name("queuedBuffer"))->offset;
                offsets_computerwindow.queuedBufferStart = cls->findProperty(Name("queuedBufferStart"))->offset;
                offsets_computerwindow.TextX = cls->findProperty(Name("TextX"))->offset;
                offsets_computerwindow.TextY = cls->findProperty(Name("TextY"))->offset;
                offsets_computerwindow.inputKey = cls->findProperty(Name("inputKey"))->offset;
                offsets_computerwindow.inputMask = cls->findProperty(Name("inputMask"))->offset;
                offsets_computerwindow.inputWindow = cls->findProperty(Name("inputWindow"))->offset;
                offsets_computerwindow.bWaitingForKey = cls->findProperty(Name("bWaitingForKey"))->offset;
                offsets_computerwindow.bEchoKey = cls->findProperty(Name("bEchoKey"))->offset;
                offsets_computerwindow.bPauseProcessing = cls->findProperty(Name("bPauseProcessing"))->offset;
                offsets_computerwindow.bIgnoreTick = cls->findProperty(Name("bIgnoreTick"))->offset;
                offsets_computerwindow.bGamePaused = cls->findProperty(Name("bGamePaused"))->offset;
                offsets_computerwindow.bIgnoreGamePaused = cls->findProperty(Name("bIgnoreGamePaused"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ListWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ListWindow\n");
            } else {
                offsets_listwindow.Delimiter = cls->findProperty(Name("Delimiter"))->offset;
                offsets_listwindow.inverseColor = cls->findProperty(Name("inverseColor"))->offset;
                offsets_listwindow.highlightTexture = cls->findProperty(Name("highlightTexture"))->offset;
                offsets_listwindow.focusTexture = cls->findProperty(Name("focusTexture"))->offset;
                offsets_listwindow.highlightColor = cls->findProperty(Name("highlightColor"))->offset;
                offsets_listwindow.focusColor = cls->findProperty(Name("focusColor"))->offset;
                offsets_listwindow.focusThickness = cls->findProperty(Name("focusThickness"))->offset;
                offsets_listwindow.rows = cls->findProperty(Name("rows"))->offset;
                offsets_listwindow.cols = cls->findProperty(Name("cols"))->offset;
                offsets_listwindow.bAutoSort = cls->findProperty(Name("bAutoSort"))->offset;
                offsets_listwindow.bAutoExpandColumns = cls->findProperty(Name("bAutoExpandColumns"))->offset;
                offsets_listwindow.bMultiSelect = cls->findProperty(Name("bMultiSelect"))->offset;
                offsets_listwindow.colMargin = cls->findProperty(Name("colMargin"))->offset;
                offsets_listwindow.rowMargin = cls->findProperty(Name("rowMargin"))->offset;
                offsets_listwindow.bHotKeys = cls->findProperty(Name("bHotKeys"))->offset;
                offsets_listwindow.hotKeyCol = cls->findProperty(Name("hotKeyCol"))->offset;
                offsets_listwindow.lineSize = cls->findProperty(Name("lineSize"))->offset;
                offsets_listwindow.ActivateSound = cls->findProperty(Name("ActivateSound"))->offset;
                offsets_listwindow.moveSound = cls->findProperty(Name("moveSound"))->offset;
                offsets_listwindow.numSelected = cls->findProperty(Name("numSelected"))->offset;
                offsets_listwindow.focusLine = cls->findProperty(Name("focusLine"))->offset;
                offsets_listwindow.anchorLine = cls->findProperty(Name("anchorLine"))->offset;
                offsets_listwindow.bDragging = cls->findProperty(Name("bDragging"))->offset;
                offsets_listwindow.lastIndex = cls->findProperty(Name("lastIndex"))->offset;
                offsets_listwindow.remainingDelay = cls->findProperty(Name("remainingDelay"))->offset;
                offsets_listwindow.hotKeyString = cls->findProperty(Name("hotKeyString"))->offset;
                offsets_listwindow.hotKeyTimer = cls->findProperty(Name("hotKeyTimer"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.RadioBoxWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.RadioBoxWindow\n");
            } else {
                offsets_radioboxwindow.bOneCheck = cls->findProperty(Name("bOneCheck"))->offset;
                offsets_radioboxwindow.toggleButtons = cls->findProperty(Name("toggleButtons"))->offset;
                offsets_radioboxwindow.currentSelection = cls->findProperty(Name("currentSelection"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ScrollAreaWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ScrollAreaWindow\n");
            } else {
                offsets_scrollareawindow.hScaleMgr = cls->findProperty(Name("hScaleMgr"))->offset;
                offsets_scrollareawindow.vScaleMgr = cls->findProperty(Name("vScaleMgr"))->offset;
                offsets_scrollareawindow.hScale = cls->findProperty(Name("hScale"))->offset;
                offsets_scrollareawindow.vScale = cls->findProperty(Name("vScale"))->offset;
                offsets_scrollareawindow.LeftButton = cls->findProperty(Name("LeftButton"))->offset;
                offsets_scrollareawindow.RightButton = cls->findProperty(Name("RightButton"))->offset;
                offsets_scrollareawindow.UpButton = cls->findProperty(Name("UpButton"))->offset;
                offsets_scrollareawindow.DownButton = cls->findProperty(Name("DownButton"))->offset;
                offsets_scrollareawindow.ClipWindow = cls->findProperty(Name("ClipWindow"))->offset;
                offsets_scrollareawindow.marginWidth = cls->findProperty(Name("marginWidth"))->offset;
                offsets_scrollareawindow.marginHeight = cls->findProperty(Name("marginHeight"))->offset;
                offsets_scrollareawindow.scrollbarDistance = cls->findProperty(Name("scrollbarDistance"))->offset;
                offsets_scrollareawindow.bHideScrollbars = cls->findProperty(Name("bHideScrollbars"))->offset;
                offsets_scrollareawindow.bHLastShow = cls->findProperty(Name("bHLastShow"))->offset;
                offsets_scrollareawindow.bVLastShow = cls->findProperty(Name("bVLastShow"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ExtString");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ExtString\n");
            } else {
                offsets_extstring.speechPage = cls->findProperty(Name("speechPage"))->offset;
                offsets_extstring.Text = cls->findProperty(Name("Text"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.TextLogWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.TextLogWindow\n");
            } else {
                offsets_textlogwindow.textTimeout = cls->findProperty(Name("textTimeout"))->offset;
                offsets_textlogwindow.Lines = cls->findProperty(Name("Lines"))->offset;
                offsets_textlogwindow.bTooTall = cls->findProperty(Name("bTooTall"))->offset;
                offsets_textlogwindow.bPaused = cls->findProperty(Name("bPaused"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.TileWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.TileWindow\n");
            } else {
                offsets_tilewindow.orientation = cls->findProperty(Name("orientation"))->offset;
                offsets_tilewindow.hDirection = cls->findProperty(Name("hDirection"))->offset;
                offsets_tilewindow.vDirection = cls->findProperty(Name("vDirection"))->offset;
                offsets_tilewindow.hMargin = cls->findProperty(Name("hMargin"))->offset;
                offsets_tilewindow.vMargin = cls->findProperty(Name("vMargin"))->offset;
                offsets_tilewindow.minorSpacing = cls->findProperty(Name("minorSpacing"))->offset;
                offsets_tilewindow.majorSpacing = cls->findProperty(Name("majorSpacing"))->offset;
                offsets_tilewindow.hChildAlign = cls->findProperty(Name("hChildAlign"))->offset;
                offsets_tilewindow.vChildAlign = cls->findProperty(Name("vChildAlign"))->offset;
                offsets_tilewindow.bWrap = cls->findProperty(Name("bWrap"))->offset;
                offsets_tilewindow.bFillParent = cls->findProperty(Name("bFillParent"))->offset;
                offsets_tilewindow.bEqualWidth = cls->findProperty(Name("bEqualWidth"))->offset;
                offsets_tilewindow.bEqualHeight = cls->findProperty(Name("bEqualHeight"))->offset;
                offsets_tilewindow.rowArray = cls->findProperty(Name("rowArray"))->offset;
            }
        }
        {
            UClass* cls = objects::findClass("Extension.ViewportWindow");
            if (!cls) {
                std::fprintf(stderr, "missing class Extension.ViewportWindow\n");
            } else {
                offsets_viewportwindow.bEnableViewport = cls->findProperty(Name("bEnableViewport"))->offset;
                offsets_viewportwindow.bClearZ = cls->findProperty(Name("bClearZ"))->offset;
                offsets_viewportwindow.bShowActor = cls->findProperty(Name("bShowActor"))->offset;
                offsets_viewportwindow.bShowWeapons = cls->findProperty(Name("bShowWeapons"))->offset;
                offsets_viewportwindow.bUseViewRotation = cls->findProperty(Name("bUseViewRotation"))->offset;
                offsets_viewportwindow.bUseEyeHeight = cls->findProperty(Name("bUseEyeHeight"))->offset;
                offsets_viewportwindow.bWatchEyeHeight = cls->findProperty(Name("bWatchEyeHeight"))->offset;
                offsets_viewportwindow.FOV = cls->findProperty(Name("FOV"))->offset;
                offsets_viewportwindow.DefaultTexture = cls->findProperty(Name("DefaultTexture"))->offset;
                offsets_viewportwindow.DefaultColor = cls->findProperty(Name("DefaultColor"))->offset;
                offsets_viewportwindow.originActor = cls->findProperty(Name("originActor"))->offset;
                offsets_viewportwindow.watchActor = cls->findProperty(Name("watchActor"))->offset;
                offsets_viewportwindow.Location = cls->findProperty(Name("Location"))->offset;
                offsets_viewportwindow.relLocation = cls->findProperty(Name("relLocation"))->offset;
                offsets_viewportwindow.Rotation = cls->findProperty(Name("Rotation"))->offset;
                offsets_viewportwindow.relRotation = cls->findProperty(Name("relRotation"))->offset;
                offsets_viewportwindow.bOriginActorDestroyed = cls->findProperty(Name("bOriginActorDestroyed"))->offset;
                offsets_viewportwindow.lastLocation = cls->findProperty(Name("lastLocation"))->offset;
                offsets_viewportwindow.lastRotation = cls->findProperty(Name("lastRotation"))->offset;
                offsets_viewportwindow.viewportFrame = cls->findProperty(Name("viewportFrame"))->offset;
            }
        }
    }
}
