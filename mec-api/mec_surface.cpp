#include "mec_surface.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
//
// mapping between surfaces
//

#include "mec_log.h"

#include <algorithm>

namespace mec {


////////////////////////////// SurfaceManager ////////////////////////////////////////


SurfaceManager::SurfaceManager() {

}

SurfaceManager::~SurfaceManager() {

}

bool SurfaceManager::init(const Preferences& prefs) {
    if (!prefs.valid()) return false;

    std::vector<std::string> keys = prefs.getKeys();
    for (std::string k : keys) {
        Preferences p(prefs.getSubTree(k));
        if (p.valid()) {
            std::shared_ptr<Surface> pS;
            std::string type = p.getString("type", "");
            if (type.size() == 0) { // plain
                pS.reset(new Surface(k));
            } else if (type == "join") {
                pS.reset(new JoinedSurface(k));
            } else if (type == "split") {
                pS.reset(new SplitSurface(k));
            } else {
                pS.reset();
                LOG_0("SurfaceManager: surface def missing type");
            }
            if (pS) {
                if (pS->load(p)) {
                    surfaces_[k] = pS;
                } else {
                    pS.reset();
                }
            }
        }
    }
    return true;
}

std::shared_ptr<Surface>  SurfaceManager::getSurface(SurfaceID id) {
    return surfaces_[id];
}

////////////////////////////// Surface ////////////////////////////////////////


Surface::Surface(SurfaceID surfaceId) :
    surfaceId_(surfaceId) {
    ;
}

Surface::~Surface() {
    ;
}

bool   Surface::load(const Preferences& prefs) {
    if (!prefs.valid()) return false;
    return true;
}

Touch Surface::map(const Touch& t) const {
    return t;
}

SurfaceID Surface::getId() {
    return surfaceId_;
}


const float UNDEFINED_SPLIT = -1.0f;
const float MIN_SPLIT = 0.0f;
const float MAX_SPLIT = 16384.0f;

////////////////////////////// SplitSurface ////////////////////////////////////////


SplitSurface::SplitSurface(SurfaceID surfaceId) :
    Surface(surfaceId) {
    ;
}

SplitSurface::~SplitSurface() {
    ;
}

bool    SplitSurface::load(const Preferences& prefs) {

    if (!prefs.valid()) return false;

    splitPoint_ = (float) prefs.getDouble("split point", 0.5);

    Preferences::Array array(prefs.getArray("surfaces"));
    for (int i = 0; i < array.getSize(); i++) {
        SurfaceID n = array.getString(i);
        if (n.size() > 0) {
            surfaces_.push_back(n);
        }
    }

    std::string axis = prefs.getString("axis", "");
    if (axis.size() > 0) {
        if (axis == "x") axis_ = C_X;
        else if (axis == "y") axis_ = C_Y;
        else if (axis == "z") axis_ = C_Z;
        else if (axis == "r") axis_ = C_R;
        else if (axis == "c") axis_ = C_C;
        else axis_ = C_X;
    }
    else {
        axis_ = C_X;
    }


    return surfaces_.size() > 0;
}

Touch SplitSurface::map(const Touch& t) const {
    // TODO
    // relationship between X-C , Y - R
    // touch id, needs to be voiced on surface
    Touch out = t;
    int n = 0;

    switch (axis_) {
    case C_X: {
        n = (t.x_ / splitPoint_ );
        n = std::min<int> (n, surfaces_.size() - 1);
        out.x_ = t.x_ - ( splitPoint_ * n );
        break;
    }
    case C_Y: {
        n = (t.y_ / splitPoint_ );
        n = std::min<int> (n, surfaces_.size() - 1);
        out.y_ = t.y_ - ( splitPoint_ * n );
        break;
    }
    case C_Z: {
        n = (t.z_ / splitPoint_ );
        n = std::min<int> (n, surfaces_.size() - 1);
        out.z_ = t.z_ - ( splitPoint_ * n );
        break;
    }
    case C_R: {
        n = (t.r_ / splitPoint_ );
        n = std::min<int> (n, surfaces_.size() - 1);
        out.r_ = t.r_ - ( splitPoint_ * n );
        break;
    }
    case C_C: {
        n = (t.c_ / splitPoint_ );
        n = std::min<int> (n, surfaces_.size() - 1);
        out.c_ = t.c_ - ( splitPoint_ * n );
        break;
    }
    default:
        LOG_0("SplitSurface : invalid axis");
        break;
    }
    out.surface_ = surfaces_[n];
    return out;
}


////////////////////////////// JoinedSurface ////////////////////////////////////////


JoinedSurface::JoinedSurface(SurfaceID surfaceId) :
    Surface(surfaceId) {
    ;
}

JoinedSurface::~JoinedSurface() {
    ;
}

bool    JoinedSurface::load(const Preferences& prefs) {
    if (!prefs.valid()) return false;

    // temp, this will come from the source surface
    surfaceSize_ = (float) prefs.getDouble("surface size", 1.0);

    Preferences::Array array(prefs.getArray("surfaces"));
    for (int i = 0; i < array.getSize(); i++) {
        SurfaceID n = array.getString(i);
        if (n.size() > 0) {
            surfaces_.push_back(n);
        }
    }

    std::string axis = prefs.getString("axis", "");
    if (axis.size() > 0) {
        if (axis == "x") axis_ = C_X;
        else if (axis == "y") axis_ = C_Y;
        else if (axis == "z") axis_ = C_Z;
        else if (axis == "r") axis_ = C_R;
        else if (axis == "c") axis_ = C_C;
        else axis_ = C_X;
    }
    else {
        axis_ = C_X;
    }


    return surfaces_.size() > 0;
}

Touch JoinedSurface::map(const Touch& t) const {
    // TODO
    // use source surface for dimension,
    // relationship between X-C , Y - R
    // touch id, needs to be voiced on surface
    Touch out = t;
    int idx = 0;
    for (SurfaceID n : surfaces_) {
        if (t.surface_ == n) {
            switch (axis_) {
            case C_X: {
                out.x_ = t.x_ + ( surfaceSize_ * idx );
                break;
            }
            case C_Y: {
                out.y_ = t.y_ + ( surfaceSize_ * idx );
                break;
            }
            case C_Z: {
                out.z_ = t.z_ + ( surfaceSize_ * idx );
                break;
            }
            case C_R: {
                out.r_ = t.r_ + ( surfaceSize_ * idx );
                break;
            }
            case C_C: {
                out.c_ = t.c_ + ( surfaceSize_ * idx );
                break;
            }
            default:
                LOG_0("JoinedSurface : invalid axis");
                break;
            }

            out.surface_ = surfaceId_;
            return out;
        }
        idx++;
    }
    return out;
}

} // namespace

