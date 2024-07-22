#include "bbmxs/bbmxs.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "utils.h"
#include <json.h>
#include "bbmxs/serial.h"

static BBMXSmodel* __models;
static int __models_len;
static BBMXScontext __cur_ctx;

static void copy_data_to_context(BBMXSinitargs* initargs)
{
  __cur_ctx.debugMode = initargs->debugMode;
  __cur_ctx.fixtureCount = initargs->fixtureCount;
  __cur_ctx.fixtures = initargs->fixtures;
  __cur_ctx.modelCount = initargs->modelCount;
  __cur_ctx.models = initargs->models;
  __cur_ctx.port = initargs->port;
  __cur_ctx.timedFunctions = initargs->timedFunctions;
  __cur_ctx.timedFunctionCount = initargs->timedFunctionCount;
  __cur_ctx.timedFlashes = initargs->timedFlashes;
  __cur_ctx.timedFlashCount = initargs->timedFlashCount;
  __cur_ctx.sndFile = initargs->sndFile;
  __cur_ctx.bpm = initargs->bpm;
  __cur_ctx.bpm_resolution = initargs->bpm_resolution;
  if (__cur_ctx.bpm > 0)
  {
    __cur_ctx.beat_time = 60000 / __cur_ctx.bpm;
  }
}

BBMXScontext* bbmxs_init(BBMXSinitargs* initargs)
{
  copy_data_to_context(initargs);

  if (!serial_open(__cur_ctx.port))
  {
    printf("bbmxs Error: Failed to open COM port: \"%s\"\n", __cur_ctx.port);
    return NULL;
  }
  if (gDebugMode) printf("[DEBUG]: Opened COM port: \"%s\"\n", __cur_ctx.port);

  return &__cur_ctx;
}

void bbmxs_close()
{
  serial_close();

  for (int i = 0; i < __cur_ctx.fixtureCount; i++)
  {
    BBMXSfixture* fx = &__cur_ctx.fixtures[i];
    free(fx->name);
  }
  free(__cur_ctx.fixtures);

  for (int i = 0; i < __cur_ctx.modelCount; i++)
  {
    BBMXSmodel* fx = &__cur_ctx.models[i];
    free(fx->name);
  }
  free(__cur_ctx.models);
  free(__cur_ctx.port);

  for (int i = 0; i < __cur_ctx.timedFunctionCount; i++)
  {
    free(__cur_ctx.timedFunctions[i].name);
  }

  if (__cur_ctx.timedFunctions != NULL)
  {
    free(__cur_ctx.timedFunctions);
  }

  for (int i = 0; i < __cur_ctx.timedFlashCount; i++)
  {
    free(__cur_ctx.timedFlashes[i].name);
  }

  if (__cur_ctx.timedFlashes != NULL)
  {
    free(__cur_ctx.timedFlashes);
  }

  if (__cur_ctx.sndFile != NULL)
  {
    free(__cur_ctx.sndFile);
  }
}

static int load_model(char* fileName, json_object* obj, int idx)
{
  BBMXSmodel model;
  BBMXSmodelopts opts;
  BBMXSchannelconfig ch_cfg;

  const char* name = json_object_get_string(json_object_object_get(obj, "name"));
  if (name == NULL)
  {
    printf("bbmxs Error: Missing 'name' in: \"%s\"\n", fileName);
    return 0;
  }

  char* modelName = malloc(strlen(name) + 1);
  memcpy(modelName, name, strlen(name));
  modelName[strlen(name)] = 0;

  model.name = modelName;

  json_object* ch_modes_obj = json_object_object_get(obj, "channel_modes");
  int len = json_object_array_length(ch_modes_obj);
  opts.channelModesLen = len;

  for (int i = 0; i < len; i++)
  {
    opts.channelModes[i] = json_object_get_uint64(ch_modes_obj);
  }

  json_object* channels_obj = json_object_object_get(obj, "channels");

  ch_cfg.ch_red = json_object_get_uint64(json_object_object_get(channels_obj, "red"));
  ch_cfg.ch_green = json_object_get_uint64(json_object_object_get(channels_obj, "green"));
  ch_cfg.ch_blue = json_object_get_uint64(json_object_object_get(channels_obj, "blue"));
  ch_cfg.ch_white = json_object_get_uint64(json_object_object_get(channels_obj, "white"));
  ch_cfg.ch_tilt = json_object_get_uint64(json_object_object_get(channels_obj, "tilt"));
  ch_cfg.ch_pan = json_object_get_uint64(json_object_object_get(channels_obj, "pan"));
  ch_cfg.ch_mtr_spd = json_object_get_uint64(json_object_object_get(channels_obj, "motor_speed"));
  ch_cfg.ch_brgt = json_object_get_uint64(json_object_object_get(channels_obj, "brightness"));

  opts.max_tilt = json_object_get_double(json_object_object_get(obj, "max_tilt"));
  opts.max_pan = json_object_get_double(json_object_object_get(obj, "max_pan"));

  opts.ch_cfg = ch_cfg;
  model.opts = opts;

  __models[idx] = model;

  return 1;
}

