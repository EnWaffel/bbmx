#include "utils.h"
#include <ctype.h>
#include <string.h>

const char* utils_get_file_ext(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

char* utils_to_lower(char* str)
{
  for(int i = 0; str[i]; i++){
    str[i] = tolower(str[i]);
  }
  return str;
}
