# RayGame Engine

A lightweight 2D game engine with inline C++17 headers and an xmake static-library
target, built on top of raylib-cpp, featuring a
Unity-style GameObject/Component architecture, a working physics layer with collision
callbacks, particles, tweening, an event bus, tilemaps, immediate-mode UI and more.

Version 0.2.0 · C++17 · MIT

See the [product readiness review and roadmap](docs/PRODUCT_READINESS_REVIEW.md)
(Chinese) for current limitations, proposed architecture and release acceptance criteria.

## Features

### Core
- **GameObject/Component architecture** with `Awake`/`Start`/`Update`/`FixedUpdate`/`Render` lifecycle
- **Transform hierarchy**: parent/child with local ↔ world position, rotation and scale
- **Deferred destruction & safe spawning**: `Destroy()` is queued; objects spawned at runtime are started automatically
- **Scene management** with deferred scene switching
- **Fixed timestep** loop for game logic and rigidbody physics (`FixedUpdate`)
- **Resource Manager**: typed texture/font/shader handles, safe unload, explicit replacement and failure diagnostics
- **Time** utilities: delta time, elapsed time, frame count

### Rendering
- **Sprite & animation** components with pivot, flip and tint
- **Global layer sorting**: components draw in `layer`/`renderOrder` order across all objects
- **Camera2D** with smooth follow, zoom, world/screen conversion, opt-in render integration
- **Resizable window & borderless fullscreen** with fixed or expanding logical canvases, aspect-ratio preservation and mapped mouse input
- **Sharp UI text** with a display-sized render target and bundled example fonts
- **Native Windows title bar** follows the system's app light/dark preference, including changes while running

### Physics
- **Rigidbody**: velocity, acceleration, mass, drag, gravity, restitution, impulses/forces
- **Colliders**: axis-aligned Box and Circle, auto-registered with the physics world
- **All pair types** collide: Box-Box, Circle-Circle, Box-Circle
- **Collision & trigger callbacks**: `OnCollisionEnter/Stay/Exit`, `OnTriggerEnter/Stay/Exit`
- **Collision layers & masks**
- **Impulse-based resolution** with contact normals, penetration correction and static/dynamic handling

### Gameplay systems
- **Tween** engine with 14 easing functions, loop/pingpong/delay, callbacks
- **Event bus**: string-keyed pub/sub with automatic owner cancellation for scenes and components
- **Object pool**: generic free-list pool (`ObjectPool<T>`)
- **Timers**: `DestroyAfter`, repeating/one-shot `TimerComponent`
- **Particle system**: point/circle/box/cone emitters, burst & continuous
- **Tilemap**: grid tiles, tileset rendering, per-tile solidity queries

### Input, Audio, UI, Save
- **Input**: injectable keyboard/mouse/gamepad snapshots, fixed-tick edge buffering, axis helpers and named **action mapping**
- **Audio**: sound/music with master volume (own `Audio.h`)
- **Immediate-mode UI**: labels, panels, buttons, progress bars
- **Save system**: dependency-free key/value persistence
- **Safe storage & diagnostics**: atomic saves with checksums and backup recovery, per-user data directories, rotating file logs

## Architecture

```
GameEngine ── Scene ── GameObject
                           ├── Transform (hierarchy)
                           └── Components
                               ├── SpriteRenderer / SpriteAnimation
                               ├── Rigidbody, BoxCollider / CircleCollider
                               ├── ParticleEmitter
                               ├── Camera2DComponent
                               ├── Tilemap
                               ├── DestroyAfter / TimerComponent
                               └── your own Component subclasses
```

Cross-cutting services are static/singleton: `PhysicsWorld`, `TweenManager`,
`EventBus`, `ResourceManager`, `AudioManager`, `Input`, `UI`, `GameTime`.
Events and tweens have application/scene/object/component lifetime scopes. Scene
changes cancel the old scene's callbacks while preserving application services.
Physics still uses one global world; independent simultaneous worlds are pending.

## Quick Start

