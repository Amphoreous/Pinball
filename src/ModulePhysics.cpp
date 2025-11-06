#include "Globals.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "PhysBody.h"
#include "raylib.h"

// Funci�n helper para filtrar v�rtices muy cercanos
std::vector<b2Vec2> FilterCloseVertices(b2Vec2* vertices, int count, float minDistance = 0.05f)
{
	std::vector<b2Vec2> filteredVertices;

	if (count <= 0) return filteredVertices;

	// Siempre agregar el primer v�rtice
	filteredVertices.push_back(vertices[0]);

	// Filtrar v�rtices subsecuentes que est�n muy cerca
	for (int i = 1; i < count; ++i)
	{
		bool tooClose = false;

		// Verificar distancia con todos los v�rtices ya agregados
		for (const auto& existing : filteredVertices)
		{
			float distSq = b2DistanceSquared(vertices[i], existing);
			if (distSq < minDistance * minDistance)
			{
				tooClose = true;
				break;
			}
		}

		if (!tooClose)
		{
			filteredVertices.push_back(vertices[i]);
		}
	}

	LOG("Filtered vertices: %d -> %d", count, (int)filteredVertices.size());
	return filteredVertices;
}

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = NULL;
	mouseJoint = NULL;
	debug = false;
	world = nullptr;
	mouse_joint = nullptr;
	ground = nullptr;
}

ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
	LOG("Creating Physics 2D environment");

	b2Vec2 gravity(0.0f, -10.0f);
	world = new b2World(gravity);

	if (!world)
	{
		LOG("ERROR: Failed to create Box2D world");
		return false;
	}

	world->SetContactListener(this);

	b2BodyDef bd;
	bd.type = b2_staticBody;
	bd.position.Set(0.0f, 0.0f);
	ground = world->CreateBody(&bd);

	return true;
}

update_status ModulePhysics::PreUpdate()
{
	if (!world) return UPDATE_CONTINUE;

	float dt = GetFrameTime();

	// Validar dt para evitar problemas
	if (dt <= 0.0f || dt > 0.033f) // M�ximo 30fps
	{
		dt = 0.016f; // 60fps por defecto
	}

	// Step del mundo con par�metros m�s conservadores
	world->Step(dt, 6, 2);

	// Limpiar bodies marcados para destruir
	for (auto body : bodiesToDestroy)
	{
		if (body && body->body)
		{
			world->DestroyBody(body->body);
			body->body = nullptr;
		}
	}
	bodiesToDestroy.clear();

	return UPDATE_CONTINUE;
}

PhysBody* ModulePhysics::CreateCircle(int x, int y, int radius, b2BodyType type)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreateCircle");
		return nullptr;
	}

	if (radius <= 0)
	{
		LOG("ERROR: Invalid radius in CreateCircle: %d", radius);
		return nullptr;
	}

	b2BodyDef body;
	body.type = type;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateCircle: (%f, %f)", posX, posY);
		return nullptr;
	}

	body.position.Set(posX, posY);

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreateCircle");
		return nullptr;
	}

	b2CircleShape shape;
	float radiusM = PIXELS_TO_METERS * radius;
	if (radiusM <= 0.0f)
	{
		LOG("ERROR: Invalid radius in meters: %f", radiusM);
		world->DestroyBody(b);
		return nullptr;
	}

	shape.m_radius = radiusM;

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.density = 1.0f;
	fixture.restitution = 0.5f;
	fixture.friction = 0.3f;

	b2Fixture* f = b->CreateFixture(&fixture);
	if (!f)
	{
		LOG("ERROR: Failed to create fixture in CreateCircle");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = radius * 2;

	return pbody;
}

// Create a static circle sensor (non-colliding) at screen coordinates
PhysBody* ModulePhysics::CreateCircleSensor(int x, int y, int radius)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreateCircleSensor");
		return nullptr;
	}

	if (radius <= 0)
	{
		LOG("ERROR: Invalid radius in CreateCircleSensor: %d", radius);
		return nullptr;
	}

	b2BodyDef body;
	body.type = b2_staticBody;

	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y);
	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateCircleSensor: (%f, %f)", posX, posY);
		return nullptr;
	}
	body.position.Set(posX, posY);

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreateCircleSensor");
		return nullptr;
	}

	b2CircleShape shape;
	shape.m_radius = PIXELS_TO_METERS * radius;

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.isSensor = true;
	if (!b->CreateFixture(&fixture))
	{
		LOG("ERROR: Failed to create fixture in CreateCircleSensor");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = radius * 2;

	return pbody;
}

