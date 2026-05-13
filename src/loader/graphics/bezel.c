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

    /*
        PNG is loaded here, but not drawn yet.
        Next step will add the actual textured quad rendering.
    */
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