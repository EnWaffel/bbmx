#include <stb/stb_vorbis.h>
#undef L
#include "bbmx.h"
#include <argparse/argparse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include "bbmx_lapi.h"
#include "globals.h"
#include <time.h>
#include <signal.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "bbmx_lapi_interface.h"
#include <math.h>

typedef struct
{
    char* buf;
    size_t sz;
    char* filename;
} PreprocessResult;

static int bbmx_run(const char* path);
static int run_script(const char* path);
static void print_lua_error(lua_State* L);
static int do_pcall(lua_State* L, int nargs, int nresults);
static void INThandler(int sig);
static int load_audio(const char* path);
static void terminate_openal();
static void update_flashes(float delta, BBMXScontext* ctx, float timePos);
static int update_timed_functions(lua_State* L, BBMXScontext* ctx);
static PreprocessResult preprocess_script(const char* path);

static ALCdevice* alc_device = NULL;
static ALCcontext* alc_context = NULL;
static ALuint al_buffer;
static ALuint al_source;
static ALint al_sample_rate;
static BBMXStimedflash* curFlash = NULL;
static BBMXSfixture* curFlashFx = NULL;
static int curFlashTemp = 0;
static float elapsed = 0.0f;

static const char *const usages[] = {
    "bbmx [options] [[--] args]",
    "bbmx [options]",
    NULL,
};