PhysBody* ModulePhysics::CreateRectangle(int x, int y, int width, int height, b2BodyType type)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreateRectangle");
		return nullptr;
	}

	if (width <= 0 || height <= 0)
	{
		LOG("ERROR: Invalid dimensions in CreateRectangle: %dx%d", width, height);
		return nullptr;
	}

	b2BodyDef body;
	body.type = type;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateRectangle: (%f, %f)", posX, posY);
		return nullptr;
	}

	body.position.Set(posX, posY);

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreateRectangle");
		return nullptr;
	}

	b2PolygonShape box;
	float halfWidth = PIXELS_TO_METERS * width * 0.5f;
	float halfHeight = PIXELS_TO_METERS * height * 0.5f;

	if (halfWidth <= 0.0f || halfHeight <= 0.0f)
	{
		LOG("ERROR: Invalid box dimensions: %fx%f", halfWidth, halfHeight);
		world->DestroyBody(b);
		return nullptr;
	}

	box.SetAsBox(halfWidth, halfHeight);

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 1.0f;
	fixture.restitution = 0.5f;
	fixture.friction = 0.3f;

	b2Fixture* f = b->CreateFixture(&fixture);
	if (!f)
	{
		LOG("ERROR: Failed to create fixture in CreateRectangle");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = width;
	pbody->height = height;

	return pbody;
}

PhysBody* ModulePhysics::CreateRectangleSensor(int x, int y, int width, int height)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreateRectangleSensor");
		return nullptr;
	}

	if (width <= 0 || height <= 0)
	{
		LOG("ERROR: Invalid dimensions in CreateRectangleSensor: %dx%d", width, height);
		return nullptr;
	}

	b2BodyDef body;
	body.type = b2_staticBody;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateRectangleSensor: (%f, %f)", posX, posY);
		return nullptr;
	}

	body.position.Set(posX, posY);

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreateRectangleSensor");
		return nullptr;
	}

	b2PolygonShape box;
	float halfWidth = PIXELS_TO_METERS * width * 0.5f;
	float halfHeight = PIXELS_TO_METERS * height * 0.5f;

	if (halfWidth <= 0.0f || halfHeight <= 0.0f)
	{
		LOG("ERROR: Invalid sensor box dimensions: %fx%f", halfWidth, halfHeight);
		world->DestroyBody(b);
		return nullptr;
	}

	box.SetAsBox(halfWidth, halfHeight);

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = 1.0f;
	fixture.isSensor = true;

	b2Fixture* f = b->CreateFixture(&fixture);
	if (!f)
	{
		LOG("ERROR: Failed to create fixture in CreateRectangleSensor");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = width;
	pbody->height = height;

	return pbody;
}

PhysBody* ModulePhysics::CreateChain(int x, int y, int* points, int point_count, b2BodyType type)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreateChain");
		return nullptr;
	}

	if (!points || point_count < 4) // M�nimo 2 puntos (4 valores)
	{
		LOG("ERROR: Invalid points or point_count in CreateChain: %d", point_count);
		return nullptr;
	}

	b2BodyDef body;
	body.type = type;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateChain: (%f, %f)", posX, posY);
		return nullptr;
	}

	body.position.Set(posX, posY);

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreateChain");
		return nullptr;
	}

	int num_points = point_count / 2;
	b2Vec2* p = new b2Vec2[num_points];

	// Convertir puntos a metros, invirtiendo Y para cada punto
	for (uint i = 0; i < num_points; ++i)
	{
		float px = PIXELS_TO_METERS * points[i * 2 + 0];
		float py = PIXELS_TO_METERS * -points[i * 2 + 1]; // **CLAVE: Invertir Y de los puntos**

		if (!b2Vec2(px, py).IsValid())
		{
			LOG("ERROR: Invalid point %d in CreateChain: (%f, %f)", i, px, py);
			delete[] p;
			world->DestroyBody(b);
			return nullptr;
		}

		p[i].Set(px, py);
	}

	std::vector<b2Vec2> filteredVertices = FilterCloseVertices(p, num_points);

	delete[] p; // Liberar el array original

	if (filteredVertices.size() < 2)
	{
		LOG("ERROR: Not enough valid vertices after filtering in CreateChain: %d", (int)filteredVertices.size());
		world->DestroyBody(b);
		return nullptr;
	}

	b2ChainShape chain;

	// Calculamos los v�rtices fantasma prev/next
	b2Vec2 prevVertex = filteredVertices[0] + (filteredVertices[0] - filteredVertices[1]);
	b2Vec2 nextVertex = filteredVertices.back() + (filteredVertices.back() - filteredVertices[filteredVertices.size() - 2]);

	// Usar los v�rtices filtrados y los v�rtices fantasma
	chain.CreateChain(filteredVertices.data(), (int)filteredVertices.size(), prevVertex, nextVertex);

	b2FixtureDef fixture;
	fixture.shape = &chain;
	fixture.friction = 0.3f;

	b2Fixture* f = b->CreateFixture(&fixture);
	if (!f)
	{
		LOG("ERROR: Failed to create fixture in CreateChain");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = 0;

	return pbody;
}

