#include "bezel.h"
#include "../config/config.h"

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>
#include <glad/gl.h>
#include <stdio.h>

extern int drawableW;
extern int drawableH;
extern SDL_Window *getSDLWindow(void);

static SDL_Surface *bezelSurface = NULL;
static GLuint bezelTexture = 0;
static GLuint bezelProgram = 0;
static GLuint bezelVao = 0;
static GLuint bezelVbo = 0;
static GLint bezelProjectionLoc = -1;
static GLint bezelTextureLoc = -1;
static GLint bezelOpacityLoc = -1;
static int bezelInitialized = 0;

typedef struct
{
    GLint enabled;
    GLint size;
    GLint stride;
    GLint type;
    GLint normalized;
    GLint buffer;
    void *pointer;
} BezelAttribState;

static const char *bezelVertexShaderSource =
    "#version 120\n"
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "uniform mat4 u_projection;\n"
    "void main()\n"
    "{\n"
    "    v_uv = a_uv;\n"
    "    gl_Position = u_projection * vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

static const char *bezelFragmentShaderSource =
    "#version 120\n"
    "uniform sampler2D u_texture;\n"
    "uniform float u_opacity;\n"
    "varying vec2 v_uv;\n"
    "void main()\n"
    "{\n"
    "    vec4 color = texture2D(u_texture, v_uv);\n"
    "    color.a *= u_opacity;\n"
    "    gl_FragColor = color;\n"
    "}\n";

static void saveBezelAttribState(GLuint index, BezelAttribState *state)
{
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &state->enabled);
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &state->size);
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &state->stride);
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &state->type);
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &state->normalized);
    glad_glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &state->buffer);
    glad_glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &state->pointer);
}

static void restoreBezelAttribState(GLuint index, const BezelAttribState *state)
{
    glad_glBindBuffer(GL_ARRAY_BUFFER, (GLuint)state->buffer);
    glad_glVertexAttribPointer(index, state->size, (GLenum)state->type, (GLboolean)state->normalized, state->stride, state->pointer);

    if (state->enabled)
        glad_glEnableVertexAttribArray(index);
    else
        glad_glDisableVertexAttribArray(index);
}

static GLuint compileBezelShader(GLenum type, const char *source)
{
    GLuint shader = glad_glCreateShader(type);
    GLint success = 0;
    char infoLog[1024];

    glad_glShaderSource(shader, 1, &source, NULL);
    glad_glCompileShader(shader);
    glad_glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glad_glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Bezel shader compile failed: %s\n", infoLog);
        glad_glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint createBezelProgram(void)
{
    GLuint vertexShader = compileBezelShader(GL_VERTEX_SHADER, bezelVertexShaderSource);
    GLuint fragmentShader = compileBezelShader(GL_FRAGMENT_SHADER, bezelFragmentShaderSource);
    GLuint program;
    GLint success = 0;
    char infoLog[1024];

    if (!vertexShader || !fragmentShader)
    {
        if (vertexShader)
            glad_glDeleteShader(vertexShader);
        if (fragmentShader)
            glad_glDeleteShader(fragmentShader);
        return 0;
    }

    program = glad_glCreateProgram();

    glad_glAttachShader(program, vertexShader);
    glad_glAttachShader(program, fragmentShader);

    glad_glBindAttribLocation(program, 0, "a_pos");
    glad_glBindAttribLocation(program, 1, "a_uv");

    glad_glLinkProgram(program);
    glad_glGetProgramiv(program, GL_LINK_STATUS, &success);

    glad_glDeleteShader(vertexShader);
    glad_glDeleteShader(fragmentShader);

    if (!success)
    {
        glad_glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Bezel shader link failed: %s\n", infoLog);
        glad_glDeleteProgram(program);
        return 0;
    }

    bezelProjectionLoc = glad_glGetUniformLocation(program, "u_projection");
    bezelTextureLoc = glad_glGetUniformLocation(program, "u_texture");
    bezelOpacityLoc = glad_glGetUniformLocation(program, "u_opacity");

    return program;
}

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

    GLint oldAlignment = 4;
    glad_glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment);
    glad_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

    glad_glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment);

    glad_glBindTexture(GL_TEXTURE_2D, 0);

    printf("Loaded bezel image: %s (%dx%d)\n", filePath, bezelSurface->w, bezelSurface->h);

    SDL_DestroySurface(bezelSurface);
    bezelSurface = NULL;

    return 1;
}

static int createBezelGeometry(void)
{
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };

    glad_glGenBuffers(1, &bezelVbo);

    if (!bezelVbo)
        return 0;

    glad_glBindBuffer(GL_ARRAY_BUFFER, bezelVbo);
    glad_glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);

    bezelVao = 0;

    return 1;
}

void initBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;

    if (!loadBezelTexture(cfg->bezelPath))
        return;

    bezelProgram = createBezelProgram();
    if (!bezelProgram)
    {
        shutdownBezelOverlay();
        return;
    }

    if (!createBezelGeometry())
    {
        shutdownBezelOverlay();
        return;
    }

    bezelInitialized = 1;
}

void drawBezelOverlay(void)
{
    EmulatorConfig *cfg = getConfig();

    if (!cfg->bezelEnabled)
        return;

    if (!bezelInitialized || bezelTexture == 0 || bezelProgram == 0 || bezelVbo == 0)
        return;

    int bezelW = 0;
    int bezelH = 0;
    SDL_Window *window = getSDLWindow();

    if (window)
        SDL_GetWindowSizeInPixels(window, &bezelW, &bezelH);

    if (bezelW <= 0 || bezelH <= 0)
    {
        bezelW = drawableW;
        bezelH = drawableH;
    }

    if (bezelW <= 0 || bezelH <= 0)
        return;

    GLboolean blendWasEnabled = glad_glIsEnabled(GL_BLEND);
    GLboolean depthWasEnabled = glad_glIsEnabled(GL_DEPTH_TEST);
    GLboolean scissorWasEnabled = glad_glIsEnabled(GL_SCISSOR_TEST);
    GLboolean cullWasEnabled = glad_glIsEnabled(GL_CULL_FACE);
    GLboolean texture0WasEnabled;
    GLboolean fragmentProgramWasEnabled = glad_glIsEnabled(GL_FRAGMENT_PROGRAM_ARB);
    GLboolean vertexProgramWasEnabled = glad_glIsEnabled(GL_VERTEX_PROGRAM_ARB);
    GLboolean oldColorMask[4];

    GLint oldViewport[4];
	GLint oldProgram = 0;
	GLint oldActiveTexture = 0;
	GLint oldTexture = 0;
	GLint oldVao = 0;
	GLint oldArrayBuffer = 0;
	GLint oldReadFramebuffer = 0;
	GLint oldDrawFramebuffer = 0;
	GLint oldBlendSrcRgb = 0;
	GLint oldBlendDstRgb = 0;
	GLint oldBlendSrcAlpha = 0;
	GLint oldBlendDstAlpha = 0;
	GLint oldBlendEquationRgb = 0;
	GLint oldBlendEquationAlpha = 0;
    BezelAttribState oldPosAttrib;
    BezelAttribState oldUvAttrib;

    glad_glGetIntegerv(GL_VIEWPORT, oldViewport);
    glad_glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glad_glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
    glad_glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glad_glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBuffer);
	glad_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFramebuffer);
	glad_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFramebuffer);
    glad_glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRgb);
    glad_glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRgb);
    glad_glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
    glad_glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);
    glad_glGetIntegerv(GL_BLEND_EQUATION_RGB, &oldBlendEquationRgb);
    glad_glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &oldBlendEquationAlpha);
    glad_glGetBooleanv(GL_COLOR_WRITEMASK, oldColorMask);

    glad_glActiveTexture(GL_TEXTURE0);
    texture0WasEnabled = glad_glIsEnabled(GL_TEXTURE_2D);
    glad_glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);

    glad_glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glad_glDisable(GL_VERTEX_PROGRAM_ARB);

    glad_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glad_glViewport(0, 0, bezelW, bezelH);
    glad_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glad_glDisable(GL_DEPTH_TEST);
    glad_glDisable(GL_SCISSOR_TEST);
    glad_glDisable(GL_CULL_FACE);

    glad_glEnable(GL_BLEND);
    glad_glBlendEquation(GL_FUNC_ADD);
    glad_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glad_glEnable(GL_TEXTURE_2D);
    glad_glBindTexture(GL_TEXTURE_2D, bezelTexture);

    glad_glUseProgram(bezelProgram);

    if (bezelTextureLoc >= 0)
        glad_glUniform1i(bezelTextureLoc, 0);

    if (bezelOpacityLoc >= 0)
        glad_glUniform1f(bezelOpacityLoc, cfg->bezelOpacity);

    if (bezelProjectionLoc >= 0)
    {
        GLfloat projection[16] = {
            2.0f / bezelW, 0.0f, 0.0f, 0.0f,
            0.0f, -2.0f / bezelH, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 1.0f
        };

        glad_glUniformMatrix4fv(bezelProjectionLoc, 1, GL_FALSE, projection);
    }

    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        (float)bezelW, 0.0f, 1.0f, 0.0f,
        (float)bezelW, (float)bezelH, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f, 0.0f,
        (float)bezelW, (float)bezelH, 1.0f, 1.0f,
        0.0f, (float)bezelH, 0.0f, 1.0f
    };

	GLint posLoc = glad_glGetAttribLocation(bezelProgram, "a_pos");
	GLint uvLoc = glad_glGetAttribLocation(bezelProgram, "a_uv");

	if (posLoc >= 0 && uvLoc >= 0)
	{
        saveBezelAttribState((GLuint)posLoc, &oldPosAttrib);
        saveBezelAttribState((GLuint)uvLoc, &oldUvAttrib);

		glad_glBindBuffer(GL_ARRAY_BUFFER, bezelVbo);
		glad_glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		glad_glEnableVertexAttribArray((GLuint)posLoc);
		glad_glVertexAttribPointer(
			(GLuint)posLoc,
			2,
			GL_FLOAT,
			GL_FALSE,
			4 * sizeof(float),
			(void *)0
		);

		glad_glEnableVertexAttribArray((GLuint)uvLoc);
		glad_glVertexAttribPointer(
			(GLuint)uvLoc,
			2,
			GL_FLOAT,
			GL_FALSE,
			4 * sizeof(float),
			(void *)(2 * sizeof(float))
	);

    glad_glDrawArrays(GL_TRIANGLES, 0, 6);

    restoreBezelAttribState((GLuint)posLoc, &oldPosAttrib);
    restoreBezelAttribState((GLuint)uvLoc, &oldUvAttrib);
}

	glad_glBindBuffer(GL_ARRAY_BUFFER, oldArrayBuffer);

    glad_glBindTexture(GL_TEXTURE_2D, (GLuint)oldTexture);
    if (texture0WasEnabled)
        glad_glEnable(GL_TEXTURE_2D);
    else
        glad_glDisable(GL_TEXTURE_2D);

    glad_glUseProgram((GLuint)oldProgram);
    if (glad_glBindVertexArray)
        glad_glBindVertexArray((GLuint)oldVao);
	glad_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)oldReadFramebuffer);
	glad_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)oldDrawFramebuffer);
	glad_glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    glad_glColorMask(
        oldColorMask[0],
        oldColorMask[1],
        oldColorMask[2],
        oldColorMask[3]
    );

    glad_glBlendEquationSeparate((GLenum)oldBlendEquationRgb, (GLenum)oldBlendEquationAlpha);
    glad_glBlendFuncSeparate((GLenum)oldBlendSrcRgb, (GLenum)oldBlendDstRgb, (GLenum)oldBlendSrcAlpha, (GLenum)oldBlendDstAlpha);

    if (fragmentProgramWasEnabled)
        glad_glEnable(GL_FRAGMENT_PROGRAM_ARB);
    else
        glad_glDisable(GL_FRAGMENT_PROGRAM_ARB);

    if (vertexProgramWasEnabled)
        glad_glEnable(GL_VERTEX_PROGRAM_ARB);
    else
        glad_glDisable(GL_VERTEX_PROGRAM_ARB);

    glad_glActiveTexture((GLenum)oldActiveTexture);

    if (blendWasEnabled)
        glad_glEnable(GL_BLEND);
    else
        glad_glDisable(GL_BLEND);

    if (depthWasEnabled)
        glad_glEnable(GL_DEPTH_TEST);
    else
        glad_glDisable(GL_DEPTH_TEST);

    if (scissorWasEnabled)
        glad_glEnable(GL_SCISSOR_TEST);
    else
        glad_glDisable(GL_SCISSOR_TEST);

    if (cullWasEnabled)
        glad_glEnable(GL_CULL_FACE);
    else
        glad_glDisable(GL_CULL_FACE);
}

void shutdownBezelOverlay(void)
{
    if (bezelTexture != 0)
    {
        glad_glDeleteTextures(1, &bezelTexture);
        bezelTexture = 0;
    }

    if (bezelVbo != 0)
    {
        glad_glDeleteBuffers(1, &bezelVbo);
        bezelVbo = 0;
    }

    if (bezelVao != 0)
    {
        glad_glDeleteVertexArrays(1, &bezelVao);
        bezelVao = 0;
    }

    if (bezelProgram != 0)
    {
        glad_glDeleteProgram(bezelProgram);
        bezelProgram = 0;
    }

    bezelProjectionLoc = -1;
    bezelTextureLoc = -1;
    bezelOpacityLoc = -1;

    if (bezelSurface)
    {
        SDL_DestroySurface(bezelSurface);
        bezelSurface = NULL;
    }

    bezelInitialized = 0;
}
