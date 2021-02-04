fs = require "fs"

print = (indent, str) ->
  console.log ("  " for i in [0...indent]).join("") + str

newline = (n = 1) -> console.log("") for i in [0...n]

path = process.argv[2]
o = JSON.parse(fs.readFileSync path)
filename = Object.keys(o)[0]
o = o[filename]

get_type = (type) ->
  if type in ["int", "uint", "long", "ulong", "float", "double"]
    return "number"
  if type == "bool"
    return "boolean"
  if type == "std::string"
    return "string"

  base_type = type.replace(/[*]/g, "").replace(/[&]/g, "")

  return base_type


#=================================================
# BEGIN
#=================================================

# CLASSES
for name, c of o.classes
  print 0, "declare class #{name} {"

  for con in c.constructors
    print 1, "constructor(#{con.parameters.map((p) -> "#{p.name}: #{get_type(p.type)}").join(", ")});"
  
  for m in c.methods
    print 1, "#{m.name}(#{m.parameters.map((p) -> "#{p.name}: #{get_type(p.type)}").join(", ")}): #{get_type m.return_type}"
  
  for v in c.variables
    print 1, "#{v.name}: #{get_type v.type};"

  print 0, "}"
  

newline 2

# FUNCTIONS
for f in o.functions
  print 0, "declare function #{f.name}(#{f.parameters.map((p) -> "#{p.name}: #{get_type(p.type)}").join(", ")}): #{get_type(f.return_type)};"
