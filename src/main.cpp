/*
Raylib + Box2D Physics Example
This example demonstrates integration between Raylib and Box2D physics engine.
Features:
- Dynamic physics bodies (falling boxes and circles)
- Static ground and walls
- Mouse interaction to spawn new bodies
- Visual debug rendering of physics bodies
- Real-time physics simulation

Controls:
- Left Mouse Button: Spawn box at mouse position
- Right Mouse Button: Spawn circle at mouse position
- R: Reset simulation
- ESC: Exit

Box2D version: 2.4.2
*/

#include "raylib.h"
#include "box2d/box2d.h"
#include "resource_dir.h"

#include <vector>
#include <memory>

// Physics constants
const float PHYSICS_SCALE = 30.0f;  // pixels per meter
const float TIME_STEP = 1.0f / 60.0f;
const int32 VELOCITY_ITERATIONS = 6;
const int32 POSITION_ITERATIONS = 2;

// Convert between Box2D world coordinates and screen coordinates
Vector2 WorldToScreen(const b2Vec2& worldPos) {
    return { worldPos.x * PHYSICS_SCALE, 600 - worldPos.y * PHYSICS_SCALE };
}

b2Vec2 ScreenToWorld(const Vector2& screenPos) {
    return b2Vec2(screenPos.x / PHYSICS_SCALE, (600 - screenPos.y) / PHYSICS_SCALE);
}

// Physics body wrapper class
class PhysicsBody {
public:
    b2Body* body;
    Color color;
    bool isCircle;
    float radius;
    Vector2 size;
    
    PhysicsBody(b2Body* b, Color c, bool circle = false, float r = 0, Vector2 s = {0, 0}) 
        : body(b), color(c), isCircle(circle), radius(r), size(s) {}
};

