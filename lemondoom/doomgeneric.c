#include "doomgeneric.h"

uint32_t* DG_ScreenBuffer = 0;

int DOOMGENERIC_RESX = 960;
int DOOMGENERIC_RESY = 600;

void dg_Create()
{
	DG_Init();

	if(!DG_ScreenBuffer)
		DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
}

