
//usr GCC builtin atomic operations
//#include <stdatomic.h>

inline static uint32_t pic_atomicinc(pic_atomic_t *p)
{
    //return (*p)++;
    return __sync_fetch_and_add((uint32_t*) p,1)+1;
}

inline static uint32_t pic_atomicdec(pic_atomic_t *p)
{
    //return (*p)--;
    return __sync_fetch_and_sub((uint32_t*)p,1)-1;
}

inline static int pic_atomiccas(pic_atomic_t *p, pic_atomic_t oval, pic_atomic_t nval)
{
    //if(*p==oval) { *p=nval;return 1;}
    //return *p==oval;
    return __sync_bool_compare_and_swap((uint32_t*) p,(uint32_t) oval,(uint32_t) nval);
}

inline static int pic_atomicptrcas(void *p, void *oval, void *nval)
{
    //void **pv=(void**) p;
    ////if(*pv==oval) { *pv=nval;return 1;}
    ////return *p==oval;
    return __sync_bool_compare_and_swap((uint32_t*) p,(uint32_t)oval,(uint32_t)nval);
}

