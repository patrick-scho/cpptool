#include <fstream>
#include <iostream>
#include <list>
#include <regex>
#include <vector>

#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_forward_declarable.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_type_alias.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>

using namespace std;
using namespace cppast;

struct CppAttribute {
  string name;
  vector<string> arguments;
};
struct CppBase {
  string comment;
  vector<CppAttribute> attributes;
};
struct CppType : CppBase {
  string name;

  CppType() = default;
  CppType(const string &type) { name = type; }
  CppType(const cpp_type &type) { name = to_string(type); }
};
struct CppVariable : CppBase {
  string name;
  CppType type;
};
struct CppFunction : CppBase {
  string name;
  CppType returnType;
  vector<CppVariable> parameters;
};
struct CppEnumValue : CppBase {
  string name;

  CppEnumValue(const string &value) { name = value; }
};
struct CppEnum : CppBase {
  string name;

  vector<CppEnumValue> values;
};
struct CppClass : CppBase {
  string name;
  vector<CppFunction> constructors;
  
  vector<CppClass> classes;
  vector<CppEnum> enums;
  vector<CppFunction> methods;
  vector<CppVariable> variables;
};
struct CppFile {
  vector<CppClass> classes;
  vector<CppEnum> enums;
  vector<CppFunction> functions;
  vector<CppVariable> variables;

  CppClass *getClass(const string &name) {
    for (CppClass &cppClass : classes) {
      if (cppClass.name == name)
        return &cppClass;
    }
    throw runtime_error("Class not found");
    return nullptr;
  }
};

CppBase *process_function(const cpp_function &x, CppFile &cppFile) {
  CppFunction cppFunction;
  cppFunction.name = x.name();
  cppFunction.returnType = x.return_type();
  for (const cpp_function_parameter &p : x.parameters()) {
    CppVariable parameter;
    parameter.name = p.name();
    parameter.type = p.type();
    cppFunction.parameters.push_back(parameter);
  }

  cppFile.functions.push_back(cppFunction);
  return &(cppFile.functions.back());
}
CppBase *process_variable(const cpp_variable &x, CppFile &cppFile) {
  CppVariable variable;
  variable.name = x.name();
  variable.type = x.type();
  if (x.comment().has_value()) {
    string s = x.comment().value();
  }
  for (auto &attr : x.attributes()) {
    string s = attr.name();
    if (attr.arguments().has_value()) {
      for (auto &arg : attr.arguments().value()) {
        string s = arg.spelling;
      }
    }
  }

  cppFile.variables.push_back(variable);
  return &(cppFile.variables.back());
}
CppBase *process_class(const cpp_class &x, CppFile &cppFile) {
  CppClass cppClass;
  cppClass.name = x.name();

  cppFile.classes.push_back(cppClass);
  return &(cppFile.classes.back());
}
CppBase *process_constructor(const cpp_constructor &x, CppFile &cppFile) {
  CppClass *cppClass = cppFile.getClass(x.name());
  CppFunction constructor;
  constructor.name = x.name();
  constructor.returnType = x.name();
  for (const cpp_function_parameter &p : x.parameters()) {
    CppVariable parameter;
    parameter.name = p.name();
    parameter.type = p.type();
    constructor.parameters.push_back(parameter);
  }

  cppClass->constructors.push_back(constructor);
  return &(cppClass->constructors.back());
}
CppBase *process_member_function(const cpp_member_function &x, CppFile &cppFile) {
  CppClass *cppClass = cppFile.getClass(x.parent().value().name());
  CppFunction function;
  function.name = x.name();
  function.returnType = x.return_type();
  for (const cpp_function_parameter &p : x.parameters()) {
    CppVariable parameter;
    parameter.name = p.name();
    parameter.type = p.type();
    function.parameters.push_back(parameter);
  }

  cppClass->methods.push_back(function);
  return &(cppClass->methods.back());
}
CppBase *process_member_variable(const cpp_member_variable &x, CppFile &cppFile) {
  CppClass *cppClass = cppFile.getClass(x.parent().value().name());
  CppVariable variable;
  variable.name = x.name();
  variable.type = x.type();

  cppClass->variables.push_back(variable);
  return &(cppClass->variables.back());
}
CppBase *process_enum(const cpp_enum &x, CppFile &cppFile) {
  CppEnum cppEnum;
  cppEnum.name = x.name();
  for (auto &value : x) {
    cppEnum.values.push_back(value.name());
  }

  cppFile.enums.push_back(cppEnum);
  return &(cppFile.enums.back());
}

