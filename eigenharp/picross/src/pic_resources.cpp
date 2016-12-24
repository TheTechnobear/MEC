
/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <picross/pic_config.h>

#ifdef PI_WINDOWS
#define UNICODE 1
#define _UNICODE 1
#define RES_SEPERATOR '\\'
#define RES_SEPERATOR_STR "\\"
#define RES_PATH_MAX MAX_PATH
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#else
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#define RES_SEPERATOR '/'
#define RES_SEPERATOR_STR "/"
#define RES_PATH_MAX PATH_MAX
#endif

#if defined(PI_LINUX) || defined(PI_MACOSX)
#include <unistd.h>
#endif

#include <picross/pic_resources.h>
#include <string.h>
#include <fcntl.h>

namespace
{
    static bool is_debug();

    static void dirname(char *buffer)
    {
        char *p = strrchr(buffer,RES_SEPERATOR);

        if(p)
        {
            *p = 0;
        }
        else
        {
            buffer[0] = RES_SEPERATOR;
            buffer[1] = 0;
        }
    }

#ifdef PI_WINDOWS
    static void get_exe(char *buffer)
    {
        WCHAR dest [RES_PATH_MAX+1];
        HINSTANCE moduleHandle = GetModuleHandle(0);
        GetModuleFileName (moduleHandle, dest, RES_PATH_MAX+1);
        WideCharToMultiByte(CP_UTF8,0,dest,-1,buffer,RES_PATH_MAX+1,0,0);
    }

    static void get_prefix(char *buffer)
    {
        if(is_debug())
        {
            WCHAR dest [RES_PATH_MAX+1];
            SHGetSpecialFolderPath (0, dest, CSIDL_PROGRAM_FILES, 0);
            WideCharToMultiByte(CP_UTF8,0,dest,-1,buffer,RES_PATH_MAX+1,0,0);
            strcat(buffer,RES_SEPERATOR_STR);
            strcat(buffer,"Eigenlabs");
        }
        else
        {
            get_exe(buffer); // .../release-1.0/bin/xx.exe
            dirname(buffer); // .../release-1.0/bin
            dirname(buffer); // .../release-1.0
            dirname(buffer); // ...
        }
    }

    static void get_lib(char *buffer)
    {
        WCHAR dest [RES_PATH_MAX+1];
        SHGetSpecialFolderPath (0, dest, CSIDL_PERSONAL, 0);
        WideCharToMultiByte(CP_UTF8,0,dest,-1,buffer,RES_PATH_MAX+1,0,0);
        strcat(buffer,RES_SEPERATOR_STR);
        strcat(buffer,"Eigenlabs");
        pic::mkdir(buffer);
    }

    static void get_pyprefix(char *buffer)
    {
        get_prefix(buffer);
        strcat(buffer,RES_SEPERATOR_STR);
        strcat(buffer,"runtime-1.0.0");
        strcat(buffer,RES_SEPERATOR_STR);
        strcat(buffer,"Python26");
    }

