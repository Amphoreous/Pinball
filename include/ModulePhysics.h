#pragma once
#include "Module.h"
#include "Globals.h"
#include "box2d/box2d.h"
#include <vector>

#define GRAVITY_X 0.0f
#define GRAVITY_Y -10.0f

class PhysBody;

class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	PhysBody* CreateCircle(int x, int y, int radius, b2BodyType type);
	PhysBody* CreateRectangle(int x, int y, int width, int height, b2BodyType type);
	PhysBody* CreateRectangleSensor(int x, int y, int width, int height);
	PhysBody* CreateChain(int x, int y, int* points, int point_count, b2BodyType type);
	PhysBody* CreatePolygonLoop(int x, int y, int* points, int point_count, b2BodyType type, float angle_rad = 0.0f);


	b2RevoluteJoint* CreateFlipper(int x, int y, int width, int height, bool isLeft, PhysBody** flipperBody);

	void BeginContact(b2Contact* contact) override;
	b2World* GetWorld();

private:
	bool debug = false;
	b2World* world = nullptr;
	b2MouseJoint* mouseJoint = nullptr;
	b2Body* ground = nullptr;
	b2Body* mouseBody = nullptr;

	std::vector<PhysBody*> bodiesToDestroy;
};