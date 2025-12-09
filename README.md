# RayGame Engine

A lightweight 2D game engine built on top of raylib-cpp, featuring ECS architecture, physics, particles, and more.

## Features

### Core Systems
- **Entity-Component System (ECS)**: Flexible GameObject and Component architecture
- **Scene Management**: Easy scene loading and switching
- **Resource Manager**: Centralized texture and resource management
- **Time System**: Delta time, frame counting, and timing utilities

### Rendering
- **Sprite Rendering**: Texture rendering with transform support
- **Sprite Animation**: Frame-based animation system with customizable timing
- **Camera System**: 2D camera with following, zoom, and smooth movement
- **Layer System**: Render order control

### Physics Engine
- **Rigidbody**: Velocity, acceleration, mass, drag, and gravity
- **Colliders**: AABB (Box) and Circle collision detection
- **Physics World**: Automatic collision detection and response
- **Trigger Support**: Event-based collision callbacks

### Particle System
- **Flexible Emitters**: Point, Circle, Box, and Cone emission shapes
- **Particle Properties**: Lifetime, velocity, acceleration, size, rotation, color
- **Emission Control**: Continuous emission and burst modes
- **Performance**: Efficient particle pooling and culling

### Input & Audio
- **Input Manager**: Keyboard, mouse, and axis-based input
- **Audio Manager**: Sound effects and music streaming
- **Easy Integration**: Simple API for common input patterns

## Architecture

```
GameObject
├── Transform (position, rotation, scale)
└── Components
    ├── SpriteRenderer
    ├── SpriteAnimation
    ├── Rigidbody
    ├── Collider (Box/Circle)
    ├── ParticleEmitter
    └── Custom Components
```

## Quick Start

### 1. Create a Scene

```cpp
class MyScene : public Scene {
public:
    void OnLoad() override {
        // Create a game object
        auto player = CreateGameObject("Player");
        player->transform->position = {400, 300};
        
        // Add components
        auto sprite = player->AddComponent<SpriteRenderer>(texture);
        auto rb = player->AddComponent<Rigidbody>();
        auto collider = player->AddComponent<CircleCollider>(16.0f);
    }
    
    void Update() override {
        Scene::Update();
        // Custom update logic
    }
};
```

### 2. Create Custom Components

```cpp
class PlayerController : public Component {
public:
    float speed = 200.0f;
    
    void Update() override {
        Vector2 input = Input::GetMovementInput();
        auto rb = gameObject->GetComponent<Rigidbody>();
        if (rb) {
            rb->SetVelocity({input.x * speed, input.y * speed});
        }
    }
};
```

### 3. Initialize the Engine

```cpp
class MyGame : public GameEngine {
public:
    MyGame() : GameEngine("My Game", 800, 600) {}
    
    void OnInit() override {
        auto scene = CreateRef<MyScene>();
        LoadScene(scene);
    }
};

int main() {
    MyGame game;
    game.Run();
    return 0;
}
```

## Building

```bash
cd raygame
xmake build
xmake run
```

## Example Game

The included `main.cpp` demonstrates a complete Space Shooter game featuring:
- Player movement and shooting
- Enemy AI with sine wave patterns
- Particle effects for explosions and engine trails
- Collision detection between bullets and enemies
- Score tracking and game over system
- Debug visualization (F1 to toggle)

### Controls
- **WASD / Arrow Keys**: Move
- **Space**: Shoot
- **F1**: Toggle debug view
- **R**: Restart (when game over)
- **ESC**: Quit

## Engine Components Reference

### Transform
```cpp
transform->position = {x, y};
transform->rotation = degrees;
transform->scale = {sx, sy};
transform->Translate({dx, dy});
transform->Rotate(degrees);
```

### SpriteRenderer
```cpp
auto sprite = obj->AddComponent<SpriteRenderer>(texture);
sprite->tint = RED;
sprite->pivot = {0.5f, 0.5f};  // Center
sprite->flipX = false;
sprite->layer = 0;
```

### SpriteAnimation
```cpp
auto anim = obj->AddComponent<SpriteAnimation>();
anim->spriteSheet = texture;
anim->CreateFramesFromGrid(32, 32, 8, 0.1f);  // 8 frames, 0.1s each
anim->loop = true;
anim->Play();
```

### Rigidbody
```cpp
auto rb = obj->AddComponent<Rigidbody>();
rb->mass = 1.0f;
rb->drag = 0.99f;
rb->useGravity = true;
rb->gravity = 500.0f;
rb->AddForce({100, 0});
rb->AddImpulse({0, -200});
```

### Colliders
```cpp
// Box collider
auto box = obj->AddComponent<BoxCollider>(Vector2{32, 32});
box->isTrigger = false;

// Circle collider
auto circle = obj->AddComponent<CircleCollider>(16.0f);
circle->isTrigger = true;
```

### ParticleEmitter
```cpp
auto particles = obj->AddComponent<ParticleEmitter>();
particles->emissionRate = 50;
particles->maxParticles = 100;
particles->shape = EmitterShape::Circle;
particles->startVelocityMin = {-100, -100};
particles->startVelocityMax = {100, 100};
particles->lifetimeMin = 1.0f;
particles->lifetimeMax = 2.0f;
particles->Burst(50);  // Emit 50 particles immediately
```

### Camera2D
```cpp
auto camera = cameraObj->AddComponent<Camera2DComponent>();
camera->SetTarget(player.get());
camera->zoom = 1.5f;
camera->smoothSpeed = 5.0f;
```

## Performance Tips

1. **Object Pooling**: Reuse game objects instead of creating/destroying
2. **Particle Limits**: Set appropriate `maxParticles` limits
3. **Collider Count**: Minimize number of active colliders
4. **Texture Atlas**: Use sprite sheets for multiple sprites
5. **Debug Mode**: Disable physics debug drawing in release builds

## License

MIT License - feel free to use in your projects!

## Credits

Built with [raylib](https://www.raylib.com/) and [raylib-cpp](https://github.com/RobLoach/raylib-cpp)

