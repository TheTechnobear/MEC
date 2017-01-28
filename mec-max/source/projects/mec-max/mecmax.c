#include "ext.h"
#include "ext_common.h"
#include "ext_obex.h"
#include "ext_time.h"
#include "ext_itm.h"

/////////////////////////////////////////////
/// this forms the max external
/////////////////////////////////////////////

#include "mecmax.h"

typedef struct mec
{
	t_object d_obj;
    void *m_impl;
    void *m_outlet;
    bool m_running; // current running status
    bool m_stop;    // request to stop
    void *m_clock;
    t_systhread_mutex   m_mutex;
    
} t_mec;



void *mec_new(t_symbol *s, long argc, t_atom *argv);
void mec_free(t_mec *x);

void mec_start(t_mec *x);
void mec_stop(t_mec *x);
void mec_tick (t_mec *x);
void mec_setled (t_mec *x, t_symbol *s, short ac, t_atom *av);


void mec_quittask(void* arg);

static t_class *s_mec_class = NULL;

static t_symbol* t_off;
static t_symbol* t_red;
static t_symbol* t_orange;
static t_symbol* t_green;


int C74_EXPORT main(void)
{
    t_class *c = class_new(	"mec", (method)mec_new, (method)mec_free, sizeof(t_mec), (method)0L, 0);
    
    class_addmethod(c, (method)mec_start,   "start",    0);
    class_addmethod(c, (method)mec_stop,	"stop",     0);
    class_addmethod(c, (method)mec_setled,  "setled",   A_GIMME,0);
    
    class_register(CLASS_BOX, c);
    
    s_mec_class = c;
    
    t_off=gensym("off");
    t_red=gensym("red");
    t_orange=gensym("orange");
    t_green=gensym("green");
    
    quittask_install((method)mec_quittask,0);
    return 0;
}


void logMsgToMax(const char* msg)
{
    post(msg);
}

void outputTouch(t_mec *x, struct TouchMsg* m )
{
    const int LIST_SIZE=5;
    if (m==NULL) return;
    
    t_atom myList[LIST_SIZE];
    float theNumbers[LIST_SIZE];
    int i;
    
    theNumbers[0] = m->t;
    theNumbers[1] = m->n;
    theNumbers[2] = m->x;
    theNumbers[3] = m->y;
    theNumbers[4] = m->z;
    for (i=0; i < LIST_SIZE; i++) {
        atom_setfloat(myList+i,theNumbers[i]);
    }
    outlet_list(x->m_outlet,0L,LIST_SIZE,myList);
}


void *mec_new()
{
	t_mec *x = (t_mec *)object_alloc(s_mec_class);
    x->m_outlet=listout(x);
    x->m_running=false;
    x->m_stop=false;
    x->m_impl=NULL;
    x->m_clock=clock_new(x, (method)mec_tick);
    systhread_mutex_new(&x->m_mutex,SYSTHREAD_MUTEX_NORMAL);
	return x;
}


void mec_free(t_mec *x)
{
    mec_stop(x);
    freeobject((t_object *)x->m_clock);
    //clock_free(x->m_clock);
    systhread_mutex_free(x->m_mutex);
}

void mec_quittask(void* arg)
{
    
}

void mec_start(t_mec *x)
{
    if(x->m_running) return;
    if(x->m_impl==NULL)
    {
        x->m_impl=createMec(logMsgToMax,"./");
    }
    x->m_running=true;
    x->m_stop=false;
    startMec(x->m_impl);
    clock_fdelay(x->m_clock,0.0);
}

void mec_stop(t_mec *x)
{
    systhread_mutex_lock(x->m_mutex);
    clock_unset(x->m_clock);
    x->m_stop=true;
    systhread_mutex_unlock(x->m_mutex);
    if(x->m_impl)
    {
        x->m_running=false;
        stopMec(x->m_impl);
        destroyMec(x->m_impl);
        x->m_impl=NULL;
    }

}

//debug code, for testing how often we get called
//static int lastcount=0;
//static t_uint32 lasttime=0;
//static t_uint64 avgtime=0;
//static t_uint32 maxtime=0;

void mec_tick(t_mec *x)
{
    if (NOGOOD(x)) return;
    
    systhread_mutex_lock(x->m_mutex);
//    t_uint32 t=systime_ms();
//    
//    if(lasttime==0) lasttime=t;
//    
//    t_uint32 dt=t-lasttime;
//    avgtime+=dt;
//    lastcount++;
//    if(dt>maxtime)
//    {
//        maxtime=dt;
//    }
//    lasttime=t;
//    if(lastcount>10000)
//    {
//        float a=avgtime / (lastcount *1.0);
//        object_post(&x->d_obj,"tick avg %f max %u", a,maxtime);
//        avgtime=0;
//        maxtime=0;
//        lastcount=0;
//    }
    
    
    if(x->m_stop)
    {
        systhread_mutex_unlock(x->m_mutex);
        return;
    }

    //lasttime=systime_ms();
    pollMec(x->m_impl);
    struct TouchMsg* pMsg;
    do
    {
        pMsg=nextTouchMsg();
        if(pMsg!=NULL)
        {
            outputTouch(x,pMsg);
            handledTouch(pMsg);
        }
    }
    while(pMsg!=NULL);
 
    clock_fdelay(x->m_clock,0.0);
    systhread_mutex_unlock(x->m_mutex);
}


void mec_setled (t_mec *x, t_symbol *s, short ac, t_atom *av)
{
    if(x->m_impl==NULL || x->m_running==false)
    {
        object_error(&x->d_obj,"mec not started");
        return;
    }
    
    int colour=-1;
    if(ac!=2)
    {
        object_error(&x->d_obj,"invalid setled message, needs 2 args");
        return;
    }
    if(av[0].a_type!=A_LONG)
    {
        object_error(&x->d_obj,"invalid setled message, arg 1 - type != long");
        return;
    }
    if(av[1].a_type != A_SYM)
    {
        object_error(&x->d_obj,"invalid setled message, arg 2 - type != symbol");
        return;
    }
    if(av[1].a_w.w_sym ==t_off)
    {
        colour=0;
    }
    else if(av[1].a_w.w_sym ==t_green)
    {
        colour=1;
    }
    else if(av[1].a_w.w_sym==t_red)
    {
        colour=2;
    }
    else if(av[1].a_w.w_sym ==t_orange)
    {
        colour=3;
    }

    if(colour<0)
    {
        object_error(&x->d_obj,"invalid colour");
        return;
    }
    setMecLed(x->m_impl,(int) av[0].a_w.w_long, (int) colour);
}


