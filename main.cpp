#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include <list>

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

static cppast::cpp_entity_index idx;
static vector<string> scope;

static string get_scope_string() {
  string result = "";
  for (int i = 0; i < scope.size(); i++) {
    if (i > 0)
      result += "::";
    result += scope[i];
  }
  return result;
}

void process_entity(const cpp_entity &e, const string &name, const string &filename, json &j);

void process_function(const cpp_function &f, const string &name, const string &filename, json &j) {
  j[filename]["functions"].push_back(json::object());
  auto &newJ = j[filename]["functions"].back();
  newJ["name"] = name;

  newJ["scope"] = json::array();
  for (auto& s: scope)
    newJ["scope"].push_back(s);
  newJ["return_type"] = to_string(f.return_type());
  newJ["parameters"] = json::array();
  if (f.comment().has_value())
    newJ["comment"] = f.comment().value();

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
    newJ["parameters"].push_back({{"name", p.name()},
        {"type", to_string(p.type())},
        {"default_value", default_value}});
  }
}

void process_class(const cpp_class &c, const string &name, const string &filename, json &j) {
  auto parent = c.parent();
  list<string> scope;
  while (parent.has_value() &&
      parent.value().kind() != cpp_entity_kind::file_t) {
    scope.push_front(parent.value().name());
    parent = parent.value().parent();
  }

  if (j[filename]["classes"].contains(name))
    return;
  auto &newJ = j[filename]["classes"][name];

  newJ["scope"] = json::array();
  for (auto& s: scope)
    newJ["scope"].push_back(s);
  newJ["bases"] = json::array();
  newJ["variables"] = json::array();
  newJ["methods"] = json::array();
  newJ["constructors"] = json::array();

  for (auto &b : c.bases())
    newJ["bases"].push_back(b.name());
}

void process_member_function(const cpp_member_function &m, const string &name, const string &filename, json &j) {
  j[filename]["classes"][m.parent().value().name()]["methods"].push_back({});
  auto &newJ =
    j[filename]["classes"][m.parent().value().name()]["methods"].back();
  newJ["name"] = name;
  newJ["return_type"] = to_string(m.return_type());
  newJ["parameters"] = json::array();

  for (auto &p : m.parameters()) {
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
    newJ["parameters"].push_back({{"name", p.name()},
        {"type", to_string(p.type())},
        {"default_value", default_value}});
  }
}

void process_constructor(const cpp_constructor &m, const string &name, const string &filename, json &j) {
  try {
    if (j[filename]["classes"][m.parent().value().name()]["constructors"]
        .empty())
      j[filename]["classes"][m.parent().value().name()]["constructors"] =
        json::array();
    j[filename]["classes"][m.parent().value().name()]["constructors"]
      .push_back({});
    auto &newJ =
      j[filename]["classes"][m.parent().value().name()]["constructors"]
      .back();
    newJ["parameters"] = json::array();

    for (auto &p : m.parameters()) {
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
      newJ["parameters"].push_back({{"name", p.name()},
          {"type", to_string(p.type())},
          {"default_value", default_value}});
    }
  } catch (exception e) {
  }
}

void process_member_variable(const cpp_member_variable &v, const string &name, const string &filename, json &j) {
  try {
    if (j[filename]["classes"][v.parent().value().name()]["variables"]
        .empty())
      j[filename]["classes"][v.parent().value().name()]["variables"] =
        json::array();
    j[filename]["classes"][v.parent().value().name()]["variables"].push_back(
        {});
    auto &newJ =
      j[filename]["classes"][v.parent().value().name()]["variables"].back();

    newJ["name"] = name;
    newJ["type"] = to_string(v.type());
  } catch (exception e) {
  }
}

void process_type_alias(const cpp_type_alias &c, const string &name, const string &filename, json &j) {
  if (name == to_string(c.underlying_type())) return;
  auto &newJ = j[filename]["type_aliases"][name];
  newJ["underlying_type"] = to_string(c.underlying_type());
  if (c.underlying_type().kind() == cpp_type_kind::user_defined_t) {
    auto &user_type =
      static_cast<const cpp_user_defined_type &>(c.underlying_type());
    auto &entity = user_type.entity();
    for (auto &id : entity.id()) {
      auto opt_entity = idx.lookup(id);
      if (opt_entity.has_value())
        process_entity(opt_entity.value(), name, filename, j);
    }
  }
}

