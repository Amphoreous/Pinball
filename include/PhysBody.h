#pragma once

#include "box2d/box2d.h"

class PhysBody
{
public:
	PhysBody();
	~PhysBody();
	
	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;
	
	int width, height;
	b2Body* body = nullptr;
	void* listener = nullptr; // Module that will listen to collisions
};