```cpp
#include "RayGame.h"
using namespace RayGame;

class MyScene : public Scene {
    void OnLoad() override {
        auto tex = ResourceManager::CreateCircleTexture("player", 16, SKYBLUE);
        auto player = CreateGameObject("Player");
        player->transform->position = {400, 300};
        player->AddComponent<SpriteRenderer>(tex);
        player->AddComponent<Rigidbody>();
        player->AddComponent<CircleCollider>(16.0f);
    }
};

class MyGame : public GameEngine {
public:
    MyGame() : GameEngine("My Game", 800, 600) {}
    void OnInit() override { LoadScene(CreateRef<MyScene>()); }
};

int main() { MyGame().Run(); }
```

## Selected APIs

### Lifecycle and timing

`AddComponent` attaches the component and calls `Awake` immediately. Configure its
fields after adding it; `Start` runs once before its first update or render. Disabled
components start when enabled. Runtime spawns are queued during scene traversal.
`active = false` pauses an object; use `Scene::Destroy` or `MarkDestroyed` to remove
it. Deferred cleanup runs `OnDestroy` once even if another `Ref` retains the object.
During `Scene::Clear`, `IsClearing()` is true and `CreateGameObject` throws
`std::logic_error`: cleanup callbacks cannot repopulate a scene being cleared.
Exceptions from destruction/unload callbacks are logged and cleanup continues.
Exceptions from initialization, updating or rendering propagate after `Run` cleans
up the scene, resources and native devices. `Shutdown` is idempotent, with a
destructor fallback for an engine initialized without calling `Run`.

Use `GameTime::DeltaTime()` in `Update` and `GameTime::FixedDeltaTime()` in
`FixedUpdate`. Rigidbody integration and collision processing run at the fixed
interval (1/60 second by default), with at most eight catch-up steps per frame.
The simulation delta used by ordinary updates and tweens is capped to the same
eight-step budget, so a long stall does not advance animations far ahead of physics.
`GameTime::UnscaledDeltaTime/UnscaledTime` retain actual elapsed time, including while
paused. Override `GameEngine::OnUnscaledUpdate` for work that must continue during pause.
Set it with `GameEngine::SetFixedTimeStep`; zero and negative intervals are ignored.
Collision layers are numbered 0–31; a nonpositive rigidbody mass is treated as static.

`LoadScene` takes effect on the next frame. The engine unloads and clears the old
scene, its physics registrations, tweens and event subscriptions before loading
the new one. Resource and audio caches persist until shutdown. `Scene::Clear`
also supports rebuilding a scene in place; call `OnLoad` and `Start` afterward.

### Transform hierarchy

`position`, `rotation` and `scale` are local values. Use `GetWorldPosition`,
`SetWorldPosition`, `TransformPoint` and `InverseTransformPoint` for world-space
operations. Reparenting preserves world position/rotation/scale by default and
rejects cycles. Removing a parent detaches children while preserving their world
pose. Sprites, camera following and tilemap queries use the hierarchy. Box colliders
remain axis aligned; sprite rendering represents rotation and scale without shear.

### Collision callbacks
```cpp
class Bullet : public Component {
    void OnTriggerEnter(Collider* other) override {
        if (other->GetGameObject()->tag == "Enemy")
            gameObject->MarkDestroyed();
    }
};
```

### Tween
```cpp
Tween::To(0, 1, 0.5f, Ease::BackOut, [obj](float v){ obj->transform->scale = {v, v}; });
Tween::Delay(2.0f, [](){ /* fire later */ });
```

### Events
```cpp
EventBus::Subscribe("score", [&](const EventData& d){ score += d.i; });
EventBus::Emit("score", EventData{10});
```

Registrations made in engine-dispatched component callbacks automatically belong
to that component; scene callbacks belong to the scene, and engine hooks belong
to the application. Dispatching an event/tween inherits its registered owner.
Destroying an object immediately cancels its component callbacks and releases
captured values (a callable already executing stays alive until it returns).
Disabled components keep subscriptions until destruction or explicit cancellation.

Outside those callbacks, pass the intended owner's token explicitly:

```cpp
EventBus::Subscribe("score", handler, component->GetCallbackScope());
Tween::Delay(2.0f, callback, scene->GetCallbackScope());
EventBus::Subscribe("settings", handler, engine.GetCallbackScope()); // survives scene changes
CallbackScope menuScope;
EventBus::Subscribe("menu", handler, menuScope.Token());
menuScope.Cancel();
```

