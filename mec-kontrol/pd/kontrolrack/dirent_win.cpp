#include "dirent_win.h"

#ifdef _WIN32

#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <utility>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void dirent_init(dirent &d, int type, const std::string &name)
{
    d.d_type = type;
    strncpy(d.d_name, name.c_str(), std::min(sizeof(dirent::d_name) - 1, name.size() + 1));
    d.d_name[sizeof(dirent::d_name) - 1] = '\0';
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

static std::wstring toWideString(const std::string &str)
{
    std::wstring result;
    int n = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), str.size(), NULL, 0);

    if (n != 0)
    {
        wchar_t *w = new wchar_t[n];

        if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, str.c_str(), str.size(), w, n) != 0)
        {
            result = std::wstring(w, n);
        }

        delete[] w;
    }

    return result;
}

static std::string toUtf8String(const std::wstring &str)
{
    std::string result;
    int n = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(), NULL, 0, NULL, NULL);

    if (n != 0)
    {
        char *c = new char[n];

        if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(), c, n, NULL, NULL) != 0)
        {
            result = std::string(c, n);
        }

        delete[] c;
    }

    return result;
}

// Limited implementation, without filter support and only recognizing directories.
int scandir(const char *dirp, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **))
{
    std::wstring directory = toWideString(dirp);

    directory = directory + L"\\*";

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(directory.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    std::vector<std::pair<std::string, int> > files;

    do
    {
        int type = DT_UNKNOWN;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            type = DT_DIR;

        files.push_back(std::make_pair(toUtf8String(findFileData.cFileName), type));
    } while (FindNextFileW(hFind, &findFileData));

    FindClose(hFind);

    *namelist = (dirent**)malloc(sizeof(dirent*) * files.size());

    int i = 0;
    for (std::vector<std::pair<std::string, int> >::iterator itr = files.begin(); itr != files.end(); ++itr)
    {
        (*namelist)[i] = (dirent*)malloc(sizeof(dirent));
        dirent_init(*(*namelist)[i], itr->second, itr->first);
        ++i;
    }

    qsort(*namelist, files.size(), sizeof(dirent*), (int(*)(const void*, const void*))compar);

    return files.size();
}

#endif // _WIN32
