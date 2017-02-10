#ifndef MEC_SURFACE_H
#define MEC_SURFACE_H

#include "mec_prefs.h"


// NOT USED YET INTERFACE EXPERIMENT ONLY
// 
// mapping between surfaces
// 



  
namespace mec {

struct Touch {
    int   id_;
    int   zone_;

    float r_;
    float c_;

    float x_;
    float y_;
    float z_;
};


class Surface {
public:
            Surface();
    virtual ~Surface();
    void    load(Preferences& prefs);

    virtual Touch map(const Touch&);

private:

};

}

#endif //MEC_SURFACE_H