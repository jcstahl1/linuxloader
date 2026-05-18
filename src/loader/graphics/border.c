#include <glad/gl.h>

#include "border.h"

void drawBorderWithOffset(int x, int y, int width, int height, float borderPercentage, float offsetPercentage, GLfloat *color)
{
    // Border thickness based on the percentage of the viewport width
    int borderWidth = (int)(width * borderPercentage);
    int borderHeight = borderWidth;

    // Offset based on the percentage of the viewport width
    int offsetX = (int)(width * offsetPercentage);
    int offsetY = offsetX;

    int left = x;
    int right = x + width;
    int top = y;
    int bottom = y + height;

    // Set the clear color based on the color parameter
    glad_glClearColor(color[0], color[1], color[2], color[3]);

    // Left side
    glad_glScissor(left + offsetX,
                   top + offsetY,
                   borderWidth,
                   height - 2 * offsetY);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Right side
    glad_glScissor(right - borderWidth - offsetX,
                   top + offsetY,
                   borderWidth,
                   height - 2 * offsetY);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Top side
    glad_glScissor(left + offsetX,
                   top + offsetY,
                   width - 2 * offsetX,
                   borderHeight);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Bottom side
    glad_glScissor(left + offsetX,
                   bottom - borderHeight - offsetY,
                   width - 2 * offsetX,
                   borderHeight);
    glad_glClear(GL_COLOR_BUFFER_BIT);
}

void drawGameBorder(int x, int y, int width, int height, float whiteBorderPercentage, float blackBorderPercentage)
{
    // Store the old clear colour
    GLfloat originalClearColour[4];
    glad_glGetFloatv(GL_COLOR_CLEAR_VALUE, originalClearColour);

    GLfloat blackColour[4] = {0.0, 0.0, 0.0, 0.0};
    GLfloat whiteColour[4] = {1.0, 1.0, 1.0, 0.0};

    glad_glEnable(GL_SCISSOR_TEST);

    drawBorderWithOffset(x, y, width, height, whiteBorderPercentage, blackBorderPercentage, whiteColour);
    drawBorderWithOffset(x, y, width, height, blackBorderPercentage, 0, blackColour);

    glad_glDisable(GL_SCISSOR_TEST);

    glad_glClearColor(originalClearColour[0], originalClearColour[1], originalClearColour[2], originalClearColour[3]);
}