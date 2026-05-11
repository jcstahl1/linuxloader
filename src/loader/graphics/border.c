#include <glad/gl.h>
#include "border.h"

static void drawBorderWithOffset(int width, int height, int borderPixels, int offsetPixels, GLfloat *color)
{
    if (borderPixels <= 0)
        return;

    if (offsetPixels < 0)
        offsetPixels = 0;

    int innerWidth = width - (2 * offsetPixels);
    int innerHeight = height - (2 * offsetPixels);

    if (innerWidth <= 0 || innerHeight <= 0)
        return;

    if (borderPixels * 2 > innerWidth)
        borderPixels = innerWidth / 2;

    if (borderPixels * 2 > innerHeight)
        borderPixels = innerHeight / 2;

    glad_glClearColor(color[0], color[1], color[2], color[3]);

    // Left
    glad_glScissor(offsetPixels, offsetPixels, borderPixels, innerHeight);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Right
    glad_glScissor(width - borderPixels - offsetPixels, offsetPixels, borderPixels, innerHeight);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Bottom
    glad_glScissor(offsetPixels, offsetPixels, innerWidth, borderPixels);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Top
    glad_glScissor(offsetPixels, height - borderPixels - offsetPixels, innerWidth, borderPixels);
    glad_glClear(GL_COLOR_BUFFER_BIT);
}

void drawGameBorder(int width, int height, int whiteBorderPixels, int blackBorderPixels)
{
    GLfloat originalClearColour[4];
    glad_glGetFloatv(GL_COLOR_CLEAR_VALUE, originalClearColour);

    GLfloat blackColour[4] = {0.0, 0.0, 0.0, 1.0};
    GLfloat whiteColour[4] = {1.0, 1.0, 1.0, 1.0};

    glad_glEnable(GL_SCISSOR_TEST);

    // Draw white border inset by the black border amount.
    drawBorderWithOffset(width, height, whiteBorderPixels, blackBorderPixels, whiteColour);

    // Draw black outer border.
    drawBorderWithOffset(width, height, blackBorderPixels, 0, blackColour);

    glad_glDisable(GL_SCISSOR_TEST);

    glad_glClearColor(
        originalClearColour[0],
        originalClearColour[1],
        originalClearColour[2],
        originalClearColour[3]
    );
}
