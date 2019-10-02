#pragma once

//================================================================================================
// Originally Based on debug_draw DX11 sample code by Guilherme R. Lampert (Public Domain)
// The framework has been extended and modified by D.Moore
//================================================================================================

#include "CommonHeader.h"

//================================================================================
// Time releated functions
//================================================================================

std::int64_t getTimeMicroseconds();

double getTimeSeconds();

// ========================================================
// Key/Mouse input + A simple 3D camera:
// ========================================================

struct Keys
{
	// For the first-person camera controls.
	bool wDown;
	bool sDown;
	bool aDown;
	bool dDown;
	// Flags:
	bool showLabels; // True if object labels are drawn. Toggle with the space bar.
	bool showGrid;   // True if the ground grid is drawn. Toggle with the return key.
};

struct Mouse
{
	enum { MaxDelta = 100 };
	int  deltaX;
	int  deltaY;
	int  lastPosX;
	int  lastPosY;
	bool leftButtonDown;
	bool rightButtonDown;
};

v2 getMousePosition();

struct Time
{
	float seconds;
	std::int64_t milliseconds;
};




// ========================================================
// Framework Application class inherit from this.
// ========================================================
class FrameworkApp
{
public:
	virtual void on_init(SystemsInterface& rSystems) = 0;
	virtual void on_update(SystemsInterface& rSystems) = 0;
	virtual void on_render(SystemsInterface& rSystems) = 0;
	virtual void on_resize(SystemsInterface& rSystems) = 0;
protected:
private:
};

// ========================================================
// Macro to define entry point.
// ========================================================

int framework_main(FrameworkApp& rApp, const char* pTitleString, HINSTANCE hInstance, int nCmdShow);

#define FRAMEWORK_IMPLEMENT_MAIN(app, appTitle) \
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)\
{\
	return framework_main(app, appTitle, hInstance, nCmdShow); \
}

//================================================================================
// Some Demo Functions for features.
//================================================================================
namespace DemoFeatures {

void editorHud(dd::ContextHandle ctx);

void drawGrid(dd::ContextHandle ctx);

void drawLabel(dd::ContextHandle ctx, ddVec3_In pos, const char * name);

void drawMiscObjects(dd::ContextHandle ctx);

void drawFrustum(dd::ContextHandle ctx);

void drawText(dd::ContextHandle ctx);
}

//================================================================================
// File loading
//================================================================================

// Loads an entire file into an allocated memory block.
memtype_t* load_file(const char* pstrName, u32& rLengthOut, const u32 kAlignment, const u32 kZeroPadding);

// Release a previously allocated block.
void release_loaded_file(memtype_t* ptr);


