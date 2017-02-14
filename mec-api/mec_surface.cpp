#include "mec_surface.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
//
// mapping between surfaces
//

namespace mec {



Surface::Surface(int surfaceId) :
    surfaceId_(surfaceId) {
    ;
}

Surface::~Surface() {
    ;
}

void    Surface::load(Preferences& prefs) {

}

int Surface::getId() {
    return surfaceId_;
}


const float UNDEFINED_VALUE = -1.0f;
const float UNDEFINED_SURFACE = -1;

////////////////////////////// SplitSurface ////////////////////////////////////////


SplitSurface::SplitSurface(int surfaceId) :
    Surface(surfaceId),
    minX_(UNDEFINED_VALUE) , maxX_(UNDEFINED_VALUE),
    minY_(UNDEFINED_VALUE) , maxY_(UNDEFINED_VALUE),
    minZ_(UNDEFINED_VALUE) , maxZ_(UNDEFINED_VALUE),
    minR_(UNDEFINED_VALUE) , maxR_(UNDEFINED_VALUE),
    minC_(UNDEFINED_VALUE) , maxC_(UNDEFINED_VALUE),
    surfaceA_(UNDEFINED_SURFACE), surfaceB_(UNDEFINED_SURFACE) {
    ;
}

SplitSurface::~SplitSurface() {
    ;
}

void    SplitSurface::load(Preferences& prefs) {
    // float minX_, maxX_;
    // float minY_, maxY_;
    // float minZ_, maxZ_;
    // float minR_, maxR_;
    // float minC_, maxC_;

    // int surfaceA_, surfaceB_;
}

Touch SplitSurface::map(const Touch& t) {
    return t;
}


////////////////////////////// JoinedSurface ////////////////////////////////////////


JoinedSurface::JoinedSurface(int surfaceId) :
    Surface(surfaceId),
    surfaceA_(UNDEFINED_SURFACE), surfaceB_(UNDEFINED_SURFACE) {
    ;
}

JoinedSurface::~JoinedSurface() {
    ;
}

void    JoinedSurface::load(Preferences& prefs) {

}

Touch JoinedSurface::map(const Touch& t)  {
    return t;
}

} // namespace

