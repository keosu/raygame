# RayGame Engine

A lightweight 2D game engine with inline C++17 headers and an xmake static-library
target, built on top of raylib-cpp, featuring a
Unity-style GameObject/Component architecture, a working physics layer with collision
callbacks, particles, tweening, an event bus, tilemaps, immediate-mode UI and more.

Version 0.2.0 · C++17 · MIT

See the [product readiness review and roadmap](docs/PRODUCT_READINESS_REVIEW.md)
(Chinese) for current limitations, proposed architecture and release acceptance criteria.

See the [cross-platform guide](docs/PLATFORM_GUIDE.md) (Chinese) for asset versus save
paths, touch ID tracking, adaptive UI anchors, lifecycle boundaries and C++ formatting.

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
- **Collision layers & masks**, sweep-and-prune broad phase
- **Queries**: circle/box overlap, swept segment casts and raycasts with layer masks
- **Impulse-based resolution** with contact normals, penetration correction and static/dynamic handling

### Gameplay systems
- **Tween** engine with 14 easing functions, loop/pingpong/delay, callbacks
- **Event bus**: string-keyed pub/sub with automatic owner cancellation for scenes and components
- **Object pool**: generic free-list pool (`ObjectPool<T>`)
- **Timers**: `DestroyAfter`, repeating/one-shot `TimerComponent`
- **Particle system**: point/circle/box/cone emitters, burst & continuous, color/size over life, drag, textures, additive blending
- **Tilemap**: grid tiles, tileset rendering, per-tile solidity queries

### Input, Audio, UI, Save
- **Input**: injectable keyboard/mouse/gamepad snapshots, fixed-tick edge buffering, axis helpers and named **action mapping**
- **Touch and layout helpers**: `TouchTracker` preserves contact IDs and cancels interrupted gestures;
  `CanvasLayout` anchors controls and centered menus on expanding canvases
- **Audio**: sound/music with master volume (own `Audio.h`)
- **System TTS**: one C++ API for Windows SAPI and Android speech; see [usage](docs/TEXT_TO_SPEECH.md) and [Engine Lab](examples/engine_lab/README.md)
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

Each `Scene` owns an independent `PhysicsWorld`, including registrations, contacts
and callback traversal state. Cross-cutting services are static/singleton: `TweenManager`,
`EventBus`, `ResourceManager`, `AudioManager`, `TextToSpeech`, `Input`, `UI`, `GameTime`.
Events and tweens have application/scene/object/component lifetime scopes. Scene
changes cancel the old scene's callbacks while preserving application services.
Physics worlds can be stepped and cleared independently. Graphics, input and time
services still assume one active application loop; this is not multi-window support.

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

The engine runs these phases in order:

| Phase | Callback / operation | Purpose |
| --- | --- | --- |
| Frame | `OnUnscaledUpdate`, tweens, `Scene/Component::Update` | Application work, frame input and non-physics updates |
| Fixed tick | `FixedUpdate` for all objects | Set velocities/forces and advance fixed simulation logic |
| Fixed tick | Rigidbody integration, then the scene's `PhysicsWorld::Step` | Move bodies, resolve contacts and dispatch collision callbacks |
| Fixed tick | `PostFixedUpdate` | Bounds correction and logic that needs completed collision results |
| Presentation | `GameEngine::OnUpdate`, `Scene/Component::LateUpdate` | Once per frame after fixed ticks; camera following uses `LateUpdate` |
| Drawing | `Render` | Read interpolated poses and draw UI |

Use `GameTime::DeltaTime()` in callbacks: inside a fixed tick it returns the fixed
interval, and `GetTime()` returns the current tick's simulation time. The frame
view is restored afterward, including on exceptions. `FixedDeltaTime()` remains
available explicitly. The interval defaults to 1/60 second, with at most eight
catch-up steps per frame. Integration runs after **all** `FixedUpdate` callbacks,
so a controller may be attached before or after its Rigidbody.
The simulation delta used by ordinary updates and tweens is capped to the same
eight-step budget, so a long stall does not advance animations far ahead of physics.
`GameTime::UnscaledDeltaTime/UnscaledTime` retain actual elapsed time, including while
paused. Override `GameEngine::OnUnscaledUpdate` for work that must continue during pause.
Set the interval between frames with `GameEngine::SetFixedTimeStep`; invalid values
are ignored and changing it during `Tick` throws. `GetFrameTiming()` exposes this
frame's fixed step count, interpolation alpha, fixed simulation time, discarded
long-frame seconds and their running total. Paused time is not counted as discarded.
The F1 overlay displays step count and discarded time. Pause, scene reload and
exceptions discard remaining catch-up work; partially applied simulation is not
transactionally rolled back. Re-entering the same engine's `Tick` is rejected.
Collision layers are numbered 0–31; a nonpositive rigidbody mass is treated as static.

