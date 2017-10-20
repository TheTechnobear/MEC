#include <mec_api.h>
#include <iostream>

#include <cassert>
#include <mec_scaler.h>
#include <mec_log.h>

void dumpScale(const mec::ScaleArray& a) {
    for (float n : a) {
        std::cout << n << "," ;
    }
    std::cout << std::endl;
}


int main (int argc, char** argv) {
    LOG_0("test started");

    mec::Preferences prefs("../mec-api/tests/test.json");
    assert(prefs.valid());
    mec::Preferences mec_prefs(prefs.getSubTree("mec"));
    assert(mec_prefs.valid());

    // validate prefs file
    mec::Preferences scale_prefs(mec_prefs.getSubTree("scales"));
    assert(scale_prefs.valid());
    assert(scale_prefs.getArray("major") != nullptr);
    assert(scale_prefs.getArray("squirrel") == nullptr);

    assert(mec::Scales::init(scale_prefs));


    // mec::ScaleArray scale = mec::Scales::getScale("major");
    // dumpScale(scale);

    mec::Scaler scaler;
    // dumpScale(scaler.getScale());
    mec::ScaleArray odd = mec::ScaleArray( {0.0f, 2.5f, 3.1f, 5.2f, 7.5f, 8.6f, 10.0f, 12.2f} );
    scaler.setScale(odd);
    // dumpScale(scaler.getScale());
    assert(scaler.getScale().size() == 8);
    assert(scaler.getScale()[2] == 3.1f);


    mec::ScaleArray chromatic = mec::Scales::getScale("chromatic") ;
    scaler.setScale(chromatic);

    mec::Touch t;
    mec::MusicalTouch mt;

    t.r_ = 0.0f;
    t.c_ = 26.0f;
    mt = scaler.map(t);
    assert(mt.note_ == t.c_);
    t.c_ = 48.62f;
    mt = scaler.map(t);
    assert(mt.note_ == t.c_);

    scaler.setScale("major");
    t.c_ = 10.0f;
    mt = scaler.map(t);
    assert(mt.note_ == 17.0f);
    t.c_ = 9.5f;
    mt = scaler.map(t);
    assert(mt.note_ == 16.5f);
    t.c_ = 10.5f;
    mt = scaler.map(t);
    assert(mt.note_ == 18.0f);

    scaler.setRowOffset(5.0f);
    scaler.setColumnOffset(1.0f);
    t.r_ = 1;
    t.c_ = 10.5f;
    mt = scaler.map(t);
    assert(mt.note_ == 24.0f);

    mec::Preferences s1(mec_prefs.getSubTree("scaler 1"));
    assert(scaler.load(s1));
    assert(scaler.getRowOffset() == 4.0f);
    t.r_ = 1;
    t.c_ = 8.5f;
    mt = scaler.map(t);
    // 12 + 2.5 + 4.0 (row o) + 1.0 (col o)
    assert(mt.note_ == 19.5f);


    LOG_0("test completed");
    return 0;
}