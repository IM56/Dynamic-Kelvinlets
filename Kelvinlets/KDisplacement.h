#pragma once
#include "CommonHeader.h"

struct KDisplacement
{
	v3 displacement;
	v3 auxDisplacement1;
	v3 auxDisplacement2;

	KDisplacement();	// Default constructor amounts to zero-initialization
};