int bbmx_init(int argc, const char* argv[])
{
    signal(SIGINT, INThandler);

    const char* runPath = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Test"),
        OPT_STRING('r', "run", &runPath, "runs a scene script", NULL, 0, 0),
        OPT_BOOLEAN('d', "debug", &gDebugMode, "prints debug messages for additional information", NULL, 0, 0),
        OPT_INTEGER('n', "mnum", &gMaxModels, "max. number of models (default: 8)", NULL, 0, 0),
        OPT_INTEGER('m', "fxnum", &gMaxFixtures, "max. number of fixtures (default: 32)", NULL, 0, 0),
        OPT_INTEGER('u', "ups", &gUPS, "updates per second", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nbbmx is a dmx fixture controller program.\nYou can script scenes via lua and run them with bbmx -r <your lua file>.", "\n");
    argc = argparse_parse(&argparse, argc, argv);

    if (runPath == NULL)
    {
        printf("No file provided! Use: bbmx -r <file> to run a script.\n");
        return -1;
    }

    return bbmx_run(runPath);
}

int bbmx_run(const char* path)
{
    char* ext = utils_to_lower(utils_get_file_ext(path));
    
    if (strcmp(ext, "lua") != 0)
    {
        printf("Invalid file: \"%s\"! Please provide a lua file.\n", path);
        return -1;
    }

    if (gDebugMode) printf("Debug-Mode Enabled!\n");

    if (gDebugMode) printf("[DEBUG]: Loading models...\n");
    if (!bbmxs_load_models())
    {
        printf("bbmx Error: Failed to load models!\n");
        return -1;
    }

    return run_script(path);
}

int run_script(const char* path)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    PreprocessResult preprocess_result = preprocess_script(path);

    if (luaL_loadbuffer(L, preprocess_result.buf, preprocess_result.sz, preprocess_result.filename) != LUA_OK)
    {
        printf("bbmx Error: Failed to load script: \"%s\"", path);
        lua_close(L);
        return -1;
    }

    lua_pcall(L, 0, 0, 0);

    BBMXSinitargs initargs;
    initargs.models = malloc(sizeof(BBMXSmodel) * gMaxModels);
    initargs.fixtures = malloc(sizeof(BBMXSfixture) * gMaxFixtures);
    initargs.fixtureCount = 0;
    initargs.modelCount = 0;
    initargs.timedFunctionCount = 0;
    initargs.timedFlashCount = 0;
    initargs.sndFile = NULL;
    initargs.timedFlashes = NULL;
    initargs.timedFunctions = NULL;
    initargs.bpm = 0;

    int result;
    if ((result = bbmx_lapi_load(L, &initargs)) != 0)
    {
        printf("bbmx (lapi) Error: Failed to load lua api. (Code: %s)\n", result);
        lua_close(L);
        return -1;
    }

    lua_getglobal(L, "BBMX_setup");
    if (!lua_isfunction(L, -1))
    {
        printf("bbmx Error: Expected 'BBMX_setup'; got 'nil'\n");
        lua_close(L);
        return -1;
    }
    if (!do_pcall(L, 0, 0))
    {
        lua_close(L);
        return -1;
    }

    BBMXScontext* ctx = bbmxs_init(&initargs);
    if (ctx == NULL)
    {
        bbmxs_close();
        lua_close(L);
        return -1;
    }

    bbmx_lapi_loaded();

    if (gDebugMode) printf("[DEBUG] Init Complete\n");

    int hasSound = ctx->sndFile != NULL;
    if (hasSound)
    {
        if (gDebugMode) printf("[DEBUG] Has Sound\n");

        alc_device = alcOpenDevice(NULL);
        alc_context = alcCreateContext(alc_device, NULL);
        alcMakeContextCurrent(alc_context);

        alGenBuffers(1, &al_buffer);
        alGenSources(1, &al_source);

        alSourcef(al_source, AL_GAIN, 0.05f);
        if (!load_audio(ctx->sndFile))
        {
            bbmxs_close();
            lua_close(L);
            terminate_openal();
        }
        
        alSourcei(al_source, AL_BUFFER, al_buffer);
        alSourcePlay(al_source);
    }

    lua_getglobal(L, "BBMX_start");
    if (!lua_isfunction(L, -1))
    {
        printf("bbmx Error: Expected 'BBMX_start'; got 'nil'\n");
        bbmxs_close();
        lua_close(L);
        return -1;
    }
    if (!do_pcall(L, 0, 0))
    {
        bbmxs_close();
        lua_close(L);
        return -1;
    }

    int lastBeat = 0;

    lua_getglobal(L, "BBMX_loop");
    int loopFunc = lua_isfunction(L, -1);
    if (loopFunc || ctx->timedFunctionCount > 0 || hasSound)
    {
        lua_pop(L, -1);
        time_t last = 0;
        while (!gShouldExit)
        {
            time_t now = (clock() / (double)CLOCKS_PER_SEC) * 1000;
            if (now >= last + (1000 / gUPS))
            {
                double delta = now - last;
                last = now;

                elapsed += delta;

                lua_pushnumber(L, elapsed);
                lua_setglobal(L, "time");

                if (loopFunc)
                {
                    lua_getglobal(L, "BBMX_loop");
                    lua_pushnumber(L, delta);
                    if (!do_pcall(L, 1, 0))
                    {
                        bbmxs_close();
                        lua_close(L);
                        return -1;
                    }
                }

                float timePos = elapsed;
                if (hasSound)
                {
                    int state;
                    alGetSourcei(al_source, AL_SOURCE_STATE, &state);
                    if (state != AL_PLAYING)
                    {
                        gShouldExit = 1;
                    }

                    int offset;
                    alGetSourcei(al_source, AL_SAMPLE_OFFSET, &offset);
                    float pos = (float)offset / (float)al_sample_rate;
                    timePos = pos * 1000.0f;

                    if (ctx->bpm > 0)
                    {
                        int curBeat = floorf(timePos / (ctx->beat_time));
                        if (lastBeat != curBeat)
                        {
                            lastBeat = curBeat;
                            lua_getglobal(L, "BBMX_beat");
                            if (lua_isfunction(L, -1))
                            {
                                lua_pushinteger(L, curBeat);
                                if (!do_pcall(L, 1, 0))
                                {
                                    terminate_openal();
                                    bbmxs_close();
                                    lua_close(L);
                                    return -1;
                                }
                            }
                        }
                    }
                }

                update_flashes(delta, ctx, timePos);
                if (!update_timed_functions(L, ctx))
                {
                    printf("bbmx Error: Something went wrong while updating timed functions!\n");
                    return -1;
                }
            }
        }
    }

    lua_getglobal(L, "BBMX_exit");
    if (lua_isfunction(L, -1))
    {
        do_pcall(L, 0, 0);
    }


    terminate_openal();
    bbmxs_close();
    lua_close(L);

    free(preprocess_result.buf);
    free(preprocess_result.filename);
    
    return 0;
}

static void update_flashes(float delta, BBMXScontext* ctx, float timePos)
{
    for (int i = 0; i < ctx->timedFlashCount; i++)
    {
        BBMXStimedflash* flash = &ctx->timedFlashes[i];
        if (flash->used) continue;

        if (timePos >= flash->t)
        {
            flash->used = 1;
            BBMXSfixture* fx = bbmxs_get_fx(flash->name);
            curFlash = flash;
            curFlashFx = fx;
            fx->color = flash->color;
        }
    }

    if (curFlash != NULL)
    {

        if (curFlashFx->color.r - curFlash->speed > 0)
        {
            curFlashFx->color.r -= curFlash->speed * delta;
        }
        if (curFlashFx->color.g - curFlash->speed > 0)
        {
            curFlashFx->color.g -= curFlash->speed * delta;
        }
        if (curFlashFx->color.b - curFlash->speed > 0)
        {
            curFlashFx->color.b -= curFlash->speed * delta;
        }
        if (curFlashFx->color.w - curFlash->speed > 0)
        {
            curFlashFx->color.w -= curFlash->speed * delta;
        }

        if (curFlashFx->color.r < 1 && curFlashFx->color.g < 1 && curFlashFx->color.b < 1 && curFlashFx->color.w < 1)
        {
            if (curFlashTemp)
            {
                curFlashTemp = 0;
                free(curFlash);
            }
            curFlash = NULL;
            curFlashFx = NULL;
        }
        else
        {
            bbmxs_fx_update_color(curFlashFx);
        }
    }
}

