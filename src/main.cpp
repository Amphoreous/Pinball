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
Vector2 WorldToScreen(const b2Vec2& worldPos, int screenHeight) {
    return { worldPos.x * PHYSICS_SCALE, screenHeight - worldPos.y * PHYSICS_SCALE };
}

b2Vec2 ScreenToWorld(const Vector2& screenPos, int screenHeight) {
    return b2Vec2(screenPos.x / PHYSICS_SCALE, (screenHeight - screenPos.y) / PHYSICS_SCALE);
}

// Physics body wrapper class
class PhysicsBody {
public:
    b2Body* body;
    Color color;
    bool isCircle;
    float radius;
    Vector2 size;
    bool visible;  // Add visibility flag
    
    PhysicsBody(b2Body* b, Color c, bool circle = false, float r = 0, Vector2 s = {0, 0}, bool vis = true) 
        : body(b), color(c), isCircle(circle), radius(r), size(s), visible(vis) {}
};

int main() {
    // Get monitor dimensions for fullscreen
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);
    
    // Initialize Raylib in fullscreen
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(monitorWidth, monitorHeight, "Raylib + Box2D Physics Example");
    
    // Get actual screen dimensions after initialization
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    
    // Convert to float for physics calculations
    const float screenWidthF = static_cast<float>(screenWidth);
    const float screenHeightF = static_cast<float>(screenHeight);
    
    // Initialize Box2D world
    b2Vec2 gravity(0.0f, -9.8f);
    b2World world(gravity);
    
    std::vector<std::unique_ptr<PhysicsBody>> bodies;
    
    // Create ground
    {
        b2BodyDef groundBodyDef;
        groundBodyDef.position.Set(screenWidthF / (2 * PHYSICS_SCALE), 1.0f);
        b2Body* groundBody = world.CreateBody(&groundBodyDef);
        
        b2PolygonShape groundBox;
        groundBox.SetAsBox(screenWidthF / (2 * PHYSICS_SCALE), 1.0f);
        groundBody->CreateFixture(&groundBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(groundBody, BROWN, false, 0, 
            Vector2{screenWidthF, 2 * PHYSICS_SCALE}));
    }
    
    // Create walls
    {
        // Left wall - position it at the edge
        b2BodyDef leftWallDef;
        leftWallDef.position.Set(0.5f, screenHeightF / (2 * PHYSICS_SCALE));
        b2Body* leftWall = world.CreateBody(&leftWallDef);
        
        b2PolygonShape leftWallBox;
        leftWallBox.SetAsBox(0.5f, screenHeightF / (2 * PHYSICS_SCALE));
        leftWall->CreateFixture(&leftWallBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(leftWall, DARKGRAY, false, 0,
            Vector2{PHYSICS_SCALE, screenHeightF}, false)); // Make invisible
    }
    
    {
        // Right wall - position it at the right edge
        b2BodyDef rightWallDef;
        rightWallDef.position.Set((screenWidthF / PHYSICS_SCALE) - 0.5f, screenHeightF / (2 * PHYSICS_SCALE));
        b2Body* rightWall = world.CreateBody(&rightWallDef);
        
        b2PolygonShape rightWallBox;
        rightWallBox.SetAsBox(0.5f, screenHeightF / (2 * PHYSICS_SCALE));
        rightWall->CreateFixture(&rightWallBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(rightWall, DARKGRAY, false, 0,
            Vector2{PHYSICS_SCALE, screenHeightF}, false)); // Make invisible
    }
    
    {
        // Top wall - prevent objects from escaping upward
        b2BodyDef topWallDef;
        topWallDef.position.Set(screenWidthF / (2 * PHYSICS_SCALE), (screenHeightF / PHYSICS_SCALE) - 0.5f);
        b2Body* topWall = world.CreateBody(&topWallDef);
        
        b2PolygonShape topWallBox;
        topWallBox.SetAsBox(screenWidthF / (2 * PHYSICS_SCALE), 0.5f);
        topWall->CreateFixture(&topWallBox, 0.0f);
        
        bodies.push_back(std::make_unique<PhysicsBody>(topWall, DARKGRAY, false, 0,
            Vector2{screenWidthF, PHYSICS_SCALE}, false)); // Make invisible
    }
    
    // Create initial falling objects
    for (int i = 0; i < 5; i++) {
    // Create dynamic box
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.bullet = true; // Enable CCD for fast-moving box
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
    bodyDef.bullet = true; // Enable CCD for fast-moving circle
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
            b2Vec2 worldPos = ScreenToWorld(mousePos, screenHeight);
            
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.bullet = true; // Enable CCD for spawned box
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
            b2Vec2 worldPos = ScreenToWorld(mousePos, screenHeight);
            
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.bullet = true; // Enable CCD for spawned circle
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
            // Clear all dynamic bodies (keep walls and ground - first 4 bodies)
            for (auto it = bodies.begin() + 4; it != bodies.end(); ) {
                world.DestroyBody((*it)->body);
                it = bodies.erase(it);
            }
        }
        
        // Toggle fullscreen
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        
        // Step the physics world
        world.Step(TIME_STEP, VELOCITY_ITERATIONS, POSITION_ITERATIONS);
        
        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw physics bodies
        for (const auto& physicsBody : bodies) {
            // Skip invisible bodies
            if (!physicsBody->visible) continue;
            
            b2Vec2 position = physicsBody->body->GetPosition();
            float angle = physicsBody->body->GetAngle();
            const float angleCW = -angle; // screen-space clockwise rotation
            Vector2 screenPos = WorldToScreen(position, screenHeight);
            
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
                // Draw rotated rectangle (position at center, pivot at center)
                Rectangle rect = {
                    screenPos.x,
                    screenPos.y,
                    physicsBody->size.x,
                    physicsBody->size.y
                };

                Vector2 origin = { physicsBody->size.x / 2, physicsBody->size.y / 2 };
                // Note: Box2D angle is CCW in a Y-up world; screen is Y-down. Use angleCW for consistency.
                DrawRectanglePro(rect, origin, angleCW * RAD2DEG, physicsBody->color);
                
                // Draw rotated outline - compute corners using same clockwise rotation (angleCW)
                float cosA = cosf(angleCW);
                float sinA = sinf(angleCW);
                float halfW = physicsBody->size.x / 2.0f;
                float halfH = physicsBody->size.y / 2.0f;

                // Local corners before rotation (relative to center)
                // top-left (-halfW, -halfH), top-right (halfW, -halfH),
                // bottom-right (halfW, halfH), bottom-left (-halfW, halfH)
                Vector2 corners[4];
                // Apply rotation by angleCW (standard CCW formulas but using angleCW already flipped)
                // x' = x*cosA - y*sinA; y' = x*sinA + y*cosA
                corners[0] = { screenPos.x + (-halfW * cosA - -halfH * sinA), screenPos.y + (-halfW * sinA + -halfH * cosA) }; // top-left
                corners[1] = { screenPos.x + ( halfW * cosA - -halfH * sinA), screenPos.y + ( halfW * sinA + -halfH * cosA) }; // top-right
                corners[2] = { screenPos.x + ( halfW * cosA -  halfH * sinA), screenPos.y + ( halfW * sinA +  halfH * cosA) }; // bottom-right
                corners[3] = { screenPos.x + (-halfW * cosA -  halfH * sinA), screenPos.y + (-halfW * sinA +  halfH * cosA) }; // bottom-left
                
                // Draw the 4 edges
                DrawLineV(corners[0], corners[1], BLACK);
                DrawLineV(corners[1], corners[2], BLACK);
                DrawLineV(corners[2], corners[3], BLACK);
                DrawLineV(corners[3], corners[0], BLACK);
            }
        }
        
        // Draw UI
        DrawText("Raylib + Box2D Physics Example", 10, 10, 20, DARKGRAY);
        DrawText("Left Click: Spawn Box", 10, 40, 16, DARKGRAY);
        DrawText("Right Click: Spawn Circle", 10, 60, 16, DARKGRAY);
        DrawText("R: Reset", 10, 80, 16, DARKGRAY);
        DrawText("F11: Toggle Fullscreen", 10, 100, 16, DARKGRAY);
        DrawText("ESC: Exit", 10, 120, 16, DARKGRAY);
        
        // Draw physics info
        DrawText(TextFormat("Bodies: %d", (int)bodies.size()), screenWidth - 150, 10, 16, DARKGRAY);
        DrawText(TextFormat("FPS: %d", GetFPS()), screenWidth - 150, 30, 16, DARKGRAY);
        DrawText(TextFormat("Resolution: %dx%d", screenWidth, screenHeight), screenWidth - 200, 50, 16, DARKGRAY);
        
        // Draw crosshair at mouse position
        Vector2 mouse = GetMousePosition();
        DrawLine((int)(mouse.x - 10), (int)mouse.y, (int)(mouse.x + 10), (int)mouse.y, RED);
        DrawLine((int)mouse.x, (int)(mouse.y - 10), (int)mouse.x, (int)(mouse.y + 10), RED);
        
        EndDrawing();
    }
    
    // Cleanup
    CloseWindow();
    return 0;
}
