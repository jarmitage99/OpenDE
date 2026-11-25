# Changes made in this branch

This browser demo branch adds touchscreen, touchpad, and keyboard controls, a level picker, a local WebAssembly build workflow, and WebGL rendering changes. The [code patch](branch.patch) captures the source changes from upstream `663bcb4264742742c259164fed8ffa84b0f258b2` (`origin/master`, tag `1.0.5`) through `8b97680d3681b48acc01619d1910e1ac5742aa69`.


## Rendering performance

- The WebAssembly textured main view uses the full BSP/span visibility traversal, including per-polygon culling, actor frustum/visibility checks, and asynchronous actor occlusion queries. Draw visible ranges from the static level vertex buffer, merging adjacent ranges. The earlier zone-only main-view shortcut is disabled; the buffer and texture upload optimizations remain enabled.
- Combine translucent level surfaces, including water, with translucent actors in one back-to-front draw list. Use BSP order, then view-space depth for ties. Keep static water and other level surfaces in the level vertex buffer.
- Share two alternating sets of WebGL vertex-buffer pages across actor batches, HUD and menu geometry, and the remaining dynamic draws. Append uploads within each frame and draw by vertex range. Place a GPU fence after the final canvas flush and give busy pages fresh storage before reuse. The native renderer keeps its mapped vertex stream.
- Batch canvas vertices into one upload per flush and replay the draw ranges in order with their original textures, blend modes, samplers, and clip rectangles. Upload vertex ranges directly from the WASM heap into draw buffers with WebGL 2's `bufferSubData` offset and length arguments. This removes the temporary JavaScript view and the staging-to-draw GPU copy from the previous actor path.
- Alternate two GPU textures for each procedural effect on the web. Upload changed pixels to the back texture and swap after the upload. Reuse matching texture storage with WebGL 2's `texSubImage2D` heap offset. Cached meshes and brushes pick up the new texture, including movers that reuse their geometry.
- Schedule wave and wet procedural effects every five rendered frames on the web. Other procedural effects, animated lightmaps, and cached zone fog retain the ten-frame schedule. Native updates retain their one-frame cadence. Track procedural dependencies and dirty pixels and discard zero-alpha detail fragments. The renderer changes are in `4fe80cd`, `ff0fdc6`, `d90fe91`, and `3dd785b`.
- Render supported ripple textures on the GPU from the simulated height field and lookup textures. The procedural texture code retains its CPU rendering path. This change is in `6247b11`.

Earlier measurements, before restoring full culling: in a local headless Chrome comparison on Liberty Island with ANGLE Metal, vertex upload calls fell 44% and p95 animation callback time went from 7.9 ms to 7.0 ms. Both runs held about 60 callbacks per second. In a separate dock view, the five- and ten-frame water schedules both measured 6.8 ms p95 callback time. These runs measure callback work on those views.

## Web page changes

- [index.html](index.html) replaces the embedded demo wrapper with a standalone page, loading progress, fullscreen, focus on canvas click, and control instructions. Both it and `cmake/emscripten/shell.html` start at 800×600 and fit the displayed canvas to the viewport while leaving the render buffer size under game control. Remove SDL's window resize listener after startup to prevent it from copying the display size into the render buffer.
- [explore.html](explore.html) links to the main menu and direct starts. Both player pages accept the same allowlist and append the mapped level name to `Module.arguments`.

| `?level=` value | Map |
| --- | --- |
| `training` | `00_Training` |
| `training_combat` | `00_TrainingCombat` |
| `training_final` | `00_TrainingFinal` |
| `game` | `01_NYC_UNATCOIsland` |
| `unatco_hq` | `01_NYC_UNATCOHQ` |

- The static page loads [lifecycle.js](lifecycle.js) to pause the main loop and suspend SDL/miniaudio when hidden or unfocused. It also saves once on deactivation, stops the ten-second save timer while inactive, and resumes work and periodic saves on return. Settings and saves use IndexedDB through IDBFS.
- The CMake-generated `bin/index.html` uses its own shell and save hooks. The custom input and lifecycle scripts are wired into `static/index.html`, so serve `static/` to get both. [run.sh](run.sh) is a macOS shortcut for serving that directory on port 7331 and opening the Explore page.


## Web / Classic mode