static int update_timed_functions(lua_State* L, BBMXScontext* ctx)
{
    int noMoreFuncToExecute = 1;
    for (int i = 0; i < ctx->timedFunctionCount; i++)
    {
        BBMXStimedfunc* timedFunc = &ctx->timedFunctions[i];
        if (timedFunc->used) continue;
        noMoreFuncToExecute = 0;

        if (elapsed >= timedFunc->t)
        {
            timedFunc->used = 1;
            lua_getglobal(L, timedFunc->name);
            if (!do_pcall(L, 0, 0))
            {
                bbmxs_close();
                lua_close(L);
                return 0;
            }
        }
    }

    int justReset = 0;
    if (gDoTimerReset)
    {
        justReset = 1;
        gDoTimerReset = 0;
        elapsed = 0.0f;
        for (int i = 0; i < ctx->timedFunctionCount; i++)
        {
            ctx->timedFunctions[i].used = 0;
        }
    }

    if (noMoreFuncToExecute && !justReset && ctx->timedFunctionCount > 0 && gExitAfterNoMoreTimedFuncs)
    {
        gShouldExit = 1;
    }

    return 1;
}

static void print_lua_error(lua_State* L)
{
    printf("%s\n", lua_tostring(L, -1));
}

static int do_pcall(lua_State* L, int nargs, int nresults)
{
    if (lua_pcall(L, nargs, nresults, 0) != LUA_OK)
    {
        print_lua_error(L);
        return 0;
    }

    return 1;
}

static void INThandler(int sig)
{
    signal(sig, SIG_IGN);
    gShouldExit = 1;
}

static int load_audio(const char* path)
{
    FILE* f;
    fopen_s(&f, path, "rb");
    if (!f)
    {
        printf("bbmx Error: Failed to open file: \"%s\"\n", path);
        return 0;
    }

    int err = VORBIS__no_error;
    stb_vorbis* v = stb_vorbis_open_file(f, 1, &err, NULL);
    if (err != VORBIS__no_error)
    {
        printf("bbmx Error: Failed to open vorbis file: \"%s\", Error Code: \"%d\"\n", path, err);
        return 0;
    }

    stb_vorbis_info i = stb_vorbis_get_info(v);
    int channels = i.channels;
    unsigned int sampleRate = i.sample_rate;
    al_sample_rate = sampleRate;

    size_t size = stb_vorbis_stream_length_in_samples(v) * channels * sizeof(short);
    short* data = malloc(size);

    stb_vorbis_get_samples_short_interleaved(v, channels, data, size);

    ALenum format = AL_INVALID_ENUM;
    if (channels == 1)
    {
        format = AL_FORMAT_MONO16;
    }
    else if (channels == 2)
    {
        format = AL_FORMAT_STEREO16;
    }
    else
    {
        printf("bbmx Error: Unsupported number of channels: %d\n", channels);
        stb_vorbis_close(v);
        fclose(f);
        return 0;
    }

    alBufferData(al_buffer, format, data, size, sampleRate);
    free(data);

    stb_vorbis_close(v);
    fclose(f);

    return 1;
}

static void terminate_openal()
{
    if (alc_device == NULL) return;

    alSourceStop(al_source);

    alDeleteBuffers(1, &al_buffer);
    alDeleteSources(1, &al_source);

    alcDestroyContext(alc_context);
    alcCloseDevice(alc_device);
}

void bbmxi_do_flash(BBMXStimedflash flash)
{
    BBMXStimedflash* f = malloc(sizeof(BBMXStimedflash));
    f->color = flash.color;
    f->name = flash.name;
    f->speed = flash.speed;

    curFlash = f;
    curFlashFx = bbmxs_get_fx(flash.name);
    curFlashTemp = 1;
}

static PreprocessResult preprocess_script(const char* path)
{
    PreprocessResult result;

    char* pfile;
    pfile = path + strlen(path);
    for (; pfile > path; pfile--)
    {
        if ((*pfile == '\\') || (*pfile == '/'))
        {
            pfile++;
            break;
        }
    }
    result.filename = pfile;
    
    char* buf = utils_read_file_to_string(path);
    char* buf1;

    buf1 = utils_str_replace(buf, "require(\"bbmx\")", "");
    free(buf);
    buf = buf1;

    result.buf = buf;
    result.sz = strlen(buf);

    return result;
}
