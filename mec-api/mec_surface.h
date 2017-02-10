#ifndef MEC_SURFACE_H
#define MEC_SURFACE_H

#include "mec_prefs.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
// 
// mapping between surfaces
// 

namespace mec {



class Surface {
public:
            Surface();
    virtual ~Surface();
    void    load(Preferences& prefs);

    virtual Touch map(const Touch&);

private:
    int surfaceId_;
};

// for now, only allow split/join A/B A+B
class SplitSurface {
public:
            SplitSurface();
    virtual ~SplitSurface();
    void    load(Preferences& prefs);

    virtual Touch map(const Touch&);

private:
    float minX_, maxX_;
    float minY_, maxY_;
    float minZ_, maxZ_;
    float minR_, maxR_;
    float minC_, maxC_;

    int surfaceA_, surfaceB_;
};


class JoinedSurface {
public:
            Surface();
    virtual ~Surface();
    void    load(Preferences& prefs);

    virtual Touch map(const Touch&);

private:
    int surfaceA_, surfaceB_;
};

}

#endif //MEC_SURFACE_H