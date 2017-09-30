#include "m_pd.h"

#include "ParameterModel.h"
// #include "OSCReceiver.h"
// #include "OSCBroadcaster.h"
#include <string>
#include <vector>

static t_class *oKontrol_class;

typedef struct _oKontrol {
  t_object  x_obj;
  std::shared_ptr<oKontrol::ParameterModel> param_model_;
} t_oKontrol;


//define pure data methods
extern "C"  {
  void    oKontrol_free(t_oKontrol*);
  void*   oKontrol_new();
  void    oKontrol_setup(void);

  void oKontrol_param(t_oKontrol *x, t_symbol *s, int argc, t_atom *argv);
  void oKontrol_page(t_oKontrol *x, t_symbol *s, int argc, t_atom *argv);
}
// puredata methods implementation - start

void oKontrol_free(t_oKontrol* x)
{
  x->param_model_->clearCallbacks();
}

void *oKontrol_new()
{
  t_oKontrol *x = (t_oKontrol *) pd_new(oKontrol_class);
  x->param_model_=oKontrol::ParameterModel::model();
  return (void *)x;
}

void oKontrol_setup(void) {
  oKontrol_class = class_new(gensym("oKontrol"),
                                   (t_newmethod) oKontrol_new,
                                   (t_method) oKontrol_free,
                                   sizeof(t_oKontrol),
                                   CLASS_DEFAULT,
                                   A_NULL);
  class_addmethod(oKontrol_class,
                  (t_method) oKontrol_param, gensym("param"),
                  A_GIMME, A_NULL);
  class_addmethod(oKontrol_class,
                  (t_method) oKontrol_page, gensym("page"),
                  A_GIMME, A_NULL);
}


//param name page type displayname min max default
//e.g param r_mix pct "mix" 0 100 50
void paramUsage() {
  post("invalid param message");
}
void oKontrol_param(t_oKontrol* x, t_symbol*, int argc, t_atom *argv) {
  t_atom *t = nullptr;
  char buf[128];

  std::vector<oKontrol::ParamValue> args;
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

  if(!x->param_model_->addParam(oKontrol::PS_LOCAL, args)) {
    post("failed to add param with %d args", args.size());
  }
}

//page page1 "reverb"
void pageUsage() {
  post("invalid page message");
}

void oKontrol_page(t_oKontrol* x, t_symbol* , int argc, t_atom *argv) {
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
  x->param_model_->addPage(oKontrol::PS_LOCAL, id, displayName, paramIds);
}
// puredata methods implementation - end

