
#include "c74_max.h"

using namespace c74::max;

#include <mec_api.h>

/////////////////////////////////////////////
/// this forms the max external
/////////////////////////////////////////////

#include <queue>
#include <iostream>
const double TICK_TIME=10;

struct TouchMsg
{
    int t;
    float n;
    float x;
    float y;
    float z;
};

std::queue<TouchMsg*> TouchMessages;


TouchMsg* nextTouchMsg() 
{
    if (TouchMessages.empty()) return NULL;
    
    TouchMsg *pM=TouchMessages.front();
    TouchMessages.pop();
    return pM;
} 


class MecMaxCallback: public IMecCallback
{
public:
    MecMaxCallback()
    {
    }
    ~MecMaxCallback()
    {
    }
    
    virtual void touchOn(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void touchContinue(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void touchOff(int touchId, float note, float x, float y, float z)
    {
        struct TouchMsg* m= new struct TouchMsg;
        m->t=touchId;
        m->n=note;
        m->x=x;
        m->y=y;
        m->z=z;
        queuePush(m);
    }
    
    virtual void control(int ctrlId, float v)
    {
        // not yet implemented
    }

private:
    void queuePush(TouchMsg* m)
    {
//        std::lock_guard<std::mutex> lock(queue_mutex);
        TouchMessages.push(m);
    }
};



struct t_mec
{
    t_object ob;
    MecApi *pApi;
    MecMaxCallback *pCallback;
    bool running; // current running status
    bool stop;    // request to stop
    t_clock *clock;
    void* outlet;
};

static t_class* this_class = nullptr;

static t_symbol* t_off;
static t_symbol* t_red;
static t_symbol* t_orange;
static t_symbol* t_green;




void mec_quittask(void* arg)
{
    
}


void outputTouch(t_mec *self, struct TouchMsg* m )
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
    outlet_list(self->outlet,0L,LIST_SIZE,myList);
}


void mec_tick(t_mec *self)
{
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "tick invalid object"); return;}

    if(self->stop)
    {
        return;
    }

    if(self->pApi) self->pApi->process();

    struct TouchMsg* pMsg;
    do
    {
        pMsg=nextTouchMsg();
        if(pMsg)
        {
            // object_post((t_object*)self, "msg z %f", pMsg->z);
            outputTouch(self,pMsg);
            delete pMsg;
        }
    }
    while(pMsg);
 
    clock_fdelay(self->clock,TICK_TIME);
}



void *mec_new(t_symbol* name, long argc, t_atom* argv)
{
    t_mec* self = (t_mec*) object_alloc(this_class);
    self->pApi=nullptr;
    self->running=false;
    self->stop=false;
    self->clock=clock_new(self, (method)mec_tick);
    self->outlet=outlet_new(self,NULL);
    return self;
}


void mec_free(t_mec* self)
{
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "free invalid object"); return;}

    object_post((t_object*)self, "mec_free");
    delete self->pApi;
    self->pApi = nullptr;

    if(OB_INVALID(self->clock)) object_warn((t_object*) self, "clock invalid - free");
    else 
    {
        if(self->running) {
            clock_unset(self->clock);
        }
       freeobject(self->clock);
    }
}


void mec_start(t_mec* self)
{ 
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "start invalid object"); return;}

    if(self->running) {
        object_warn((t_object*)self, "already started");
        return;
    }
    object_post((t_object*)self, "mec_start");
    if(self->pApi) 
    {
        if(self->pCallback) 
        {
            self->pApi->unsubscribe(self->pCallback);
            delete self->pCallback; 
            self->pCallback = 0;
        }
        delete self->pApi;
    }

    self->pApi = new MecApi("/Users/kodiak/Library/Application Support/MEC/mec.json");
    self->pCallback = new MecMaxCallback();
    self->pApi->subscribe(self->pCallback);

    self->pApi->init();
    self->running=true;
    self->stop=false;

    if(OB_INVALID(self->clock)) object_warn((t_object*) self, "clock invalid - start");
    else clock_fdelay(self->clock,TICK_TIME);
}
  

void mec_stop(t_mec* self)
{
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "stop invalid object"); return;}

    if(!self->running) {
        object_warn((t_object*)self, "not started ");
        return;
    }

    if(OB_INVALID(self->clock)) 
        object_warn((t_object*) self, "clock invalid - stop");
    else 
        clock_unset(self->clock);

    self->stop=true;
    self->running=false;

    self->pApi->unsubscribe(self->pCallback);
    if(self->pCallback) delete self->pCallback; 
    self->pCallback = 0;

    delete self->pApi;
    self->pApi = nullptr;
    object_post((t_object*)self, "mec_stop");
}

void mec_error(t_mec* self)
{
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "error invalid object"); return;}

    object_error((t_object*)self, "I say error ");
}


void mec_setled (t_mec* self, t_symbol* s, short ac, t_atom* av)
{
    if(OB_INVALID((t_object*)self)) {object_error((t_object*)self, "setled invalid object"); return;}

    object_post((t_object*)self, "mec_setled");
}


void ext_main(void* r)
{
    this_class = class_new( "mec-max", (method)mec_new, (method)mec_free, sizeof(t_mec), 0L, A_GIMME, 0);

    class_addmethod(this_class, (method)mec_start,   "start",    0);
    class_addmethod(this_class, (method)mec_stop,    "stop",     0);
    class_addmethod(this_class, (method)mec_error,   "error",     0);
    class_addmethod(this_class, (method)mec_setled,  "setled",   A_GIMME, 0);

    class_register(CLASS_BOX, this_class);

    t_off=gensym("off");
    t_red=gensym("red");
    t_orange=gensym("orange");
    t_green=gensym("green");
    
    quittask_install((method)mec_quittask,0);
}








