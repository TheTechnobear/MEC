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

#ifndef __PIC_FUNCTOR__
#define __PIC_FUNCTOR__

#include <picross/pic_weak.h>
#include <picross/pic_nocopy.h>
#include <picross/pic_ref.h>
#include <picross/pic_fastalloc.h>
#include <picross/pic_stl.h>
#include <picross/pic_flipflop.h>
#include <list>

namespace pic
{
    template <class RT> struct PIC_DECLSPEC_INLINE_CLASS sigtraits
    {
        typedef RT rt_t;
        typedef struct {} p1_t;
        typedef struct {} p2_t;
        typedef struct {} p3_t;
        struct invoke_t { virtual RT invoke() const = 0; };
    };

    template <class RT> struct PIC_DECLSPEC_INLINE_CLASS sigtraits<RT()>
    {
        typedef RT rt_t;
        typedef struct {} p1_t;
        typedef struct {} p2_t;
        typedef struct {} p3_t;
        struct invoke_t { virtual ~invoke_t() {}; virtual RT invoke() const = 0; };
    };

    template <class RT, class P1> struct PIC_DECLSPEC_INLINE_CLASS sigtraits<RT(P1)>
    {
        typedef RT rt_t;
        typedef P1 p1_t;
        typedef struct {} p2_t;
        typedef struct {} p3_t;
        struct invoke_t { virtual ~invoke_t() {}; virtual RT invoke(p1_t) const = 0; };
    };

    template <class RT, class P1, class P2> struct PIC_DECLSPEC_INLINE_CLASS sigtraits<RT(P1,P2)>
    {
        typedef RT rt_t;
        typedef P1 p1_t;
        typedef P2 p2_t;
        typedef struct {} p3_t;
        struct invoke_t { virtual ~invoke_t() {}; virtual RT invoke(p1_t, p2_t) const = 0; };
    };

    template <class RT, class P1, class P2, class P3> struct PIC_DECLSPEC_INLINE_CLASS sigtraits<RT(P1,P2,P3)>
    {
        typedef RT rt_t;
        typedef P1 p1_t;
        typedef P2 p2_t;
        typedef P3 p3_t;
        struct invoke_t { virtual ~invoke_t() {}; virtual RT invoke(p1_t, p2_t, p3_t) const = 0; };
    };

    template <class SIG> struct PIC_DECLSPEC_INLINE_CLASS sink_t: public sigtraits<SIG>::invoke_t, public nocopy_t, virtual public atomic_counted_t, virtual public pic::lckobject_t
    {
        public:
            sink_t() {}
            virtual ~sink_t() {}
            virtual bool iscallable() const = 0;
            virtual bool compare(const sink_t *s) const { return s==this; }
            virtual int  gc_visit(void *,void *) const { return 0; }
    };

    template <class RT> struct rttraits { static RT def() { RT v; return v; } };
    template <> struct rttraits<void> { static void def() { return; } };
    template <> struct rttraits<bool> { static int def() { return false; } };
    template <> struct rttraits<float> { static float def() { return 0; } };
    template <> struct rttraits<double> { static float def() { return 0; } };
    template <> struct rttraits<char> { static int def() { return 0; } };
    template <class P> struct rttraits<P *> { static int def() { return 0; } };
    template <> struct rttraits<signed char> { static signed char def() { return 0; } };
    template <> struct rttraits<signed short> { static signed short def() { return 0; } };
    template <> struct rttraits<signed int> { static signed int def() { return 0; } };
    template <> struct rttraits<signed long> { static signed long def() { return 0; } };
    template <> struct rttraits<signed long long> { static signed long long def() { return 0; } };
    template <> struct rttraits<unsigned char> { static unsigned char def() { return 0; } };
    template <> struct rttraits<unsigned short> { static unsigned short def() { return 0; } };
    template <> struct rttraits<unsigned int> { static unsigned int def() { return 0; } };
    template <> struct rttraits<unsigned long> { static unsigned long def() { return 0; } };
    template <> struct rttraits<unsigned long long> { static unsigned long long def() { return 0; } };

    template <class RT> struct PIC_DECLSPEC_INLINE_CLASS default_t
    {
        RT operator()() { return rttraits<RT>::def(); }
    };