An empty token (`ScopeToken{}`) has global lifetime, until unsubscribe/clear or
engine shutdown. Unscoped registrations made outside runtime callbacks use that
lifetime. `EventBus::Clear()` and `TweenManager::Clear()` still clear the entire
service; ordinary scene changes no longer call them. `Scene::Clear()` cancels its
old scope and prepares a fresh empty scene. For manual restart, use `Clear()` then
`Load()` (which runs scoped `OnLoad()` and `Start()`), as the examples do.
Direct calls to user overrides do not automatically activate an owner: use
`ScopeActivation(owner->GetCallbackScope())` around custom dispatch.

Handlers removed during dispatch are skipped; newly added handlers wait until
the next emission. Tweens created inside callbacks start on the next update.
Self-cancellation, owner destruction and throwing callbacks restore dispatch
context correctly. Scope and service operations run on the runtime thread.

### Object pool
```cpp
ObjectPool<Bullet> pool(256);
Bullet* b = pool.Acquire();
pool.Release(b);
```

### Input actions
```cpp
Input::BindAction("jump", {KEY_SPACE, KEY_W}, {GAMEPAD_BUTTON_RIGHT_FACE_DOWN});
if (Input::GetActionDown("jump")) { /* ... */ }
```

`GameEngine` captures raylib input once before its per-frame shortcuts/update.
`GetKeyDown`/`GetActionDown` and mouse/gamepad equivalents read pressed edges;
`GetKey` reads the held state. In `FixedUpdate` and physics callbacks, edges and
mouse delta/wheel accumulate across frames with no fixed tick, then are consumed
by the first fixed tick. Additional catch-up ticks see held state without repeated
edges. Ordinary update/render still see the original frame snapshot.

Buffering coalesces each button's pressed/released flags: a short press and release
can both be true in one tick. Repeated transitions between ticks are not an ordered
event stream or a full replay system. Pause/resume, focus loss and scene changes
discard pending edges; held input can resume without a synthetic press. Focus loss
also clears held input. Real time and unscaled UI updates continue during pause.

For headless tests or a custom loop, set one snapshot per frame (or call
`CaptureFrame()` after pumping raylib events). `SetFrame` derives edges from held
transitions and also accepts explicit edges. Mouse positions remain in window
coordinates and are converted through the current viewport on read.

```cpp
InputFrame frame;
frame.keys.held[KEY_SPACE] = true;
Input::SetFrame(frame);
// Advance your engine tick; standalone fixed loops use Input::FixedTickScope.
```

### Resource handles and migration

```cpp
auto texture = ResourceManager::LoadTexture("player", "assets/player.png");
object->AddComponent<SpriteRenderer>(texture);
if (!texture) { /* inspect texture.Error() and texture.State() */ }
bool replaced = ResourceManager::ReloadTexture("player", "assets/player-v2.png");
ResourceManager::UnloadTextureNamed("player"); // all copies now test false
```

Texture/font/shader APIs return `TextureHandle`, `FontHandle`, `ShaderHandle`.
Migrate `auto*` variables to `auto`, raw resource fields to the corresponding
handle, and manual `OnLoad(); Start();` restart sequences to `Load()`.
SpriteRenderer, SpriteAnimation, Tilemap and UI store handles directly. Empty or
unloaded sprite/tile resources draw magenta placeholders; fonts fall back to
raylib's default font. Missing/invalid shaders produce a failed handle: select a
fallback before entering shader mode. Default raylib resources are never owned
or unloaded by the resource manager.

`Load*` reuses a ready named entry; `ReloadTexture/ReloadFont/ReloadShader` explicitly
replace it. A successful replacement updates all live handles and increments
`Revision()`; failure returns false, records `Error()` and preserves the previous
resource. Failed first loads have `ResourceState::Failed` and can be retried.
Unload/shutdown marks every retained handle `Unloaded`. Reusing a name creates a
new slot and never revives old handles. Sprite/animation source rectangles remain
explicit gameplay data; update them if a replacement changes atlas layout.

