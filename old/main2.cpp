#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_forward_declarable.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_type_alias.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>

#include <nlohmann/json.hpp>

using namespace std;
using namespace cppast;
using json = nlohmann::json;

void process_entity(const cpp_entity &e, const string &filename, json &j) {
  if (e.kind() == cpp_entity_kind::function_t) {
    auto &f = static_cast<const cpp_function &>(e);

    j[filename]["functions"][f.name()] = {
        {"return_type", to_string(f.return_type())},
        {"parameters", json::array()}};

    for (auto &p : f.parameters()) {
      string default_value = "";
      if (p.default_value().has_value()) {
        if (p.default_value().value().kind() ==
            cpp_expression_kind::literal_t) {
          auto &l = static_cast<const cpp_literal_expression &>(
              p.default_value().value());
          default_value = l.value();
        } else if (p.default_value().value().kind() ==
                   cpp_expression_kind::unexposed_t) {
          auto &l = static_cast<const cpp_unexposed_expression &>(
              p.default_value().value());
          default_value = l.expression().as_string();
        }
      }
      j[filename]["functions"][f.name()]["parameters"].push_back(
          {{"name", p.name()},
           {"type", to_string(p.type())},
           {"default_value", default_value}});
    }
  } else if (e.kind() == cpp_entity_kind::class_t) {
    auto &c = static_cast<const cpp_class &>(e);

    auto parent = c.parent();
    string scope = "";
    while (parent.has_value() &&
           parent.value().kind() != cpp_entity_kind::file_t) {
      scope = parent.value().name() + "::" + scope;
      parent = parent.value().parent();
    }
    j[filename]["classes"][c.name()] = {{"scope", scope},
                                        {"bases", json::array()}};
    for (auto &b : c.bases())
      j[filename]["classes"][c.name()]["bases"].push_back(b.name());
  } else if (e.kind() == cpp_entity_kind::member_function_t) {
    auto &m = static_cast<const cpp_member_function &>(e);
    if (m.parent().has_value() &&
        classes.find(m.parent().value().name()) != classes.end()) {
      s_function n_fun;
      n_fun.name = m.name();
      n_fun.return_type = &m.return_type();
      for (auto &p : m.parameters()) {
        if (p.default_value().has_value())
          classes[m.parent().value().name()].funs.push_back(n_fun);
        s_variable n_var;
        n_var.name = p.name();
        n_var.type = &p.type();
        n_fun.params.push_back(n_var);
      }
      classes[m.parent().value().name()].funs.push_back(n_fun);
    }
  }
  /*
     else if (e.kind() == cpp_entity_kind::constructor_t)
     {
     auto& m = static_cast<const cpp_constructor&>(e);
     if (m.parent().has_value() && classes.find(m.parent().value().name()) !=
classes.end())
     {
     s_function n_fun;
     n_fun.constructor = true;
     for (auto& p: m.parameters())
     {
     if (p.default_value().has_value())
     classes[m.parent().value().name()].funs.push_back(n_fun);
     s_variable n_var;
     n_var.name = p.name();
     n_var.type = &p.type();
     n_fun.params.push_back(n_var);
     }
     classes[m.parent().value().name()].funs.push_back(n_fun);
     }
     }
     else if (e.kind() == cpp_entity_kind::member_variable_t)
     {
     auto& v = static_cast<const cpp_member_variable&>(e);
     if (v.parent().has_value() && classes.find(v.parent().value().name()) !=
classes.end())
     {
     s_variable n_var;
     n_var.name = v.name();
     n_var.type = &v.type();
     classes[v.parent().value().name()].vars.push_back(n_var);
     }
     }
     else if (e.kind() == cpp_entity_kind::type_alias_t)
     {
     auto& c = static_cast<const cpp_type_alias&>(e);
     if (classes.find(e.name()) == classes.end() &&
     typedefs.find(e.name()) == typedefs.end() &&
     enums.find(e.name()) == enums.end())
     {
     s_typedef n_typedef;
     n_typedef.name = c.name();
     n_typedef.ref = &c.underlying_type();
     typedefs.emplace(c.name(), n_typedef);
     }
     }
     else if (e.kind() == cpp_entity_kind::enum_t)
     {
     auto& a = static_cast<const cpp_enum&>(e);
     if (classes.find(e.name()) == classes.end() &&
     typedefs.find(e.name()) == typedefs.end() &&
     enums.find(e.name()) == enums.end())
     {
  s_enum n_enum;
  n_enum.name = a.name();
  enums.emplace(a.name(), n_enum);
}
}
else if (e.kind() == cpp_entity_kind::enum_value_t)
{
  auto& v = static_cast<const cpp_enum_value&>(e);
  if (v.parent().has_value() && enums.find(v.parent().value().name()) !=
enums.end())
  {
    s_enum_value n_val;
    n_val.name = v.name();
    n_val.has_value = v.value().has_value();
    if (n_val.has_value && v.value().value().kind() ==
cpp_expression_kind::literal_t) n_val.value = static_cast<const
cpp_literal_expression&>(v.value().value()).value();
  }
}
*/
}

void process_ast(const cppast::cpp_file &file, const string &filename,
                 json &j) {

  cppast::visit(file,
                [&](const cppast::cpp_entity &e, cppast::visitor_info info) {
                  if (e.kind() == cppast::cpp_entity_kind::file_t ||
                      cppast::is_templated(e) || cppast::is_friended(e))
                    return true;
                  else {
                    if (info.access == cpp_access_specifier_kind::cpp_public)
                      process_entity(e, filename, j);
                  }

                  return true;
                });
}

unique_ptr<cppast::cpp_file>
parse_file(const cppast::libclang_compile_config &config,
           const cppast::diagnostic_logger &logger, const string &filename,
           bool fatal_error) {
  cppast::cpp_entity_index idx;

  cppast::libclang_parser parser(type_safe::ref(logger));

  auto file = parser.parse(idx, filename, config);
  if (fatal_error && parser.error())
    return nullptr;
  return file;
}

int main(int argc, char **argv) {
  // the compile config stores compilation flags
  cppast::libclang_compile_config config;

  /*
     flags |= cppast::compile_flag::gnu_extensions;
     flags |= cppast::compile_flag::ms_extensions;
     flags |= cppast::compile_flag::ms_compatibility;
     */

  /*
     config.set_flags(cppast::cpp_standard::cpp_98, flags);
     config.set_flags(cppast::cpp_standard::cpp_03, flags);
     config.set_flags(cppast::cpp_standard::cpp_11, flags);
     config.set_flags(cppast::cpp_standard::cpp_14, flags);
     */

  // cppast::compile_flags flags;
  // flags |= cppast::compile_flag::gnu_extensions;
  // config.set_flags(cppast::cpp_standard::cpp_1z, flags);

  json j;

  cppast::stderr_diagnostic_logger logger;
  for (int i = 1; i < argc; i++) {
    string s = argv[i];
    try {
      cerr << "Parsing " << s << endl;
      auto file = parse_file(config, logger, s, false);
      if (!file)
        continue;

      cerr << "Done parsing " << s << endl;
      cerr << "Processing " << s << endl;

      j[s] = {{"functions", {}}, {"classes", {}}};

      process_ast(*file, s, j);

      cerr << "Done processing " << s << endl;
    } catch (exception e) {
      cerr << "An Error ocurred: " << e.what() << endl;
    }
  }

  cout << j.dump(2) << endl;

  return 0;
}