The checked Web/Classic option enables web controls. Unchecking switches immediately to the shipped demo's keyboard bindings and native mouse/wheel input, with no touch gestures, click debounce, scroll movement, or keyboard remapping. The action buttons remain usable in either mode and their hints update to match. Classic uses `[` for Zoom, `;` for Reload, `I` for Items, hold `X` for crouch, `F`/`End` for Recenter, and wheel-down for Next Item. `C`/`Z` turn and `R`/`V` look up/down at native speeds.

Startup restores the `Engine.Input` section from the packaged `DefUser.ini`, removing previous web binding changes while preserving other settings sections. INI sections are parsed line by line with CRLF normalization; startup repairs joined headers from the earlier rewrite and restores missing player sections from the packaged defaults. Web-only slow look axes use unbound keypad keys; physical presses of these helper keys are ignored in Classic. Mode switches release held shim inputs. Web key translations happen in JavaScript so switching does not require a reload or engine rebuild.

## Touchscreen controls

- Drag one finger right away to walk and turn. Hold still for 350 ms to send one right-click, then drag to look around. With two fingers, use the left finger to move and strafe and the right finger to look.
- A single tap left-clicks after a 250 ms double-tap window. Double-tap holds End for 120 ms to recenter. The Jump+Fwd button holds W immediately, adds Space at 80 ms, releases Space at 300 ms, and releases W at 420 ms.
- The shortcut panel is available with touch or mouse below the game in normal view. Fullscreen shows only the game area, without the shortcut panel. It includes Crouch toggle, Next weapon, Zoom, Reload, Items, and Menu. Items opens and closes Inventory.
- Touch releases browser pointer lock and suppresses further capture requests while playing by touch. A mouse click on the game can restore capture.

## Touchpad and mouse changes

- Delay primary mouse input by 50 ms so a secondary click or context-menu gesture can cancel an overlapping primary click and briefly suppress new ones. Forward real secondary clicks immediately. For a quick primary tap, dispatch both the down and up events after the delay. Clear pending mouse and wheel input on focus loss, page hide, or pointer unlock.
- Scroll down to move forward and up to move backward. Scroll left to turn right and right to turn left. The two axes hold and release independently, so diagonal scrolling moves and turns at the same time. Each axis releases 180 ms after its last event. Physical W or S takes priority over vertical scrolling.
- In [controls.js](controls.js), capture real wheel events over the canvas or while it holds pointer lock. Send synthetic W/S for movement and the original Z/C turn bindings for turning. Use dedicated keypad axes at speeds of −50 and +50 for web turning; keep classic `TurnLeft` and `TurnRight` aliases at ±150. The handler lets Ctrl+wheel pass through.
- For a touchpad-only change, keep the wheel-to-movement handler, turn alias edits, primary-click debounce, and related focus cleanup. Keep physical C/Z on their original turn bindings and leave the keyboard binding edits out. The relevant history starts at `519fd8d`, `a3e2236`, and `f7428c5`. The last commit also includes early keyboard edits.

## Keyboard binding changes

| Physical key | Action | Browser implementation |
| --- | --- | --- |
| C | Hold to crouch | Hold the game's X (`Duck`) binding |
| X | Toggle crouch | Latch the same X binding once per press. Either crouch input can keep it held |
| T | Throw or drop the held item | Forward Tab with `Tab=DropItem`. Classic T remains unbound |
| F / V | Next / previous belt item | Emit one synthetic wheel step per press using `MouseWheelDown=NextBeltItem` and `MouseWheelUp=PrevBeltItem`. Classic F/V keep CenterView/LookDown |
| Y | Equip or unequip | Forward Backspace |
| R | Reload | Forward Semicolon |
| Z | Zoom | Forward Left Bracket, the demo's zoom binding |
| Tab | Inventory | Forward I with `I=ShowInventoryWindow`. Classic Tab retains DropItem |

Load [controls.js](controls.js) after the page defines `Module` and before Emscripten's loader. In `onRuntimeInitialized`, after restoring browser storage and before the game starts, read the saved `/home/web_user/.local/share/deusex/User.ini` or fall back to `/game/System/User.ini`. Write the stable native binding map and helper axes to the browser's user file, preserve unrelated settings, and force `bToggleCrouch=False` so the web shim can own crouch state. Track synthetic events so the keyboard remaps leave synthetic C/Z from scrolling alone and the wheel handler lets synthetic F/V item cycling through. Release remapped keys and crouch state when focus or pointer capture is lost.