PhysBody* ModulePhysics::CreatePolygonLoop(int x, int y, int* points, int point_count, b2BodyType type, float angle_rad)
{
	if (!world)
	{
		LOG("ERROR: World is null in CreatePolygonLoop");
		return nullptr;
	}

	if (!points || point_count < 6) // M�nimo 3 puntos (6 valores)
	{
		LOG("ERROR: Invalid points or point_count in CreatePolygonLoop: %d", point_count);
		return nullptr;
	}

	int num_points = point_count / 2;
	if (num_points < 3)
	{
		LOG("CreatePolygonLoop: Error, polygon must have at least 3 vertices.");
		return nullptr;
	}

	b2BodyDef body;
	body.type = type;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreatePolygonLoop: (%f, %f)", posX, posY);
		return nullptr;
	}

	body.position.Set(posX, posY);
	body.angle = -angle_rad; // **CLAVE: Invertir �ngulo tambi�n**

	b2Body* b = world->CreateBody(&body);
	if (!b)
	{
		LOG("ERROR: Failed to create body in CreatePolygonLoop");
		return nullptr;
	}

	b2Vec2* p = new b2Vec2[num_points];

	// Convertir puntos a metros, invirtiendo Y para cada punto
	for (uint i = 0; i < num_points; ++i)
	{
		float px = PIXELS_TO_METERS * points[i * 2 + 0];
		float py = PIXELS_TO_METERS * -points[i * 2 + 1]; // **CLAVE: Invertir Y de los puntos**

		if (!b2Vec2(px, py).IsValid())
		{
			LOG("ERROR: Invalid point %d in CreatePolygonLoop: (%f, %f)", i, px, py);
			delete[] p;
			world->DestroyBody(b);
			return nullptr;
		}

		p[i].Set(px, py);
	}

	std::vector<b2Vec2> filteredVertices = FilterCloseVertices(p, num_points);

	delete[] p; // Liberar el array original

	if (filteredVertices.size() < 3)
	{
		LOG("ERROR: Not enough valid vertices after filtering in CreatePolygonLoop: %d", (int)filteredVertices.size());
		world->DestroyBody(b);
		return nullptr;
	}

	b2ChainShape chain;
	chain.CreateLoop(filteredVertices.data(), (int)filteredVertices.size());

	b2FixtureDef fixture;
	fixture.shape = &chain;
	fixture.density = 1.0f;
	fixture.restitution = 0.5f;
	fixture.friction = 0.3f;

	b2Fixture* f = b->CreateFixture(&fixture);
	if (!f)
	{
		LOG("ERROR: Failed to create fixture in CreatePolygonLoop");
		world->DestroyBody(b);
		return nullptr;
	}

	PhysBody* pbody = new PhysBody();
	pbody->body = b;
	b->GetUserData().pointer = (uintptr_t)pbody;
	pbody->width = pbody->height = 0;

	return pbody;
}

