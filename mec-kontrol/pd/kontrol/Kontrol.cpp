#include "m_pd.h"

#include "ParameterModel.h"
#include <string>
#include <vector>

static t_class *Kontrol_class;

typedef struct _Kontrol {
  t_object  x_obj;
} t_Kontrol;


//define pure data methods
extern "C"  {
  void    Kontrol_free(t_Kontrol*);
  void*   Kontrol_new(t_symbol* name);
  void    Kontrol_setup(void);
}
// puredata methods implementation - start

void Kontrol_free(t_Kontrol* x)
{
}

void *Kontrol_new(t_symbol* name)
{
  t_Kontrol *x = (t_Kontrol *) pd_new(Kontrol_class);
  if(name && name->s_name) {
    std::string sname = name->s_name;
    Kontrol::ParameterModel::model()->loadParameterDefinitions(sname + ".json");
    Kontrol::ParameterModel::model()->dumpParameters();
  }
  return (void *)x;
}

void Kontrol_setup(void) {
  Kontrol_class = class_new(gensym("Kontrol"),
                                   (t_newmethod) Kontrol_new,
                                   (t_method) Kontrol_free,
                                   sizeof(t_Kontrol),
                                   CLASS_DEFAULT,
                                   A_SYMBOL, A_NULL);
}

// puredata methods implementation - end

