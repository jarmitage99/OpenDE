const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const test = require("node:test");
const vm = require("node:vm");

function createGame(ini = "[Engine.Input]\nX=Duck\nTab=DropItem\n") {
    const listeners = new Map();
    const canvasListeners = new Map();
    const shortcutListeners = new Map();
    const shortcuts = {
        addEventListener(type, listener) { shortcutListeners.set(type, listener); },
        contains() { return false; }
    };
    const actions = [];
    const keyEvents = [];
    const touchEvents = [];
    const mouseEvents = [];
    let settings;
    let accumulatedWheel = 0;
    class InputEvent {
        constructor(type, options) { Object.assign(this, { type }, options); }
        preventDefault() { this.defaultPrevented = true; }
        stopImmediatePropagation() { this.stopped = true; }
    }
    class WheelEvent extends InputEvent {}
    WheelEvent.DOM_DELTA_LINE = 1;
    function dispatch(event) {
        for (const listener of listeners.get(event.type) || []) {
            listener(event);
            if (event.stopped) break;
        }
    }
    const canvas = {
        addEventListener(type, listener) { canvasListeners.set(type, listener); },
        focus() {},
        dispatchEvent(event) {
            event.target = canvas;
            dispatch(event);
            if (!event.stopped) canvasListeners.get(event.type)?.(event);
            if (event.stopped) return;
            if (event.type === "keydown" || event.type === "keyup") {
                keyEvents.push([event.type, event.code]);
            }
            if (event.type.startsWith("touch")) touchEvents.push(event.type);
            if (event.type.startsWith("mouse")) mouseEvents.push([event.type, event.button]);
            if (event.type !== "wheel") return;
            // SDL_emscriptenevents.c converts DOM lines to wheel steps;
            // SDL_mouse.c accumulates fractions before setting integer wheel.y.
            assert.equal(event.deltaMode, WheelEvent.DOM_DELTA_LINE);
            const delta = -event.deltaY / 3;
            if (Math.sign(delta) !== Math.sign(accumulatedWheel)) accumulatedWheel = 0;
            accumulatedWheel += delta;
            const wheelY = Math.trunc(accumulatedWheel);
            accumulatedWheel -= wheelY;
            if (wheelY) actions.push(wheelY > 0 ? "previous" : "next");
        }
    };
    const Module = { postRun: [] };
    vm.runInNewContext(fs.readFileSync(path.join(__dirname, "../controls.js"), "utf8"), {
        Module,
        FS: {
            analyzePath: () => ({ exists: true }), readFile: () => ini,
            mkdirTree() {}, writeFile: (file, text) => { settings = text; }
        },
        document: { getElementById: id => id === "canvas" ? canvas : id === "game-shortcuts" ? shortcuts : null, addEventListener() {} },
        window: {
            addEventListener(type, listener) {
                if (!listeners.has(type)) listeners.set(type, []);
                listeners.get(type).push(listener);
            }
        },
        Event: InputEvent, KeyboardEvent: InputEvent, MouseEvent: InputEvent, WheelEvent, setTimeout, clearTimeout
    });
    Module.onRuntimeInitialized();
    Module.postRun.forEach(callback => callback());
    return {
        actions, keyEvents, touchEvents, mouseEvents, settings,
        jumpForward() {
            shortcutListeners.get("click")({ target: { closest: () => ({ dataset: { action: "jump-forward" } }) } });
        },
        recenter() {
            shortcutListeners.get("click")({ target: { closest: () => ({ dataset: { key: "End" } }) } });
        },
        blur() { dispatch(new InputEvent("blur", {})); },
        touch(type, timeStamp, clientX = 100, clientY = 100) {
            const point = { identifier: 1, clientX, clientY };
            canvas.dispatchEvent(new InputEvent(type, { changedTouches: [point], timeStamp }));
        },
        tap(timeStamp) {
            this.touch("touchstart", timeStamp);
            this.touch("touchend", timeStamp + 30);
        },
        key(type, code) {
            const event = new InputEvent(type, { code, target: canvas });
            dispatch(event);
            return event;
        }
    };
}

