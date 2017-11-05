#ifndef MEC_SCALER_H
#define MEC_SCALER_H

#include "mec_prefs.h"
#include "mec_api.h"

#include <vector>
#include <map>

// NOT USED YET INTERFACE EXPERIMENT ONLY
// 
// Scaler is used to map a surface to a musical output .. notes
// e.g. r/c  to note

// row/column
// consider ROW as a guitar string (so string N is offset N* rowOffset)
// consider columns as a positon (like a fret) on that string. (its fretless, i.e. fractional position)
// note: presently linear interp between scale positions

// scale can be chromatic or not, intervales are determined in float
// e.g.
// major : 0.0, 2.0, 4.0, 5.0, 7.0, 9.0, 11.0, 12.0
// minor  : 0.0, 2.0, 3.0, 5.0, 7.0, 8.0, 10.0, 12.0
// chromatic: 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0
// notes:
// a 12 note scales has 13 entries, we need this for the last interval, and also octave size.
// we dont care what the last number is, it just has to be same tone as 0.0, but an octave higher
// we use linear interp beween notes in scale



namespace mec {

typedef std::vector<float> ScaleArray;

class Scales {
public:
    Scales();
    virtual ~Scales();
    bool load(const Preferences &prefs);

    // static/singleton interface
    static const ScaleArray &getScale(const std::string &name);
    static bool init(const Preferences &);

private:
    std::map<std::string, ScaleArray> scales_;
};


class Scaler {
public:
    Scaler();
    virtual ~Scaler();
    bool load(const Preferences &prefs);

    virtual MusicalTouch map(const Touch &t) const;

    float getTonic() const;
    float getRowOffset() const;
    float getColumnOffset() const;
    const ScaleArray &getScale() const;

    void setTonic(float);
    void setRowOffset(float);
    void setColumnOffset(float);

    void setScale(const ScaleArray &scale);
    void setScale(const std::string &name);

private:
    float tonic_;
    float rowOffset_;
    float columnOffset_;

    ScaleArray scale_;
};

}

#endif //MEC_SCALER_H