int main() {
    // Initialize Raylib
    const int screenWidth = 1280;
    const int screenHeight = 800;
    
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "Raylib + Box2D Physics Example");
    
    // Initialize Box2D world
    b2Vec2 gravity(0.0f, -9.8f);
    b2World world(gravity);
    
    std::vector<std::unique_ptr<PhysicsBody>> bodies;
    
    // Create ground
    {
        b2BodyDef groundBodyDef;
        groundBodyDef.position.Set(screenWidth / (2 * PHYSICS_SCALE), 1.0f);
        b2Body* groundBody = world.CreateBody(&groundBodyDef);
        
        b2PolygonShape groundBox;
        groundBox.SetAsBox(screenWidth / (2 * PHYSICS_SCALE), 1.0f);
        groundBody->CreateFixture(&groundBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(groundBody, BROWN, false, 0, 
            Vector2{screenWidth, 2 * PHYSICS_SCALE}));
    }
    
    // Create walls
    {
        // Left wall
        b2BodyDef leftWallDef;
        leftWallDef.position.Set(1.0f, screenHeight / (2 * PHYSICS_SCALE));
        b2Body* leftWall = world.CreateBody(&leftWallDef);
        
        b2PolygonShape leftWallBox;
        leftWallBox.SetAsBox(1.0f, screenHeight / (2 * PHYSICS_SCALE));
        leftWall->CreateFixture(&leftWallBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(leftWall, DARKGRAY, false, 0,
            Vector2{2 * PHYSICS_SCALE, screenHeight}));
    }
    
    {
        // Right wall
        b2BodyDef rightWallDef;
        rightWallDef.position.Set((screenWidth / PHYSICS_SCALE) - 1.0f, screenHeight / (2 * PHYSICS_SCALE));
        b2Body* rightWall = world.CreateBody(&rightWallDef);
        
        b2PolygonShape rightWallBox;
        rightWallBox.SetAsBox(1.0f, screenHeight / (2 * PHYSICS_SCALE));
        rightWall->CreateFixture(&rightWallBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(rightWall, DARKGRAY, false, 0,
            Vector2{2 * PHYSICS_SCALE, screenHeight}));
    }
    
    // Create initial falling objects
    for (int i = 0; i < 5; i++) {
        // Create dynamic box
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.Set(5.0f + i * 2.0f, 15.0f);
        b2Body* body = world.CreateBody(&bodyDef);
        
        b2PolygonShape dynamicBox;
        dynamicBox.SetAsBox(0.5f, 0.5f);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &dynamicBox;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        fixtureDef.restitution = 0.4f;
        body->CreateFixture(&fixtureDef);
        
        Color boxColor = {
            (unsigned char)(GetRandomValue(100, 255)),
            (unsigned char)(GetRandomValue(100, 255)),
            (unsigned char)(GetRandomValue(100, 255)),
            255
        };
        
        bodies.push_back(std::make_unique<PhysicsBody>(body, boxColor, false, 0,
            Vector2{PHYSICS_SCALE, PHYSICS_SCALE}));
    }
    
    // Create initial falling circles
    for (int i = 0; i < 3; i++) {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.Set(15.0f + i * 2.0f, 20.0f);
        b2Body* body = world.CreateBody(&bodyDef);
        
        b2CircleShape circle;
        circle.m_radius = 0.7f;
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circle;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        fixtureDef.restitution = 0.6f;
        body->CreateFixture(&fixtureDef);
        
        Color circleColor = {
            (unsigned char)(GetRandomValue(100, 255)),
            (unsigned char)(GetRandomValue(100, 255)),
            (unsigned char)(GetRandomValue(100, 255)),
            255
        };
        
        bodies.push_back(std::make_unique<PhysicsBody>(body, circleColor, true, 
            0.7f * PHYSICS_SCALE, Vector2{0, 0}));
    }
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Handle input
        Vector2 mousePos = GetMousePosition();
        
        // Spawn box on left mouse click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            b2Vec2 worldPos = ScreenToWorld(mousePos);
            
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(worldPos.x, worldPos.y);
            b2Body* body = world.CreateBody(&bodyDef);
            
            b2PolygonShape dynamicBox;
            float size = GetRandomValue(20, 40) / PHYSICS_SCALE;
            dynamicBox.SetAsBox(size / 2, size / 2);
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.3f;
            fixtureDef.restitution = GetRandomValue(20, 80) / 100.0f;
            body->CreateFixture(&fixtureDef);
            
            Color boxColor = {
                (unsigned char)(GetRandomValue(100, 255)),
                (unsigned char)(GetRandomValue(100, 255)),
                (unsigned char)(GetRandomValue(100, 255)),
                255
            };
            
            bodies.push_back(std::make_unique<PhysicsBody>(body, boxColor, false, 0,
                Vector2{size * PHYSICS_SCALE, size * PHYSICS_SCALE}));
        }
        
        // Spawn circle on right mouse click
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            b2Vec2 worldPos = ScreenToWorld(mousePos);
            
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(worldPos.x, worldPos.y);
            b2Body* body = world.CreateBody(&bodyDef);
            
            b2CircleShape circle;
            float radius = GetRandomValue(15, 35) / PHYSICS_SCALE;
            circle.m_radius = radius;
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &circle;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.3f;
            fixtureDef.restitution = GetRandomValue(40, 90) / 100.0f;
            body->CreateFixture(&fixtureDef);
            
            Color circleColor = {
                (unsigned char)(GetRandomValue(100, 255)),
                (unsigned char)(GetRandomValue(100, 255)),
                (unsigned char)(GetRandomValue(100, 255)),
                255
            };
            
            bodies.push_back(std::make_unique<PhysicsBody>(body, circleColor, true,
                radius * PHYSICS_SCALE, Vector2{0, 0}));
        }
        
        // Reset simulation
        if (IsKeyPressed(KEY_R)) {
            // Clear all dynamic bodies (keep walls and ground)
            for (auto it = bodies.begin() + 3; it != bodies.end(); ) {
                world.DestroyBody((*it)->body);
                it = bodies.erase(it);
            }
        }
        
        // Step the physics world
        world.Step(TIME_STEP, VELOCITY_ITERATIONS, POSITION_ITERATIONS);
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw physics bodies
        for (const auto& physicsBody : bodies) {
            b2Vec2 position = physicsBody->body->GetPosition();
            float angle = physicsBody->body->GetAngle();
            Vector2 screenPos = WorldToScreen(position);
            
            if (physicsBody->isCircle) {
                DrawCircleV(screenPos, physicsBody->radius, physicsBody->color);
                DrawCircleLinesV(screenPos, physicsBody->radius, BLACK);
                
                // Draw radius line to show rotation
                Vector2 endPos = {
                    screenPos.x + physicsBody->radius * cosf(angle),
                    screenPos.y - physicsBody->radius * sinf(angle)
                };
                DrawLineV(screenPos, endPos, BLACK);
            } else {
                Rectangle rect = {
                    screenPos.x - physicsBody->size.x / 2,
                    screenPos.y - physicsBody->size.y / 2,
                    physicsBody->size.x,
                    physicsBody->size.y
                };
                
                // Draw rotated rectangle
                Vector2 origin = { physicsBody->size.x / 2, physicsBody->size.y / 2 };
                DrawRectanglePro(rect, origin, angle * RAD2DEG, physicsBody->color);
                DrawRectangleLinesEx(
                    {rect.x, rect.y, rect.width, rect.height}, 
                    1, BLACK
                );
            }
        }
        
        // Draw UI
        DrawText("Raylib + Box2D Physics Example", 10, 10, 20, DARKGRAY);
        DrawText("Left Click: Spawn Box", 10, 40, 16, DARKGRAY);
        DrawText("Right Click: Spawn Circle", 10, 60, 16, DARKGRAY);
        DrawText("R: Reset", 10, 80, 16, DARKGRAY);
        DrawText("ESC: Exit", 10, 100, 16, DARKGRAY);
        
        // Draw physics info
        DrawText(TextFormat("Bodies: %d", (int)bodies.size()), screenWidth - 150, 10, 16, DARKGRAY);
        DrawText(TextFormat("FPS: %d", GetFPS()), screenWidth - 150, 30, 16, DARKGRAY);
        
        // Draw crosshair at mouse position
        Vector2 mouse = GetMousePosition();
        DrawLine(mouse.x - 10, mouse.y, mouse.x + 10, mouse.y, RED);
        DrawLine(mouse.x, mouse.y - 10, mouse.x, mouse.y + 10, RED);
        
        EndDrawing();
    }
    
    // Cleanup
    CloseWindow();
    return 0;
}