    template <class SIG, class CALLABLE> class PIC_DECLSPEC_INLINE_CLASS callable_t: public sink_t<SIG>
    {
        public:
            typedef typename sigtraits<SIG>::rt_t rt_t;
            typedef typename sigtraits<SIG>::p1_t p1_t;
            typedef typename sigtraits<SIG>::p2_t p2_t;
            typedef typename sigtraits<SIG>::p3_t p3_t;

            callable_t(CALLABLE f): callable_(f) {}

        public:
            rt_t invoke() const { return callable_(); }
            rt_t invoke(p1_t p1) const { return callable_(p1); }
            rt_t invoke(p1_t p1, p2_t p2) const { return callable_(p1,p2); }
            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const { return callable_(p1,p2,p3); }

            virtual bool iscallable() const { return true; }
            virtual bool compare(const sink_t<SIG> *s) const { const callable_t *c = dynamic_cast<const callable_t *>(s); if(c && c->callable_==callable_) return true; return false; }
            virtual ~callable_t(){}

        private:
            CALLABLE callable_;
    };

    template <class SIG, class O, class MP> class PIC_DECLSPEC_INLINE_CLASS boundmethod_t: public sink_t<SIG>
    {
        public:
            typedef typename sigtraits<SIG>::rt_t rt_t;
            typedef typename sigtraits<SIG>::p1_t p1_t;
            typedef typename sigtraits<SIG>::p2_t p2_t;
            typedef typename sigtraits<SIG>::p3_t p3_t;
            typedef typename weak_t<O>::guard_t wgrd_t;

            boundmethod_t(O *o, MP m): _object(o), _method(m) {}

        public:
            rt_t invoke() const { wgrd_t g(_object); if(!g.value()) return rttraits<rt_t>::def(); return ((*g.value()).*_method)(); }
            rt_t invoke(p1_t p1) const { wgrd_t g(_object); if(!g.value()) return rttraits<rt_t>::def(); return ((*g.value()).*_method)(p1); }
            rt_t invoke(p1_t p1, p2_t p2) const { wgrd_t g(_object); if(!g.value()) return rttraits<rt_t>::def(); return ((*g.value()).*_method)(p1,p2); }
            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const { wgrd_t g(_object); if(!g.value()) return rttraits<rt_t>::def(); return ((*g.value()).*_method)(p1,p2,p3); }

            virtual bool iscallable() const { return true; }

            virtual bool compare(const sink_t<SIG> *s) const
            {
                const boundmethod_t *c = dynamic_cast<const boundmethod_t *>(s);
                if(c && c->_object==_object && c->_method==_method) return true;
                return false;
            }
            virtual ~boundmethod_t(){}

        private:
            weak_t<O> _object;
            MP _method;
    };

    template <class SIG> class PIC_DECLSPEC_INLINE_CLASS indirect_sink_t: public sink_t<SIG>
    {
        public:
            typedef typename sigtraits<SIG>::rt_t rt_t;
            typedef typename sigtraits<SIG>::p1_t p1_t;
            typedef typename sigtraits<SIG>::p2_t p2_t;
            typedef typename sigtraits<SIG>::p3_t p3_t;

            typedef ref_t<sink_t<SIG> > sink_ref_t;
            typedef flipflop_t<sink_ref_t> sink_flipflop_t;
            typedef typename sink_flipflop_t::guard_t sink_guard_t;

            indirect_sink_t(const sink_ref_t &f) { set_target(f); }

            void set_target(const sink_ref_t &f) { target_.set(f); }
            void clear_target() { target_.set(sink_ref_t()); }

            int gc_visit(void *v, void *a) const
            {
                sink_ref_t s(target_.current());

                if(s.isvalid())
                {
                    int r = s->gc_visit(v,a);
                    return r;
                }

                return 0;
            }
            virtual ~indirect_sink_t(){}

        public:
            rt_t invoke() const { sink_guard_t g(target_); if(!g.value().isvalid()) return rttraits<rt_t>::def(); return g.value()->invoke(); }
            rt_t invoke(p1_t p1) const { sink_guard_t g(target_); if(!g.value().isvalid()) return rttraits<rt_t>::def(); return g.value()->invoke(p1); }
            rt_t invoke(p1_t p1, p2_t p2) const { sink_guard_t g(target_); if(!g.value().isvalid()) return rttraits<rt_t>::def(); return g.value()->invoke(p1,p2); }
            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const { sink_guard_t g(target_); if(!g.value().isvalid()) return rttraits<rt_t>::def(); return g.value()->invoke(p1,p2,p3); }

            virtual bool iscallable() const { sink_guard_t g(target_); return g.value().isvalid() && g.value()->iscallable(); }

            virtual bool compare(const sink_t<SIG> *s) const
            {
                if(s && target_.current().isvalid())
                    return s->compare(target_.current().ptr());
                return false;

            }

        private:
            sink_flipflop_t target_;
    };

