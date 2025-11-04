#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"
#include "PhysBody.h"

#include "p2Point.h"

#include <math.h>

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	debug = true;
}

// Destructor
ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
	LOG("Creating Physics 2D environment");
	
	// Create Box2D world with gravity
	b2Vec2 gravity(0.0f, -10.0f); // Gravity pointing down (in Box2D, +Y is up)
	world = new b2World(gravity);
	world->SetContactListener(this);
	
	// Create ground body for mouse joint
	b2BodyDef bd;
	ground = world->CreateBody(&bd);
	
	return true;
}

update_status ModulePhysics::PreUpdate()
{
	// Step the physics world with higher iterations for better collision detection
	// velocityIterations: 10 (increased from 8)
	// positionIterations: 8 (increased from 3)
	world->Step(1.0f / 60.0f, 10, 8);
	
	// Handle mouse joint for dragging objects in debug mode
	if(debug && mouse_joint == nullptr && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		// Use vector version to avoid any potential rounding differences
		Vector2 mp = GetMousePosition();
		b2Vec2 p;
		// Convert screen coords to Box2D coords
		p.x = mp.x * PIXELS_TO_METERS;
		p.y = (SCREEN_HEIGHT - mp.y) * PIXELS_TO_METERS;
		
		LOG("MOUSE CLICK: Screen(%.0f, %.0f) -> Box2D(%.2f, %.2f)", 
			mp.x, mp.y, p.x, p.y);
		
		int bodyCount = 0;
		b2Body* bestBody = nullptr;
		float bestDist = 1e9f;
		
		// Check if we clicked on a body (robust picking: TestPoint, then circle-distance fallback)
		for(b2Body* b = world->GetBodyList(); b; b = b->GetNext())
		{
			bodyCount++;
			b2Vec2 pos = b->GetPosition();
			LOG("Body #%d: type=%d, pos=(%.2f, %.2f)", 
				bodyCount, (int)b->GetType(), pos.x, pos.y);
			
			if(b->GetType() == b2_dynamicBody)
			{
				LOG("  -> Dynamic body found!");
				for(b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
				{
					bool hit = f->TestPoint(p);
					
					// If TestPoint missed, try a distance check for circle shapes (more tolerant)
					if(!hit && f->GetShape()->GetType() == b2Shape::e_circle)
					{
						b2CircleShape* cs = (b2CircleShape*)f->GetShape();
						b2Vec2 center = b->GetWorldPoint(cs->m_p);
						float dist = b2Distance(center, p);
						float tol = cs->m_radius + 0.10f; // 0.1m tolerance (~5px)
						hit = dist <= tol;
						LOG("  -> Circle dist=%.3f, radius=%.3f, tol=%.3f => %s", dist, cs->m_radius, tol, hit?"HIT":"miss");
						if(!hit && dist < bestDist)
						{
							bestDist = dist;
							bestBody = b;
						}
					}
					else
					{
						LOG("  -> TestPoint result: %s", hit ? "HIT!" : "miss");
					}
					
					if(hit)
					{
						LOG("SUCCESS! Creating mouse joint");
						// Create mouse joint with proper parameters
						b2MouseJointDef md;
						md.bodyA = ground;
						md.bodyB = b;
						md.target = p;
						md.collideConnected = true;
						md.maxForce = 500.0f * b->GetMass();
						md.stiffness = 20.0f;
						md.damping = 0.7f;
						mouse_joint = (b2MouseJoint*)world->CreateJoint(&md);
						b->SetAwake(true);
						LOG("Mouse joint created successfully!");
						break;
					}
				}
			}
			if(mouse_joint) break;
		}
		
		// Fallback: if we didn't hit exactly, but we clicked close to a circle body, attach to the closest one within a reasonable range
		if(!mouse_joint && bestBody && bestDist <= 0.45f) // ~22px
		{
			LOG("Fallback grab: closest circle at dist=%.3f m (<= 0.45)", bestDist);
			b2MouseJointDef md;
			md.bodyA = ground;
			md.bodyB = bestBody;
			md.target = p;
			md.collideConnected = true;
			md.maxForce = 500.0f * bestBody->GetMass();
			md.stiffness = 20.0f;
			md.damping = 0.7f;
			mouse_joint = (b2MouseJoint*)world->CreateJoint(&md);
			bestBody->SetAwake(true);
			LOG("Mouse joint created via fallback proximity test.");
		}
		
		if(!mouse_joint)
		{
			LOG("FAILED: No body was hit");
		}
	}
	
	// Update mouse joint target
	if(mouse_joint && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		b2Vec2 p;
		// Convert screen coords to Box2D coords
		p.x = GetMouseX() * PIXELS_TO_METERS;
		p.y = (SCREEN_HEIGHT - GetMouseY()) * PIXELS_TO_METERS;
		mouse_joint->SetTarget(p);
	}
	
	// Release mouse joint
	if(mouse_joint && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		LOG("Releasing mouse joint");
		world->DestroyJoint(mouse_joint);
		mouse_joint = nullptr;
	}

	return UPDATE_CONTINUE;
}

update_status ModulePhysics::PostUpdate()
{
	if(IsKeyPressed(KEY_F1))
	{
		debug = !debug;
		LOG("Debug mode: %s", debug ? "ON" : "OFF");
	}

	if(!debug)
	{
		return UPDATE_CONTINUE;
	}

	// Draw debug text
	DrawText("DEBUG MODE (F1 to toggle) - Click and drag ball", 10, SCREEN_HEIGHT - 30, 20, RED);

	// Visualize mouse position in both screen and world space for diagnostics
	{
		Vector2 mp = GetMousePosition();
		b2Vec2 p;
		p.x = mp.x * PIXELS_TO_METERS;
		p.y = (SCREEN_HEIGHT - mp.y) * PIXELS_TO_METERS;
		// Draw a small cross at the mouse position in screen space
		DrawLine((int)mp.x - 6, (int)mp.y, (int)mp.x + 6, (int)mp.y, YELLOW);
		DrawLine((int)mp.x, (int)mp.y - 6, (int)mp.x, (int)mp.y + 6, YELLOW);
		// Also label the Box2D coords
		DrawText(TextFormat("Mouse B2D: (%.2f, %.2f)", p.x, p.y), 10, SCREEN_HEIGHT - 55, 16, YELLOW);
	}
	
	// Draw all Box2D shapes for debugging
	for(b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		for(b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			switch(f->GetType())
			{
				case b2Shape::e_circle:
				{
					b2CircleShape* shape = (b2CircleShape*)f->GetShape();
					b2Vec2 pos = f->GetBody()->GetPosition();
					// Convert Box2D coords to screen coords
					DrawCircle((int)(pos.x * METERS_TO_PIXELS), 
					          (int)(SCREEN_HEIGHT - pos.y * METERS_TO_PIXELS), 
					          shape->m_radius * METERS_TO_PIXELS, 
					          Color{255, 0, 0, 128});
				}
				break;
				
				case b2Shape::e_polygon:
				{
					b2PolygonShape* shape = (b2PolygonShape*)f->GetShape();
					int32 count = shape->m_count;
					b2Vec2 prev, v;
					
					for(int32 i = 0; i < count; ++i)
					{
						v = b->GetWorldPoint(shape->m_vertices[i]);
						if(i > 0)
							DrawLine((int)(prev.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - prev.y * METERS_TO_PIXELS),
							        (int)(v.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - v.y * METERS_TO_PIXELS), 
							        Color{255, 0, 0, 255});
						prev = v;
					}
					
					v = b->GetWorldPoint(shape->m_vertices[0]);
					DrawLine((int)(prev.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - prev.y * METERS_TO_PIXELS),
					        (int)(v.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - v.y * METERS_TO_PIXELS), 
					        Color{255, 0, 0, 255});
				}
				break;
				
				case b2Shape::e_chain:
				{
					b2ChainShape* shape = (b2ChainShape*)f->GetShape();
					int32 count = shape->m_count;
					b2Vec2 prev, v;
					
					for(int32 i = 0; i < count; ++i)
					{
						v = b->GetWorldPoint(shape->m_vertices[i]);
						if(i > 0)
							DrawLine((int)(prev.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - prev.y * METERS_TO_PIXELS),
							        (int)(v.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - v.y * METERS_TO_PIXELS), 
							        Color{0, 255, 0, 255});
						prev = v;
					}
				}
				break;
			}
		}
	}
	
	// Draw mouse joint
	if(mouse_joint)
	{
		b2Vec2 p1 = mouse_joint->GetAnchorA();
		b2Vec2 p2 = mouse_joint->GetAnchorB();
		DrawLine((int)(p1.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - p1.y * METERS_TO_PIXELS),
		        (int)(p2.x * METERS_TO_PIXELS), (int)(SCREEN_HEIGHT - p2.y * METERS_TO_PIXELS), 
		        Color{200, 200, 200, 255});
	}

	return UPDATE_CONTINUE;
}

bool ModulePhysics::CleanUp()
{
	LOG("Destroying physics world");

	// Delete the whole physics world!
	if(world != nullptr)
	{
		delete world;
		world = nullptr;
	}

	return true;
}

PhysBody* ModulePhysics::CreateCircle(int x, int y, int radius, b2BodyType type)
{
	PhysBody* pbody = new PhysBody();
	
	b2BodyDef body;
	body.type = type;
	// Convert screen coords to Box2D coords
	body.position.Set(x * PIXELS_TO_METERS, (SCREEN_HEIGHT - y) * PIXELS_TO_METERS);
	
	// Enable CCD (Continuous Collision Detection) to prevent tunneling
	body.bullet = true;
	
	b2Body* b = world->CreateBody(&body);
	
	b2CircleShape shape;
	shape.m_radius = radius * PIXELS_TO_METERS;
	
	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.density = 1.0f;
	fixture.restitution = 0.6f; // Bounciness for pinball
	fixture.friction = 0.3f;     // Surface friction
	
	b->CreateFixture(&fixture);
	
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = radius * 2;
	
	return pbody;
}

PhysBody* ModulePhysics::CreateRectangle(int x, int y, int width, int height, b2BodyType type)
{
	PhysBody* pbody = new PhysBody();
	
	b2BodyDef body;
	body.type = type;
	// Convert screen coords to Box2D coords
	body.position.Set(x * PIXELS_TO_METERS, (SCREEN_HEIGHT - y) * PIXELS_TO_METERS);
	
	b2Body* b = world->CreateBody(&body);
	
	b2PolygonShape box;
	box.SetAsBox(width * 0.5f * PIXELS_TO_METERS, height * 0.5f * PIXELS_TO_METERS);
	
	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 1.0f;
	
	b->CreateFixture(&fixture);
	
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = width;
	pbody->height = height;
	
	return pbody;
}

PhysBody* ModulePhysics::CreateRectangleSensor(int x, int y, int width, int height)
{
	PhysBody* pbody = new PhysBody();
	
	b2BodyDef body;
	body.type = b2_staticBody;
	// Convert screen coords to Box2D coords
	body.position.Set(x * PIXELS_TO_METERS, (SCREEN_HEIGHT - y) * PIXELS_TO_METERS);
	
	b2Body* b = world->CreateBody(&body);
	
	b2PolygonShape box;
	box.SetAsBox(width * 0.5f * PIXELS_TO_METERS, height * 0.5f * PIXELS_TO_METERS);
	
	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 1.0f;
	fixture.isSensor = true;
	
	b->CreateFixture(&fixture);
	
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = width;
	pbody->height = height;
	
	return pbody;
}

PhysBody* ModulePhysics::CreateChain(int x, int y, const int* points, int size, b2BodyType type)
{
	PhysBody* pbody = new PhysBody();
	
	b2BodyDef body;
	body.type = type;
	// Convert screen coords to Box2D coords
	body.position.Set(x * PIXELS_TO_METERS, (SCREEN_HEIGHT - y) * PIXELS_TO_METERS);
	
	b2Body* b = world->CreateBody(&body);
	
	b2ChainShape shape;
	b2Vec2* p = new b2Vec2[size / 2];
	
	for(uint i = 0; i < size / 2; ++i)
	{
		p[i].x = points[i * 2 + 0] * PIXELS_TO_METERS;
		// Flip Y coordinates for chain points
		p[i].y = -points[i * 2 + 1] * PIXELS_TO_METERS;
	}
	
	// Use CreateChain (open) instead of CreateLoop (closed) for pinball boundaries
	shape.CreateChain(p, size / 2, b2Vec2(0,0), b2Vec2(0,0));
	
	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.friction = 0.3f;      // Add friction to chain
	fixture.restitution = 0.4f;   // Add some bounce to walls
	
	b->CreateFixture(&fixture);
	
	delete[] p;
	
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = 0;
	
	return pbody;
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	PhysBody* physA = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	PhysBody* physB = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
	
	if(physA && physA->listener != nullptr)
		((Module*)physA->listener)->OnCollision(physA, physB);
	
	if(physB && physB->listener != nullptr)
		((Module*)physB->listener)->OnCollision(physB, physA);
}

void ModulePhysics::EndContact(b2Contact* contact)
{
	// Optional: handle end of contact
}