For a keyboard-only change, keep the key handlers, key-related INI edits, and key-state cleanup. Leave real wheel input on the game's belt bindings alongside F/V and retain the original turn speeds. Review `c848a9a` and `321ba34` together for the final bindings. The tests in `static/tests/controls.test.cjs` cover keyboard bindings, double-tap jumping, and hold gestures.

## Recreate and run the branch

1. From a clean checkout of the upstream repository, create a branch at the recorded base and apply [branch.patch](branch.patch). Copy the patch outside the checkout first if switching the same worktree.

   ```sh
   git switch -c opende-web-demo 663bcb4264742742c259164fed8ffa84b0f258b2
   git apply --check --whitespace=nowarn /path/to/branch.patch
   git apply --whitespace=nowarn /path/to/branch.patch
   ```

   The patch contains handwritten C++, HTML, JavaScript, shell scripts, and tests. The checkout also had generated `static/deusex.js`, `static/deusex.wasm`, `static/deusex_original.wasm`, and `static/deusex.data`. The original WASM is a reference copy. [source-manifest.json](source-manifest.json) records the cached demo's origin and asset inventory. Its change notes describe earlier revisions. The branch also adds a root README note about the Parallels build and smoke test and a `src/.DS_Store` ignore rule. These files and the tracked Finder metadata are outside the code patch.

2. Build the browser bundle with Emscripten 6.0.12 and the Deus Ex 1002f demo data. The data directory must contain `System`, `Maps`, `Textures`, `Sounds`, and `Music`. The startup change in `a9da390` builds and calls the Bots menu hook only with `DX_NETWORKING` enabled and exits the hook if the demo lacks `MenuChoice_ServerMode`.

   On macOS with the Parallels Ubuntu build VM, run `./build_web.sh --vm "Ubuntu 24.04.3 ARM64" --game-data /path/in/ubuntu/to/demo-data`. It builds an isolated source snapshot in the guest, verifies the bundle against SHA-256 checksums, and copies `index.js`, `index.wasm`, and `index.data` to `static/` as `deusex.js`, `deusex.wasm`, and `deusex.data`.

   With the same SDK available locally, build and copy the matching files yourself.

   ```sh
   emcmake cmake -S . -B build-web -G Ninja \
     -DCMAKE_BUILD_TYPE=Release -DSKIP_QA=ON -DSKIP_UT=ON \
     -DDEUSEX_GAME_DATA="$PWD/demo-data"
   cmake --build build-web --target deusex --parallel
   cp build-web/bin/index.js static/deusex.js
   cp build-web/bin/index.wasm static/deusex.wasm
   cp build-web/bin/index.data static/deusex.data
   ```

   `static/index.html` maps the generated loader's `index.wasm` and `index.data` requests to the `deusex.*` names. Use the loader, WASM, and data from the same build.

3. Run `python3 -m http.server 7331 --bind 127.0.0.1 --directory static` and open `http://127.0.0.1:7331/explore.html` in a WebGL 2 browser. Run `node --test static/tests/controls.test.cjs` for the keyboard tests. With Chrome or Chromium available, `node scripts/check-web.mjs` checks title-screen startup, controls, and pause/resume. Pass `training` or `game` to check a direct launch into Training or Liberty Island. Set `CHROME_BIN` if the browser is installed at a custom path.

### Game-data download and cache

`data-source.js` points the page at `demo_v1/deusex.data.gz`. Its HTTP
`Content-Encoding: gzip` is decoded by the browser; the generated loader still
receives the original LZ4 package, with the same size and SHA-256 identity.

At the top of the page, a read-only probe checks Emscripten's IndexedDB metadata.
On a confirmed cache miss it adds a CORS fetch preload so the browser can start
the download before WASM initialization. The loader uses the same URL and request
credentials to reuse that preload. Existing metadata suppresses the hint, and a
late/blocked probe never adds a preload after the loader has selected the URL.
Browsers without fetch-preload support simply use the normal loader request.

Emscripten's existing `--use-preload-cache` remains responsible for storing the
bundle in `EM_PRELOAD_CACHE`, validating the package UUID and recovering from
cache failures. Changing to the gzip URL does not invalidate a valid existing
package cache. This is browser-managed storage: clearing site data, storage
pressure, or private browsing can remove or limit it. No second asset cache or
persistent-storage permission is introduced.