void process_entity(const cpp_entity &e, CppFile &cppFile) {
  CppBase *base;
  /**/ if (e.kind() == cpp_entity_kind::function_t)
    base = process_function(static_cast<const cpp_function &>(e), cppFile);

  else if (e.kind() == cpp_entity_kind::variable_t)
    base = process_variable(static_cast<const cpp_variable &>(e), cppFile);
  
  else if (e.kind() == cpp_entity_kind::class_t)
    base = process_class(static_cast<const cpp_class &>(e), cppFile);
  
  else if (e.kind() == cpp_entity_kind::constructor_t)
    base = process_constructor(static_cast<const cpp_constructor &>(e), cppFile);
  
  else if (e.kind() == cpp_entity_kind::member_function_t)
    base = process_member_function(static_cast<const cpp_member_function &>(e), cppFile);
  
  else if (e.kind() == cpp_entity_kind::member_variable_t)
    base = process_member_variable(static_cast<const cpp_member_variable &>(e), cppFile);
  
  else if (e.kind() == cpp_entity_kind::enum_t)
    base = process_enum(static_cast<const cpp_enum &>(e), cppFile);
  
  // else if (e.kind() == cpp_entity_kind::type_alias_t)
  //   process_type_alias(static_cast<const cpp_type_alias &>(e), cppFile);
  
  // else if (e.kind() == cpp_entity_kind::macro_definition_t)
  //   process_macro(static_cast<const cpp_macro_definition &>(e), cppFile);
  
  // else if (e.kind() == cpp_entity_kind::enum_value_t)
  //   process_enum_value(static_cast<const cpp_enum_value &>(e), cppFile);

  if (e.comment().has_value()) {
    base->comment = e.comment().value();
  }
  for (auto &attr : e.attributes()) {
    CppAttribute cppAttribute;
    cppAttribute.name = attr.name();
    if (attr.arguments().has_value()) {
      for (auto &arg : attr.arguments().value()) {
        cppAttribute.arguments.push_back(arg.spelling);
      }
    }
  }
}

CppFile process_ast(const cpp_file &file, const string &filename) {
  CppFile result;

  visit(file, [&](const cpp_entity &e, visitor_info info) {
    if (info.event != visitor_info::container_entity_exit &&
        e.kind() != cpp_entity_kind::file_t)
      process_entity(e, result);

    return true;
  });

  return result;
}

int main(int argc, char **argv) {
  cppast::libclang_compile_config config;

  cppast::compile_flags flags;
  // flags |= cppast::compile_flag::gnu_extensions;
  config.set_flags(cppast::cpp_standard::cpp_1z, flags);

  config.remove_comments_in_macro(true);
  config.fast_preprocessing(false);

  for (int i = 1; i < argc; i++) {
    string s = argv[i];
    if (s == "-I" && i + 1 < argc) {
      config.add_include_dir(argv[i + 1]);
      i++;
    }
  }

  string s(argv[1]);
  /*
  cppast::cpp_entity_index index;
  cppast::libclang_compilation_database database(argv[1]);
  cppast::libclang_compile_config config;
  cppast::simple_file_parser<cppast::libclang_parser>
  parser(type_safe::ref(index)); cppast::parse_database(parser, database);
  */

  try {
    cppast::cpp_entity_index idx;
    cppast::stderr_diagnostic_logger logger;
    cppast::libclang_parser parser(type_safe::ref(logger));
    auto f = parser.parse(idx, s, config);

    CppFile cpp = process_ast(*f, s);

    cout << cpp.classes.size() << endl;

  } catch (exception e) {
    cerr << "An Error ocurred: " << e.what() << endl;
  }

  cout << "Done" << endl;

  return 0;
}
