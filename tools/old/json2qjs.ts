import { Cpp } from "./CppInterface"

let fs = require("fs")

//#region Utility Functions
function print(indent: number, str: string): void {
  console.log(Array.from(Array(indent).keys()).map(_ => "  ").join("") + str);
}

function newline(n: number = 1) {
  for (let i = 0; i < n; i++)
    console.log("");
}
//#endregion

//#region
function get_param_line(type: string, varname: string, argname: string): string {
  if (type == "int" || type == "uint")
    return `${type} ${varname}; JS_ToInt32(ctx, (int*)&${varname}, ${argname});`
  if (type == "long" || type == "ulong")
    return `${type} ${varname}; JS_ToInt64(ctx, (long*)&${varname}, ${argname});`
  if (type == "bool")
    return `${type} ${varname}; JS_ToBool(ctx, &${varname}, ${argname});`
  if (type == "float" || type == "double")
    return `${type} ${varname}; JS_ToFloat64(ctx, (double*)&${varname}, ${argname});`
  if (type == "std::string")
    return `${type} ${varname}(JS_ToCString(ctx, ${argname}));`

  let num_pointers = (type.match(/[*]/g) || []).length
  let num_references = (type.match(/[&]/g) || []).length
  let base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  return `${type} ${varname} = ${num_pointers < 1 ? "*" : ""}(${base_type}*)JS_GetOpaque(${argname}, class_id_${base_type});`
}

function get_return_line(type: string): string {
  if (type == "int" || type == "uint")
    return "return JS_NewInt32(ctx, __result);"
  if (type == "long" || type == "ulong")
    return "return JS_NewInt64(ctx, __result);"
  if (type == "bool")
    return "return JS_NewBool(ctx, __result);"
  if (type == "float" || type == "double")
    return "return JS_NewFloat64(ctx, __result);"
  if (type == "std::string")
    return "return JS_NewStringLen(ctx, __result.c_str(), __result.length());"

  let num_pointers = (type.match(/[*]/g) || []).length
  let num_references = (type.match(/[&]/g) || []).length
  let base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  return `JSValue __obj = JS_NewObjectClass(ctx, class_id_${base_type}); __object *__tmp = new __object; __tmp->ptr = (void*)__result; __tmp->created = false; JS_SetOpaque(__obj, (void*)__tmp); return __obj;`
}

function get_func_def(f: Cpp.Function, call: string, context: string): void {
  print(1, `JS_SetPropertyStr(ctx, ${context}, \"${f.name}\", JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {`)
  for (let i in f.parameters) {
    let p = f.parameters[i];
    print(2, get_param_line(p.type, `__arg${i}`, `argv[${i}]`));
  }

  newline(1)

  if (f.return_type == "void") {
    print(2, call)
    newline(1)
    print(2, "return JS_UNDEFINED;")
  } else {
    print(2, `${f.return_type} __result = ${call}`)
    newline(1)
    print(2, get_return_line(f.return_type))
  }
  print(1, `}, \"${f.name}\", 0));`)
}
//#endregion

let path = process.argv[2];
let j = JSON.parse(fs.readFileSync(path)) as Record<string, Cpp.File>;
let filename = Object.keys(j)[0];
let o = j[filename];

//=================================================
// BEGIN
//=================================================

print(0, "#include <quickjs.h>")
print(0, "#include <string>")

newline(2)

print(0, "typedef struct __object {")
print(1, "void *ptr;")
print(1, "int created;")
print(0, "} __object;")

newline(2)

print(0, "void test_qjs(JSContext *ctx) {")
print(1, "JSValue global = JS_GetGlobalObject(ctx);")

newline(1)

// CLASSES
for (let name in o.classes) {
  let c = o.classes[name];

  print(1, `static JSClassDef class_def_${name} = { \"${name}\" };`)
  print(1, `static JSClassID class_id_${name} = 0;`)
  print(1, `class_def_${name}.finalizer = [](JSRuntime *rt, JSValue val) { __object *__obj = (__object*)JS_GetOpaque(val, class_id_${name}); if (__obj->created) delete (${name}*)__obj->ptr; delete __obj; };`)
  print(1, `JS_NewClass(JS_GetRuntime(ctx), JS_NewClassID(&class_id_${name}), &class_def_${name});`)
  print(1, `JS_SetPropertyStr(ctx, global, \"${name}\", JS_NewCFunction2(ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {`)
  print(2, `JSValue __result = JS_NewObjectClass(ctx, class_id_${name});`)
  print(2, `__object *__obj;`)

  for (let con of c.constructors) {
    print(2, `if (argc == ${con.parameters.length}) {`)
    for (let i in con.parameters) {
      let p = con.parameters[i]
      print(3, get_param_line(p.type, `__arg${i}`, `argv[${i}]`))
    }
    print(3, `__obj = new __object; __obj->ptr = (void*)new ${name}(${(Array.from(con.parameters.keys()).map(i => `__arg${i}`)).join(", ")}); __obj->created = true;`)
    print(2, `}`)
  }

  print(2, `JS_SetOpaque(__result, (void*)__obj);`)
  print(2, `return __result;`)
  print(1, `}, \"${name}\", 0, JS_CFUNC_constructor, 0));`)

  print(1, `JSValue class_proto_${name} = JS_NewObject(ctx);`)
  for (let m of c.methods) {
    let call = `((${name}*)((__object*)JS_GetOpaque(this_val, class_id_${name}))->ptr)->${m.name}(${(Array.from(m.parameters.keys()).map(i => `__arg${i}`)).join(", ")});`
    get_func_def(m, call, `class_proto_${name}`)
  }
  for (let v of c.variables) {
    print(1, `JSCFunctionListEntry get_set_func_def_${name}_${v.name} = { \"${v.name}\", JS_PROP_CONFIGURABLE, JS_DEF_CGETSET, 0 };`)
    print(1, `get_set_func_def_${name}_${v.name}.u.getset.get.getter = [](JSContext *ctx, JSValueConst this_val) -> JSValue {`)
    print(2, `${v.type} __result = ((${name}*)((__object*)JS_GetOpaque(this_val, class_id_${name}))->ptr)->${v.name};`)
    print(2, get_return_line(v.type))
    print(1, `};`)
    print(1, `get_set_func_def_${name}_${v.name}.u.getset.set.setter = [](JSContext *ctx, JSValueConst this_val, JSValueConst val) -> JSValue {`)
    print(2, get_param_line(v.type, "__value", "val"))
    print(2, `((${name}*)((__object*)JS_GetOpaque(this_val, class_id_${name}))->ptr)->${v.name} = __value;`)
    print(2, `return JS_UNDEFINED;`)
    print(1, `};`)
    print(1, `static const JSCFunctionListEntry get_set_func_list_${name}_${v.name}[] = { get_set_func_def_${name}_${v.name} };`)
    print(1, `JS_SetPropertyFunctionList(ctx, class_proto_${name}, get_set_func_list_${name}_${v.name}, 1);`)
  }
  print(1, `JS_SetClassProto(ctx, class_id_${name}, class_proto_${name});`)
}

newline(2)

// FUNCTIONS
for (let f of o.functions) {
  let call = `${f.name}(${(Array.from(f.parameters.keys()).map(i => `__arg${i}`)).join(", ")});`
  get_func_def(f, call, `global`)
}

newline(1)

print(1, `JS_FreeValue(ctx, global);`)
print(0, `}`)
