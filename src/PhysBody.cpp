#include "Globals.h"
#include "PhysBody.h"

PhysBody::PhysBody()
{
	width = 0;
	height = 0;
}

PhysBody::~PhysBody()
{
}

void PhysBody::GetPosition(int& x, int& y) const
{
	b2Vec2 pos = body->GetPosition();
	x = METERS_TO_PIXELS * pos.x;
	y = SCREEN_HEIGHT - (METERS_TO_PIXELS * pos.y);
}

float PhysBody::GetRotation() const
{
	return body->GetAngle() * RADTODEG;
}

bool PhysBody::Contains(int x, int y) const
{
	// Convert screen coords to Box2D coords
	b2Vec2 p(x * PIXELS_TO_METERS, (SCREEN_HEIGHT - y) * PIXELS_TO_METERS);
	
	const b2Fixture* fixture = body->GetFixtureList();
	
	while(fixture != nullptr)
	{
		if(fixture->TestPoint(p))
			return true;
		fixture = fixture->GetNext();
	}
	
	return false;
}

int PhysBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
	int ret = -1;
	
	b2RayCastInput input;
	b2RayCastOutput output;
	
	// Convert screen coords to Box2D coords
	input.p1.Set(x1 * PIXELS_TO_METERS, (SCREEN_HEIGHT - y1) * PIXELS_TO_METERS);
	input.p2.Set(x2 * PIXELS_TO_METERS, (SCREEN_HEIGHT - y2) * PIXELS_TO_METERS);
	input.maxFraction = 1.0f;
	
	const b2Fixture* fixture = body->GetFixtureList();
	
	while(fixture != nullptr)
	{
		if(fixture->RayCast(&output, input, 0))
		{
			float fx = (float)(x2 - x1);
			float fy = (float)(y2 - y1);
			float dist = sqrtf((fx*fx) + (fy*fy));
			
			normal_x = output.normal.x;
			normal_y = -output.normal.y; // Flip Y normal back to screen coords
			
			return (int)(output.fraction * dist);
		}
		fixture = fixture->GetNext();
	}
	
	return ret;
}
