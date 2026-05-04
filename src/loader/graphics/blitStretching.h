#pragma once
#include <stdbool.h>

typedef struct
{
    int W;
    int H;
    int X;
    int Y;
    float gameScale;
} Dest;

extern Dest dest;
extern int drawableW;
extern int drawableH;
extern int blitWidth;
extern int blitHeight;
extern int fboInitialized;

void initBlitting();
void blitSetWidthandHeightSize();
int blitInitializeFbo();
void blitStretch();