    static void get_pubtool(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

    static void get_pritool(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

    static void get_priexe(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

#endif

#ifdef PI_MACOSX
    static void get_exe(char *buffer)
    {
        Dl_info exeInfo;
        dladdr ((const void*) get_exe, &exeInfo);
        realpath(exeInfo.dli_fname,buffer);
    }

    static void get_prefix(char *buffer)
    {
        if(is_debug())
        {
            strcpy(buffer,"/usr/local/pi");
        }
        else
        {
            get_exe(buffer); // .../release-1.0/bin/xx.exe
            dirname(buffer); // .../release-1.0/bin
            dirname(buffer); // .../release-1.0
            dirname(buffer); // ...
        }
    }

    static void get_lib(char *buffer)
    {
        strcpy(buffer,getenv("HOME"));
        strcat(buffer,"/Library/Eigenlabs");
        pic::mkdir(buffer);
    }

    static void get_pyprefix(char *buffer)
    {
        strcpy(buffer,PI_PREFIX);
    }

    static void get_pubtool(char *buffer)
    {
        if(!is_debug())
        {
            strcpy(buffer,"/Applications/Eigenlabs/");
            strcat(buffer,PI_RELEASE);
        }
        else
        {
            get_exe(buffer);
            dirname(buffer);
            dirname(buffer);
            strcat(buffer,RES_SEPERATOR_STR);
            strcat(buffer,"app");
        }
    }

    static void get_pritool(char *buffer)
    {
        if(is_debug())
        {
            get_exe(buffer);
            dirname(buffer);
            dirname(buffer);
            strcat(buffer,RES_SEPERATOR_STR);
            strcat(buffer,"app");
        }
        else
        {
            get_exe(buffer);
            dirname(buffer);
            dirname(buffer);
            strcat(buffer,RES_SEPERATOR_STR);
            strcat(buffer,"Applications");
        }
    }

    static void get_priexe(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }


#endif

#ifdef PI_LINUX
    static void get_exe(char *buffer)
    {
        Dl_info exeInfo;
        dladdr ((const void*) get_exe, &exeInfo);
        if(!realpath(exeInfo.dli_fname,buffer)) buffer[0]=0;
    }

    static void get_lib(char *buffer)
    {
        strcpy(buffer,getenv("HOME"));
        strcat(buffer,"/.Eigenlabs");
        pic::mkdir(buffer);
    }

    static void get_prefix(char *buffer)
    {
        if(is_debug())
        {
            strcpy(buffer,"/usr/local/pi");
        }
        else
        {
            get_exe(buffer); // .../release-1.0/bin/xx.exe
            dirname(buffer); // .../release-1.0/bin
            dirname(buffer); // .../release-1.0
            dirname(buffer); // ...
        }
    }

    static void get_pyprefix(char *buffer)
    {
        strcpy(buffer,"/usr");
    }

    static void get_pubtool(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

    static void get_pritool(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

    static void get_priexe(char *buffer)
    {
        get_exe(buffer);
        dirname(buffer);
    }

#endif
    static int __is_debug = 0;

    bool is_debug()
    {
        char buffer[RES_PATH_MAX+1];

        if(__is_debug != 0)
        {
            return (__is_debug>0);
        }

        get_exe(buffer); // .../release-1.0/bin/xx.exe
        dirname(buffer); // .../release-1.0/bin
        dirname(buffer); // .../release-1.0

        char *p = strrchr(buffer,RES_SEPERATOR);
        
        if(!p)
        {
            __is_debug=1;
            return true;
        }

        if(p[0] && !strcmp(&p[1],"tmp"))
        {
            __is_debug=1;
            return true;
        }

        __is_debug=-1;
        return false;
    }
};

char pic::platform_seperator()
{
    return RES_SEPERATOR;
}

std::string pic::global_resource_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_prefix(buffer);
    return buffer;
}

std::string pic::python_prefix_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_pyprefix(buffer);
    return buffer;
}

std::string pic::contrib_root_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_prefix(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,"contrib-");
    strcat(buffer,PI_RELEASE);
    return buffer;
}

std::string pic::contrib_compatible_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_prefix(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,"contrib-");
    strcat(buffer,PI_COMPATIBLE);
    return buffer;
}

std::string pic::release_root_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_exe(buffer);
    dirname(buffer);
    dirname(buffer);
    return buffer;
}

std::string pic::release_resource_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_exe(buffer);
    dirname(buffer);
    dirname(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,"resources");
    return buffer;
}

std::string pic::global_library_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_lib(buffer);
    return buffer;
}


std::string pic::release_library_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_lib(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,PI_RELEASE);
    return buffer;
}

std::string pic::public_tools_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_pubtool(buffer);
    return buffer;
}

std::string pic::private_tools_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_pritool(buffer);
    return buffer;
}

std::string pic::private_exe_dir()
{
    char buffer[RES_PATH_MAX+1];
    get_priexe(buffer);
    return buffer;
}

std::string pic::release()
{
    return PI_RELEASE;
}

std::string pic::lockfile(const std::string &name)
{
    char buffer[RES_PATH_MAX+1];
    get_lib(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,"Lock");
    pic::mkdir(buffer);
    strcat(buffer,RES_SEPERATOR_STR);
    strcat(buffer,name.c_str());
    strcat(buffer,".lck");
    return buffer;
}

std::string pic::username()
{
#ifdef PI_WINDOWS
    return getenv("USERNAME");
#else
    return getenv("USER");
#endif
}

int pic::mkdir(std::string dirname)
{
    return pic::mkdir(dirname.c_str());
}

int pic::mkdir(const char *dirname)
{
#ifndef PI_WINDOWS
    return ::mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
    int wchars_num = MultiByteToWideChar(CP_UTF8,0,dirname,-1,NULL,0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8,0,dirname,-1,wstr,wchars_num);

    int result = ::_wmkdir(wstr);

    delete[] wstr;

    return result;
#endif
}

int pic::remove(std::string path)
{
    return pic::remove(path.c_str());
}