b2RevoluteJoint* ModulePhysics::CreateFlipper(int x, int y, int width, int height, bool isLeft, PhysBody** flipperBody)
{
	if (!world || !flipperBody)
	{
		LOG("ERROR: World is null or flipperBody is null in CreateFlipper");
		return nullptr;
	}

	if (width <= 0 || height <= 0)
	{
		LOG("ERROR: Invalid dimensions in CreateFlipper: %dx%d", width, height);
		return nullptr;
	}

	// Crear base est�tica
	b2BodyDef baseDef;
	baseDef.type = b2_staticBody;

	// **CLAVE: Invertir Y para que coincida con el sistema de Box2D**
	float posX = PIXELS_TO_METERS * x;
	float posY = PIXELS_TO_METERS * (SCREEN_HEIGHT - y); // Inversi�n de Y

	if (!b2Vec2(posX, posY).IsValid())
	{
		LOG("ERROR: Invalid position in CreateFlipper: (%f, %f)", posX, posY);
		return nullptr;
	}

	baseDef.position.Set(posX, posY);
	b2Body* base = world->CreateBody(&baseDef);

	if (!base)
	{
		LOG("ERROR: Failed to create base body in CreateFlipper");
		return nullptr;
	}

	b2CircleShape baseShape;
	baseShape.m_radius = PIXELS_TO_METERS * 5;
	b2FixtureDef baseFixture;
	baseFixture.shape = &baseShape;
	base->CreateFixture(&baseFixture);

	PhysBody* basePBody = new PhysBody();
	basePBody->body = base;
	base->GetUserData().pointer = (uintptr_t)basePBody;

	// Crear flipper din�mico
	b2BodyDef flipperDef;
	flipperDef.type = b2_dynamicBody;
	flipperDef.position.Set(posX, posY);
	b2Body* flipper = world->CreateBody(&flipperDef);

	if (!flipper)
	{
		LOG("ERROR: Failed to create flipper body in CreateFlipper");
		world->DestroyBody(base);
		delete basePBody;
		return nullptr;
	}

	b2PolygonShape flipperShape;
	float flipperWidth = PIXELS_TO_METERS * width;
	float flipperHeight = PIXELS_TO_METERS * height;

	if (flipperWidth <= 0.0f || flipperHeight <= 0.0f)
	{
		LOG("ERROR: Invalid flipper dimensions: %fx%f", flipperWidth, flipperHeight);
		world->DestroyBody(base);
		world->DestroyBody(flipper);
		delete basePBody;
		return nullptr;
	}

	b2Vec2 vertices[4];
	if (isLeft) {
		vertices[0].Set(0, -flipperHeight / 2);
		vertices[1].Set(flipperWidth * 0.8f, -flipperHeight / 4);
		vertices[2].Set(flipperWidth * 0.8f, flipperHeight / 4);
		vertices[3].Set(0, flipperHeight / 2);
	}
	else {
		vertices[0].Set(0, -flipperHeight / 2);
		vertices[1].Set(-flipperWidth * 0.8f, -flipperHeight / 4);
		vertices[2].Set(-flipperWidth * 0.8f, flipperHeight / 4);
		vertices[3].Set(0, flipperHeight / 2);
	}
	flipperShape.Set(vertices, 4);

	b2FixtureDef flipperFixture;
	flipperFixture.shape = &flipperShape;
	flipperFixture.density = 10.0f;
	flipperFixture.friction = 0.5f;
	flipper->CreateFixture(&flipperFixture);

	*flipperBody = new PhysBody();
	(*flipperBody)->body = flipper;
	flipper->GetUserData().pointer = (uintptr_t)(*flipperBody);
	(*flipperBody)->width = width;
	(*flipperBody)->height = height;

	// Crear joint
	b2RevoluteJointDef jointDef;
	jointDef.bodyA = base;
	jointDef.bodyB = flipper;
	jointDef.localAnchorA.Set(0, 0);

	if (isLeft) {
		jointDef.localAnchorB.Set(PIXELS_TO_METERS * -width * 0.3f, 0);
		// **CLAVE: Invertir �ngulos para el nuevo sistema de coordenadas**
		jointDef.lowerAngle = -0.15f * b2_pi;
		jointDef.upperAngle = 0.25f * b2_pi;
	}
	else {
		jointDef.localAnchorB.Set(PIXELS_TO_METERS * width * 0.3f, 0);
		// **CLAVE: Invertir �ngulos para el nuevo sistema de coordenadas**
		jointDef.lowerAngle = -0.25f * b2_pi;
		jointDef.upperAngle = 0.15f * b2_pi;
	}

	jointDef.enableLimit = true;
<<<<<<< HEAD
=======
	
	// Set angle limits - flippers rotate from down to up
	if(isLeft)
	{
		jointDef.lowerAngle = -5 * DEGTORAD;  // Resting position (slightly down)
		jointDef.upperAngle = 45 * DEGTORAD;  // Activated position (up)
	}
	else
	{
		jointDef.lowerAngle = -45 * DEGTORAD; // Activated position (up)  
		jointDef.upperAngle = 5 * DEGTORAD;   // Resting position (slightly down)
	}
	
	jointDef.maxMotorTorque = 2000.0f; // Very strong motor for snappy flippers
	jointDef.motorSpeed = 0.0f;
>>>>>>> 409bfea (WIP: TMX-driven objects, flipper fixes, DrawTextEx font usage)
	jointDef.enableMotor = true;
	jointDef.maxMotorTorque = 100.0f;

	b2RevoluteJoint* joint = (b2RevoluteJoint*)world->CreateJoint(&jointDef);

	if (!joint)
	{
		LOG("ERROR: Failed to create joint in CreateFlipper");
		world->DestroyBody(base);
		world->DestroyBody(flipper);
		delete basePBody;
		delete* flipperBody;
		*flipperBody = nullptr;
		return nullptr;
	}

	return joint;
}

