const assert = require("node:assert/strict");
const test = require("node:test");
const vm = require("node:vm");
const { script } = require("./shell.cjs");

function createPage({ focused = true, hidden = false } = {}) {
    const events = new Map();
    const timers = new Map();
    const calls = { pause: 0, resume: 0, save: 0 };
    let nextTimer = 0;
    const listen = (name, callback) => {
        if (!events.has(name)) events.set(name, []);
        events.get(name).push(callback);
    };
    function audio() {
        let listener;
        return {
            state: "running", resumes: 0, suspends: 0,
            addEventListener(name, callback) { listener = callback; },
            suspend() { this.suspends++; this.state = "suspended"; listener?.(); return Promise.resolve(); },
            resume() { this.resumes++; this.state = "running"; listener?.(); return Promise.resolve(); }
        };
    }
    const sdl = audio();
    const mini = audio();
    const document = { hidden, hasFocus: () => focused, addEventListener: listen };
    const Module = {
        SDL2: { audioContext: sdl }, postRun: [],
        pauseMainLoop() { calls.pause++; }, resumeMainLoop() { calls.resume++; }
    };
    vm.runInNewContext(script("lifecycle"), {
        Module, document, console,
        window: { addEventListener: listen, miniaudio: { devices: [{ webaudio: mini }] } },
        persist() { calls.save++; },
        setInterval(callback, delay) { const id = ++nextTimer; timers.set(id, { callback, delay }); return id; },
        clearInterval(id) { timers.delete(id); }
    });
    return {
        calls, timers, sdl, mini,
        start() { Module.postRun.forEach(callback => callback()); },
        event(name, properties = {}) {
            if (name === "blur") focused = false;
            if (name === "focus") focused = true;
            if (properties.hidden !== undefined) document.hidden = properties.hidden;
            for (const callback of events.get(name) || []) callback(properties);
        }
    };
}

test("blur pauses simulation and both audio backends, saves once, and cancels periodic saves", () => {
    const page = createPage();
    page.start();
    assert.equal(page.timers.size, 1);
    assert.equal([...page.timers.values()][0].delay, 10000);
    page.event("blur");
    page.event("visibilitychange", { hidden: true });
    assert.deepEqual(page.calls, { pause: 1, resume: 0, save: 1 });
    assert.equal(page.timers.size, 0);
    assert.equal(page.sdl.state, "suspended");
    assert.equal(page.mini.state, "suspended");
    page.event("focus"); // A hidden page must remain paused even if focused.
    assert.equal(page.calls.resume, 0);
    page.event("visibilitychange", { hidden: false });
    assert.equal(page.calls.resume, 1);
    assert.equal(page.sdl.state, "running");
    assert.equal(page.mini.state, "running");
    page.event("pageshow");
    assert.equal(page.timers.size, 1);
});

test("loading in an unfocused tab starts idle and resumes after focus returns", () => {
    const page = createPage({ focused: false });
    page.start();
    assert.equal(page.calls.pause, 1);
    assert.equal(page.timers.size, 0);
    page.event("focus");
    assert.equal(page.calls.resume, 1);
    assert.equal(page.timers.size, 1);
});

test("pagehide and pageshow preserve one save timer and pause until the page is shown", () => {
    const page = createPage();
    page.start();
    page.event("pagehide");
    page.event("focus");
    assert.equal(page.calls.resume, 0);
    assert.equal(page.timers.size, 0);
    page.event("pageshow");
    assert.equal(page.calls.resume, 1);
    assert.equal(page.timers.size, 1);
});

test("late audio resumes cannot restart sound in an inactive tab", async () => {
    const page = createPage();
    page.start();
    page.event("blur");
    await page.mini.resume();
    assert.equal(page.mini.state, "suspended");
    const resumes = page.sdl.resumes;
    page.event("click", { isTrusted: true });
    assert.equal(page.sdl.resumes, resumes);
    page.event("focus");
    await page.sdl.suspend();
    page.event("click", { isTrusted: false });
    assert.equal(page.sdl.state, "suspended");
    page.event("click", { isTrusted: true });
    assert.equal(page.sdl.state, "running");
});
