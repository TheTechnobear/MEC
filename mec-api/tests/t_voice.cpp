#include <mec_api.h>

#include <cassert>
#include <iostream>

#include <mec_voice.h>
#include <mec_prefs.h>
#include <mec_log.h>

int main (int argc, char** argv) {
    LOG_0("test started");

    mec::Preferences prefs("../mec-api/tests/test.json");
    assert(prefs.valid());
    mec::Preferences mec_prefs(prefs.getSubTree("mec"));
    assert(mec_prefs.valid());

    mec::Voices voices(3, 2); // 3 voices , 2 vel count

    mec::Voices::Voice * v;
    v = voices.startVoice(1);
    v = voices.startVoice(2);
    v = voices.startVoice(3);

    assert(voices.oldestActiveVoice()->id_ == 1);

    assert(voices.startVoice(4) == nullptr);
    voices.stopVoice(voices.oldestActiveVoice());
    voices.startVoice(4);
    assert(voices.oldestActiveVoice()->id_ == 2);

    LOG_0("test completed");
    return 0;
}