void process_enum(const cpp_enum &c, const string &name, const string &filename, json &j) {
  if (j[filename]["enums"].contains(name))
    return;
  auto &newJ = j[filename]["enums"][name];
  newJ["values"] = json::array();

  for (auto &v : c) {
    string value = "";
    if (v.value().has_value()) {
      if (v.value().value().kind() == cpp_expression_kind::literal_t) {
        auto &l =
          static_cast<const cpp_literal_expression &>(v.value().value());
        value = l.value();
      } else if (v.value().value().kind() ==
          cpp_expression_kind::unexposed_t) {
        auto &l =
          static_cast<const cpp_unexposed_expression &>(v.value().value());
        value = l.expression().as_string();
      }
    }
    newJ["values"].push_back({{"name", v.name()}, {"value", value}});
  }
}

void process_macro(const cpp_macro_definition &m, const string &name, const string &filename, json &j) {
  j[filename]["macros"].push_back(
      {{"name", name}, {"replacement", m.replacement()}});
  auto &newJ = j[filename]["macros"].back();
}

void process_entity(const cpp_entity &e, const string &name, const string &filename, json &j) {
  if (e.kind() == cpp_entity_kind::function_t) {
    auto &x = static_cast<const cpp_function &>(e);
    process_function(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::class_t) {
    auto &x = static_cast<const cpp_class &>(e);
    process_class(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::member_function_t) {
    auto &x = static_cast<const cpp_member_function &>(e);
    process_member_function(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::constructor_t) {
    auto &x = static_cast<const cpp_constructor &>(e);
    process_constructor(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::member_variable_t) {
    auto &x = static_cast<const cpp_member_variable &>(e);
    process_member_variable(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::type_alias_t) {
    auto &x = static_cast<const cpp_type_alias &>(e);
    process_type_alias(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::enum_t) {
    auto &x = static_cast<const cpp_enum &>(e);
    process_enum(x, name, filename, j);
  } else if (e.kind() == cpp_entity_kind::macro_definition_t) {
    auto &x = static_cast<const cpp_macro_definition &>(e);
    process_macro(x, name, filename, j);
  }
  /*
     else if (e.kind() == cpp_entity_kind::enum_value_t)
     {
     auto& v = static_cast<const cpp_enum_value&>(e);
     string value = "";
     if (!v.value().has_value() || !v.parent().has_value())
     return;
     if (v.value().value().kind() == cpp_expression_kind::literal_t)
     {
     auto& l = static_cast<const cpp_literal_expression&>(v.value().value());
     value = l.value();
     }
     else if (v.value().value().kind() == cpp_expression_kind::unexposed_t)
     {
     auto& l = static_cast<const cpp_unexposed_expression&>(v.value().value());
     value = l.expression().as_string();
     }
     j[filename]["enums"][v.parent().value().name()]["values"].push_back({
     {"name", name},
     {"value", value}
     });
     }
     */
}

void process_ast(const cppast::cpp_file &file, const string &filename, json &j) {
  cppast::visit(file,
      [&](const cppast::cpp_entity &e, cppast::visitor_info info) {
        if (e.kind() == cpp_entity_kind::namespace_t) {
          auto &n = static_cast<const cpp_namespace &>(e);
          if (info.event == visitor_info::container_entity_enter)
            scope.push_back(n.name());
          else
            scope.pop_back();

          return true;
        }
        if (info.event != visitor_info::container_entity_exit &&
            e.kind() != cppast::cpp_entity_kind::file_t &&
            !cppast::is_templated(e) &&
            !cppast::is_friended(e))
            //info.access == cpp_access_specifier_kind::cpp_public)
          process_entity(e, e.name(), filename, j);

        return true;
      });
}

/*
void test(const cpp_file &file) {
  visit(file, [](const cpp_entity &e, visitor_info info) {
      cout << to_string(e.kind()) << endl;
      });
}
*/

int main(int argc, char **argv) {
  // the compile config stores compilation flags
  cppast::libclang_compile_config config;

  // cppast::compile_flags flags;
  // flags |= cppast::compile_flag::gnu_extensions;
  // config.set_flags(cppast::cpp_standard::cpp_1z, flags);
  cppast::compile_flags flags;
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

  json j;
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

    // for (auto& f: parser.files()) {
    //  cerr << "Processing file " << f.name() << endl;
    j[s] = {{"functions", json::array()},
      {"macros", json::array()},
      {"classes", json::object()},
      {"enums", json::object()},
      {"type_aliases", json::object()}};

    process_ast(*f, s, j);
    //test(*f);
    //}
  } catch (exception e) {
    cerr << "An Error ocurred: " << e.what() << endl;
  }

  cout << j.dump(2) << endl;

  return 0;
}
