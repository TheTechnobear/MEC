
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

#ifndef __PIC_FLOATUTILS__
#define __PIC_FLOATUTILS__

#include <cmath>
#include <picross/pic_stdint.h>
#include <picross/pic_config.h>

#ifdef PI_MACOSX
#include <Accelerate/Accelerate.h>
#endif

#ifdef PI_WINDOWS
#include <float.h>
#include <memory.h>
#endif

#define PIC_PI   3.14159265358979323846f
#define PIC_LN10 2.30258509299404590109f
#define PIC_ONEOVERLN10 0.43429448190325176116f

namespace pic
{
    inline float interp(float xv, const float *table, unsigned size)
    {
        if(!(xv>0.f)) return table[0];

        unsigned iv = (unsigned)xv;
        unsigned nv = iv+1;

        if(!(nv<size)) return table[size-1];

        float dv = xv-iv;
        return (1.f-dv)*table[iv]+dv*table[nv];
    }

    namespace vector
    {
        inline float dotprod(const float *a, unsigned sa, const float *b, unsigned sb, unsigned n)
        {
            float ret = 0;
#ifdef PI_MACOSX
            vDSP_dotpr(a,sa,b,sb,&ret,n);
#else
            while(n>0)
            {
                ret += ((*a)*(*b));
                a+=sa; b+=sb;
                --n;
            }
#endif
            return ret;
        }

        inline void vectadd(const float *a, unsigned sa, const float *b, unsigned sb, float *c, unsigned sc, unsigned n)
        {
#ifdef PI_MACOSX
        vDSP_vadd(a,sa,b,sb,c,sc,n);
#else
            while(n>0)
            {
                *c = (*a)+(*b);
                a+=sa;
                b+=sb;
                c+=sc;
                --n;
            }
#endif
        }

        inline void vectmul(const float *a, unsigned sa, const float *b, unsigned sb, float *c, unsigned sc, unsigned n)
        {
#ifdef PI_MACOSX
            vDSP_vmul((a),(sa),(b),(sb),(c),(sc),(n));
#else
            while(n>0)
            {
                *c = (*a)*(*b);
                a+=sa;
                b+=sb;
                c+=sc;
                --n;
            }
#endif
        }

        inline void vectscalmul(const float *a, unsigned sa, const float *b, float *c, unsigned sc, unsigned n)
        {
#ifdef PI_MACOSX
            vDSP_vsmul((a),(sa),(b),(c),(sc),(n));
#else
            while(n>0)
            {
                *c = (*a)*(*b);
                a+=sa;
                c+=sc;
                --n;
            }
#endif
        }

        inline void vectmuladd(const float *a, unsigned sa, const float *b, const float *c, unsigned sc, float *d, unsigned sd, unsigned n)
        {
#ifdef PI_MACOSX
            vDSP_vsma(a,sa,b,c,sc,d,sd,n);
#else
            while(n>0)
            {
                *d = (*a)*(*b)+(*c);
                a+=sa;
                c+=sc;
                d+=sd;
                --n;
            }
#endif
        }

        inline void vectdiv(float *a, unsigned sa, float *b, unsigned sb, float *c, unsigned sc, unsigned n)
        {
#ifdef PI_MACOSX
            vDSP_vdiv((a),(sa),(b),(sb),(c),(sc),(n));
#else
            while(n>0)
            {
                *c = (*b)/(*a);
                a+=sa;
                b+=sb;
                c+=sc;
                --n;
            }
#endif
        }

        inline void vectasm(const float *a, unsigned sa, const float *b, unsigned sb, const float *c, float *d, unsigned sd, unsigned n)
        {
#ifdef PI_MACOSX
            vDSP_vasm((float *)(a),(sa),(float *)(b),(sb),(float *)(c),(d),(sd),(n));
#else
            float c_ = *c;
            while(n>0)
            {
                *d = ((*a)+(*b))*c_;
                a+=sa;
                b+=sb;
                d+=sd;
                --n;
            }
#endif
        }
    }

    namespace approx
    {
        inline float exp(float x)
        {
            // good for -1..1
            return (6.f+x*(6.f+x*(3.f+x)))*0.16666666f; 
        }

        inline float tanh(float x)
        {
            // valid only from -3..3
            float x2=x*x;
            return x*(27.f+x2)/(27.f+9.f*x2);
        }

        inline float ln(float x)
        {
            float y=(x-1.f)/(x+1.f);
            float y2=y*y;
            return 2.f*y*(15.f-4.f*y2)/(15.f-9*y2);
        }

        inline float pow2(float x)
        {
            union
            {
                float f;
                int32_t i;
            } c;

            int ix=(int)floorf(x);
            float dx=x-(float)ix;

            c.i=(ix+127)<<23;
            c.f*=(0.33977f*dx*dx+(1.f-0.33977f)*dx+1);

            return c.f;
        }

        inline float pow(float x, float y)
        {
            return pic::approx::exp(pic::approx::ln(x)*y);
        }
    }

#ifndef PI_WINDOWS
    template<typename T>
    inline int isnormal(T f) { return std::isnormal(f); }

    template<typename T>
    inline int isnan(T f) { return std::isnan(f); }
#else
    // isnormal(float) and isnan(float) missing on windows

    template<typename T>
    inline int isnormal(T f) { return (f!=0); }

    template<>
    inline int isnormal(double f) { return _fpclass(f)==_FPCLASS_PN || _fpclass(f)==_FPCLASS_NN; }

    template<>
    inline int isnormal(float f)
    {
        // stolen from boost
        static const uint32_t exponent = 0x7f800000;
        uint32_t bits;
        memcpy(&bits,&f,4);
        bits &= exponent;
        return (bits!=0) && (bits<exponent);
    }

    template<typename T>
    inline int isnan(T f) { return false; }

    template<>
    inline int isnan(double f) { return _isnan(f); }

    template<>
    inline int isnan(float f) { return _isnan(static_cast<double>(f)); }

#endif
}
 
#ifndef PI_WINDOWS
#define pic_log2(n) log2(n)
#define pic_asinh(n) asinh(n)
#else
// log2 missing on windows
inline double pic_log2(double n) { return log(n)/log(2.0); }
inline double pic_asinh(double n) { return log(n+sqrt(n*n+1)); }
#endif

#endif
