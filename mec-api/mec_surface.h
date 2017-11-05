#ifndef MEC_SURFACE_H
#define MEC_SURFACE_H

#include "mec_api.h"
#include "mec_prefs.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
//
// mapping between surfaces
//


#include <map>
#include <memory>

namespace mec {


class Surface;

class SurfaceManager {
public:
    SurfaceManager();
    virtual ~SurfaceManager();
    bool init(const Preferences &prefs);

    std::shared_ptr<Surface> getSurface(SurfaceID id);
private:
    std::map<SurfaceID, std::shared_ptr<Surface>> surfaces_;
};


class Surface {
public:
    Surface(SurfaceID surfaceId);
    virtual ~Surface();

    SurfaceID getId();
    virtual bool load(const Preferences &prefs);
    virtual Touch map(const Touch &) const;

protected:
    SurfaceID surfaceId_;
};

// simple split with even split division
class SplitSurface : public Surface {
public:
    SplitSurface(SurfaceID surfaceId);
    virtual ~SplitSurface();

    virtual bool load(const Preferences &prefs) override;
    virtual Touch map(const Touch &) const override;

private:
    enum {
        C_X,
        C_Y,
        C_Z,
        C_R,
        C_C
    } axis_;
    std::vector<SurfaceID> surfaces_;
    float splitPoint_;
};


// simple join with a constant offsets
// (later surfaceSize will be taken from originating surface)
class JoinedSurface : public Surface {
public:
    JoinedSurface(SurfaceID surfaceId);
    virtual ~JoinedSurface();

    virtual bool load(const Preferences &prefs) override;
    virtual Touch map(const Touch &) const override;

private:
    enum {
        C_X,
        C_Y,
        C_Z,
        C_R,
        C_C
    } axis_;
    std::vector<SurfaceID> surfaces_;
    float surfaceSize_;
};

}

#endif //MEC_SURFACE_H