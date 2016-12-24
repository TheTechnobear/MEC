#ifndef VOICES_H_
#define VOICES_H_

#define MAX_VOICES 16

#define VELOCITY_COUNT 3

class Voices {
public:
    Voices() {
        for (int i = 0; i < MAX_VOICES; i++) {
            voices_[i].i_=i;
            voices_[i].state_ = Voice::INACTIVE;
            voices_[i].id_ = -1;
        }

    };
    virtual ~Voices() {};


    struct Voice {
        int i_;
        int id_;
        int note_;
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
        for (int i = 0; i < MAX_VOICES; i++) {
            if (voices_[i].id_ == id)
                return &voices_[i];
        }
        return NULL;
    }

    Voice*    startVoice(unsigned id) {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (voices_[i].state_ == Voice::INACTIVE) {
                voices_[i].id_ = id;
                voices_[i].state_ = Voice::PENDING;
                voices_[i].v_=0;
                voices_[i].velSum_ = 0;
                voices_[i].velCount_ = 0;

                return &voices_[i];
            }
        }
        return NULL;
    }

    void   addPressure(Voice* voice, float p) {
        if(voice->state_ == Voice::PENDING) {
            voice->velSum_ += p;
            voice->velCount_++;
            if(voice->velCount_ == VELOCITY_COUNT) {
                voice->state_ = Voice::ACTIVE;
                voice->v_ = voice->velSum_ / voice->velCount_; // refine!
            }
        }
    }

    void   stopVoice(Voice* voice) {
        if (!voice) return;
        for (int i = 0; i < MAX_VOICES; i++) {
            if (voices_[i].id_ == voice->id_) {
                voices_[i].id_ = -1;
                voices_[i].state_ = Voice::INACTIVE;
                return;
            }
        }
    }


private:
    Voice voices_[MAX_VOICES];
};

#endif //VOICES_H_
