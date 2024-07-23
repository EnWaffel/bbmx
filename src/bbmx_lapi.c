#include "bbmx_lapi.h"
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbmx_lapi_interface.h"

// SETUP start

static BBMXSinitargs* __initargs = NULL;
static BBMXSmodel* __cur_model = NULL;
static uint8_t __cur_universe = 1;
static uint8_t __cur_channel_mode = 0;
static int __cur_fx_idx = 0;
static int __addr_multiplier = 0;
static int __loaded = 0;

static BBMXSfixture* get_fixture_by_name(const char* name)
{
  for (int i = 0; i < __initargs->fixtureCount; i++)
  {
    BBMXSfixture* fx = &__initargs->fixtures[i];
    if (strcmp(fx->name, name) == 0)
    {
      return fx;
    }
  }
  return NULL;
}

static int l_bbmx_using(lua_State* L)
{
  if (__loaded) luaL_error(L, "'bbmx_using' can only be called on setup");

  const char* name = luaL_checkstring(L, 1);

  __cur_model = bbmxs_get_model(name);
  __cur_channel_mode = 0;
  __cur_universe = 1;

  if (gDebugMode) printf("[DEBUG]: Using Model: %s\n", name);
  return 0;
}

static int l_bbmx_opt(lua_State* L)
{
  if (__loaded) luaL_error(L, "'bbmx_opt' can only be called on setup");

  const char* option = luaL_checkstring(L, 1);

  if (strcmp(option, "universe") == 0)
  {
    int uv = luaL_checkinteger(L, 2);
    __cur_universe = uv;
    if (gDebugMode) printf("[DEBUG]: Option: %s | Value: %d\n", option, uv);
  }
  else if (strcmp(option, "channel-mode") == 0)
  {
    int ch = luaL_checkinteger(L, 2);
    __cur_channel_mode = ch;
    if (gDebugMode) printf("[DEBUG]: Option: %s | Value: %d\n", option, ch);
  }

  return 0;
}

static int l_bbmx_port(lua_State* L)
{
  if (__loaded) luaL_error(L, "'bbmx_port' can only be called on setup");

  size_t portLen;
  const char* port = luaL_checklstring(L, 1, &portLen);
  char* portCopy = malloc(portLen + 1);
  memcpy(portCopy, port, portLen);
  portCopy[portLen] = 0;

  __initargs->port = portCopy;

  if (gDebugMode) printf("[DEBUG]: Using COM Port: \"%s\"\n", port);

  return 0;
}

static int l_bbmx_fixture(lua_State* L)
{
  if (__loaded) luaL_error(L, "'bbmx_fixture' can only be called on setup");
  
  size_t nameLen;
  const char* name = luaL_checklstring(L, 1, &nameLen);
  char* fxName = malloc(nameLen + 1);
  memcpy(fxName, name, nameLen);
  fxName[nameLen] = 0;

  uint8_t address;

  if (lua_isinteger(L, -1))
  {
    address = luaL_checkinteger(L, 2);
  }
  else
  {
    address = __cur_channel_mode * __addr_multiplier;
  }

  BBMXScolor c;
  c.r = 0;
  c.g = 0;
  c.b = 0;
  c.w = 0;

  BBMXSfixture fx;
  fx.model = __cur_model;
  fx.name = fxName;
  fx.color = c;
  fx.tilt = 0.0f;
  fx.pan = 0.0f;
  fx.address = address;
  fx.channel_mode = __cur_channel_mode;
  fx.universe = __cur_universe;

  __initargs->fixtures[__initargs->fixtureCount] = fx;
  __initargs->fixtureCount++;
  __addr_multiplier++;

  if (gDebugMode) printf("[DEBUG]: Created Fixture: \"%s\" | Model: \"%d\" | Address: \"%d\"\n", fx.name, 0, address);

  return 0;
}

