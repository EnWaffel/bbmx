#include "bbmxs/serial.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static HANDLE __handle = NULL;

int serial_open(const char* port)
{
  char portName[32] = "\\\\.\\";
  strcat(portName, port);

  __handle = CreateFile(portName, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
  if (__handle == INVALID_HANDLE_VALUE)
  {
    __handle = NULL;
    return 0;
  }

  if (!FlushFileBuffers(__handle))
  {
      serial_close();
      __handle = NULL;
      return 0;
  }

  COMMTIMEOUTS timeouts = { 0 };
  timeouts.ReadIntervalTimeout = 1;
  timeouts.ReadTotalTimeoutConstant = 1000;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 1000;
  timeouts.WriteTotalTimeoutMultiplier = 0;

  if (!SetCommTimeouts(__handle, &timeouts))
  {
      serial_close();
      __handle = NULL;
      return 0;
  }

  DCB state = { 0 };
  state.DCBlength = sizeof(DCB);
  state.BaudRate = CBR_115200;
  state.ByteSize = 8;
  state.Parity = NOPARITY;
  state.StopBits = ONESTOPBIT;

  if (!SetCommState(__handle, &state))
  {
    serial_close();
    __handle = NULL;
    return 0;
  }

  return 1;
}

void serial_close()
{
  if (__handle == NULL) return;
  CloseHandle(__handle);
}

int serial_write(char* buf, size_t len)
{
  int written;
  if (!WriteFile(__handle, buf, len, &written, NULL))
  {
    return -1;
  }

  return written;
}

int serial_read(char* buf, size_t len)
{
  int read;
  if (!ReadFile(__handle, buf, len, &read, NULL))
  {
    return -1;
  }

  return read;
}
