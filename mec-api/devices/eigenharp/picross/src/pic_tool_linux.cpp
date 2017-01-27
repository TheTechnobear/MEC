
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
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <unistd.h>

struct pic::bgprocess_t::impl_t
{
    impl_t(const std::string &dir,const char *name,bool keeprunning): started_(false), name_(name), keeprunning_(keeprunning)
    {
        path_ = dir;
        path_ = path_+"/"+name;
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
            kill(pid_,9);

            while(kill(pid_,0)>=0)
            {
                sleep(1);
            }
            
            waitpid(pid_,0,WNOHANG);
        }
    }

    bool is_running()
    {
        if(started_)
        {
            if(kill(pid_,0)>=0)
            {
                return true;
            }

            started_=false;
        }

        return false;
    }

    void start()
    {
        pid_ = fork();

        if(!pid_) // child
        {
            execl(path_.c_str(),name_.c_str(),(char *)0);
            exit(-1);
        }

        started_=true;
    }

    bool started_;
    std::string name_;
    std::string path_;
    pid_t pid_;
    bool keeprunning_;
};

pic::bgprocess_t::bgprocess_t(const char *dir,const char *name,bool keeprunning)
{
    impl_ = new impl_t(dir,name,keeprunning);
}

pic::bgprocess_t::bgprocess_t(const std::string &dir,const char *name,bool keeprunning)
{
    impl_ = new impl_t(dir,name,keeprunning);
}

pic::bgprocess_t::~bgprocess_t()
{
    delete impl_;
}

bool pic::bgprocess_t::isrunning()
{
    return impl_->is_running();
}

void pic::bgprocess_t::start()
{
    impl_->start();
}

void pic::bgprocess_t::quit()
{
    impl_->quit();
}

struct pic::tool_t::impl_t
{
    impl_t(const std::string &dir,const char *name): started_(false), name_(name)
    {
        path_ = dir;
        path_ = path_+"/"+name;
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
            kill(pid_,9);

            while(kill(pid_,0)>=0)
            {
                sleep(1);
            }
            
            waitpid(pid_,0,WNOHANG);
        }
    }

    bool is_running()
    {
        if(started_)
        {
            if(kill(pid_,0)>=0)
            {
                return true;
            }

            started_=false;
        }

        return false;
    }

    void start()
    {
        pid_ = fork();

        if(!pid_) // child
        {
            execl(path_.c_str(),name_.c_str(),(char *)0);
            exit(-1);
        }

        started_=true;
    }

    bool started_;
    std::string name_;
    std::string path_;
    pid_t pid_;
};

pic::tool_t::tool_t(const char *dir,const char *name)
{
    impl_ = new impl_t(dir,name);
}

pic::tool_t::tool_t(const std::string &dir,const char *name)
{
    impl_ = new impl_t(dir,name);
}

pic::tool_t::~tool_t()
{
    delete impl_;
}

bool pic::tool_t::isrunning()
{
    return impl_->is_running();
}

void pic::tool_t::start()
{
    impl_->start();
}

void pic::tool_t::quit()
{
    impl_->quit();
}

bool pic::tool_t::isavailable()
{
    return true;
}

