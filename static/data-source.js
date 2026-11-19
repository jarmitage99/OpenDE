// Keep the early request and the Emscripten loader on exactly the same URL.
window.OpenDEDataSource = {
    url: "https://storage.googleapis.com/demos_opende/demo_v1/deusex.data.gz",
    loaderStarted: false
};

(() => {
    function preload() {
        // A slow/blocked cache lookup must not start a second, late preload.
        if (OpenDEDataSource.loaderStarted) return;
        const link = document.createElement("link");
        link.rel = "preload";
        link.as = "fetch";
        // Matches the generated loader's fetch(): CORS, same-origin credentials.
        link.crossOrigin = "anonymous";
        link.href = OpenDEDataSource.url;
        document.head.append(link);
    }

    // Emscripten still owns cache creation, writes, UUID checks and recovery.
    // This read-only probe only decides whether an early network hint is useful.
    // Any existing metadata suppresses the hint; stale/broken caches are handled
    // by the loader normally, without risking an unnecessary warm-cache download.
    try {
        if (!window.indexedDB) { preload(); return; }
        const request = indexedDB.open("EM_PRELOAD_CACHE", 1);
        let missing = false;
        let blocked = false;
        request.onupgradeneeded = () => {
            missing = true;
            request.transaction.abort(); // Do not create an incomplete Emscripten DB.
        };
        request.onerror = () => { if (missing && !blocked) preload(); };
        request.onblocked = () => { blocked = true; };
        request.onsuccess = () => {
            const db = request.result;
            if (blocked) { db.close(); return; }
            try {
                if (!db.objectStoreNames.contains("METADATA")) { db.close(); return; }
                const directory = location.pathname.substring(0, location.pathname.lastIndexOf("/") + 1);
                const key = `metadata/${encodeURIComponent(directory)}bin/index.data`;
                const transaction = db.transaction("METADATA", "readonly");
                const get = transaction.objectStore("METADATA").get(key);
                get.onsuccess = () => { if (!get.result) preload(); };
                transaction.oncomplete = transaction.onabort = transaction.onerror = () => db.close();
            } catch (_) {
                db.close(); // Let the normal loader recover from unavailable storage.
            }
        };
    } catch (_) {
        // Storage can be disabled by browser policy; the normal loader falls back.
    }
})();
