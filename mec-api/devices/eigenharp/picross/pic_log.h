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

#ifndef __PIC_LOG__
#define __PIC_LOG__

#include "pic_exports.h"

#include <exception>

#include "pic_error.h"
#include "pic_ref.h"
#include "pic_thread.h"
#include "pic_stl.h"

namespace pic
{
    struct PIC_DECLSPEC_CLASS logger_t
    {
        virtual ~logger_t() {}
        virtual void log(const char *) = 0;
        static pic::tsd_t logger__;
        static void tsd_setlogger(logger_t *logger) { logger__.set(logger); }
        static void tsd_clearlogger() { tsd_setlogger(0); }
        static logger_t *tsd_getlogger() { return (pic::logger_t *)(logger__.get()); }

    };

    class PIC_DECLSPEC_CLASS msg_t
    {
        private:
            struct rep_t: public pic::counted_t, public nbostringstream_t
            {
            };

        public:
            msg_t(): rep_(ref(new rep_t)), dispose_(0) {}
            msg_t(void (*f)(const msg_t &)): rep_(ref(new rep_t)), dispose_(f) {}
            msg_t(const msg_t &m) { rep_=m.rep_; dispose_=m.dispose_; }
            ~msg_t() { if(dispose_) dispose_(*this); }

            void clear() { rep_=ref(new rep_t); }
            nbstring_t str() const { return rep_->str(); }
            msg_t &operator<<(void (*f)(const msg_t &)) { f(*this); return *this; }
            msg_t &operator<<(const msg_t &m) { (*rep_) << m.str(); return *this; }
            msg_t &operator<<(logger_t &l) { l.log(str().c_str()); return *this; }
            msg_t &operator<<(logger_t *l) { l->log(str().c_str()); return *this; }
            template <class D> msg_t &operator<<(const D &d) { (*rep_) << d; return *this; }
            template <class D> msg_t &operator<<(D &d) { (*rep_) << d; return *this; }

        private:
            pic::ref_t<rep_t> rep_;
            void (*dispose_)(const msg_t &);
    };

    inline void tsd_log(const char *msg)
    {
        logger_t *logger = logger_t::tsd_getlogger();

        if(logger)
        {
            logger->log(msg);
        }
        else
        {
            std::cerr << "log:" << msg << "\n";
        }
    }


    inline void hurl(const msg_t &m) { PIC_THROW(m.str().c_str()); }
    inline void print(const msg_t &m) { std::cout << m.str() << '\n'; std::cout.flush(); }
    inline void log(const msg_t &m) { tsd_log(m.str().c_str()); }

    inline msg_t msg() { return msg_t(); }
    inline msg_t logmsg() { return msg_t(log); }
    inline msg_t printmsg() { return msg_t(print); }
    inline msg_t hurlmsg() { return msg_t(hurl); }

}

inline std::ostream &operator<<(std::ostream &o, const pic::msg_t &m) { o << m.str(); return o; }

#define CATCHLOG() \
    catch(std::exception &e) \
    { \
        pic::msg() << e.what() << " caught at " << __FILE__ << ':' << __LINE__ << pic::log; \
    } \
    catch(const char *e) \
    { \
        pic::msg() << e << " caught at " << __FILE__ << ':' << __LINE__ << pic::log; \
    } \
    catch(...) \
    { \
        pic::msg() << "unknown exception caught at " << __FILE__ << ':' << __LINE__ << pic::log; \
    }

#define PIC_WARN(e) do { if(!(e)) pic::logmsg() << "warning: " << #e; } while(0)

#endif
