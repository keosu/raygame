# Optional Box2D physics

Configure `xmake f --physics_box2d=y` to enable the pinned, statically linked
Box2D **2.4.2** backend. The root project enables it by default; projects that
include RayGame as a subproject opt in. `--physics_box2d=n` builds the original
engine and examples without this dependency or Goo Journey. Box2D 2.4.2 retains
the project's Android API 26 baseline. The installed SDK records the option and
its consumer helper requests the matching package automatically.

```cpp
#include <RayGame.h>

auto scene = RayGame::CreateRef<RayGame::PhysicsScene2D>("bridge");
RayGame::Body2DOptions fixed;
fixed.motion = RayGame::Motion2D::Static;
auto anchor = scene->dynamics.Circle({300, 200}, 15, fixed);
auto ball = scene->dynamics.Circle({400, 200}, 15);
RayGame::Spring2DOptions spring;
spring.frequency = 5;       // Hz; zero makes a rigid distance constraint
spring.dampingRatio = 0.7f;
auto joint = scene->dynamics.DistanceJoint(anchor, ball, spring);
game.LoadScene(scene);
```

`PhysicsScene2D` steps after all components' `FixedUpdate` callbacks and before
`PostFixedUpdate`. Apply forces in the former; inspect poses and joint reactions
in the latter. Read `Position(handle, scene.GetInterpolationAlpha())` while
drawing. Scene clearing, switching and destruction invalidate its handles.
An independent `Physics2DWorld` can also be stepped explicitly for headless
simulation. Do not step a scene-owned world a second time yourself.

The coordinate system follows RayGame: x right, y down. Positions, sizes,
velocities and gravity are in pixels; the default conversion is 50 pixels per
meter. Mass is in kilograms, force in newtons, impulse in newton-seconds,
rotation in radians. A material's mass applies to its single shape. World
steps accept positive finite intervals up to 0.1 seconds; default stepping is
four substeps with 12 velocity and 6 position iterations. Keep a fixed interval.
Forces remain active through all substeps and clear at the end of the tick.

Supported shapes are circles and rotated boxes, with static/dynamic/kinematic
motion, friction, restitution, damping, category/mask filtering and bullet mode.
Distance joints connect body centers; positive frequency enables a damped spring.
Drag joints have a movable target and bounded force. `JointForce` returns the
magnitude of the last substep's reaction force; a game should filter sustained
stress before breaking a joint. `DebugDraw` renders the current shapes/anchors.

`Body2D` and `Joint2D` are opaque, process-unique handles. Queries on invalid or
foreign handles throw `std::invalid_argument`; `Valid`/`Destroy` return false.
Deleting a body deletes all attached joints, including drag joints. `Teleport`
resets that body's interpolation history and wakes it. It is appropriate for
respawning; normal dragging should use a drag joint.

These bodies belong exclusively to Box2D. They do not use the original
`Rigidbody`/`Collider` and do not collide with that separate physics world.
The original collision callbacks and queries currently cover the original world
only. This first adapter does not expose polygon/chain fixtures, off-center
anchors, contact callbacks, or the remaining Box2D joint types.

`examples/goo_journey` demonstrates a full three-level campaign. Its rods are
visual connections between physical nodes; they do not have separate collision
fixtures. Construction checks terrain intersections, and dynamic nodes collide
with terrain while connected-node self collision is disabled. Travelers follow
the graph and distribute 1.8 N of load between their supporting endpoints.

Validation targets: `xmake run goo_tests` (one-second placement intervals),
`xmake run goo_tests 120` (two seconds), and `goo_journey --smoke --data <absolute
directory>` for actual input, rendering, progression and persistence. See the
current AGENT_STATE for results and platform limits.

Box2D is MIT licensed; the notice is in the game's assets and installed SDK.
