#include <mec_api.h>

#include <cassert>
#include <iostream>

#include <mec_surface.h>
#include <mec_prefs.h>
#include <mec_log.h>

int main (int argc, char** argv) {
    LOG_0("test started");

    mec::Preferences prefs("../mec-api/tests/test.json");
    assert(prefs.valid());
    mec::Preferences mec_prefs(prefs.getSubTree("mec"));
    assert(mec_prefs.valid());


    // validate prefs file
    mec::Preferences sm_prefs(mec_prefs.getSubTree("surfaces"));
    assert(sm_prefs.valid());

    mec::SurfaceManager mgr;
    assert(mgr.init(sm_prefs));

    mec::Touch t;
    mec::Touch out;

    // simple split
    std::shared_ptr<mec::Surface> split1 = mgr.getSurface("1");
    assert(split1!=nullptr);
    t.surface_ = "a1";
    t.x_ = 0.1f;
    out = split1->map(t);
    assert(out.x_ == t.x_);
    assert(out.surface_ == "10");
    t.x_ = 0.8f;
    out = split1->map(t);
    assert(out.x_ == 0.3f);
    assert(out.surface_ == "11");



    // simple join
    std::shared_ptr<mec::Surface> join1 = mgr.getSurface("2");
    assert(join1!=nullptr);
    t.x_ = 0.1f;
    t.surface_ = "20";
    out = join1->map(t);
    assert(out.x_ == t.x_);
    assert(out.surface_ == "2");
    t.surface_ = "21";
    out = join1->map(t);
    assert(out.x_ == 1.1f);
    assert(out.surface_ == "2");

    LOG_0("test completed");
    return 0;    
}