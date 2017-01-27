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

#ifndef __PIC_FLIPFLOP_H__
#define __PIC_FLIPFLOP_H__

#include <picross/pic_atomic.h>
#include <picross/pic_nocopy.h>
#include <picross/pic_thread.h>

namespace pic
{
    class flipbase_t: nocopy_t
    {
        private:
            mutable pic_atomic_t _state[2];
            void *_data[2];
            volatile unsigned _alternate;

        public:
            flipbase_t(void *d1, void *d2)
            {
                _data[0]=d1;
                _data[1]=d2;
                _alternate=1;
                _state[1-_alternate]=1;
                _state[_alternate]=0;
            }

            void *acquire() const
            {
                for(;;)
                {
                    unsigned p = 1-_alternate;
                    unsigned c = _state[p];

                    if(c>0 && pic_atomiccas(&(_state[p]),c,c+1))
                    {
                        return _data[p];
                    }
                }
            }

            void release(void *d) const
            {
                if(d==_data[0])
                {
                    for(;;)
                    {
                        unsigned c = _state[0];
                        if(pic_atomiccas(&(_state[0]), c, c-1))
                        {
                            return;
                        }
                    }
                }

                if(d==_data[1])
                {
                    for(;;)
                    {
                        unsigned c = _state[1];
                        if(pic_atomiccas(&(_state[1]), c, c-1))
                        {
                            return;
                        }
                    }
                }
            }

            void exchange(void **a, void **c)
            {
                unsigned alternate = _alternate;
                pic_atomic_t *ps = &_state[1-alternate];
                pic_atomic_t *as = &_state[alternate];
                pic_atomiccas(as, 0, 1);
                alternate = 1-alternate;
                _alternate = alternate;
                while(!pic_atomiccas(ps, 1, 0)) { /* pic_thread_yield(); */ }

                *a = _data[alternate];
                *c = _data[1-alternate];
            }

            void *alternate() { return _data[_alternate]; }
            const void *current() const { return _data[1-_alternate]; }
    };

    class guarded_t
    {
        public:
            virtual ~guarded_t() {};
            virtual void acquire() = 0;
            virtual void release() = 0;
    };

    class untyped_guard_t
    {
        public:
            untyped_guard_t(guarded_t *g): guarded_(g)
            {
                acquire();
            }

            untyped_guard_t(guarded_t &g): guarded_(&g)
            {
                acquire();
            }

            ~untyped_guard_t()
            {
                release();
            }

            void acquire()
            {
                guarded_->acquire();
            }

            void release()
            {
                guarded_->release();
            }

        private:
            guarded_t *guarded_;
    };

    template <class X> class flipflop_t
    {
        public:
            class guard_t
            {
                public:
                    guard_t(const flipflop_t *l): _list(l)
                    {
                        acquire();
                    }

                    guard_t(const flipflop_t &l): _list(&l)
                    {
                        acquire();
                    }

                    ~guard_t()
                    {
                        release();
                    }

                    void acquire()
                    {
                        _data=_list->acquire();
                    }

                    void release()
                    {
                        _list->release(_data);
                        _data=0;
                    }

                    const X &value()
                    {
                        return *_data;
                    }

                private:
                    const flipflop_t *_list;
                    const X *_data;
            };

            class unguard_t
            {
                public:
                    unguard_t(guard_t &g): guard_(g)
                    {
                        guard_.release();
                    }

                    ~unguard_t()
                    {
                        guard_.acquire();
                    }

                private:
                    guard_t &guard_;
            };

        public:
            flipflop_t(): _ff(&_data[0],&_data[1])
            {
            }

            flipflop_t(const X &x): _ff(&_data[0],&_data[1])
            {
                set(x);
            }

            void set(const X &x)
            {
                alternate()=x;
                exchange();
            }

            const X *acquire() const
            {
                return (X *)(_ff.acquire());
            }

            void release(const X *r) const
            {
                _ff.release((void *)r);
            }

            const X &current() const
            {
                return *(const X *)(_ff.current());
            }

            X &alternate()
            {
                return *(X *)(_ff.alternate());
            }

            void exchange()
            {
                void *a,*c;
                _ff.exchange(&a,&c);
                *(X *)(a) = *(const X *)(c);
            }

        private:
            flipbase_t _ff;
            X _data[2];
    };

#define SETMARK(d)  (((uintptr_t)(d))|((uintptr_t)1))
#define ISMARKED(d) ((((uintptr_t)(d))&1)!=0)

template<class T, void (*DATADROP_INCREF)(T), void (*DATADROP_DECREF)(T)>
    class datadrop_t : public nocopy_t
    {
        public:
            datadrop_t() : data_(0) {}

            void set(T d)
            {
                volatile T *ptr = &data_;
                T o;

				DATADROP_INCREF(d);

				do
                {
                    o = *ptr;
                } while(!pic_atomicptrcas(&data_,o,d));

                if(!ISMARKED(o))
                {
                    DATADROP_DECREF(o);
                }
            }

            T get()
            {
                volatile T *ptr = &data_;
                T u,m;

				do
                {
                    u = *ptr;
                    m = (T)SETMARK(u);
                } while(ISMARKED(u) || !pic_atomicptrcas(&data_,u,m));

                DATADROP_INCREF(u);

				if(!pic_atomicptrcas(&data_,m,u))
                {
                    DATADROP_DECREF(u);
                }
                return u;
            }

        private:
            T data_;
    };
};

#endif
