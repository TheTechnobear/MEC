#ifndef MEC_SURFACE_MAPPER_H
#define MEC_SURFACE_MAPPER_H

#include "mec_prefs.h"

const int MAX_KEYS = 256;


/*
This class takes a devices 'key' and converts it into a midi note

a couple of configurations are supported so far in a preferences file

an array of note values, one for each key -  "notes" : [1,2,3]
*/

class MecSurfaceMapper {
public:
	MecSurfaceMapper();
	int noteFromKey(int key);
	void load(MecPreferences& prefs);
private:
    void loadNoteArray(MecPreferences& prefs);
    void loadCalcDefinition(MecPreferences& prefs);

    enum mode {
        SM_NoMapping,
        SM_Notes,
        SM_Calculated
    } mode_;

    //note mode
	int	notes_[MAX_KEYS];

    //calc mode
    // r = k % keyInCol , c = k / keyInCol, note = (r * rowM) + (c * colM) + offset  
    int keyInCol_;
    int rowMult_;
    int colMult_;
    int noteOffset_; 
};

#endif //MEC_SURFACE_MAPPER_H