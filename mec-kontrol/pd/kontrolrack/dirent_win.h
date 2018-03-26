#pragma once

#ifdef _WIN32

#include <string>

enum
{
    DT_UNKNOWN = 0,
    DT_DIR = 1,
};

struct dirent
{
    int d_type;
    char d_name[512];
};

int alphasort(const struct dirent **a, const struct dirent **b);
int scandir(const char *dirp, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **));

#endif // _WIN32
