#pragma once
#include "CommonHeader.h"

struct Kelvinlet
{
	v3 loadCentre;
	float epsilon;
	v3 forceParams;
	float startTime;
	float age;
	float lifespan;
	int type;	// 0 = null Kelvinlet, 1 = Impulse, 2 = Pinch, 3 = Scale

	Kelvinlet();	// Default initialization is a null Kelvinlet
};