int bbmxs_load_models()
{
  HANDLE handle;
  WIN32_FIND_DATA findData;
  ZeroMemory(&findData, sizeof(WIN32_FIND_DATA));

  handle = FindFirstFile("models\\*", &findData);
  if (handle == INVALID_HANDLE_VALUE)
  {
    printf("bbmxs Error: Unable to search directory 'models' (WinAPI Error: %d)\n", GetLastError());
    return 0;
  }

  __models = malloc(sizeof(BBMXSmodel) * gMaxModels);

  int i = 0;
  while (FindNextFile(handle, &findData))
  {
    if (findData.cFileName[0] == '.' || strcmp(utils_get_file_ext(findData.cFileName), "json") != 0) continue;
    char fileName[MAX_PATH] = "models\\";
    strcat(fileName, findData.cFileName);

    json_object* obj = json_object_from_file(fileName);
    if (obj == NULL)
    {
      printf("bbmxs Error: Failed to parse json file: \"%s\"\n", fileName);
      continue;
    }

    if (!load_model(fileName, obj, i))
    {
      printf("bbmxs Error: Failed to load model: \"%s\"\n", fileName);
    }

    json_object_put(obj);

    i++;
  }

  __models_len = i + 1;

  return 1;
}

BBMXSmodel* bbmxs_get_model(const char* name)
{
  for (int i = 0; i < __models_len; i++)
  {
    BBMXSmodel* model = &__models[i];
    if (strcmp(model->name, name) == 0)
    {
      return model;
    }
  }

  return NULL;
}

BBMXSfixture* bbmxs_get_fx(const char* name)
{
  for (int i = 0; i < __cur_ctx.fixtureCount; i++)
  {
    BBMXSfixture* fx = &__cur_ctx.fixtures[i];
    if (strcmp(fx->name, name) == 0)
    {
      return fx;
    }
  }

  return NULL;
}

void bbmxs_fx_update_color(BBMXSfixture* fx)
{
  char buf[9];
  buf[0] = 4; // number of writes in this command
  buf[1] = fx->model->opts.ch_cfg.ch_red;
  buf[2] = fx->color.r;
  buf[3] = fx->model->opts.ch_cfg.ch_green;
  buf[4] = fx->color.g;
  buf[5] = fx->model->opts.ch_cfg.ch_blue;
  buf[6] = fx->color.b;
  buf[7] = fx->model->opts.ch_cfg.ch_white;
  buf[8] = fx->color.w;

  bbmxs_send_command(BBMXS_CMD_DMX_WRITE, buf, sizeof(buf));
}

void __read()
{
  uint8_t buf[64];
  ZeroMemory(buf, sizeof(buf));
  serial_read(buf, sizeof(buf));
  buf[63] = 0;

  printf("Read: \"%s\"\n", buf);
}

int bbmxs_send_command(BBMXScmd cmd, void* data, size_t size)
{
  switch (cmd)
  {
    case BBMXS_CMD_DMX_WRITE:
    {
      uint8_t* _data = (uint8_t*)data;
      uint8_t buf[64];
      buf[0] = size + 1;
      buf[1] = BBMXS_CMD_DMX_WRITE;
      memcpy(&buf[2], _data, size);
      serial_write(buf, size + 2);
    }
  }

  //__read();
  uint8_t receivedCmd = -1;
  serial_read(&receivedCmd, sizeof(receivedCmd));

  if (receivedCmd != cmd)
  {
    printf("bbmxs Warning: Received command is not the sent command!\n");
    return 1;
  }

  return 1;
}

BBMXScontext* bbmxs_get_cur_ctx()
{
  return &__cur_ctx;
}
