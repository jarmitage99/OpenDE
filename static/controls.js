(() => {
    const canvas = document.getElementById("canvas");
    const shortcuts = document.getElementById("game-shortcuts");
    const crouchShortcut = document.getElementById("shortcut-crouch");
    const touchLaptop = document.getElementById("touch-laptop");
    const touchLaptopEnabled = () => touchLaptop?.checked ?? true;
    // Increase this for longer steps per wheel tick.
    const holdMilliseconds = 180;
    const turnSpeed = 50; // Original turn speed is 150.
    // Give a second touchpad finger time to turn a primary press into a secondary press.
    const clickDebounceMilliseconds = 50;
    const touchDeadZone = 8;
    const touchFastLookDistance = 36;
    const touchHoldMilliseconds = 350;
    const doubleTapMilliseconds = 250;
    const doubleTapDistance = 32;
    const recenterHoldMilliseconds = 120;
    let recenterTimer = null;
    // Jump+Fwd phases: run up, hold jump, then keep moving after releasing jump.
    const forwardJumpLeadMilliseconds = 80;
    const forwardJumpHoldMilliseconds = 220;
    const forwardJumpTailMilliseconds = 120;
    const keys = {
        KeyW: { key: "w", code: "KeyW", keyCode: 87, which: 87 },
        KeyA: { key: "a", code: "KeyA", keyCode: 65, which: 65 },
        KeyS: { key: "s", code: "KeyS", keyCode: 83, which: 83 },
        KeyD: { key: "d", code: "KeyD", keyCode: 68, which: 68 },
        KeyZ: { key: "z", code: "KeyZ", keyCode: 90, which: 90 },
        KeyC: { key: "c", code: "KeyC", keyCode: 67, which: 67 },
        KeyX: { key: "x", code: "KeyX", keyCode: 88, which: 88 },
        KeyF: { key: "f", code: "KeyF", keyCode: 70, which: 70 },
        KeyR: { key: "r", code: "KeyR", keyCode: 82, which: 82 },
        Escape: { key: "Escape", code: "Escape", keyCode: 27, which: 27 },
        End: { key: "End", code: "End", keyCode: 35, which: 35 },
        Space: { key: " ", code: "Space", keyCode: 32, which: 32 },
        Backspace: { key: "Backspace", code: "Backspace", keyCode: 8, which: 8 },
        Tab: { key: "Tab", code: "Tab", keyCode: 9, which: 9 },
        KeyI: { key: "i", code: "KeyI", keyCode: 73, which: 73 },
        BracketLeft: { key: "[", code: "BracketLeft", keyCode: 219, which: 219 },
        Delete: { key: "Delete", code: "Delete", keyCode: 46, which: 46 },
        PageDown: { key: "PageDown", code: "PageDown", keyCode: 34, which: 34 }
    };
    const movementKeys = new Set(["KeyW", "KeyA", "KeyS", "KeyD"]);
    const touchKeys = new Set();
    const touchLookKeys = [
        ["KeyZ", "KeyC", "Delete", "PageDown"]
    ];
    // Targets are the actions bound in the shipped demo's User.ini.
    // Use the shipped letter binding; function-key translation varies in the web build.
    const remappedKeys = { KeyT: "Tab", KeyZ: "BracketLeft", Tab: "KeyI" };
    const remappedHeld = new Map();
    const cycleKeysHeld = new Set();
    const physicalKeys = new Set();
    const syntheticEvents = new WeakSet();
    let touchMode = touchLaptopEnabled() && (window.matchMedia?.("(hover: none) and (pointer: coarse)").matches ?? false);
    let ready = false;
    let crouchHeld = false;
    let crouchToggled = false;
    let crouchToggleKeyHeld = false;
    let crouchSent = false;
    const scrollAxes = {
        vertical: { code: null, timer: null, pressed: false },
        horizontal: { code: null, timer: null, pressed: false }
    };

    // SDL can replay a deferred pointer-lock request during a touch callback.
    // Keep the guard on the canvas so it covers both deferred and direct requests.
    const requestPointerLock = canvas.requestPointerLock;
    if (requestPointerLock) {
        canvas.requestPointerLock = function (...args) {
            if (!touchMode) return requestPointerLock.apply(this, args);
        };
    }

    function enterTouchMode() {
        if (!touchLaptopEnabled()) return;
        touchMode = true;
        if (document.pointerLockElement === canvas) document.exitPointerLock?.();
    }

    canvas.addEventListener("pointerdown", event => {
        if (event.pointerType === "touch") {
            enterTouchMode();
        } else if (touchMode && event.isTrusted && event.pointerType === "mouse") {
            resetTouch();
            touchMode = false;
        }
    }, true);

    // Run after both the demo package and saved settings have loaded, before main().
    const previousRuntimeInitialized = Module.onRuntimeInitialized;
    Module.onRuntimeInitialized = () => {
        const path = "/home/web_user/.local/share/deusex/User.ini";
        const source = FS.analyzePath(path).exists ? path : "/game/System/User.ini";
        const bindings = { End: "CenterView", F: "NextBeltItem", T: "DropItem", V: "PrevBeltItem", Tab: "DropItem", X: "Duck", Y: "PutInHand", R: "ReloadWeapon", I: "ShowInventoryWindow", F1: "ShowInventoryWindow", MouseWheelDown: "NextBeltItem", MouseWheelUp: "PrevBeltItem" };
        const remaining = new Set(Object.keys(bindings));
        const settingsText = FS.readFile(source, { encoding: "utf8" });
        const newline = settingsText.includes("\r\n") ? "\r\n" : "\n";
        let inInput = false;
        const lines = settingsText.split(/\r?\n/);
        // Use unassigned keypad keys for independent slow and fast look axes.
        // Leave saved keypad bindings alone and fall back to the original controls as needed.
        if (typeof navigator !== "undefined" && navigator.maxTouchPoints > 0) {
            const input = settingsText.split(/^\s*\[Engine\.Input\]\s*$/im)[1]?.split(/^\s*\[/m)[0] ?? "";
            const keypad = new Map(Array.from(input.matchAll(/^\s*NumPad([0-7])[^\S\r\n]*=([^\r\n]*)/gim),
                match => [Number(match[1]), match[2].trim()]));
            const axisSpeeds = [
                ["aBaseX", -turnSpeed], ["aBaseX", turnSpeed],
                ["aLookUp", 35], ["aLookUp", -35]
            ];
            for (let tier = 0; tier < 2; tier++) {
                touchLookKeys[tier] = axisSpeeds.map(([axis, speed], direction) => {
                    const digit = tier * 4 + direction;
                    const code = `Numpad${digit}`;
                    const iniKey = `NumPad${digit}`;
                    const command = `Axis ${axis} Speed=${speed * (tier + 1)}.0`;
                    if (keypad.get(digit) && keypad.get(digit) !== command) return touchLookKeys[0][direction];
                    keys[code] = { key: String(digit), code, keyCode: 96 + digit, which: 96 + digit, location: 3 };
                    bindings[iniKey] = command;
                    remaining.add(iniKey);
                    return code;
                });
            }
        }
        const bindingNames = new Map(Object.keys(bindings).map(key => [key.toLowerCase(), key]));
        const output = [];
        function appendMissingBindings() {
            for (const key of remaining) output.push(`${key}=${bindings[key]}`);
            remaining.clear();
        }
        for (let line of lines) {
            const section = line.match(/^\s*\[([^\]]+)\]/);
            if (section) {
                if (inInput) appendMissingBindings();
                inInput = section[1].toLowerCase() === "engine.input";
            }
            if (inInput) {
                const binding = line.match(/^\s*(\w+)\s*=/);
                const key = binding && bindingNames.get(binding[1].toLowerCase());
                if (key) {
                    line = `${key}=${bindings[key]}`;
                    remaining.delete(key);
                }
                // Continuous key holds use a slower axis speed rather than key pulses.
                line = line.replace(
                    /^(\s*Aliases\[\d+\]\s*=\s*\(Command=")[^"]*(",\s*Alias=Turn(Left|Right)\s*\))\s*$/i,
                    (_, prefix, suffix, direction) => `${prefix}Axis aBaseX Speed=${direction.toLowerCase() === "left" ? "-" : "+"}${turnSpeed}.0${suffix}`
                );
            }
            output.push(line);
        }
        if (inInput) appendMissingBindings();
        FS.mkdirTree("/home/web_user/.local/share/deusex");
        // The shim supplies both hold and toggle behavior, so the game must use hold mode.
        let settings = output.join(newline);
        const playerSection = /(^\[DeusEx\.DeusExPlayer\][^\[]*)/im;
        if (playerSection.test(settings)) {
            settings = settings.replace(playerSection, section =>
                /^\s*bToggleCrouch\s*=/im.test(section)
                    ? section.replace(/^(\s*bToggleCrouch\s*=)[^\r\n]*/im, "$1False")
                    : section + `bToggleCrouch=False${newline}`);
        } else {
            settings += `${newline}[DeusEx.DeusExPlayer]${newline}bToggleCrouch=False${newline}`;
        }
        FS.writeFile(path, settings);
        previousRuntimeInitialized?.();
    };

    function sendKey(type, code, applyShortcuts = false) {
        // SDL's Emscripten keyboard handler reads both code and legacy keyCode.
        const event = new KeyboardEvent(type, {
            ...keys[code], bubbles: true, cancelable: true
        });
        if (!applyShortcuts) syntheticEvents.add(event);
        canvas.dispatchEvent(event);
    }

    function releaseRecenter() {
        if (recenterTimer === null) return;
        clearTimeout(recenterTimer);
        recenterTimer = null;
        sendKey("keyup", "End");
    }

    function recenterView() {
        // CenterView is "Button bSnapLevel" in the demo: keep it down across
        // a game frame so the player can consume the input before keyup clears it.
        if (recenterTimer === null) sendKey("keydown", "End");
        clearTimeout(recenterTimer);
        recenterTimer = setTimeout(releaseRecenter, recenterHoldMilliseconds);
    }

    function updateCrouch() {
        crouchShortcut?.setAttribute("aria-pressed", String(crouchToggled));
        const pressed = crouchHeld || crouchToggled;
        if (pressed === crouchSent) return;
        crouchSent = pressed;
        sendKey(pressed ? "keydown" : "keyup", "KeyX");
    }

    function releaseCrouch() {
        crouchHeld = false;
        crouchToggled = false;
        crouchToggleKeyHeld = false;
        updateCrouch();
    }

    function releaseAxis(axis) {
        clearTimeout(axis.timer);
        if (axis.pressed && axis.code && !physicalKeys.has(axis.code) && !touchKeys.has(axis.code)) {
            sendKey("keyup", axis.code);
        }
        axis.code = null;
        axis.pressed = false;
    }

    function release() {
        Object.values(scrollAxes).forEach(releaseAxis);
    }

    function holdAxis(axis, code) {
        if (axis.code !== code) {
            releaseAxis(axis);
            axis.code = code;
            axis.pressed = true;
            if (!touchKeys.has(code)) sendKey("keydown", code);
        }
        clearTimeout(axis.timer);
        axis.timer = setTimeout(() => releaseAxis(axis), holdMilliseconds);
    }

    Module.postRun.push(() => {
        ready = true;
        if (shortcuts) shortcuts.disabled = false;
    });

    shortcuts?.addEventListener("pointerdown", event => {
        if (event.pointerType === "touch") enterTouchMode();
    });
    // Using the buttons by pointer should keep canvas focus and held inputs.
    shortcuts?.addEventListener("mousedown", event => {
        if (event.target.closest("button")) event.preventDefault();
    });
    shortcuts?.addEventListener("click", event => {
        const button = event.target.closest("button");
        if (!ready || !button) return;
        if (event.detail) canvas.focus({ preventScroll: true });
        if (button.dataset.action === "jump-forward") {
            startForwardJump();
            updateTouchKeys();
            return;
        }
        if (button.dataset.mouse === "2") {
            sendMouse("mousedown", event, 2);
            sendMouse("mouseup", event, 2);
            return;
        }
        const code = button.dataset.key;
        if (!code) return;
        if (code === "End") {
            recenterView();
            return;
        }
        sendKey("keydown", code, true);
        sendKey("keyup", code, true);
    });
    // Enter and Space activate a focused button through the browser.
    for (const type of ["keydown", "keyup", "keypress"]) {
        shortcuts?.addEventListener(type, event => {
            if (event.key === "Enter" || event.code === "Space") event.stopPropagation();
        });
    }

    // Capture before SDL so wheel movement replaces the game's wheel binding.
    window.addEventListener("wheel", event => {
        if (!touchLaptopEnabled() || syntheticEvents.has(event) || !ready || event.ctrlKey || (!event.deltaY && !event.deltaX)) return;
        if (event.target !== canvas && document.pointerLockElement !== canvas) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        canvas.focus({ preventScroll: true });
        const horizontal = Math.abs(event.deltaX || 0);
        const vertical = Math.abs(event.deltaY || 0);
        if (horizontal > 0) {
            // Synthetic events bypass remapping: original Z/C mean TurnLeft/TurnRight.
            holdAxis(scrollAxes.horizontal, event.deltaX < 0 ? "KeyC" : "KeyZ");
        }
        if (vertical && !touchKeys.has("KeyW") && !touchKeys.has("KeyS") &&
            !physicalKeys.has("KeyW") && !physicalKeys.has("KeyS")) {
            holdAxis(scrollAxes.vertical, event.deltaY < 0 ? "KeyS" : "KeyW");
        }
    }, { capture: true, passive: false });

    // Send the game's existing wheel action directly, bypassing our movement shim.
    // One press cycles once; auto-repeat must not skip through the belt.
    function cycleItem(direction) {
        const event = new WheelEvent("wheel", {
            // SDL converts three DOM lines into one integral game wheel step.
            deltaY: direction * 3, deltaMode: WheelEvent.DOM_DELTA_LINE,
            bubbles: true, cancelable: true
        });
        syntheticEvents.add(event);
        canvas.dispatchEvent(event);
    }

    window.addEventListener("keydown", event => {
        if (syntheticEvents.has(event)) return;
        if (ready && (event.code === "KeyF" || event.code === "KeyV") && overGame(event)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            if (!cycleKeysHeld.has(event.code)) {
                cycleKeysHeld.add(event.code);
                cycleItem(event.code === "KeyF" ? 1 : -1);
            }
            return;
        }
        if (ready && (event.code === "KeyC" || event.code === "KeyX") && overGame(event)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            if (event.code === "KeyC") {
                crouchHeld = true;
            } else if (!crouchToggleKeyHeld) {
                crouchToggleKeyHeld = true;
                crouchToggled = !crouchToggled;
            }
            updateCrouch();
            return;
        }
        if (ready && remappedKeys[event.code] && overGame(event)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            if (!remappedHeld.has(event.code)) {
                const target = remappedKeys[event.code];
                remappedHeld.set(event.code, target);
                sendKey("keydown", target);
            }
            return;
        }
        if (event.code === "Space" && shortcuts?.contains(event.target)) return;
        if (movementKeys.has(event.code) || event.code === "Space" || touchLookKeys.some(tier => tier.includes(event.code))) {
            physicalKeys.add(event.code);
            if (event.code === "KeyW" || event.code === "KeyS") releaseAxis(scrollAxes.vertical);
        } else if (event.code === "Escape") {
            release();
            releaseCrouch();
            resetTouch();
        }
    }, true);
    window.addEventListener("keyup", event => {
        if (syntheticEvents.has(event)) return;
        if ((event.code === "KeyC" && crouchHeld) || (event.code === "KeyX" && crouchToggleKeyHeld)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            if (event.code === "KeyC") crouchHeld = false;
            else crouchToggleKeyHeld = false;
            updateCrouch();
            return;
        }
        if (cycleKeysHeld.delete(event.code)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            return;
        }
        if (remappedHeld.has(event.code)) {
            event.preventDefault();
            event.stopImmediatePropagation();
            sendKey("keyup", remappedHeld.get(event.code));
            remappedHeld.delete(event.code);
            return;
        }
        physicalKeys.delete(event.code);
        if (touchKeys.has(event.code)) event.stopImmediatePropagation();
    }, true);

    const leftPresses = new Set();
    let currentLeftPress = null;
    let rightHeld = false;
    let suppressLeftUntil = 0;

    function overGame(event) {
        return event.target === canvas || document.pointerLockElement === canvas;
    }

    function sendMouse(type, original, button = 0) {
        const event = new MouseEvent(type, {
            bubbles: true, cancelable: true, view: window,
            button, buttons: type === "mousedown" ? (button === 2 ? 2 : 1) : 0,
            clientX: original.clientX, clientY: original.clientY,
            screenX: original.screenX, screenY: original.screenY,
            ctrlKey: original.ctrlKey, shiftKey: original.shiftKey,
            altKey: original.altKey, metaKey: original.metaKey,
            detail: original.detail
        });
        syntheticEvents.add(event);
        canvas.dispatchEvent(event);
    }

    function cancelLeftPresses() {
        for (const press of leftPresses) {
            clearTimeout(press.timer);
            if (press.sent) sendMouse("mouseup", press.up || press.down);
        }
        leftPresses.clear();
        currentLeftPress = null;
    }

    function secondaryPress() {
        cancelLeftPresses();
        suppressLeftUntil = Date.now() + clickDebounceMilliseconds;
    }

    window.addEventListener("mousedown", event => {
        if (!touchLaptopEnabled() || !ready || syntheticEvents.has(event) || !overGame(event)) return;
        if (event.sourceCapabilities?.firesTouchEvents) {
            event.stopImmediatePropagation();
            return;
        }
        if (event.button === 2) {
            rightHeld = true;
            secondaryPress();
            return; // Forward the real secondary click immediately.
        }
        if (event.button !== 0) return;
        // Keep native focus/click behavior, but hold SDL's primary mouse input.
        event.stopImmediatePropagation();
        if (rightHeld || Date.now() < suppressLeftUntil) return;
        const press = { down: event, up: null, sent: false, timer: null };
        currentLeftPress = press;
        leftPresses.add(press);
        press.timer = setTimeout(() => {
            press.sent = true;
            sendMouse("mousedown", press.down);
            if (press.up) {
                sendMouse("mouseup", press.up);
                leftPresses.delete(press);
            }
        }, clickDebounceMilliseconds);
    }, true);

    window.addEventListener("mouseup", event => {
        if (!touchLaptopEnabled() || !ready || syntheticEvents.has(event)) return;
        if (overGame(event) && event.sourceCapabilities?.firesTouchEvents) {
            event.stopImmediatePropagation();
            return;
        }
        if (event.button === 2 && (rightHeld || overGame(event))) {
            rightHeld = false;
            secondaryPress();
            return;
        }
        if (event.button !== 0 || (!currentLeftPress && !overGame(event))) return;
        event.stopImmediatePropagation();
        const press = currentLeftPress;
        currentLeftPress = null;
        if (!press) return;
        press.up = event;
        if (press.sent) {
            sendMouse("mouseup", event);
            leftPresses.delete(press);
        }
    }, true);

    // Some macOS/browser combinations announce the secondary gesture here first.
    window.addEventListener("contextmenu", event => {
        if (touchLaptopEnabled() && ready && overGame(event)) secondaryPress();
    }, true);

    const fingers = new Map();
    let moveFinger = null;
    let lookFinger = null;
    let stationaryFinger = null;
    let touchHoldTimer = null;
    let soloLook = false;
    let pendingTap = null;
    let forwardJumpTimer = null;
    let forwardJumpHeld = false;

    function setTouchKeys(next) {
        const scrollHolds = code => Object.values(scrollAxes).some(axis => axis.pressed && axis.code === code);
        for (const code of touchKeys) {
            if (next.has(code)) continue;
            touchKeys.delete(code);
            if (!physicalKeys.has(code) && !scrollHolds(code)) sendKey("keyup", code);
        }
        for (const code of next) {
            if (touchKeys.has(code)) continue;
            touchKeys.add(code);
            if (!physicalKeys.has(code) && !scrollHolds(code)) sendKey("keydown", code);
        }
        if (next.has("KeyW") || next.has("KeyS")) releaseAxis(scrollAxes.vertical);
    }

    function updateTouchKeys() {
        const next = new Set();
        if (moveFinger && !soloLook) {
            const dx = moveFinger.point.clientX - moveFinger.startX;
            const dy = moveFinger.point.clientY - moveFinger.startY;
            if (forwardJumpTimer === null && Math.abs(dy) > touchDeadZone) next.add(dy < 0 ? "KeyW" : "KeyS");
            if (lookFinger && Math.abs(dx) > touchDeadZone) next.add(dx < 0 ? "KeyA" : "KeyD");
        }
        const look = lookFinger || moveFinger;
        if (look) {
            const dx = look.point.clientX - look.startX;
            const dy = look.point.clientY - look.startY;
            const codes = distance => touchLookKeys[Number(Math.abs(distance) >= touchFastLookDistance)] || touchLookKeys[0];
            if (Math.abs(dx) > touchDeadZone) next.add(codes(dx)[dx < 0 ? 0 : 1]);
            if ((lookFinger || soloLook) && Math.abs(dy) > touchDeadZone) next.add(codes(dy)[dy < 0 ? 2 : 3]);
        }
        if (forwardJumpTimer !== null) {
            next.add("KeyW");
            if (forwardJumpHeld) next.add("Space");
        }
        setTouchKeys(next);
    }

    function updateTouchRoles() {
        const [first = null, second = null] = fingers.values();
        // Keep both roles until a participating finger lifts, even if they cross.
        if ((first === moveFinger && second === lookFinger) ||
            (second && first === lookFinger && second === moveFinger)) return;
        soloLook = false;
        [moveFinger, lookFinger] = second && first.point.clientX > second.point.clientX
            ? [second, first] : [first, second];
        for (const finger of [moveFinger, lookFinger]) {
            if (!finger) continue;
            finger.startX = finger.point.clientX;
            finger.startY = finger.point.clientY;
        }
    }

    function tapDistance(a, b) {
        return Math.hypot(a.clientX - b.clientX, a.clientY - b.clientY);
    }

    function clearPendingTap() {
        if (!pendingTap) return null;
        const { point, timer } = pendingTap;
        clearTimeout(timer);
        pendingTap = null;
        return point;
    }

    function sendPendingTap() {
        const point = clearPendingTap();
        if (!point) return;
        // SDL's touch-to-mouse path positions the menu cursor without adding
        // relative look motion on first contact. Replay only a confirmed tap.
        const touch = {
            identifier: -1, target: canvas, clientX: point.clientX, clientY: point.clientY,
            screenX: point.screenX, screenY: point.screenY, pageX: point.pageX, pageY: point.pageY
        };
        for (const type of ["touchstart", "touchend"]) {
            const event = new Event(type, { bubbles: true, cancelable: true });
            const touches = type === "touchstart" ? [touch] : [];
            Object.assign(event, { touches, targetTouches: touches, changedTouches: [touch] });
            syntheticEvents.add(event);
            canvas.dispatchEvent(event);
        }
    }

    function clearTouchHold() {
        clearTimeout(touchHoldTimer);
        touchHoldTimer = null;
        stationaryFinger = null;
    }

    function activateTouchHold() {
        const point = stationaryFinger.point;
        clearTouchHold();
        soloLook = true;
        sendPendingTap();
        sendMouse("mousedown", point, 2);
        sendMouse("mouseup", point, 2);
    }

    function resetTouch() {
        clearTouchHold();
        clearPendingTap();
        clearTimeout(forwardJumpTimer);
        forwardJumpTimer = null;
        forwardJumpHeld = false;
        fingers.clear();
        updateTouchRoles();
        updateTouchKeys();
    }

    function startForwardJump() {
        clearTimeout(forwardJumpTimer);
        forwardJumpHeld = false;
        forwardJumpTimer = setTimeout(() => {
            forwardJumpHeld = true;
            updateTouchKeys();
            forwardJumpTimer = setTimeout(() => {
                forwardJumpHeld = false;
                updateTouchKeys();
                forwardJumpTimer = setTimeout(() => {
                    forwardJumpTimer = null;
                    updateTouchKeys();
                }, forwardJumpTailMilliseconds);
            }, forwardJumpHoldMilliseconds);
        }, forwardJumpLeadMilliseconds);
    }

    function handleTouch(event) {
        if (!touchLaptopEnabled() || !ready || syntheticEvents.has(event)) return;
        event.preventDefault();
        // Intercept before SDL. The document's capture listener can still unlock audio.
        event.stopImmediatePropagation();
        if (event.type === "touchcancel") {
            resetTouch();
            return;
        }

        if (event.type === "touchstart") {
            enterTouchMode();
            canvas.focus({ preventScroll: true });
            for (const point of event.changedTouches) {
                fingers.set(point.identifier, { point, initial: point, startedAt: event.timeStamp });
            }
            if (fingers.size === 1) {
                stationaryFinger = fingers.values().next().value;
                if (pendingTap) {
                    if (event.timeStamp <= pendingTap.deadline &&
                        tapDistance(pendingTap.point, stationaryFinger.point) <= doubleTapDistance) {
                        clearTimeout(pendingTap.timer);
                    } else {
                        sendPendingTap();
                    }
                }
                touchHoldTimer = setTimeout(activateTouchHold, touchHoldMilliseconds);
            } else {
                clearTouchHold();
                sendPendingTap();
            }
            updateTouchRoles();
            updateTouchKeys();
            return;
        }

        for (const point of event.changedTouches) {
            const finger = fingers.get(point.identifier);
            if (!finger) continue;
            finger.point = point;
            const ended = event.type === "touchend";
            const dragged = tapDistance(finger.initial, point) > touchDeadZone;
            if (finger === stationaryFinger && (ended || dragged)) {
                if (event.timeStamp - finger.startedAt >= touchHoldMilliseconds) {
                    // Input can arrive before a delayed timer callback.
                    activateTouchHold();
                } else {
                    clearTouchHold();
                    if (dragged) {
                        sendPendingTap();
                    } else if (pendingTap && tapDistance(pendingTap.point, point) <= doubleTapDistance) {
                        clearPendingTap();
                        recenterView();
                    } else {
                        sendPendingTap();
                        pendingTap = {
                            point,
                            deadline: event.timeStamp + doubleTapMilliseconds,
                            timer: setTimeout(sendPendingTap, doubleTapMilliseconds)
                        };
                    }
                }
            }
            if (ended) fingers.delete(point.identifier);
        }
        if (event.type === "touchend") updateTouchRoles();
        updateTouchKeys();
    }

    for (const type of ["touchstart", "touchmove", "touchend", "touchcancel"]) {
        canvas.addEventListener(type, handleTouch, { capture: true, passive: false });
    }

    function reset() {
        releaseRecenter();
        release();
        releaseCrouch();
        for (const code of remappedHeld.values()) sendKey("keyup", code);
        remappedHeld.clear();
        cycleKeysHeld.clear();
        physicalKeys.clear();
        resetTouch();
        cancelLeftPresses();
        rightHeld = false;
        suppressLeftUntil = 0;
    }
    window.addEventListener("blur", reset);
    touchLaptop?.addEventListener("change", () => {
        reset();
        touchMode = false;
        if (touchLaptopEnabled() && window.matchMedia?.("(hover: none) and (pointer: coarse)").matches) {
            enterTouchMode();
        }
    });
    window.addEventListener("pagehide", reset);
    canvas.addEventListener("blur", reset);
    document.addEventListener("visibilitychange", () => {
        if (document.hidden) reset();
    });
    document.addEventListener("pointerlockchange", () => {
        if (touchMode) {
            // A desktop request already in flight can settle after touch begins.
            if (document.pointerLockElement === canvas) document.exitPointerLock?.();
        } else if (document.pointerLockElement !== canvas) {
            reset();
        }
    });
})();
