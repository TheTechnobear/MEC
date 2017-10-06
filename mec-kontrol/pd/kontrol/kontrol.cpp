#include "m_pd.h"

#include "ParameterModel.h"
// #include "OSCReceiver.h"
// #include "OSCBroadcaster.h"
#include <string>
#include <vector>

static t_class *Kontrol_class;

typedef struct _Kontrol {
  t_object  x_obj;
  std::shared_ptr<Kontrol::ParameterModel> param_model_;
} t_Kontrol;


//define pure data methods
extern "C"  {
  void    Kontrol_free(t_Kontrol*);
  void*   Kontrol_new();
  void    Kontrol_setup(void);

  void Kontrol_param(t_Kontrol *x, t_symbol *s, int argc, t_atom *argv);
  void Kontrol_page(t_Kontrol *x, t_symbol *s, int argc, t_atom *argv);
}
// puredata methods implementation - start

void Kontrol_free(t_Kontrol* x)
{
  x->param_model_->clearCallbacks();
}

void *Kontrol_new()
{
  t_Kontrol *x = (t_Kontrol *) pd_new(Kontrol_class);
  x->param_model_=Kontrol::ParameterModel::model();
  return (void *)x;
}

void Kontrol_setup(void) {
  Kontrol_class = class_new(gensym("Kontrol"),
                                   (t_newmethod) Kontrol_new,
                                   (t_method) Kontrol_free,
                                   sizeof(t_Kontrol),
                                   CLASS_DEFAULT,
                                   A_NULL);
  class_addmethod(Kontrol_class,
                  (t_method) Kontrol_param, gensym("param"),
                  A_GIMME, A_NULL);
  class_addmethod(Kontrol_class,
                  (t_method) Kontrol_page, gensym("page"),
                  A_GIMME, A_NULL);
}


//param name page type displayname min max default
//e.g param r_mix pct "mix" 0 100 50
void paramUsage() {
  post("invalid param message");
}
void Kontrol_param(t_Kontrol* x, t_symbol*, int argc, t_atom *argv) {
  t_atom *t = nullptr;
  char buf[128];

  std::vector<Kontrol::ParamValue> args;
  for(int i=0;i<argc;i++) {
    t = &argv[i];
    switch(t->a_type) {
      case A_FLOAT : {
        args.push_back(atom_getfloat(t));
        break;
      }
      case A_SYMBOL : {
        atom_string(t, buf, sizeof(buf));
        args.push_back(std::string(buf));
        break;
      }
      default:
        break;
    }
  }

  if(!x->param_model_->addParam(Kontrol::PS_LOCAL, args)) {
    post("failed to add param with %d args", args.size());
  }
}

//page page1 "reverb"
void pageUsage() {
  post("invalid page message");
}

void Kontrol_page(t_Kontrol* x, t_symbol* , int argc, t_atom *argv) {
  t_atom *t = nullptr;
  char buf[128];
  int i = 0;
  if (i > argc) { pageUsage(); return; }
  t = &argv[i];
  if (t->a_type != A_SYMBOL)  { pageUsage(); return; }
  atom_string(t, buf, sizeof(buf));
  std::string id = buf;

  i++;
  if (i > argc) { pageUsage(); return; }
  t = &argv[i];
  if (t->a_type != A_SYMBOL)  { pageUsage(); return; }
  atom_string(t, buf, sizeof(buf));
  std::string displayName = buf;

  std::vector<std::string> paramIds;

  for( i++ ; i < argc; i++) {
    t = &argv[i];
    if (t->a_type != A_SYMBOL)  { pageUsage(); return; }
    atom_string(t, buf, sizeof(buf));
    paramIds.push_back(buf);
  }
  x->param_model_->addPage(Kontrol::PS_LOCAL, id, displayName, paramIds);
}
// puredata methods implementation - end

