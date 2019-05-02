#include <Bela.h>

#include <mec_api.h>
#include <math.h>

mec::MecApi* 	gMecApi=NULL;
class BelaMecCallback;
BelaMecCallback* 	gMecCallback=NULL;


AuxiliaryTask gMecProcessTask;


class BelaMecCallback : public mec::Callback {
public:
	BelaMecCallback()  {
		;
	}
	
    void touchOn(int touchId, float note, float x, float y, float z) override {
    	// rt_printf("touchOn %i , %f, %f %f %f\n", touchId,note,x,y,z);
    	if(note>=1024) {
			button(note-1024,true); 		
    	} else {
			if(!active_) {
				active_=true;
				touchId_=touchId;
				note_=note;
				x_=x;
				y_=y;
				z_=z;
			}
    	}
    }

    void touchContinue(int touchId, float note, float x, float y, float z) override {
    	// rt_printf("touchContinue %i , %f, %f %f %f\n", touchId,note,x,y,z);
	   	if(note>=1024) {
    		;
    	} else {
			if(active_ && touchId==touchId_) {
				note_=note;
				x_=x;
				y_=y;
				z_=z;
			}
	    }
    }

    void touchOff(int touchId, float note, float x, float y, float z) override {
    	// rt_printf("touchOff %i , %f, %f %f %f\n", touchId,note,x,y,z);
	   	if(note>=1024) {
			button(note-1024,false); 		
	    } else {
			if(active_ && touchId==touchId_) {
				active_=false;
				note_=note;
				x_=x;
				y_=y;
				z_=z;
			}
	    }
    }
    
    void control(int ctrlId, float v) override {
    	// rt_printf("control %i , %f\n", ctrlId,v);
    	switch (ctrlId) {
    		case 0 : {
    			breath_=v;
    			break;
    		}
    		case 17 : {
    			ribbon_=v;
    			break;
    		}
    	}
    }
    void button(unsigned butId, bool state) {
    	// rt_printf("button %i , %i\n", butId,state);
    	// pico = 48,49,50,51
    }
    
    void render(BelaContext *context) {
		float a0=transpose(note_,0,0);
		float a1=active_;
		float a2=scaleY(y_,1.0f);
		float a3=pressure(z_,1.0f);
		float a4=breath_;
		float a5=ribbon_;
		float a6= 0.0f;
		float a7= 0.0f;
		
		for(unsigned int n = 0; n < context->analogFrames; n++) {
			analogWriteOnce(context, n, 0,a0);
			analogWriteOnce(context, n, 1,a1);
			analogWriteOnce(context, n, 2,a2);
			analogWriteOnce(context, n, 3,a3);
			analogWriteOnce(context, n, 4,a4);
			analogWriteOnce(context, n, 5,a5);
			analogWriteOnce(context, n, 6,a6);
			analogWriteOnce(context, n, 7,a7);
		}    	
    }
    
    
private: 

	static constexpr float PRESSURE_CURVE=0.5f;

	float audioAmp(float z, float mult ) {
		return powf(z, PRESSURE_CURVE) * mult;
	}

	float pressure(float z, float mult ) {
		return ( (powf(z, PRESSURE_CURVE) * mult * ( 1.0f-ZERO_OFFSET) ) ) + ZERO_OFFSET;	
	}
	

	float scaleY(float y, float mult) {
		return ( (y * mult)  * ( 1.0f-ZERO_OFFSET) )  + ZERO_OFFSET ;	
	}

	float scaleX(float x, float mult) {
		return ( (x * mult)  * ( 1.0f-ZERO_OFFSET) )  + ZERO_OFFSET ;	
	}
	
	float transpose (float pitch, int octave, int semi) {
		return pitch + (((( START_OCTAVE + octave) * 12 ) + semi) *  semiMult_ );
	}



#ifdef SALT
	static constexpr float 	OUT_VOLT_RANGE=10.0f;
	static constexpr float 	ZERO_OFFSET=0.5f;
	static constexpr int   	START_OCTAVE=5;
#else 
	static constexpr float 	OUT_VOLT_RANGE=5.0f;
	static constexpr float 	ZERO_OFFSET=0;
	static constexpr int 	START_OCTAVE=1.0f;
#endif 

	static constexpr float semiMult_ = (1.0f / (OUT_VOLT_RANGE * 12.0f)); // 1.0 = 10v = 10 octaves 

	float touchId_;   
    float note_;
    float x_,y_,z_;
    float active_;

    float breath_;
    float ribbon_;
    

private:
};

void mecProcess(void* pvMec) {
	mec::MecApi *pMecApi = (mec::MecApi*) pvMec;
	pMecApi->process();
}


bool setup(BelaContext *context, void *userData) {
	gMecApi=new mec::MecApi();
	gMecCallback=new BelaMecCallback();
	gMecApi->init();
	gMecApi->subscribe(gMecCallback);
	
    // Initialise auxiliary tasks

	if((gMecProcessTask = Bela_createAuxiliaryTask(&mecProcess, BELA_AUDIO_PRIORITY - 1, "mecProcess", gMecApi)) == 0)
		return false;

	return true;
}

// render is called 2750 per second (44000/16)
const int decimation = 5;  // = 550/seconds
long renderFrame = 0;
void render(BelaContext *context, void *userData)
{
	Bela_scheduleAuxiliaryTask(gMecProcessTask);
	
	renderFrame++;
	// silence audio buffer
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, 0.0f);
		}
	}
	
	gMecCallback->render(context);
	
	
	if(decimation <= 1 || ((renderFrame % decimation) ==0) ) {	
	}

}

void cleanup(BelaContext *context, void *userData)
{
	gMecApi->unsubscribe(gMecCallback);
	delete gMecCallback;
	delete gMecApi;
}