static int l_bbmx_group(lua_State* L)
{
  if (__loaded) luaL_error(L, "'bbmx_group' can only be called on setup");

  size_t nameLen;
  const char* name = luaL_checklstring(L, 1, &nameLen);
  char* groupName = malloc(nameLen + 1);
  memcpy(groupName, name, nameLen);
  groupName[nameLen] = 0;

  BBMXSgroup group;
  group.fixtures = malloc(sizeof(BBMXSfixture*) * gMaxFixtures);
  group.name = groupName;
  group.fixtureCount = 0;

  lua_pushnil(L);
  int idx = 1;
  while (lua_next(L, -2) != 0) {
    if (!lua_isstring(L, -1))
    {
      free(group.name);
      free(group.fixtures);
      luaL_error(L, "At index: '%d': Expected 'string'; got '%s'", idx, lua_typename(L, lua_type(L, -1)));
    }

    const char* fxName = lua_tostring(L, -1);
    BBMXSfixture* fx = get_fixture_by_name(fxName);
    if (fx == NULL)
    {
      free(group.name);
      free(group.fixtures);
      luaL_error(L, "At index: '%d': Can't find fixture named: '%s'", idx, fxName);
    }

    group.fixtures[group.fixtureCount] = fx;
    group.fixtureCount++;

    lua_pop(L, 1);
    idx++;
  }

  if (gDebugMode)
  {
    printf("[DEBUG]: Created Group: \"%s\":\n", name);
    for (int i = 0; i < group.fixtureCount; i++)
    {
      printf("Fixture #%d: \"%s\"\n", i + 1, group.fixtures[i]->name);
    }
  }

  return 0;
}

// SETUP end

static int l_bbmx_exit(lua_State* L)
{
  gShouldExit = 1;
  return 0;
}

static int l_bbmx_fx_r(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t c = luaL_checkinteger(L, 2);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.r = c;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_g(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t c = luaL_checkinteger(L, 2);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.g = c;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_b(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t c = luaL_checkinteger(L, 2);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.b = c;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_w(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t c = luaL_checkinteger(L, 2);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.w = c;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_rgb(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t r = luaL_checkinteger(L, 2);
  uint8_t g = luaL_checkinteger(L, 3);
  uint8_t b = luaL_checkinteger(L, 4);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.r = r;
  fx->color.g = g;
  fx->color.b = b;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_rgbw(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t r = luaL_checkinteger(L, 2);
  uint8_t g = luaL_checkinteger(L, 3);
  uint8_t b = luaL_checkinteger(L, 4);
  uint8_t w = luaL_checkinteger(L, 5);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.r = r;
  fx->color.g = g;
  fx->color.b = b;
  fx->color.w = w;

  bbmxs_fx_update_color(fx);

  return 0;
}

static int l_bbmx_fx_brgt(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  uint8_t b = luaL_checkinteger(L, 2);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->brightness = b;

  uint8_t buf[3];
  buf[0] = 1;
  buf[1] = fx->model->opts.ch_cfg.ch_brgt;
  buf[2] = fx->brightness;

  if (!bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf)))
  {
    luaL_error(L, "Failed to send brightness data");
  }

  return 0;
}

static int l_bbmx_fx_tilt(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  float angle = luaL_checknumber(L, 2);
  float speed = luaL_checknumber(L, 3);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->tilt = angle;

  uint8_t buf[8];
  buf[0] = 1;
  buf[1] = fx->model->opts.ch_cfg.ch_tilt;
  buf[2] = (angle / fx->model->opts.max_tilt) * 255;

  if (!bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf)))
  {
    luaL_error(L, "Failed to send tilt data");
  }

  return 0;
}

static int l_bbmx_fx_pan(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  float angle = luaL_checknumber(L, 2);
  float speed = luaL_checknumber(L, 3);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->pan = angle;

  uint8_t buf[8];
  buf[0] = 1;
  buf[1] = fx->model->opts.ch_cfg.ch_pan;
  buf[2] = (angle / fx->model->opts.max_pan) * 255;

  if (!bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf)))
  {
    luaL_error(L, "Failed to send pan data");
  }

  return 0;
}

