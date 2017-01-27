
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

#include <picross/pic_tool.h>
#include <string>
#include <windows.h>

struct pic::tool_t::impl_t
{
    impl_t(const std::string &dir,const char *name): started_(false)
    {
        path_ = dir;
        path_ = path_+'\\'+name+".exe";
    }

    ~impl_t()
    {
        quit();
    }

    void quit()
    {
        if(started_)
        {
            started_=false;
            TerminateProcess(info_.hProcess,0);
            CloseHandle(info_.hProcess);
            CloseHandle(info_.hThread);
        }
    }

    bool is_running()
    {
        if(started_)
        {
            DWORD code;

            if(GetExitCodeProcess(info_.hProcess,&code))
            {
                if(code==STILL_ACTIVE)
                {
                    return true;
                }
            }

            started_=false;
            CloseHandle(info_.hProcess);
            CloseHandle(info_.hThread);
        }

        return false;
    }

    void start()
    {
        printf("opening %s\n",path_.c_str());

        STARTUPINFO startup_info;
        memset(&startup_info,0,sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);

        if(CreateProcessA(path_.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &info_))
        {
            started_=true;
        }
        else
        {
            printf("create process failed\n");
        }
    }

    bool is_available()
    {
        if(GetFileAttributesA(path_.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            return true;
        }

        return false;
    }

    std::string path_;
    bool started_;
    PROCESS_INFORMATION info_;
};

pic::tool_t::tool_t(const std::string &dir_env,const char *name)
{
    impl_ = new impl_t(dir_env,name);
}

pic::tool_t::tool_t(const char *dir_env,const char *name)
{
    impl_ = new impl_t(dir_env,name);
}

pic::tool_t::~tool_t()
{
    delete impl_;
}

void pic::tool_t::start()
{
    impl_->start();
}

void pic::tool_t::quit()
{
    impl_->quit();
}

bool pic::tool_t::isrunning()
{
    return impl_->is_running();
}

bool pic::tool_t::isavailable()
{
    return impl_->is_available();
}

struct pic::bgprocess_t::impl_t
{
    impl_t(const std::string &dir,const char *name,bool keeprunning): started_(false), keeprunning_(keeprunning)
    {
        path_ = dir;
        path_ = path_+'\\'+name+".exe";
    }

    ~impl_t()
    {
        if(!keeprunning_)
        {
            quit();
        }
    }

    void quit()
    {
        if(started_)
        {
            started_=false;
            TerminateProcess(info_.hProcess,0);
            CloseHandle(info_.hProcess);
            CloseHandle(info_.hThread);
        }
    }

    bool is_running()
    {
        if(started_)
        {
            DWORD code;

            if(GetExitCodeProcess(info_.hProcess,&code))
            {
                if(code==STILL_ACTIVE)
                {
                    return true;
                }
            }

            started_=false;
            CloseHandle(info_.hProcess);
            CloseHandle(info_.hThread);
        }

        return false;
    }

    void start()
    {
        printf("opening %s\n",path_.c_str());

        STARTUPINFO startup_info;
        memset(&startup_info,0,sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);

        if(CreateProcessA(path_.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &info_))
        {
            started_=true;
        }
        else
        {
            printf("create process failed\n");
        }
    }

    std::string path_;
    bool started_;
    PROCESS_INFORMATION info_;
    bool keeprunning_;
};

pic::bgprocess_t::bgprocess_t(const std::string &dir_env,const char *name,bool keeprunning)
{
    impl_ = new impl_t(dir_env,name,keeprunning);
}

pic::bgprocess_t::bgprocess_t(const char *dir_env,const char *name,bool keeprunning)
{
    impl_ = new impl_t(dir_env,name,keeprunning);
}

pic::bgprocess_t::~bgprocess_t()
{
    delete impl_;
}

void pic::bgprocess_t::start()
{
    impl_->start();
}

void pic::bgprocess_t::quit()
{
    impl_->quit();
}

bool pic::bgprocess_t::isrunning()
{
    return impl_->is_running();
}
