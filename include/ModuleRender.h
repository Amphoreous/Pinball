#pragma once
#include "Module.h"
#include "Globals.h"

#include <limits.h>

class ModuleRender : public Module
{
public:
	ModuleRender(Application* app, bool start_enabled = true);
	~ModuleRender();

	bool Init();
	update_status PreUpdate();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

    void SetBackgroundColor(Color color);
	bool Draw(Texture2D texture, int x, int y, const Rectangle* section = NULL, double angle = 0, int pivot_x = 0, int pivot_y = 0) const;
    bool DrawText(const char* text, int x, int y, Font font, int spacing, Color tint) const;
	bool DrawCircle(int x, int y, int radius, Color color) const;
	bool DrawLine(int x1, int y1, int x2, int y2, Color color) const;

public:

	Color background;
    Rectangle camera;
};