static int l_bbmx_fx_reset(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);

  BBMXSfixture* fx = bbmxs_get_fx(name);
  if (fx == NULL) luaL_error(L, "Can't find fixture named: %s", name);

  fx->color.r = 0;
  fx->color.g = 0;
  fx->color.b = 0;
  fx->color.w = 0;

  fx->tilt = 0;
  fx->pan = 0;

  bbmxs_fx_update_color(fx);

  uint8_t buf[8];
  buf[0] = 1;
  buf[2] = 0;
  if (fx->model->supports_tilt)
  {
    buf[1] = fx->model->opts.ch_cfg.ch_tilt;
    if (!bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf)))
    {
      luaL_error(L, "Failed to send tilt data");
    }
  }

  if (fx->model->supports_pan)
  {
    buf[1] = fx->model->opts.ch_cfg.ch_pan;
    if (!bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf)))
    {
      luaL_error(L, "Failed to send pan data");
    }
  }

  return 0;
}

static int l_bbmx_timed(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  float t = luaL_checknumber(L, 2);

  char* nameCopy = malloc(strlen(name) + 1);
  memcpy(nameCopy, name, strlen(name));
  nameCopy[strlen(name)] = 0;

  BBMXStimedfunc timedFunc;
  timedFunc.name = nameCopy;
  timedFunc.t = t;
  timedFunc.used = 0;

  if (__initargs->timedFunctionCount == 0)
  {
    __initargs->timedFunctions = malloc(sizeof(BBMXStimedfunc));
    __initargs->timedFunctions[0] = timedFunc;
    __initargs->timedFunctionCount = 1;
  }
  else
  {
    __initargs->timedFunctions = realloc(__initargs->timedFunctions, sizeof(BBMXStimedfunc) * (__initargs->timedFunctionCount + 1));
    __initargs->timedFunctions[__initargs->timedFunctionCount] = timedFunc;
    __initargs->timedFunctionCount++;
  }

  return 0;
}

static int l_bbmx_reset_timer(lua_State* L)
{
  gDoTimerReset = 1;
  return 0;
}

static int l_bbmx_snd_flash(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);
  float t = luaL_checknumber(L, 2);

  char* nameCopy = malloc(strlen(name) + 1);
  memcpy(nameCopy, name, strlen(name));
  nameCopy[strlen(name)] = 0;

  BBMXStimedflash timedFlash;
  timedFlash.speed = luaL_checknumber(L, 3);
  timedFlash.color.r = luaL_checkinteger(L, 4);
  timedFlash.color.g = luaL_checkinteger(L, 5);
  timedFlash.color.b = luaL_checkinteger(L, 6);
  timedFlash.color.w = luaL_checkinteger(L, 7);
  timedFlash.t = t;
  timedFlash.used = 0;
  timedFlash.name = nameCopy;

  if (__initargs->timedFlashCount == 0)
  {
    __initargs->timedFlashes = malloc(sizeof(BBMXStimedflash));
    __initargs->timedFlashes[0] = timedFlash;
    __initargs->timedFlashCount = 1;
  }
  else
  {
    __initargs->timedFlashes = realloc(__initargs->timedFlashes, sizeof(BBMXStimedflash) * (__initargs->timedFlashCount + 1));
    __initargs->timedFlashes[__initargs->timedFlashCount] = timedFlash;
    __initargs->timedFlashCount++;
  }

  return 0;
}

static int l_bbmx_snd(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  char* pathCopy = malloc(strlen(path) + 1);
  memcpy(pathCopy, path, strlen(path));
  pathCopy[strlen(path)] = 0;

  __initargs->sndFile = pathCopy;

  if (lua_isnumber(L, -1))
  {
    __initargs->bpm = lua_tonumber(L, -1);
    __initargs->bpm_resolution = 1;
  }

  if (lua_isnumber(L, -1))
  {
    __initargs->bpm_resolution = lua_tonumber(L, -1);
  }

  return 0;
}

