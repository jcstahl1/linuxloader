#include "bezel.h"
#include "../config/config.h"

#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>
#include <glad/gl.h>
#include <stdio.h>

extern int drawableW;
extern int drawableH;

static SDL_Surface *bezelSurface = NULL;
static GLuint bezelTexture = 0;
static int bezelInitialized = 0;

static int loadBezelTexture(const char *filePath)
{
    if (filePath == NULL || filePath[0] == '\0')
    {
        fprintf(stderr, "Bezel overlay enabled, but BEZEL_PATH is empty\n");
        return 0;
    }

    SDL_Surface *surface = IMG_Load(filePath);
    if (!surface)
    {
        fprintf(stderr, "Failed to load bezel PNG: %s\n", SDL_GetError());
        return 0;
    }

    bezelSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	
    SDL_DestroySurface(surface);

    if (!bezelSurface)
    {
        fprintf(stderr, "Failed to convert bezel PNG: %s\n", SDL_GetError());
        return 0;
    }

    glad_glGenTextures(1, &bezelTexture);
    glad_glBindTexture(GL_TEXTURE_2D, bezelTexture);

    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glad_glTexImage2D(
        GL_TEXTURE_2D,
		0,
		GL_RGBA,
		bezelSurface->w,
		bezelSurface->h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		bezelSurface->pixels
    );

    glad_glBindTexture(GL_TEXTURE_2D, 0);

    bezelInitialized = 1;

    printf("Loaded bezel image: %s (%dx%d)\n", filePath, bezelSurface->w, bezelSurface->h);
    return 1;
}

void initBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;

    loadBezelTexture(cfg->bezelPath);
}

void drawBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;

    if (!bezelInitialized || bezelTexture == 0)
        return;

    if (drawableW <= 0 || drawableH <= 0)
        return;

    GLboolean blendWasEnabled = glad_glIsEnabled(GL_BLEND);
    GLboolean textureWasEnabled = glad_glIsEnabled(GL_TEXTURE_2D);
    GLboolean depthWasEnabled = glad_glIsEnabled(GL_DEPTH_TEST);
    GLboolean scissorWasEnabled = glad_glIsEnabled(GL_SCISSOR_TEST);
    GLboolean cullWasEnabled = glad_glIsEnabled(GL_CULL_FACE);

    GLint oldViewport[4];
    GLint oldTexture = 0;
    GLint oldProgram = 0;
    GLint oldMatrixMode = 0;
    GLint oldActiveTexture = 0;

    glad_glGetIntegerv(GL_VIEWPORT, oldViewport);
    glad_glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);

    glad_glActiveTexture(GL_TEXTURE0);
    glad_glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);

    glad_glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glad_glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);

    glad_glUseProgram(0);

    glad_glViewport(0, 0, drawableW, drawableH);

    glad_glDisable(GL_DEPTH_TEST);
    glad_glDisable(GL_SCISSOR_TEST);
    glad_glDisable(GL_CULL_FACE);

    glad_glEnable(GL_BLEND);
    glad_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glad_glEnable(GL_TEXTURE_2D);
    glad_glBindTexture(GL_TEXTURE_2D, bezelTexture);
    glad_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glad_glMatrixMode(GL_PROJECTION);
    glad_glPushMatrix();
    glad_glLoadIdentity();
    glad_glOrtho(0, drawableW, drawableH, 0, -1, 1);

    glad_glMatrixMode(GL_MODELVIEW);
    glad_glPushMatrix();
    glad_glLoadIdentity();

    glad_glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glad_glBegin(GL_QUADS);

    glad_glTexCoord2f(0.0f, 0.0f);
    glad_glVertex2f(0.0f, 0.0f);

    glad_glTexCoord2f(1.0f, 0.0f);
    glad_glVertex2f((GLfloat)drawableW, 0.0f);

    glad_glTexCoord2f(1.0f, 1.0f);
    glad_glVertex2f((GLfloat)drawableW, (GLfloat)drawableH);

    glad_glTexCoord2f(0.0f, 1.0f);
    glad_glVertex2f(0.0f, (GLfloat)drawableH);

    glad_glEnd();

    glad_glPopMatrix();

    glad_glMatrixMode(GL_PROJECTION);
    glad_glPopMatrix();

    glad_glMatrixMode(oldMatrixMode);

    glad_glBindTexture(GL_TEXTURE_2D, oldTexture);
    glad_glActiveTexture((GLenum)oldActiveTexture);

    glad_glUseProgram((GLuint)oldProgram);
    glad_glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    if (!textureWasEnabled)
        glad_glDisable(GL_TEXTURE_2D);

    if (!blendWasEnabled)
        glad_glDisable(GL_BLEND);

    if (depthWasEnabled)
        glad_glEnable(GL_DEPTH_TEST);

    if (scissorWasEnabled)
        glad_glEnable(GL_SCISSOR_TEST);

    if (cullWasEnabled)
        glad_glEnable(GL_CULL_FACE);
}

void shutdownBezelOverlay(void)
{
    if (bezelTexture != 0)
    {
        glad_glDeleteTextures(1, &bezelTexture);
        bezelTexture = 0;
    }

    if (bezelSurface)
    {
        SDL_DestroySurface(bezelSurface);
        bezelSurface = NULL;
    }

    bezelInitialized = 0;
}