`LoadScene` takes effect on the next frame. The engine unloads and clears the old
scene, its physics registrations, tweens and event subscriptions before loading
the new one. Resource and audio caches persist until shutdown. `Scene::Clear`
also supports rebuilding a scene in place; call `Clear(); Load();` outside component
traversal. A clear during a component callback drains after traversal, and remaining
fixed phases are skipped. Clearing one scene does not clear another scene's world.

### Physics world migration

The former static `PhysicsWorld::Step/Clear/GetColliderCount` calls now use an
instance: `scene.GetPhysicsWorld().Step()` and the corresponding instance methods.
`CheckCollisions()` remains an instance alias of `Step()`. Normal games let the
engine perform the steps; `Step()` only detects/resolves contacts, it does not
integrate bodies. `Scene::FixedUpdate()` is now the pre-physics phase; manual loops
must also run `Scene::IntegratePhysics(dt)`, collision processing and `PostFixedUpdate`.
`Rigidbody::Integrate(dt)` replaces direct calls to its old `FixedUpdate` implementation.

Scene colliders register automatically on `Start`. A standalone `GameObject` has
no implicit world: explicitly add its collider to a `PhysicsWorld` instance.
Registering a scene collider in a different world is rejected. Disabled colliders
retain their binding and stop participating; surviving contacts receive Exit on
the next step. `GetColliderCount()` counts enabled registrations, and
`GetContactCount()` exposes the current contact table. Clear/destruction detaches
bindings and clears contacts without dispatching teardown collision events.

### Transform hierarchy

`position`, `rotation` and `scale` are local values. Use `GetWorldPosition`,
`SetWorldPosition`, `TransformPoint` and `InverseTransformPoint` for world-space
operations. Reparenting preserves world position/rotation/scale by default and
rejects cycles. Removing a parent detaches children while preserving their world
pose. Sprites, camera following and tilemap queries use the hierarchy. Box colliders
remain axis aligned; sprite rendering represents rotation and scale without shear.

Rigidbody enables render interpolation at `Start` by default (set its `interpolate`
field to false to opt out). Fixed logic without a body can call
`transform->SetInterpolationEnabled(true)`. Sprites, animations, tilemaps and example
renderers use `GetRenderPosition/Rotation/Scale` or `TransformRenderPoint`; physics
continues to use authoritative `GetWorld*` values. Interpolation blends the previous
and current fixed poses, with up to one fixed interval of visual latency, and follows
the parent hierarchy without modifying simulation state. Custom renderers should use
these render accessors too. Use `Teleport(worldPosition)` or `ResetInterpolation()`
after discontinuous movement; the engine resets history on pause/resume and viewport
changes. Frame-based animation can leave interpolation disabled.

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

### Rendering, particles and depth sorting

The canvas is presented without blending, so translucent overlays no longer darken
toward the window margin color. `Display::PresentOpaque(texture, dest)` applies the
same rule to your own render targets. To bake a texture while a frame is being drawn,
use a scope; it suspends the canvas and restores its logical projection afterwards:

```cpp
{ Display::OffscreenScope bake(target); ClearBackground(BLANK); DrawStuff(); }
```

Set `ySort = true` on any component for top-down/2.5D depth: within the same layer and
`renderOrder`, objects lower on screen draw later. Override `RenderDepth()` for a custom key.

`ParticleEmitter` interpolates `startColor` → `endColor` (including alpha) over each
lifetime and adds `endSizeScale`, `drag`, an optional `texture` (with visible rotation)
and `blend = ParticleBlend::Additive` for glows and sparks. Existing emitters that only
set `startColor` now fade toward the default white end color; set
`endColor = Fade(startColor, 0)` to keep a pure alpha fade, as the examples do.

### Physics queries and broad phase

`PhysicsWorld` answers gameplay queries without dispatching callbacks:
`OverlapCircle`, `OverlapBox`, `SegmentCast` (optionally swept by a radius, hits sorted
nearest first) and `Raycast`, each with a layer mask and a trigger filter. Worlds with
48 or more colliders prune pairs with a sweep along x before the narrow test
(`SetBroadphaseThreshold` changes the cutoff). Pairs keep registration order, so
callback order is unchanged; a pair that only overlaps after another contact was
resolved earlier in the same step is reported on the next step.

