"use strict";
exports.__esModule = true;
var fs = require("fs");
//#region Utility Functions
function print(indent, str) {
    console.log(Array.from(Array(indent).keys()).map(function (_) { return "  "; }).join("") + str);
}
function newline(n) {
    if (n === void 0) { n = 1; }
    for (var i = 0; i < n; i++)
        console.log("");
}
//#endregion
var path = process.argv[2];
var j = JSON.parse(fs.readFileSync(path));
var filename = Object.keys(j)[0];
var o = j[filename];
var header_name = filename.split("/").pop();
var module_name = header_name.split(".")[0];
//=================================================
// BEGIN
//=================================================
print(0, "#ifndef " + (module_name.toUpperCase() + "__QJS"));
print(0, "#define " + (module_name.toUpperCase() + "__QJS"));
newline(1);
print(0, "#include <quickjspp.hpp>");
print(0, "#include <quickjspp_aux.hpp>");
newline(1);
print(0, "#include \"" + header_name + "\"");
newline(2);
print(0, "static void " + module_name + "_qjs(qjs::Context &ctx) {");
print(1, "auto val = ctx.global();");
newline(1);
// MODULES
var modules = new Set();
for (var name_1 in o.classes) {
    var c = o.classes[name_1];
    modules.add(c.scope.join("_"));
}
for (var _i = 0, _a = o.functions; _i < _a.length; _i++) {
    var f = _a[_i];
    modules.add(f.scope.join("_"));
}
modules.forEach(function (m) {
    if (m == "")
        return;
    print(1, "auto val_" + m + " = ctx.newObject();");
    print(1, "val[\"" + m + "\"] = val_" + m + ";");
});
newline(1);
// CLASSES
for (var name_2 in o.classes) {
    var c = o.classes[name_2];
    var context = c.scope.length == 0 ? "val" : "val_" + c.scope.join("_");
    var longname = c.scope.map(function (_) { return _ + "_"; }).join("") + name_2;
    var cname = longname.replace("_", "::");
    print(1, "auto proto_" + longname + " = ctx.newObject();");
    for (var _b = 0, _c = c.constructors; _b < _c.length; _b++) {
        var con = _c[_b];
        print(1, context + "[\"" + name_2 + "\"] = qjs::ctor_wrapper<" + cname + con.parameters.map(function (_) { return ", " + _.type; }).join("") + ">{\"" + name_2 + "\"};");
    }
    for (var _d = 0, _e = c.methods; _d < _e.length; _d++) {
        var m = _e[_d];
        print(1, "proto_" + longname + ".add<&" + cname + "::" + m.name + ">(\"" + m.name + "\");");
    }
    for (var _f = 0, _g = c.variables; _f < _g.length; _f++) {
        var v = _g[_f];
        print(1, "proto_" + longname + ".add<&" + cname + "::" + v.name + ">(\"" + v.name + "\");");
    }
    print(1, "ctx.registerClass<" + cname + ">(\"" + longname + "\", std::move(proto_" + longname + "));");
}
newline(1);
// FUNCTIONS
for (var _h = 0, _j = o.functions; _h < _j.length; _h++) {
    var f = _j[_h];
    var context = "val" + f.scope.map(function (_) { return "_" + _; }).join("");
    print(1, context + ".add<&" + (f.scope.map(function (_) { return _ + "::"; }).join("") + f.name) + ">(\"" + f.name + "\");");
}
print(0, "}");
print(0, "#endif");