    template <class SIG> class PIC_DECLSPEC_INLINE_CLASS list_sink_t: public sink_t<SIG>
    {
        public:
            typedef typename sigtraits<SIG>::rt_t rt_t;
            typedef typename sigtraits<SIG>::p1_t p1_t;
            typedef typename sigtraits<SIG>::p2_t p2_t;
            typedef typename sigtraits<SIG>::p3_t p3_t;

            typedef ref_t<sink_t<SIG> > sink_ref_t;
            typedef typename pic::lcklist_t<sink_ref_t>::nbtype sink_list_t;
            typedef typename sink_list_t::iterator sink_iter_t;
            typedef typename sink_list_t::const_iterator sink_const_iter_t;
            typedef flipflop_t<sink_list_t> sink_flipflop_t;
            typedef typename sink_flipflop_t::guard_t sink_guard_t;

            list_sink_t()
            {
            }
            virtual ~list_sink_t(){}

            void connect(const sink_ref_t &f)
            {
                sink_list_t &l(list_.alternate());
                l.push_back(f);
                list_.exchange();
            }

            void disconnect(const sink_ref_t &f)
            {
                sink_list_t &l(list_.alternate());

                for(sink_iter_t i=l.begin(); i!=l.end(); i++)
                {
                    if((*i) == f)
                    {
                        l.erase(i);
                        break;
                    }
                }

                list_.exchange();
            }

            int gc_visit(void *v,void *a) const
            {
                const sink_list_t &l(list_.current());

                for(sink_const_iter_t i=l.begin(); i!=l.end(); i++)
                {
                    int r = (*i)->gc_visit(v,a);

                    if(r)
                    {
                        return r;
                    }
                }

                return 0;
            }

        public:
            rt_t invoke() const
            {
                sink_guard_t g(list_);
                for(sink_const_iter_t i=g.value().begin(); i!=g.value().end(); i++) { (*i)->invoke(); }
                return rttraits<rt_t>::def();
            }

            rt_t invoke(p1_t p1) const
            {
                sink_guard_t g(list_);
                for(sink_const_iter_t i=g.value().begin(); i!=g.value().end(); i++) { (*i)->invoke(p1); }
                return rttraits<rt_t>::def();
            }

            rt_t invoke(p1_t p1, p2_t p2) const
            {
                sink_guard_t g(list_);
                for(sink_const_iter_t i=g.value().begin(); i!=g.value().end(); i++) { (*i)->invoke(p1,p2); }
                return rttraits<rt_t>::def();
            }

            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const
            {
                sink_guard_t g(list_);
                for(sink_const_iter_t i=g.value().begin(); i!=g.value().end(); i++) { (*i)->invoke(p1,p2,p3); }
                return rttraits<rt_t>::def();
            }

            virtual bool iscallable() const { return true; }

        private:
            sink_flipflop_t list_;
    };

    template <class SIG> class PIC_DECLSPEC_INLINE_CLASS functor_t
    {
        public:
            typedef typename sigtraits<SIG>::rt_t rt_t;
            typedef typename sigtraits<SIG>::p1_t p1_t;
            typedef typename sigtraits<SIG>::p2_t p2_t;
            typedef typename sigtraits<SIG>::p3_t p3_t;
            typedef sink_t<SIG> sinktype_t;
            typedef SIG sig_t;

        public:
            functor_t() {}
            functor_t(const functor_t &f): _sink(f._sink) {}
            functor_t &operator=(const functor_t &f) { set(f); return *this; }
            functor_t(const ref_t<sinktype_t> &s): _sink(s) {}

            void set(const functor_t &f) { _sink=f._sink; }
            bool operator==(const functor_t &f) const { return _sink==f._sink; }

            void clear() { _sink.clear(); }
            bool iscallable() const { return _sink.isvalid() && _sink->iscallable(); }

            rt_t invoke() const { if(!iscallable()) return rttraits<rt_t>::def(); return _sink->invoke(); }
            rt_t invoke(p1_t p1) const { if(!iscallable()) return rttraits<rt_t>::def(); return _sink->invoke(p1); }
            rt_t invoke(p1_t p1, p2_t p2) const { if(!iscallable()) return rttraits<rt_t>::def(); return _sink->invoke(p1,p2); }
            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const { if(!iscallable()) return rttraits<rt_t>::def(); return _sink->invoke(p1,p2,p3); }

            rt_t operator()() const { return invoke(); }
            rt_t operator()(p1_t p1) const { return invoke(p1); }
            rt_t operator()(p1_t p1, p2_t p2) const { return invoke(p1,p2); }
            rt_t operator()(p1_t p1, p2_t p2, p3_t p3) const { return invoke(p1,p2,p3); }

            template <class F> static functor_t callable(F f) { return functor_t(ref(new callable_t<SIG,F>(f))); }
            template <class O, class M> static functor_t method(O *o, M m) { return functor_t(ref(new boundmethod_t<SIG,O,M>(o,m))); }
            static functor_t indirect(const functor_t &f) { return functor_t(ref(new indirect_sink_t<SIG>(f.get_sink()))); }
            static functor_t list() { return functor_t(ref(new list_sink_t<SIG>())); }
            ref_t<sinktype_t> get_sink() const { return _sink; }

            int gc_clear()
            {   
                clear();
                return 0;
            }

            int gc_traverse(void *visit, void *arg) const
            {
                if(!_sink.isvalid())
                {
                    return 0;
                }

                return _sink->gc_visit(visit,arg);
            }

        private:
            ref_t<sinktype_t> _sink;
    };