update_status ModulePhysics::PostUpdate()
{
	if (IsKeyPressed(KEY_F1))
	{
		debug = !debug;
	}

	if (!debug || !world)
		return UPDATE_CONTINUE;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		int mouseX = GetMouseX();
		int mouseY = GetMouseY();

		// **CLAVE: Convertir coordenadas del mouse al sistema de Box2D**
		b2Vec2 mousePos(PIXELS_TO_METERS * mouseX, PIXELS_TO_METERS * (SCREEN_HEIGHT - mouseY));

		if (!mousePos.IsValid())
		{
			return UPDATE_CONTINUE;
		}

		b2AABB aabb;
		aabb.lowerBound = mousePos - b2Vec2(0.1f, 0.1f);
		aabb.upperBound = mousePos + b2Vec2(0.1f, 0.1f);

		class QueryCallback : public b2QueryCallback {
		public:
			b2Vec2 m_point;
			b2Body* m_body = nullptr;
			bool ReportFixture(b2Fixture* fixture) override {
				if (fixture->GetBody()->GetType() == b2_dynamicBody) {
					if (fixture->TestPoint(m_point)) {
						m_body = fixture->GetBody();
						return false;
					}
				}
				return true;
			}
		} callback;

		callback.m_point = mousePos;
		world->QueryAABB(&callback, aabb);

		if (callback.m_body && !mouseJoint && ground) {
			b2MouseJointDef def;
			def.bodyA = ground;
			def.bodyB = callback.m_body;
			def.target = mousePos;
			def.maxForce = 1000.0f * callback.m_body->GetMass();
			def.damping = 0.0f;
			def.stiffness = 50.0f;
			mouseJoint = (b2MouseJoint*)world->CreateJoint(&def);
			if (mouseJoint) {
				callback.m_body->SetAwake(true);
			}
		}
	}

	if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouseJoint) {
		int mouseX = GetMouseX();
		int mouseY = GetMouseY();
		// **CLAVE: Convertir coordenadas del mouse al sistema de Box2D**
		b2Vec2 target(PIXELS_TO_METERS * mouseX, PIXELS_TO_METERS * (SCREEN_HEIGHT - mouseY));

		if (target.IsValid())
		{
			mouseJoint->SetTarget(target);
			DrawLine(mouseX, mouseY,
				METERS_TO_PIXELS * mouseJoint->GetBodyB()->GetPosition().x,
				SCREEN_HEIGHT - (METERS_TO_PIXELS * mouseJoint->GetBodyB()->GetPosition().y), // **CLAVE: Convertir de vuelta**
				Color{ 0, 255, 0, 100 });
		}
	}

	if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouseJoint) {
		world->DestroyJoint(mouseJoint);
		mouseJoint = NULL;
	}

	// Dibujar shapes para debug
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			switch (f->GetType())
			{
			case b2Shape::e_circle:
			{
				b2CircleShape* shape = (b2CircleShape*)f->GetShape();
				b2Vec2 pos = f->GetBody()->GetPosition();
				// **CLAVE: Convertir coordenadas de Box2D de vuelta a pantalla**
				int x = METERS_TO_PIXELS * pos.x;
				int y = SCREEN_HEIGHT - (METERS_TO_PIXELS * pos.y);
				int radius = METERS_TO_PIXELS * shape->m_radius;
				DrawCircleLines(x, y, radius, WHITE);
			}
			break;

			case b2Shape::e_polygon:
			{
				b2PolygonShape* shape = (b2PolygonShape*)f->GetShape();
				int vertexCount = shape->m_count;
				assert(vertexCount <= b2_maxPolygonVertices);
				b2Vec2 vertices[b2_maxPolygonVertices];

				for (int i = 0; i < vertexCount; ++i)
				{
					vertices[i] = b->GetWorldPoint(shape->m_vertices[i]);
				}

				for (int i = 0; i < vertexCount; ++i)
				{
					b2Vec2 p1 = vertices[i];
					b2Vec2 p2 = vertices[(i + 1) % vertexCount];
					// **CLAVE: Convertir coordenadas de Box2D de vuelta a pantalla**
					int x1 = METERS_TO_PIXELS * p1.x;
					int y1 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p1.y);
					int x2 = METERS_TO_PIXELS * p2.x;
					int y2 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p2.y);
					DrawLine(x1, y1, x2, y2, WHITE);
				}
			}
			break;

			case b2Shape::e_chain:
			{
				b2ChainShape* shape = (b2ChainShape*)f->GetShape();
				int vertexCount = shape->m_count;
				b2Vec2* vertices = shape->m_vertices;

				b2Vec2 p1, p2;
				for (int i = 0; i < vertexCount - 1; ++i)
				{
					p1 = b->GetWorldPoint(vertices[i]);
					p2 = b->GetWorldPoint(vertices[i + 1]);
					// **CLAVE: Convertir coordenadas de Box2D de vuelta a pantalla**
					int x1 = METERS_TO_PIXELS * p1.x;
					int y1 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p1.y);
					int x2 = METERS_TO_PIXELS * p2.x;
					int y2 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p2.y);
					DrawLine(x1, y1, x2, y2, WHITE);
				}

				if (shape->m_prevVertex.LengthSquared() > 0 && shape->m_nextVertex.LengthSquared() > 0)
				{
					b2Vec2 first = b->GetWorldPoint(vertices[0]);
					b2Vec2 last = b->GetWorldPoint(vertices[vertexCount - 1]);
					b2Vec2 prev = b->GetWorldPoint(shape->m_prevVertex);
					b2Vec2 next = b->GetWorldPoint(shape->m_nextVertex);

					if (b2DistanceSquared(first, next) < 0.001f * 0.001f && b2DistanceSquared(last, prev) < 0.001f * 0.001f)
					{
						// **CLAVE: Convertir coordenadas de Box2D de vuelta a pantalla**
						int x1 = METERS_TO_PIXELS * last.x;
						int y1 = SCREEN_HEIGHT - (METERS_TO_PIXELS * last.y);
						int x2 = METERS_TO_PIXELS * first.x;
						int y2 = SCREEN_HEIGHT - (METERS_TO_PIXELS * first.y);
						DrawLine(x1, y1, x2, y2, WHITE);
					}
				}
			}
			break;

			case b2Shape::e_edge:
			{
				b2EdgeShape* shape = (b2EdgeShape*)f->GetShape();
				b2Vec2 p1, p2;
				p1 = b->GetWorldPoint(shape->m_vertex1);
				p2 = b->GetWorldPoint(shape->m_vertex2);
				// **CLAVE: Convertir coordenadas de Box2D de vuelta a pantalla**
				int x1 = METERS_TO_PIXELS * p1.x;
				int y1 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p1.y);
				int x2 = METERS_TO_PIXELS * p2.x;
				int y2 = SCREEN_HEIGHT - (METERS_TO_PIXELS * p2.y);
				DrawLine(x1, y1, x2, y2, WHITE);
			}
			break;
			}
		}
	}

	return UPDATE_CONTINUE;
}

