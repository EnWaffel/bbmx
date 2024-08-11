#ifndef __UTILS_H
#define __UTILS_H

const char* utils_get_file_ext(const char* filename);
char* utils_to_lower(char* str);
char* utils_read_file_to_string(const char* path);
char* utils_str_replace(char* orig, char* rep, char* with);

#endif