```cpp
for (Collider* c : world.OverlapCircle(tower, range, 1u << kEnemyLayer)) { /* in range */ }
auto line = world.SegmentCast(origin, end, 8.0f); // piercing beam, nearest first
```

### Time scale, pointer capture, audio and international text

- `GameEngine::SetTimeScale(2)` fast-forwards simulation without changing the fixed
  step; the catch-up budget grows with the scale. `0` freezes simulation while UI and
  unscaled time keep running. Components can read `GameTime::GetTimeScale()`.
- `UI::IsMouseOverUI()` reports whether the pointer was over a panel, button or
  progress bar in the last rendered frame, so world clicks under the HUD can be ignored.
  Call `UI::Capture(rect)` for custom widgets.
- `AudioManager::LoadSound(name, path, voices)` lets rapid repeats overlap,
  `SetSoundCooldown` throttles spam, and `SetChannelVolume(AudioChannel::Sfx/Music, v)`
  scales each group for settings menus. `PlaySound` returns whether it played.
- `ResourceManager::CodepointsFromText(text)` gathers the glyphs a UTF-8 string needs
  for `LoadFontResource(..., codepoints)`; `SetFontOptions(name, {true, TEXTURE_FILTER_TRILINEAR})`
  adds mipmaps for large CJK atlases drawn small. `scripts/subset-font.py` shrinks a
  font to the characters used by your sources and assets.

### Save
```cpp
SaveSystem save;
save.SetInt("highscore", 4200);
save.Save("save.txt");
```

## Android APK (experimental)

All six examples, including FORGE and Engine Lab, can be packaged as ARM64 NativeActivity APKs with xmake and raylib.
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
| `bouncing_balls` | Shape Lab: circles, squares and bars; solid/outline styles; mixed circle/box physics |
| `neon_breakout` | Layered neon rendering, tweens, timers, trigger callbacks, camera shake, HUD, save |
| `bullet_hell` | Boss fight with briefing, practice mode, automatic fire, pooled bullets and parent-child turrets |
| `engine_lab` | Six interactive pages: components, physics, sprites, particles/events/pools, tilemaps, system TTS and saves |

[Engine Lab](examples/engine_lab/README.md) demonstrates the APIs with live objects and controls.
Run `xmake run engine_lab`; use its Services page for Windows/Android speech, sound and saves.
Use `1`–`6` to change pages, `R` to rebuild the scene, `F11` for fullscreen, and Esc/Back to exit.

Shape Lab lets you choose shape and drawing style independently, then click/tap the
arena to add bodies. `1`–`4` select mixed/circle/square/bar, `F` cycles mixed/solid/outline,
and `G` toggles gravity. Outlines use the same solid circle/box collision as filled shapes.

In Bullet Hell, you control the cyan ship: move under the boss to hit it with automatic
fire and dodge colored bullets with the small white hitbox. Empty the boss health bar
before losing three shields. Start with **Practice / No Damage**, or choose **Start Mission**;
both give three seconds to prepare. Use WASD/arrows and hold Shift for slow movement;
on Android drag to move and hold **FOCUS** with a second finger. `P`/Android Back pauses.

The original four game examples share the following window controls:

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
License notices live in `examples/common/assets/fonts`; xmake copies them to `bin/assets` through
the shared `example_assets` target. They work without installing system fonts.

In `bouncing_balls`, press `Space` or click in the arena to add a ball, `G` to toggle
gravity, and `R` to reset. Balls share one texture and the arena is capped at 128
balls to keep the physics playground responsive.

## Building

The root `xmake.lua` defines the engine and shared dependencies, then includes
`examples/xmake.lua` and `tests/xmake.lua` for their targets. Run xmake from the
repository root. Each example owns its assets, artwork and Android/Windows packaging
resources under `examples/<name>/`; the five small examples share fonts in
`examples/common/assets/fonts`. The engine's Android raylib recipe is in
`xmake/packages/raylib.lua`. Desktop assets are copied to `bin/assets/fonts` and
`bin/assets/<name>/`; each APK contains only common fonts and its own assets.
Android packages remain in `bin/android/<arch>/<mode>`. See the
[example directory guide](examples/README.md) for ownership and build entries.