static int l_bbmx_fx_flash(lua_State* L)
{
  const char* name = luaL_checkstring(L, 1);

  BBMXStimedflash timedFlash;
  timedFlash.speed = luaL_checknumber(L, 2);
  timedFlash.color.r = luaL_checkinteger(L, 3);
  timedFlash.color.g = luaL_checkinteger(L, 4);
  timedFlash.color.b = luaL_checkinteger(L, 5);
  timedFlash.color.w = luaL_checkinteger(L, 6);
  timedFlash.name = name;

  printf("asdhasid: %d\n", luaL_checknumber(L, 2));

  bbmxi_do_flash(timedFlash);

  return 0;
}

static int l_lerp(lua_State* L)
{
  double a = luaL_checknumber(L, 1);
  double b = luaL_checknumber(L, 2);
  double f = luaL_checknumber(L, 3);

  lua_pushnumber(L, a + f * (b - a));

  return 1;
}

int bbmx_lapi_load(lua_State* L, BBMXSinitargs* initargs)
{
  if (__initargs != NULL)
  {
    printf("Fatal Error (lapi): bbmx_lapi_load was called twice! (somehow?)\n");
    return -1;
  }
  __initargs = initargs;

  lua_pushcfunction(L, l_bbmx_using);
  lua_setglobal(L, "bbmx_using");

  lua_pushcfunction(L, l_bbmx_opt);
  lua_setglobal(L, "bbmx_opt");

  lua_pushcfunction(L, l_bbmx_port);
  lua_setglobal(L, "bbmx_port");

  lua_pushcfunction(L, l_bbmx_fixture);
  lua_setglobal(L, "bbmx_fixture");

  lua_pushcfunction(L, l_bbmx_group);
  lua_setglobal(L, "bbmx_group");

  lua_pushcfunction(L, l_bbmx_exit);
  lua_setglobal(L, "bbmx_exit");

  lua_pushcfunction(L, l_bbmx_fx_r);
  lua_setglobal(L, "bbmx_fx_r");

  lua_pushcfunction(L, l_bbmx_fx_g);
  lua_setglobal(L, "bbmx_fx_g");

  lua_pushcfunction(L, l_bbmx_fx_b);
  lua_setglobal(L, "bbmx_fx_b");

  lua_pushcfunction(L, l_bbmx_fx_w);
  lua_setglobal(L, "bbmx_fx_w");

  lua_pushcfunction(L, l_bbmx_fx_rgb);
  lua_setglobal(L, "bbmx_fx_rgb");

  lua_pushcfunction(L, l_bbmx_fx_rgbw);
  lua_setglobal(L, "bbmx_fx_rgbw");

  lua_pushcfunction(L, l_bbmx_fx_brgt);
  lua_setglobal(L, "bbmx_fx_brgt");

  lua_pushcfunction(L, l_bbmx_fx_tilt);
  lua_setglobal(L, "bbmx_fx_tilt");

  lua_pushcfunction(L, l_bbmx_fx_pan);
  lua_setglobal(L, "bbmx_fx_pan");

  lua_pushcfunction(L, l_bbmx_fx_reset);
  lua_setglobal(L, "bbmx_fx_reset");

  lua_pushcfunction(L, l_bbmx_timed);
  lua_setglobal(L, "bbmx_timed");

  lua_pushcfunction(L, l_bbmx_reset_timer);
  lua_setglobal(L, "bbmx_reset_timer");

  lua_pushcfunction(L, l_bbmx_snd);
  lua_setglobal(L, "bbmx_snd");

  lua_pushcfunction(L, l_bbmx_snd_flash);
  lua_setglobal(L, "bbmx_snd_flash");

  lua_pushcfunction(L, l_bbmx_fx_flash);
  lua_setglobal(L, "bbmx_fx_flash");
  
  lua_pushcfunction(L, l_lerp);
  lua_setglobal(L, "lerp");
  
  return 0;
}

void bbmx_lapi_loaded()
{
  __loaded = 1;
}
