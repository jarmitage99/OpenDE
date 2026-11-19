(() => {
    let ready = false;
    let focused = document.hasFocus();
    let pageShown = true;
    let active = null;
    let saveTimer = null;
    const audioContexts = new Set();
    const suspendedContexts = new Set();

    function suspendAudio(context) {
        if (context.state === "closed") return;
        suspendedContexts.add(context);
        context.suspend().catch(error => console.error("Audio suspend failed", error));
    }

    function collectAudioContexts() {
        const contexts = [
            Module.SDL2?.audioContext,
            ...(window.miniaudio?.devices || []).map(device => device?.webaudio)
        ];
        for (const context of contexts) {
            if (!context || audioContexts.has(context)) continue;
            audioContexts.add(context);
            // Try playback on startup too; autoplay may already be permitted.
            suspendedContexts.add(context);
            // Keep delayed engine/audio-unlock callbacks from restarting audio in the background.
            context.addEventListener("statechange", () => {
                if (active === false && context.state === "running") suspendAudio(context);
            });
        }
    }

    function update() {
        if (!ready) return;
        collectAudioContexts();
        const nextActive = pageShown && focused && !document.hidden;
        if (nextActive === active) return;
        const wasActive = active;
        active = nextActive;
        if (!active) {
            Module.pauseMainLoop();
            clearInterval(saveTimer);
            saveTimer = null;
            for (const context of audioContexts) suspendAudio(context);
            persist();
        } else {
            // Always enqueue resume, even if a preceding suspend is still settling.
            for (const context of suspendedContexts) {
                if (context.state !== "closed") {
                    context.resume().catch(error => console.error("Audio resume failed", error));
                }
            }
            suspendedContexts.clear();
            if (wasActive === false) Module.resumeMainLoop();
            saveTimer = setInterval(persist, 10000);
        }
    }

    function unlockAudio(event) {
        // Synthetic game-control events must not trigger audio resumes.
        if (!event.isTrusted || !ready || !pageShown || !document.hasFocus() || document.hidden) return;
        collectAudioContexts();
        for (const context of audioContexts) {
            if (context.state === "suspended" || context.state === "interrupted") {
                context.resume().catch(error => console.error("Audio resume failed", error));
            }
        }
    }

    // Autoplay-blocked contexts need a real gesture. Include keyboard play,
    // and run before the game controls consume the event.
    for (const type of ["keydown", "click", "touchend"]) {
        window.addEventListener(type, unlockAudio, true);
    }

    Module.postRun.push(() => {
        ready = true;
        // The runtime may finish loading after the user has switched tabs.
        focused = document.hasFocus();
        update();
    });
    window.addEventListener("blur", () => { focused = false; update(); });
    window.addEventListener("focus", () => { focused = true; update(); });
    window.addEventListener("pagehide", () => { pageShown = false; update(); });
    window.addEventListener("pageshow", () => {
        pageShown = true;
        focused = document.hasFocus();
        update();
    });
    document.addEventListener("visibilitychange", () => {
        focused = document.hasFocus();
        update();
    });
})();
