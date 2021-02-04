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

let path = process.argv[2];
let j = JSON.parse(fs.readFileSync(path)) as Record<string, Cpp.File>;
let filename = Object.keys(j)[0];
let o = j[filename];
let header_name = filename.split("/").pop()
let module_name = header_name.split(".")[0]

let scope = [];

function get_type(type: string): string {
  let base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  if (["int", "uint", "long", "ulong", "float", "double"].includes(base_type))
    return "number"
  if (base_type == "bool")
    return "boolean"
  if (base_type == "std::string")
    return "string"

  return base_type.replace("::", ".")
}

function print_scope(new_scope: string[]): void {
  let to_close = scope.length - new_scope.length;
  for (let i = 0; i < new_scope.length; i++) {
    if (scope[i] != new_scope[i]) {
      to_close = scope.length - i;
      break;
    }
  }
  for (let i = 0; i < to_close; i++) {
    print(0, "}");
  }
  let equal = scope.length - to_close;
  for (let i = equal; i < new_scope.length; i++) {
    print(0, `${i == 0 ? "declare " : ""}namespace ${new_scope[i]} {`);
  }
  scope = new_scope;
}

//=================================================
// BEGIN
//=================================================

// CLASSES
for (let name in o.classes) {
  let c = o.classes[name];
  print_scope(c.scope);
  print(0, `${c.scope.length == 0 ? "declare " : ""}class ${name} {`)

  for (let con of c.constructors)
    print(1, `constructor(${con.parameters.map(p => `${p.name}: ${get_type(p.type)}`).join(", ")});`)

  for (let m of c.methods)
    print(1, `${m.name}(${m.parameters.map(p => `${p.name}: ${get_type(p.type)}`).join(", ")}): ${get_type(m.return_type)}`)

  for (let v of c.variables)
    print(1, `${v.name}: ${get_type(v.type)};`)

  print(0, "}")
}

newline(1)

// FUNCTIONS
for (let f of o.functions) {
  print_scope(f.scope);
  print(0, `${f.scope.length == 0 ? "declare " : ""}function ${f.name}(${f.parameters.map(p => `${p.name}: ${get_type(p.type)}`).join(", ")}): ${get_type(f.return_type)};`)
}

for (let s in scope)
  print(0, "}")