test("every F/V press produces a full next/previous step, including alternating directions", () => {
    const game = createGame();
    const codes = ["KeyF", "KeyV", "KeyF", "KeyF", "KeyV", "KeyV"];
    for (const code of codes) {
        assert.equal(game.key("keydown", code).defaultPrevented, true);
        game.key("keyup", code);
    }
    assert.deepEqual(game.actions, ["next", "previous", "next", "next", "previous", "previous"]);
});

test("holding a cycle key advances once, and releasing permits the next press", () => {
    const game = createGame();
    game.key("keydown", "KeyF");
    game.key("keydown", "KeyF");
    game.key("keydown", "KeyF");
    assert.deepEqual(game.actions, ["next"]);
    game.key("keyup", "KeyF");
    game.key("keydown", "KeyF");
    assert.deepEqual(game.actions, ["next", "next"]);
});


test("T forwards the throw/drop binding rather than cycling weapons", () => {
    const game = createGame();
    game.key("keydown", "KeyT");
    game.key("keydown", "KeyT");
    game.key("keyup", "KeyT");
    assert.deepEqual(game.actions, []);
    assert.deepEqual(game.keyEvents, [["keydown", "Tab"], ["keyup", "Tab"]]);
    assert.match(game.settings, /T=DropItem/);
    assert.match(game.settings, /Tab=DropItem/);
});

test("X toggles crouch once per press and C remains hold-to-crouch", () => {
    const game = createGame();
    game.key("keydown", "KeyX");
    game.key("keydown", "KeyX"); // Ignore auto-repeat.
    game.key("keyup", "KeyX");
    assert.deepEqual(game.keyEvents, [["keydown", "KeyX"]]);
    game.key("keydown", "KeyC");
    game.key("keyup", "KeyC"); // Releasing C must preserve latched crouch.
    assert.equal(game.keyEvents.length, 1);
    game.key("keydown", "KeyX");
    game.key("keyup", "KeyX");
    assert.deepEqual(game.keyEvents, [["keydown", "KeyX"], ["keyup", "KeyX"]]);
    game.key("keydown", "KeyC");
    game.key("keyup", "KeyC");
    assert.deepEqual(game.keyEvents.slice(-2), [["keydown", "KeyX"], ["keyup", "KeyX"]]);
});

test("turning off toggle while C is held waits for C release, and blur clears crouch", () => {
    const game = createGame();
    for (const type of ["keydown", "keyup"]) game.key(type, "KeyX");
    game.key("keydown", "KeyC");
    for (const type of ["keydown", "keyup"]) game.key(type, "KeyX");
    assert.deepEqual(game.keyEvents, [["keydown", "KeyX"]]);
    game.key("keyup", "KeyC");
    assert.deepEqual(game.keyEvents.at(-1), ["keyup", "KeyX"]);
    for (const type of ["keydown", "keyup"]) game.key(type, "KeyX");
    game.blur();
    assert.deepEqual(game.keyEvents.at(-1), ["keyup", "KeyX"]);
});

test("saved native toggle mode is disabled without altering other player settings", () => {
    const game = createGame("[DeusEx.DeusExPlayer]\r\nbToggleCrouch=True\r\nbAlwaysRun=True\r\n[Engine.Input]\r\nX=Duck\r\n");
    assert.match(game.settings, /bToggleCrouch=False\r\n/);
    assert.match(game.settings, /bAlwaysRun=True\r\n/);
    assert.match(game.settings, /V=PrevBeltItem/);
});