int pic::remove(const char *path)
{
#ifndef PI_WINDOWS
    return ::remove(path);
#else
    int wchars_num = MultiByteToWideChar(CP_UTF8,0,path,-1,NULL,0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8,0,path,-1,wstr,wchars_num);

    int result = ::_wremove(wstr);

    delete[] wstr;

    return result;
#endif
}

FILE *pic::fopen(std::string filename, const char *mode)
{
    return pic::fopen(filename.c_str(), mode);
}

FILE *pic::fopen(const char *filename, const char *mode)
{
#ifndef PI_WINDOWS
    return ::fopen(filename, mode);
#else
    int wchars_num1 = MultiByteToWideChar(CP_UTF8,0,filename,-1,NULL,0);
    wchar_t* wstr1 = new wchar_t[wchars_num1];
    MultiByteToWideChar(CP_UTF8,0,filename,-1,wstr1,wchars_num1);

    int wchars_num2 = MultiByteToWideChar(CP_UTF8,0,mode,-1,NULL,0);
    wchar_t* wstr2 = new wchar_t[wchars_num2];
    MultiByteToWideChar(CP_UTF8,0,mode,-1,wstr2,wchars_num2);

    FILE *file = ::_wfopen(wstr1, wstr2);

    delete[] wstr1;
    delete[] wstr2;

    return file;
#endif
}

int pic::open(std::string filename, int oflags)
{
    return pic::open(filename, oflags, 0);
}

int pic::open(const char *filename, int oflags)
{
    return pic::open(filename, oflags, 0);
}

int pic::open(std::string filename, int oflags, int cflags)
{
    return pic::open(filename.c_str(), oflags);
}

int pic::open(const char *filename, int oflags, int cflags)
{
#ifndef PI_WINDOWS
    if(oflags & O_CREAT)
    {
        return ::open(filename, oflags, cflags);
    }
    else
    {
        return ::open(filename, oflags);
    }
#else
    int wchars_num = MultiByteToWideChar(CP_UTF8,0,filename,-1,NULL,0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8,0,filename,-1,wstr,wchars_num);

    int result;
    if(oflags & O_CREAT)
    {
        result = ::_wopen(wstr, oflags, cflags);
    }
    else
    {
        result = ::_wopen(wstr, oflags);
    }

    delete[] wstr;

    return result;
#endif
}

#ifndef PI_WINDOWS

struct pic::lockfile_t::impl_t
{
    impl_t(const std::string &name): locked_(false)
    {
        fd_ = open(name.c_str(),O_RDWR|O_CREAT,0777);
        printf("open lock file %s %d\n",name.c_str(),fd_);
    }

    ~impl_t()
    {
        if(fd_>=0)
            ::close(fd_);
    }

    bool lock()
    {
        if(locked_)
        {
            return true;
        }

        if(fd_ < 0)
        {
            return false;
        }

        if(0==flock(fd_,LOCK_EX|LOCK_NB))
        {
            printf("locked lock file\n");
            locked_ = true;
            return true;
        }

        return false;
    }

    int fd_;
    bool locked_;
};

#else

struct pic::lockfile_t::impl_t
{
    impl_t(const std::string &name): locked_(false)
    {
        int wchars_num = MultiByteToWideChar(CP_UTF8,0,name.c_str(),-1,NULL,0);
        WCHAR wstr [RES_PATH_MAX+1];
        MultiByteToWideChar(CP_UTF8,0,name.c_str(),-1,wstr,wchars_num);

        fd_ = CreateFile(wstr,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,0,NULL);
    }

    ~impl_t()
    {
        if(fd_ != INVALID_HANDLE_VALUE)
            CloseHandle(fd_);
    }

    bool lock()
    {
        if(locked_)
        {
            return true;
        }

        if(fd_ == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        if(LockFile(fd_,0,0,1,0))
        {
            locked_ = true;
            return true;
        }

        return false;
    }

    HANDLE fd_;
    bool locked_;
};

#endif

pic::lockfile_t::lockfile_t(const std::string &name): name_(pic::lockfile(name)), impl_(0)
{
}

pic::lockfile_t::~lockfile_t()
{
    if(impl_) delete impl_;
}

bool pic::lockfile_t::lock()
{
    if(!impl_)
    {
        impl_ = new impl_t(name_);
    }

    return impl_->lock();
}

void pic::lockfile_t::unlock()
{
    if(impl_)
    {
        delete impl_;
        impl_ = 0;
    }
}