```bash
xmake            # build the library and all six desktop examples, including FORGE
xmake run neon_breakout
xmake run bullet_hell
xmake run forge_runner
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

Desktop engine archives live in `build/lib/<platform>/<arch>/<mode>`, so Debug and
Release libraries do not overwrite one another. To build only the engine, configure
with `--build_examples=n --build_tests=n`. Install with
`xmake install -o <sdk-dir> raygame`; independent xmake projects can use the installed
headers and library through the supplied SDK integration script. See
[SDK setup and verification](docs/SDK.md).

All six desktop examples build as Windows GUI applications by default: launching an EXE
does not open a console, and raylib trace output is disabled. To enable the console
and logs for debugging the four small examples, run `xmake f --example_console=y` and rebuild. Use
`xmake f --example_console=n` to return to quiet launches. The `tests` and `smoke`
targets always keep their console and diagnostic output.

Each small example has its own embedded EXE icon, native window icon and Android launcher
icon, including adaptive and themed Android variants. Editable SVG sources and
regeneration instructions are in [the example icon guide](examples/common/ICONS.md).

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

## FORGE reference game

[FORGE / The last delivery](examples/forge_runner/README.md) is a three-sector
industrial sci-fi platform game and an independent SDK consumer, with jump buffering,
coyote time, double jumps, dashes, moving platforms, checkpoints, menus/settings,
saves, procedural artwork and original synthesized audio.
The game switches between Simplified Chinese and English in Settings, with bundled
Noto Sans SC font subsets for readable Chinese menus and level text. The root
`xmake build` includes FORGE on desktop; use `xmake run forge_runner` to play.
Its executable, assets and notices live together in `bin/forge_runner/`.
Build and run its headless tests with `xmake build forge_tests` and `xmake run forge_tests`.
The separate installed-SDK entry remains available on Windows through
`pwsh -NoProfile -File scripts/build-forge.ps1 -Mode release -Verify`.
An experimental Android build adds multitouch movement/jump/dash controls and touch
menus: `pwsh -NoProfile -File scripts/build-android.ps1 -Game forge_runner`.
APK build checks pass locally; Android device execution still needs manual validation.

## Tests & CI

`tests/main.cpp` and `tests/regressions.h` cover math, lifecycle and deferred
destruction, render ordering, fixed physics, scene switching, transform hierarchy,
tilemaps, tween callback mutation, timers, pooling, events and saves.
`tests/header_compat.cpp` checks standard-library header interoperability.
`xmake lua scripts/verify-sdk.lua` installs and relocates an SDK, then builds and runs
an independent consumer with no engine source/example paths. It checks fixed-step
physics, input, storage, and configuration mismatch diagnostics; `--graphics` also
checks a native window and texture cleanup. Desktop CI runs the headless consumer.
GitHub Actions builds all six desktop examples and runs the engine and FORGE logic
suites in Debug and Release on Linux and Windows; Windows explicitly selects MSVC
and also verifies FORGE's standalone SDK build. A separate Windows job builds and verifies
the six ARM64 Android Release APKs with JDK 17, SDK 35 and NDK r28c. All jobs use
xmake 3.1.1 (`.github/workflows/ci.yml`). CI runs on pushes/PRs to `main` and supports
manual **Actions → CI → Run workflow** runs.

Successful runs provide downloadable artifacts for 14 days:

| Artifact | Contents |
| --- | --- |
| `raygame-windows-x64-release` | Five small-example Release EXEs (including Engine Lab), shared `assets/`, and complete FORGE in `forge_runner/` |
| `raygame-windows-x64-debug` | The same six examples and assets in Debug mode |
| `raygame-forge-windows-x64-<mode>` | FORGE EXE, its own `assets/`, README and third-party notices |
| `raygame-forge-linux-x86_64-<mode>` | FORGE Linux executable, `assets/`, README and third-party notices |
| `raygame-android-arm64-release` | Six APKs including FORGE and Engine Lab, and `verification.json` with SHA-256 hashes |

Open a CI run and download from **Artifacts**. Keep `assets/` next to the extracted
Windows EXEs. The separate FORGE artifacts put the executable and assets at the
archive root; after extracting the Linux artifact, run `chmod +x forge_runner`.
Android CI generates a fresh debug signing key for each run; builds
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
window icons, interpolated sprite pixels, font loading, initialization/render failure
cleanup, and a 16-case matrix of four
games at 4:3, 16:9, 21:9 and 9:16. Rendered previews are saved to `bin/screenshots`.
The logic suite covers viewport conversion, pause/long-frame timing, callback failure
recovery, Unicode save paths, backup recovery, write failures and log rotation.

## License

MIT License.

## Credits

Built with [raylib](https://www.raylib.com/) and [raylib-cpp](https://github.com/RobLoach/raylib-cpp).