    template<class F> void indirect_clear(F &list)
    {
        indirect_sink_t<typename F::sig_t> *s = dynamic_cast<indirect_sink_t<typename F::sig_t> *>(list.get_sink().ptr());

        if(s)
        {
            s->clear_target();
        }
    }

    template<class F> void indirect_settarget(F &list, const F &tgt)
    {
        indirect_sink_t<typename F::sig_t> *s = dynamic_cast<indirect_sink_t<typename F::sig_t> *>(list.get_sink().ptr());

        if(s)
        {
            s->set_target(tgt.get_sink());
        }
    }

    template<class F> void functorlist_connect(F &list, const F &tgt)
    {
        list_sink_t<typename F::sig_t> *s = dynamic_cast<list_sink_t<typename F::sig_t> *>(list.get_sink().ptr());

        if(s)
        {
            s->connect(tgt.get_sink());
        }
    }

    template<class F> void functorlist_disconnect(F &list, const F &tgt)
    {
        list_sink_t<typename F::sig_t> *s = dynamic_cast<list_sink_t<typename F::sig_t> *>(list.get_sink().ptr());

        if(s)
        {
            s->disconnect(tgt.get_sink());
        }
    }

    typedef functor_t<void()> notify_t;
    typedef functor_t<void(bool)> status_t;
    typedef functor_t<void(int)> f_int_t;
    typedef functor_t<void(const char *)> f_string_t;
    typedef functor_t<float(float,float)> ff2f_t;
    typedef functor_t<float(float)> f2f_t;
    typedef functor_t<float(int)> i2f_t;

    template <class FUNCTOR> class PIC_DECLSPEC_INLINE_CLASS flipflop_functor_t
    {
        public:
            typedef FUNCTOR sink_functor_t;
            typedef typename sink_functor_t::rt_t rt_t;
            typedef typename sink_functor_t::p1_t p1_t;
            typedef typename sink_functor_t::p2_t p2_t;
            typedef typename sink_functor_t::p3_t p3_t;
            typedef flipflop_t<sink_functor_t> sink_flipflop_t;
            typedef typename sink_flipflop_t::guard_t sink_guard_t;

        public:
            flipflop_functor_t() {}
            flipflop_functor_t(const flipflop_functor_t &f): _sink(f._sink) {}
            flipflop_functor_t(const sink_functor_t &f): _sink(f) {}
            flipflop_functor_t &operator=(const flipflop_functor_t &f) { set(f._sink); return *this; }
            flipflop_functor_t &operator=(const sink_functor_t &f) { set(f); return *this; }

            void set(const sink_functor_t &f) { _sink.alternate()=f; _sink.exchange(); }
            void clear() { _sink.alternate().clear(); _sink.exchange(); }

            rt_t invoke() const { sink_guard_t g(_sink); return g.value().invoke(); }
            rt_t invoke(p1_t p1) const { sink_guard_t g(_sink); return g.value().invoke(p1); }
            rt_t invoke(p1_t p1, p2_t p2) const { sink_guard_t g(_sink); return g.value().invoke(p1,p2); }
            rt_t invoke(p1_t p1, p2_t p2, p3_t p3) const { sink_guard_t g(_sink); return g.value().invoke(p1,p2,p3); }

            rt_t operator()() const { return invoke(); }
            rt_t operator()(p1_t p1) const { return invoke(p1); }
            rt_t operator()(p1_t p1, p2_t p2) const { return invoke(p1,p2); }
            rt_t operator()(p1_t p1, p2_t p2, p3_t p3) const { return invoke(p1,p2,p3); }

            int gc_clear()
            {   
                clear();
                return 0;
            }

            int gc_traverse(void *visit, void *arg) const
            {
                return _sink.current().gc_traverse(visit,arg);
            }

        private:
            sink_flipflop_t _sink;
    };

};

#endif
