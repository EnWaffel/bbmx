#ifndef __BBMXS_H
#define __BBMXS_H

#include <stdint.h>
#include "models.h"

#define BBMXS_CMD_DMX_WRITE 0x01

typedef uint8_t BBMXSbool;
typedef uint8_t DMXChannel;
typedef uint8_t BBMXScmd;

typedef struct
{
  float r;
  float g;
  float b;
  float w;
} BBMXScolor;

// Relative to fixture address
typedef struct
{
  DMXChannel ch_red;
  DMXChannel ch_green;
  DMXChannel ch_blue;
  DMXChannel ch_white;
  DMXChannel ch_tilt;
  DMXChannel ch_pan;
  DMXChannel ch_mtr_spd; // motor speed
  DMXChannel ch_brgt; // brightness
} BBMXSchannelconfig;

typedef struct
{
  uint8_t channelModes[8];
  uint8_t channelModesLen;
  BBMXSchannelconfig ch_cfg;
  float max_tilt;
  float max_pan;
} BBMXSmodelopts;

typedef struct
{
  char* name;
  BBMXSmodelopts opts;
  int supports_tilt;
  int supports_pan;
  int supports_white;
} BBMXSmodel;

typedef struct
{
  uint8_t channel_mode;
  uint8_t universe;
  char* name;
  BBMXSmodel* model;
  BBMXScolor color;
  uint8_t brightness;
  float tilt;
  float pan;
  DMXChannel address;
} BBMXSfixture;

typedef struct
{
  char* name;
  BBMXSfixture** fixtures;
  uint16_t fixtureCount;
  BBMXScolor color;
  uint8_t brightness;
  float tilt;
  float pan;
} BBMXSgroup;

typedef struct
{
  char* name;
  float t;
  int used;
} BBMXStimedfunc;

typedef struct
{
  char* name;
  BBMXScolor color;
  float t;
  int used;
  float speed;
} BBMXStimedflash;

typedef struct
{
  BBMXSbool debugMode;
  BBMXSmodel* models;
  uint16_t modelCount;
  char* port;
  BBMXSfixture* fixtures;
  uint16_t fixtureCount;
  BBMXStimedfunc* timedFunctions;
  size_t timedFunctionCount;
  BBMXStimedflash* timedFlashes;
  size_t timedFlashCount;
  char* sndFile;
  float bpm;
  int bpm_resolution;
} BBMXSinitargs;

typedef struct
{
  BBMXSbool debugMode;
  BBMXSmodel* models;
  uint16_t modelCount;
  char* port;
  BBMXSfixture* fixtures;
  uint16_t fixtureCount;
  BBMXStimedfunc* timedFunctions;
  size_t timedFunctionCount;
  BBMXStimedflash* timedFlashes;
  size_t timedFlashCount;
  char* sndFile;
  float bpm;
  int bpm_resolution;
  float beat_time;
} BBMXScontext;

BBMXScontext* bbmxs_init(BBMXSinitargs* initargs);
void bbmxs_close();
int bbmxs_load_models();
BBMXSmodel* bbmxs_get_model(const char* name);
BBMXSfixture* bbmxs_get_fx(const char* name);
void bbmxs_fx_update_color(BBMXSfixture* fx);
int bbmxs_send_command(BBMXScmd cmd, void* data, size_t size);
BBMXScontext* bbmxs_get_cur_ctx();

#endif // __BBMXS_H