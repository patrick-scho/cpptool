import { Cpp } from "./CppInterface"

let fs = require("fs")

//#region Utility Functions
function print(indent: number, str: string): void {
  console.log(Array.from(Array(indent).keys()).map(_ => "  ").join("") + str)
}

function newline(n: number = 1) {
  for (let i = 0; i < n; i++)
    console.log("")
}
//#endregion

let path = process.argv[2]
let j = JSON.parse(fs.readFileSync(path)) as {[index: string]: Cpp.File}
let filename = Object.keys(j)[0]
let o = j[filename]
let header_name = filename.split("/").pop()
let module_name = header_name.split(".")[0]

//=================================================
// BEGIN
//=================================================

print(0, `#ifndef ${module_name.toUpperCase() + "__QJS"}`)
print(0, `#define ${module_name.toUpperCase() + "__QJS"}`)

newline(1)

print(0, `#include <quickjspp.hpp>`)
print(0, `#include <quickjspp_aux.hpp>`)

newline(1)

print(0, `#include "${header_name}"`)

newline(2)

print(0, `static void ${module_name}_qjs(qjs::Context &ctx) {`)
print(1, `auto val = ctx.global();`)

newline(1)

// MODULES
let modules = new Set<string>();
for (let name in o.classes) {
  let c = o.classes[name]
  modules.add(c.scope.join("_"))
}
for (let f of o.functions) {
  modules.add(f.scope.join("_"))
}
modules.forEach((m) => {
  if (m == "")
    return;
  print(1, `auto val_${m} = ctx.newObject();`)
  print(1, `val["${m}"] = val_${m};`)
})

newline(1)

// CLASSES
for (let name in o.classes) {
  let c = o.classes[name]

  let context = c.scope.length == 0 ? "val" : "val_" + c.scope.join("_");
  let longname = c.scope.map(_ => _ + "_").join("") + name;
  let cname = longname.replace("_", "::")

  print(1, `auto proto_${longname} = ctx.newObject();`)
    
  for (let con of c.constructors) {
    print(1, `${context}["${name}"] = qjs::ctor_wrapper<${cname}${con.parameters.map(_ => ", " + _.type).join("")}>{"${name}"};`)
  }
  for (let m of c.methods) {
    print(1, `proto_${longname}.add<&${cname}::${m.name}>("${m.name}");`)
  }
  for (let v of c.variables) {
    print(1, `proto_${longname}.add<&${cname}::${v.name}>("${v.name}");`)
  }

  print(1, `ctx.registerClass<${cname}>("${longname}", std::move(proto_${longname}));`)
}

newline(1)

// FUNCTIONS
for (let f of o.functions) {
  let context = `val${f.scope.map(_ => "_" + _).join("")}`;
  
  print(1, `${context}.add<&${f.scope.map(_ => _ + "::").join("") + f.name}>("${f.name}");`)
}

print(0, `}`)
print(0, `#endif`)
