#ifndef __BBMXS_SERIAL_H
#define __BBMXS_SERIAL_H

#include <ctype.h>

int serial_open(const char* port);
void serial_close();
int serial_write(char* buf, size_t len);
int serial_read(char* buf, size_t len);

#endif // __BBMXS_SERIAL_H