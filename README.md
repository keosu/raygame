# RayGame Engine

A lightweight 2D game engine with inline C++17 headers and an xmake static-library
target, built on top of raylib-cpp, featuring a
Unity-style GameObject/Component architecture, a working physics layer with collision
callbacks, particles, tweening, an event bus, tilemaps, immediate-mode UI and more.

Version 0.2.0 · C++17 · MIT

## Features

### Core
- **GameObject/Component architecture** with `Awake`/`Start`/`Update`/`FixedUpdate`/`Render` lifecycle
- **Transform hierarchy**: parent/child with local ↔ world position, rotation and scale
- **Deferred destruction & safe spawning**: `Destroy()` is queued; objects spawned at runtime are started automatically
- **Scene management** with deferred scene switching
- **Fixed timestep** loop for game logic and rigidbody physics (`FixedUpdate`)
- **Resource Manager**: textures, fonts, shaders, procedural textures, individual unload
- **Time** utilities: delta time, elapsed time, frame count

### Rendering
- **Sprite & animation** components with pivot, flip and tint
- **Global layer sorting**: components draw in `layer`/`renderOrder` order across all objects
- **Camera2D** with smooth follow, zoom, world/screen conversion, opt-in render integration
- **Resizable window & borderless fullscreen** with a fixed logical canvas, aspect-ratio preservation and mapped mouse input
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
- **Event bus**: string-keyed pub/sub with a small payload
- **Object pool**: generic free-list pool (`ObjectPool<T>`)
- **Timers**: `DestroyAfter`, repeating/one-shot `TimerComponent`
- **Particle system**: point/circle/box/cone emitters, burst & continuous
- **Tilemap**: grid tiles, tileset rendering, per-tile solidity queries

### Input, Audio, UI, Save
- **Input**: keyboard, mouse, gamepad, axis helpers, and named **action mapping**
- **Audio**: sound/music with master volume (own `Audio.h`)
- **Immediate-mode UI**: labels, panels, buttons, progress bars
- **Save system**: dependency-free key/value persistence

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

Use `GameTime::DeltaTime()` in `Update` and `GameTime::FixedDeltaTime()` in
`FixedUpdate`. Rigidbody integration and collision processing run at the fixed
interval (1/60 second by default), with at most eight catch-up steps per frame.
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

Store the subscription id and unsubscribe in `OnDestroy` when a handler captures
a component. Handlers removed during dispatch are skipped; newly added handlers
wait until the next emission. Tweens created inside callbacks likewise start on
the next update; callbacks may safely cancel or clear tweens.

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

### Save
```cpp
SaveSystem save;
save.SetInt("highscore", 4200);
save.Save("save.txt");
```

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

The window is resizable. Game coordinates stay at the example's original
resolution; unused space is letterboxed, and mouse input follows the same viewport.
The previous window size and position are restored when leaving fullscreen.
On Windows, the native title bar follows the light/dark app theme and respects
high-contrast mode. Theme changes are picked up within half a second.
For your own games, use `GameEngine::SetFullscreen`, `SetPaused`, or
`SetCanvasSize`. `GetScreenWidth/Height` on the engine return logical canvas units;
raylib's global functions return the actual window size. Use `Input::GetMousePosition`
for game coordinates and `Input::IsMouseInCanvas` to ignore clicks in the margins.

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
Windows; Windows explicitly selects MSVC (`.github/workflows/ci.yml`).

An optional integration test runs all four examples in a hidden window, including
runtime spawning, collisions, scene changes, restarts and resource cleanup:

```bash
xmake build smoke
xmake run smoke
```

It requires an OpenGL 3.3-capable display even though the window is hidden.
It also checks fullscreen/window restoration, the native Windows title-bar theme,
font loading, viewport scaling and
resource cleanup, and saves rendered previews to `bin/screenshots`. The logic suite
covers letterboxing, coordinate conversion and pause timing without a window.

## License

MIT License.

## Credits

Built with [raylib](https://www.raylib.com/) and [raylib-cpp](https://github.com/RobLoach/raylib-cpp).