bool ModulePhysics::CleanUp()
{
	LOG("Destroying physics world");

	if (mouseJoint)
	{
		world->DestroyJoint(mouseJoint);
		mouseJoint = NULL;
	}

	if (world)
	{
		b2Body* body = world->GetBodyList();
		while (body)
		{
			b2Body* nextBody = body->GetNext();

			if (body->GetUserData().pointer)
			{
				PhysBody* pbody = (PhysBody*)body->GetUserData().pointer;
				delete pbody;
				body->GetUserData().pointer = 0;
			}

			world->DestroyBody(body);
			body = nextBody;
		}
	}

	if (world)
	{
		delete world;
		world = NULL;
	}

	return true;
}

b2World* ModulePhysics::GetWorld()
{
	return world;
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	if (!contact) return;

	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	if (!fixtureA || !fixtureB) return;

	b2Body* bodyA = fixtureA->GetBody();
	b2Body* bodyB = fixtureB->GetBody();

	if (!bodyA || !bodyB) return;

	PhysBody* physA = (PhysBody*)bodyA->GetUserData().pointer;
	PhysBody* physB = (PhysBody*)bodyB->GetUserData().pointer;

	if (physA && physA->listener != NULL)
		((Module*)physA->listener)->OnCollision(physA, physB);

	if (physB && physB->listener != NULL)
		((Module*)physB->listener)->OnCollision(physB, physA);
}