A handle retains metadata, not the GPU allocation. `Get()` returns a short-lived
borrowed pointer; do not cache it or copied native IDs across reload/unload.
Checked `operator->`/`operator*` throw on invalid handles. Upload, replacement,
unload and handle reads must run on the graphics thread while the owning window
is alive. Asynchronous loading and full asset dependency tracking are pending.

### Save
```cpp
SaveSystem save;
save.SetInt("highscore", 4200);
save.Save("save.txt");
```

## Android APK (experimental)

The four games can be packaged as ARM64 NativeActivity APKs with xmake and raylib.
The current build targets Android 8.0+ (API 26), target SDK 35, and includes bundled
fonts, touch controls, pause/exit UI and application-private saves/logs.

```powershell
pwsh -NoProfile -File scripts/build-android.ps1
```

Outputs: `bin/android/arm64-v8a/release/*.apk`. These use Release native code and a
local debug signing key. The script keeps the desktop xmake configuration intact
and checks signatures, native entry points, assets and 16 KiB page alignment.
Device execution is still pending manual validation. See [Android setup, controls
and platform limits](docs/ANDROID.md) (Chinese) for prerequisites and commands.

## Examples

| Target | Showcases |
| --- | --- |
| `space_shooter` | Custom renderers, particles, enemy AI |
| `bouncing_balls` | Rigidbodies, circle colliders, physics resolution |
| `neon_breakout` | Layered neon rendering, tweens, timers, trigger callbacks, camera shake, HUD, save |
| `bullet_hell` | Object pooling (thousands of bullets), parent-child turrets, bullet patterns, HUD |

All four examples share the following window controls:

| Key | Action |
| --- | --- |
| `F11` or `Alt+Enter` | Toggle borderless fullscreen on the current monitor |
| `Esc` | Leave fullscreen; in windowed mode, quit |
| `P` | Pause or resume |
| `F1` | Toggle debug overlay |
| `R` | Retry after game over; reset the bouncing-balls playground |

The window is resizable. The four examples use an expanding canvas: wider windows
show more horizontal game space, and taller windows show more vertical space.
Sprites retain their proportions; backgrounds, HUDs, movement bounds and mouse input
follow the new layout. Resizing preserves the current run, recenters brick rows and
keeps the paddle anchored to the bottom. This intentionally changes the available
play area; choose `Fit` for games that require identical playfield geometry.
The previous window size and position are restored when leaving fullscreen.
On Windows, the native title bar follows the light/dark app theme and respects
high-contrast mode. Theme changes are picked up within half a second.
For your own games, use `GameEngine::SetFullscreen`, `SetPaused`, or
`SetCanvasSize`. The default `CanvasScaleMode::Fit` preserves the reference canvas
and letterboxes unused space. Select `Expand` before `Run` to fill the window:

```cpp
game.SetCanvasScaleMode(CanvasScaleMode::Expand);
game.Run();
```

Use `Display::GetWidth/GetHeight/GetSize` for the current logical dimensions and
override `Scene::OnViewportChanged(previous, current)` to reposition your game's
objects or layout. The engine's `GetScreenWidth/Height` return rounded logical units;
raylib's global functions return actual window dimensions. Use `Input::GetMousePosition`
for game coordinates and `Input::IsMouseInCanvas` to ignore clicks in the margins.
The render target is quantized to reduce resize allocations and capped at 4096 pixels
per axis. At extreme resolutions it scales up; a failed resize allocation keeps the
last working target.

The examples use **Rajdhani SemiBold** for headings and scores and **Barlow Medium**
for labels, with matching HUDs and retry panels. The TTF files and SIL Open Font
License notices live in `assets/fonts`; xmake copies them to `bin/assets` through
the shared `example_assets` target. They work without installing system fonts.

In `bouncing_balls`, press `Space` or click in the arena to add a ball, `G` to toggle
gravity, and `R` to reset. Balls share one texture and the arena is capped at 128
balls to keep the physics playground responsive.

## Building

```bash
xmake            # build the library and all four examples
xmake run neon_breakout
xmake run bullet_hell
xmake build tests
xmake run tests   # unit tests (no window)
```

On Windows, install Visual Studio 2022 with the **Desktop development with C++**
workload and configure MSVC explicitly (works in PowerShell and POSIX shells):

