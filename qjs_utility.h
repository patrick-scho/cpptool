#include "../libs/quickjs/quickjs.h"
#include <string>
#include <vector>

using namespace std;

namespace qjs {
  struct __object {
    void *ptr;
    bool created;
  };
  template<typename C>
  class Class {
  public:
    Class(JSContext *ctx, string name) {
      static JSClassDef class_def = { name.c_str() };
      static JSClassID class_id = 0;
      class_def.finalizer =
      [](JSRuntime *rt, JSValue val) {
        __object *__obj = (__object*)JS_GetOpaque(val, class_id);
        if (__obj->created)
          delete (C*)__obj->ptr;
        delete __obj;
      };
      JS_NewClass(JS_GetRuntime(ctx), JS_NewClassID(&class_id), &class_def);
      JSValue class_proto = JS_NewObject(ctx);
      JS_SetClassProto(ctx, class_id, class_proto);

      this->ctx = ctx;
      this->name = name;
      this->classId = class_id;
      
      return result;
    }
    template<typename L>
    Class& Method(string name, L l) {
      
      return this;
    }
    template<typename L>
    Class& Variable(string name, L l) {
      
      return this;
    }
  private:
    string name;
    JSClassID classId;
    JSContext *ctx;
  };
  void Run(JSContext *ctx, string input) {
    JS_Eval(ctx, input.c_str(), input.length(), "", JS_EVAL_TYPE_GLOBAL);
  }
  template<typename L>
  void Function(JSContext *ctx, string name, L l) {
    static vector<L> ls;

    JSValue global = JS_GetGlobalObject(ctx);

    JSCFunctionMagic *f = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
      ls[magic]();
      
      return JS_UNDEFINED;
    };
    JSValue cfunc = JS_NewCFunctionMagic(ctx, f, name.c_str(), 0, JSCFunctionEnum::JS_CFUNC_constructor_or_func_magic, ls.size());
    JS_SetPropertyStr(ctx, global, name.c_str(), cfunc);

    ls.push_back(l);

    JS_FreeValue(ctx, global);
  }
}