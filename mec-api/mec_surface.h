#ifndef MEC_SURFACE_H
#define MEC_SURFACE_H

#include "mec_api.h"
#include "mec_prefs.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
// 
// mapping between surfaces
// 

namespace mec {



class Surface {
public:
            Surface(int surfaceId);
    virtual ~Surface();
    virtual void    load(Preferences& prefs);

    int getId();

    virtual Touch map(const Touch&)=0;

private:
    int surfaceId_;
};

// for now, only allow split/join A/B A+B
class SplitSurface : public Surface {
public:
            SplitSurface(int surfaceId);
    virtual ~SplitSurface();
    virtual void    load(Preferences& prefs) override;

    virtual Touch map(const Touch&) override;

private:
    float minX_, maxX_;
    float minY_, maxY_;
    float minZ_, maxZ_;
    float minR_, maxR_;
    float minC_, maxC_;

    int surfaceA_, surfaceB_;
};


class JoinedSurface : public Surface {
public:
            JoinedSurface(int surfaceId);
    virtual ~JoinedSurface();
    virtual void    load(Preferences& prefs) override;

    virtual Touch map(const Touch&) override;

private:
    int surfaceA_, surfaceB_;
};

}

#endif //MEC_SURFACE_H