fs = require "fs"

print = (indent, str) ->
  console.log ("  " for i in [0...indent]).join("") + str

newline = (n = 1) -> console.log("") for i in [0...n]

path = process.argv[2]
o = JSON.parse(fs.readFileSync path)
filename = Object.keys(o)[0]
o = o[filename]

get_param_line = (type, varname, argname) ->
  if type == "int" or type == "uint"
    return "#{type} #{varname}; JS_ToInt32(ctx, (int*)&#{varname}, #{argname});"
  if type == "long" or type == "ulong"
    return "#{type} #{varname}; JS_ToInt64(ctx, (long*)&#{varname}, #{argname});"
  if type == "bool"
    return "#{type} #{varname}; JS_ToBool(ctx, &#{varname}, #{argname});"
  if type == "float" or type == "double"
    return "#{type} #{varname}; JS_ToFloat64(ctx, (double*)&#{varname}, #{argname});"
  if type == "std::string"
    return "#{type} #{varname}(JS_ToCString(ctx, #{argname}));"

  num_pointers = (type.match(/[*]/g) or []).length
  num_references = (type.match(/[&]/g) or []).length
  base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  return "#{type} #{varname} = #{if num_pointers < 1 then "*" else ""}(#{base_type}*)JS_GetOpaque(#{argname}, class_id_#{base_type});"

get_return_line = (type) ->
  if type == "int" or type == "uint"
    return "return JS_NewInt32(ctx, __result);"
  if type == "long" or type == "ulong"
    return "return JS_NewInt64(ctx, __result);"
  if type == "bool"
    return "return JS_NewBool(ctx, __result);"
  if type == "float" or type == "double"
    return "return JS_NewFloat64(ctx, __result);"
  if type == "std::string"
    return "return JS_NewStringLen(ctx, __result.c_str(), __result.length());"

  num_pointers = (type.match(/[*]/g) or []).length
  num_references = (type.match(/[&]/g) or []).length
  base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  return "JSValue __obj = JS_NewObjectClass(ctx, class_id_#{base_type}); __object *__tmp = new __object; __tmp->ptr = (void*)__result; __tmp->created = false; JS_SetOpaque(__obj, (void*)__tmp); return __obj;"

get_func_def = (f, call, context) ->
  print 1, "JS_SetPropertyStr(ctx, #{context}, \"#{f.name}\", JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {"
  for i in [0...f.parameters.length]
    p = f.parameters[i]
    print 2, get_param_line(p.type, "__arg#{i}", "argv[#{i}]")

  newline 1

  if f.return_type == "void"
    print 2, call
    newline 1
    print 2, "return JS_UNDEFINED;"
  else
    print 2, "#{f.return_type} __result = #{call}"
    newline 1
    print 2, get_return_line f.return_type
  print 1, "}, \"#{f.name}\", 0));"

#=================================================
# BEGIN
#=================================================

print 0, "#include <quickjs.h>"
print 0, "#include <string>"

newline 2

print 0, "typedef struct __object {"
print 1, "void *ptr;"
print 1, "int created;"
print 0, "} __object;"

newline 2

print 0, "void test_qjs(JSContext *ctx) {"
print 1, "JSValue global = JS_GetGlobalObject(ctx);"

newline 1

# CLASSES
for name, c of o.classes
  print 1, "static JSClassDef class_def_#{name} = { \"#{name}\" };"
  print 1, "static JSClassID class_id_#{name} = 0;"
  print 1, "class_def_#{name}.finalizer = [](JSRuntime *rt, JSValue val) { __object *__obj = (__object*)JS_GetOpaque(val, class_id_#{name}); if (__obj->created) delete (#{name}*)__obj->ptr; delete __obj; };"
  print 1, "JS_NewClass(JS_GetRuntime(ctx), JS_NewClassID(&class_id_#{name}), &class_def_#{name});"
  print 1, "JS_SetPropertyStr(ctx, global, \"#{name}\", JS_NewCFunction2(ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) -> JSValue {"
  print 2, "JSValue __result = JS_NewObjectClass(ctx, class_id_#{name});"
  print 2, "__object *__obj;"

  for con in c.constructors
    print 2, "if (argc == #{con.parameters.length}) {"
    for i in [0...con.parameters.length]
      p = con.parameters[i]
      print 3, get_param_line(p.type, "__arg#{i}", "argv[#{i}]")
    print 3, "__obj = new __object; __obj->ptr = (void*)new #{name}(#{("__arg#{i}" for i in [0...con.parameters.length]).join(", ")}); __obj->created = true;"
    print 2, "}"

  print 2, "JS_SetOpaque(__result, (void*)__obj);"
  print 2, "return __result;"
  print 1, "}, \"#{name}\", 0, JS_CFUNC_constructor, 0));"

  print 1, "JSValue class_proto_#{name} = JS_NewObject(ctx);"
  for m in c.methods
    call = "((#{name}*)((__object*)JS_GetOpaque(this_val, class_id_#{name}))->ptr)->#{m.name}(#{("__arg#{i}" for i in [0...m.parameters.length]).join(", ")});"
    get_func_def(m, call, "class_proto_#{name}")
  for v in c.variables
    print 1, "JSCFunctionListEntry get_set_func_def_#{name}_#{v.name} = { \"#{v.name}\", JS_PROP_CONFIGURABLE, JS_DEF_CGETSET, 0 };"
    print 1, "get_set_func_def_#{name}_#{v.name}.u.getset.get.getter = [](JSContext *ctx, JSValueConst this_val) -> JSValue {"
    print 2, "#{v.type} __result = ((#{name}*)((__object*)JS_GetOpaque(this_val, class_id_#{name}))->ptr)->#{v.name};"
    print 2, get_return_line v.type
    print 1, "};"
    print 1, "get_set_func_def_#{name}_#{v.name}.u.getset.set.setter = [](JSContext *ctx, JSValueConst this_val, JSValueConst val) -> JSValue {"
    print 2, get_param_line(v.type, "__value", "val")
    print 2, "((#{name}*)((__object*)JS_GetOpaque(this_val, class_id_#{name}))->ptr)->#{v.name} = __value;"
    print 2, "return JS_UNDEFINED;"
    print 1, "};"
    print 1, "static const JSCFunctionListEntry get_set_func_list_#{name}_#{v.name}[] = { get_set_func_def_#{name}_#{v.name} };"
    print 1, "JS_SetPropertyFunctionList(ctx, class_proto_#{name}, get_set_func_list_#{name}_#{v.name}, 1);"
  print 1, "JS_SetClassProto(ctx, class_id_#{name}, class_proto_#{name});"

newline 2

# FUNCTIONS
for f in o.functions
  call = "#{f.name}(#{("__arg#{i}" for i in [0...f.parameters.length]).join(", ")});"
  get_func_def(f, call, "global")

newline 1

print 1, "JS_FreeValue(ctx, global);"
print 0, "}"