test("forward jump leads with movement, releases jump first, and cancels on blur", async () => {
    const game = createGame();
    game.jumpForward();
    const presses = [["keydown", "KeyW"], ["keydown", "Space"]];
    const releases = [["keyup", "Space"], ["keyup", "KeyW"]];
    assert.deepEqual(game.keyEvents, presses.slice(0, 1));
    await new Promise(resolve => setTimeout(resolve, 100));
    assert.deepEqual(game.keyEvents, presses);
    await new Promise(resolve => setTimeout(resolve, 350));
    assert.deepEqual(game.keyEvents, [...presses, ...releases]);
    assert.deepEqual(game.touchEvents, []);
    assert.deepEqual(game.actions, []);

    game.jumpForward();
    game.blur();
    assert.deepEqual(game.keyEvents.slice(-2), [["keydown", "KeyW"], ["keyup", "KeyW"]]);
    const eventCount = game.keyEvents.length;
    await new Promise(resolve => setTimeout(resolve, 100));
    assert.equal(game.keyEvents.length, eventCount);
});

test("finger double-tap centers the view without jumping or firing", async () => {
    const game = createGame();
    game.tap(0);
    game.tap(100);
    assert.match(game.settings, /End=CenterView/);
    assert.deepEqual(game.keyEvents, [["keydown", "End"]]);
    await new Promise(resolve => setTimeout(resolve, 300));
    assert.deepEqual(game.keyEvents, [["keydown", "End"], ["keyup", "End"]]);
    assert.deepEqual(game.touchEvents, []);
    assert.deepEqual(game.mouseEvents, []);
    assert.deepEqual(game.actions, []);
});

test("Recenter holds End across a frame, releases it, and cancels on blur", async () => {
    const game = createGame();
    game.recenter();
    await new Promise(resolve => setTimeout(resolve, 30));
    assert.deepEqual(game.keyEvents, [["keydown", "End"]]);
    await new Promise(resolve => setTimeout(resolve, 120));
    assert.deepEqual(game.keyEvents, [["keydown", "End"], ["keyup", "End"]]);
    game.recenter();
    game.recenter();
    game.blur();
    assert.deepEqual(game.keyEvents.slice(2), [["keydown", "End"], ["keyup", "End"]]);
    await new Promise(resolve => setTimeout(resolve, 150));
    assert.equal(game.keyEvents.length, 4);
});

test("a stationary hold right-clicks once before release and cancels cleanly", async () => {
    const game = createGame();
    game.touch("touchstart", 0);
    game.touch("touchmove", 100, 102, 100);
    assert.deepEqual(game.mouseEvents, []);
    await new Promise(resolve => setTimeout(resolve, 375));
    assert.deepEqual(game.mouseEvents, [["mousedown", 2], ["mouseup", 2]]);
    game.touch("touchend", 500, 102, 100);
    assert.equal(game.mouseEvents.length, 2);
    game.touch("touchstart", 600);
    game.touch("touchcancel", 700);
    await new Promise(resolve => setTimeout(resolve, 375));
    assert.equal(game.mouseEvents.length, 2);
    assert.deepEqual(game.touchEvents, []);
});

test("hold-and-drag right-clicks then looks, while a quick drag keeps moving and turning", () => {
    const game = createGame();
    game.touch("touchstart", 0);
    game.touch("touchmove", 400, 120, 80);
    game.touch("touchend", 450, 120, 80);
    assert.deepEqual(game.keyEvents.splice(0), [
        ["keydown", "KeyC"], ["keydown", "Delete"], ["keyup", "KeyC"], ["keyup", "Delete"]
    ]);
    assert.deepEqual(game.mouseEvents.splice(0), [["mousedown", 2], ["mouseup", 2]]);

    game.touch("touchstart", 600);
    game.touch("touchmove", 680, 120, 80);
    game.touch("touchmove", 1000, 125, 75);
    game.touch("touchend", 1050, 125, 75);
    assert.deepEqual(game.keyEvents, [
        ["keydown", "KeyW"], ["keydown", "KeyC"], ["keyup", "KeyW"], ["keyup", "KeyC"]
    ]);
    assert.deepEqual(game.mouseEvents, []);
    assert.deepEqual(game.touchEvents, []);
});
