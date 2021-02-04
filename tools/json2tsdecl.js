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
var path = process.argv[2];
var j = JSON.parse(fs.readFileSync(path));
var filename = Object.keys(j)[0];
var o = j[filename];
var header_name = filename.split("/").pop();
var module_name = header_name.split(".")[0];
var scope = [];
function get_type(type) {
    var base_type = type.replace(/[*]/g, "").replace(/[&]/g, "");
    if (["int", "uint", "long", "ulong", "float", "double"].includes(base_type))
        return "number";
    if (base_type == "bool")
        return "boolean";
    if (base_type == "std::string")
        return "string";
    return base_type.replace("::", ".");
}
function print_scope(new_scope) {
    var to_close = scope.length - new_scope.length;
    for (var i = 0; i < new_scope.length; i++) {
        if (scope[i] != new_scope[i]) {
            to_close = scope.length - i;
            break;
        }
    }
    for (var i = 0; i < to_close; i++) {
        print(0, "}");
    }
    var equal = scope.length - to_close;
    for (var i = equal; i < new_scope.length; i++) {
        print(0, (i == 0 ? "declare " : "") + "namespace " + new_scope[i] + " {");
    }
    scope = new_scope;
}
//=================================================
// BEGIN
//=================================================
// CLASSES
for (var name_1 in o.classes) {
    var c = o.classes[name_1];
    print_scope(c.scope);
    print(0, (c.scope.length == 0 ? "declare " : "") + "class " + name_1 + " {");
    for (var _i = 0, _a = c.constructors; _i < _a.length; _i++) {
        var con = _a[_i];
        print(1, "constructor(" + con.parameters.map(function (p) { return p.name + ": " + get_type(p.type); }).join(", ") + ");");
    }
    for (var _b = 0, _c = c.methods; _b < _c.length; _b++) {
        var m = _c[_b];
        print(1, m.name + "(" + m.parameters.map(function (p) { return p.name + ": " + get_type(p.type); }).join(", ") + "): " + get_type(m.return_type));
    }
    for (var _d = 0, _e = c.variables; _d < _e.length; _d++) {
        var v = _e[_d];
        print(1, v.name + ": " + get_type(v.type) + ";");
    }
    print(0, "}");
}
newline(1);
// FUNCTIONS
for (var _f = 0, _g = o.functions; _f < _g.length; _f++) {
    var f = _g[_f];
    print_scope(f.scope);
    print(0, (f.scope.length == 0 ? "declare " : "") + "function " + f.name + "(" + f.parameters.map(function (p) { return p.name + ": " + get_type(p.type); }).join(", ") + "): " + get_type(f.return_type) + ";");
}
for (var s in scope)
    print(0, "}");
