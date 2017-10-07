#include "mec_surfacemapper.h"

#include "mec_log.h"

namespace mec {

SurfaceMapper::SurfaceMapper() : mode_(SM_NoMapping) {
}

void SurfaceMapper::load(Preferences& prefs){
	LOG_2("load surface mapping");

	mode_ = SM_NoMapping;
	if(prefs.exists("notes")) { loadNoteArray(prefs); return ;}
	if(prefs.exists("calculated")) { loadCalcDefinition(prefs); return ;}
}

int SurfaceMapper::noteFromKey(int key) {
	switch (mode_) {
		case SM_NoMapping: 	return key;
		case SM_Notes: 		return notes_[key % MAX_KEYS];
		case SM_Calculated: return ((key / keyInCol_) * colMult_ ) + (( key % keyInCol_ ) * rowMult_)  + noteOffset_;
	}
	return key;
}	

void SurfaceMapper::loadNoteArray(Preferences& prefs) {
	mode_ = SM_Notes;
	Preferences::Array array(prefs.getArray("notes"));
	int sz = array.getSize();
	for(int i = 0; i < sz; i++) {
		notes_[i] = array.getInt(i);
	}
	for(int i=sz;i<MAX_KEYS;i++) {
		notes_[i] = i;
	}

	LOG_2("loaded surface mapping (notes) # keys : " << sz);
}

void SurfaceMapper::loadCalcDefinition(Preferences& p) {
	Preferences prefs(p.getSubTree("calculated"));
	mode_ = SM_Calculated;
	keyInCol_ 	= prefs.getInt("keys in col", 127);
    rowMult_	= prefs.getInt("row multiplier", 1);
    colMult_ 	= prefs.getInt("col multipler", keyInCol_ );
    noteOffset_	= prefs.getInt("note offset", 0); 

    LOG_2("loaded surface mapping (calc) " <<  keyInCol_ << " , " << rowMult_ << " , " << colMult_ << " , " << noteOffset_);

}

}