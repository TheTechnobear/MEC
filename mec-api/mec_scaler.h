#ifndef MEC_SCALER_H
#define MEC_SCALER_H

#include "mec_prefs.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
// 
// Scaler is used to map a surface to a musical output .. notes
// e.g. r/c x/y to note
// 
// scale can be chromatic or not, intervales are determined in float
// e.g.
//  major : 0.0, 2.0, 4.0, 5.0, 7.0, 9.0, 11.0, 12.0
//  minor  : 0.0, 2.0, 3.0, 5.0. 7.0, 8.0, 10.0, 12.0
//  chromatic: 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0
  
namespace mec {

struct MusicalTouch : public Touch {
    float note_;
};



class Scaler {

public:
            Scaler();
    virtual ~Scaler();
    void  load(Preferences& prefs);

    virtual MusicalTouch mapToNote(const Touch& t);
 
    void  setScale(float[] notes);
private:

};

}

#endif //MEC_SCALER_H