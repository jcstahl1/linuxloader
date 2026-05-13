#include "bezel.h"
#include "../config/config.h"
#include <glad/gl.h>

extern int drawableW;
extern int drawableH;

void initBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;
}

void drawBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;

    if (drawableW <= 0 || drawableH <= 0)
        return;

    GLfloat oldClearColor[4];
    GLboolean scissorWasEnabled = glad_glIsEnabled(GL_SCISSOR_TEST);

    glad_glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);

    glad_glEnable(GL_SCISSOR_TEST);

    // Temporary visible test overlay: red side bars.
    // This proves BEZEL_ENABLED is being respected before we add PNG loading.
    int barWidth = drawableW / 12;

    glad_glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

    glad_glScissor(0, 0, barWidth, drawableH);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    glad_glScissor(drawableW - barWidth, 0, barWidth, drawableH);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    if (!scissorWasEnabled)
        glad_glDisable(GL_SCISSOR_TEST);

    glad_glClearColor(
        oldClearColor[0],
        oldClearColor[1],
        oldClearColor[2],
        oldClearColor[3]
    );
}

void shutdownBezelOverlay(void)
{
}