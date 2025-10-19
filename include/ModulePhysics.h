#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d\box2d.h"

class PhysBody;

// Module --------------------------------------
class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	// Factory methods to create physics bodies
	PhysBody* CreateCircle(int x, int y, int radius, b2BodyType type = b2_dynamicBody);
	PhysBody* CreateRectangle(int x, int y, int width, int height, b2BodyType type = b2_staticBody);
	PhysBody* CreateRectangleSensor(int x, int y, int width, int height);
	PhysBody* CreateChain(int x, int y, const int* points, int size, b2BodyType type = b2_staticBody);
	
	// Collision callbacks from b2ContactListener
	void BeginContact(b2Contact* contact) override;
	void EndContact(b2Contact* contact) override;
	
	// Access to Box2D world
	b2World* GetWorld() { return world; }

private:

	bool debug;
	b2World* world = nullptr;
	b2MouseJoint* mouse_joint = nullptr;
	b2Body* ground = nullptr;
	
};
