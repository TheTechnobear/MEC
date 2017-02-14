#include <mec_api.h>

#include <cassert>
#include <iostream>

#include <mec_surface.h>
#include <mec_prefs.h>

int main (int argc, char** argv) {
    mec::Preferences prefs("../mec-api/tests/test.json");
    assert(prefs.valid());
    mec::Preferences mec_prefs(prefs.getSubTree("mec"));
    assert(mec_prefs.valid());

    mec::JoinedSurface js(1);
    assert(js.getId() == 1);


    mec::SplitSurface ss(2);
    assert(ss.getId() == 2);

    std::cout << "test completed" << std::endl;
    return 0;    
}