#ifndef MEC_VOICES_H_
#define MEC_VOICES_H_

#include <vector>
#include <list>

class MecVoices {
public:
    MecVoices(unsigned voiceCount = 15, unsigned velocityCount = 5) 
     : maxVoices_(voiceCount), velocityCount_(velocityCount){
        voices_.resize(maxVoices_);
        for (int i = 0; i < maxVoices_; i++) {
            voices_[i].i_=i;
            voices_[i].state_ = Voice::INACTIVE;
            voices_[i].id_ = -1;
			freeVoices_.push_back(&voices_[i]);
        }

    };
    virtual ~MecVoices() {};


    struct Voice {
        int i_;
        int id_;
        float note_;
        float x_;
        float y_;
        float z_;
        float v_;
        unsigned long t_;
        enum {
            INACTIVE,
            PENDING, // velocity
            ACTIVE,
        } state_;

        //velocity
        float velSum_;
        int velCount_;
    };

    Voice*     voiceId(unsigned id) {
        for (int i = 0; i < maxVoices_; i++) {
            if (voices_[i].id_ == id)
                return &voices_[i];
        }
        return NULL;
    }

    Voice*    startVoice(unsigned id) {
        Voice* voice;
        if(freeVoices_.size() > 0) {
           voice = freeVoices_.back();
           freeVoices_.pop_back();
        } else { // all voices used, so we have to steal the oldest one
           voice = usedVoices_.front();
           usedVoices_.pop_front();
        }
        voice->id_ = id;
        voice->state_ = Voice::PENDING;
        voice->v_=0;
        voice->velSum_ = 0;
        voice->velCount_ = 0;
        usedVoices_.push_back(voice);

        return voice;
    }

    void   addPressure(Voice* voice, float p) {
        if(voice->state_ == Voice::PENDING) {
            voice->velSum_ += p;
            voice->velCount_++;
            if(voice->velCount_ == velocityCount_) {
                voice->state_ = Voice::ACTIVE;
                voice->v_ = voice->velSum_ / voice->velCount_; // refine!
            }
        }
    }

    void   stopVoice(Voice* voice) {
        if (!voice) return;
        usedVoices_.remove(voice);
        voice->id_ = -1;
        voice->state_ = Voice::INACTIVE;
        freeVoices_.push_back(voice);
    }


private:
    std::vector<Voice> voices_;
    std::list<Voice*> freeVoices_;
    std::list<Voice*> usedVoices_;
    unsigned maxVoices_;
    unsigned velocityCount_;
};

#endif //MEC_VOICES_H_