```bash
xmake f -p windows -a x64 --toolchain=msvc -m debug -y
xmake
```

Use `-m release` for an optimized build. xmake downloads raylib-cpp/raylib and
compiles the dependencies for the selected toolchain. Targets use UTF-8 source
encoding and put executables in `bin`.

The four examples build as Windows GUI applications by default: launching an EXE
does not open a console, and raylib trace output is disabled. To enable the console
and logs for debugging, run `xmake f --example_console=y` and rebuild. Use
`xmake f --example_console=n` to return to quiet launches. The `tests` and `smoke`
targets always keep their console and diagnostic output.

Quiet examples write raylib diagnostics to per-user rotating logs instead:
`%LOCALAPPDATA%/raygame.<example>/logs/game.log` on Windows and
`$XDG_DATA_HOME/raygame.<example>/logs/game.log` (default `~/.local/share`) on Linux.
Each active log is capped at 1 MiB with one `.previous` file. Custom games can use
`Diagnostics::Initialize` and `InstallRaylibCallback`; log shutdown belongs to the
application that initialized it.

Neon Breakout stores its save in the same per-user application directory and imports
an existing `breakout_save.txt` from the working or executable directory on first use.
`SaveSystem` reads legacy key/value files and writes a versioned, checksummed format
(maximum 1 MiB). Writes use a synced sibling temporary file and atomic replacement;
the previous valid save becomes `.bak`. `Load` tries the backup when the main file
is unreadable or fails validation; `WasRecovered()` reports this case. The checksum
detects corruption and is not an anti-cheat mechanism. A save file should have one
writer process; in-process writes are serialized. `FileSystem::UserDataDirectory`
provides an application-specific writable directory.

When including individual headers, replace the old `#include "Time.h"` with
`#include "GameTime.h"`. Windows treats `Time.h` and the C runtime's `time.h` as
the same filename, so keeping the old header on the include path breaks `<ctime>`
and other standard headers. Including `RayGame.h` needs no change.

## Tests & CI

`tests/main.cpp` and `tests/regressions.h` cover math, lifecycle and deferred
destruction, render ordering, fixed physics, scene switching, transform hierarchy,
tilemaps, tween callback mutation, timers, pooling, events and saves.
`tests/header_compat.cpp` checks standard-library header interoperability.
GitHub Actions builds and runs the logic suite in Debug and Release on Linux and
Windows; Windows explicitly selects MSVC. A separate Windows job builds and verifies
the four ARM64 Android Release APKs with JDK 17, SDK 35 and NDK r28c. All jobs use
xmake 3.1.1 (`.github/workflows/ci.yml`). CI runs on pushes/PRs to `main` and supports
manual **Actions → CI → Run workflow** runs.

Successful runs provide downloadable artifacts for 14 days:

| Artifact | Contents |
| --- | --- |
| `raygame-windows-x64-release` | Four Release EXEs and `assets/` (fonts and notices) |
| `raygame-windows-x64-debug` | Four Debug EXEs and `assets/` |
| `raygame-android-arm64-release` | Four APKs and `verification.json` with SHA-256 hashes |

Open a CI run and download from **Artifacts**. Keep `assets/` next to the extracted
Windows EXEs. Android CI generates a fresh debug signing key for each run; builds
from different runs cannot update one another in place. Android execution remains
a manual device check; CI does not launch an emulator. See [Android notes](docs/ANDROID.md).

An optional integration test runs all four examples in a hidden window, including
runtime spawning, collisions, scene changes, restarts and resource cleanup:

```bash
xmake build smoke
xmake run smoke
```

It requires an OpenGL 3.3-capable display even though the window is hidden.
It also checks fullscreen/window restoration, the native Windows title-bar theme,
font loading, initialization/render failure cleanup, and a 16-case matrix of four
games at 4:3, 16:9, 21:9 and 9:16. Rendered previews are saved to `bin/screenshots`.
The logic suite covers viewport conversion, pause/long-frame timing, callback failure
recovery, Unicode save paths, backup recovery, write failures and log rotation.

## License

MIT License.

## Credits

Built with [raylib](https://www.raylib.com/) and [raylib-cpp](https://github.com/RobLoach/raylib-cpp).
