#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const char* utils_get_file_ext(const char* filename)
{
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

char* utils_read_file_to_string(const char* path)
{
  FILE* f = fopen(path, "r");
  if (f == NULL) return NULL;

  fseek(f, 0L, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0L, SEEK_SET);

  char* buf = malloc(size + 1);
  fread(buf, size, 1, f);
  buf[size] = 0;

  fclose(f);
  return buf;
}

char* utils_str_replace(char* orig, char* rep, char* with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}
