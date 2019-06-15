
#include <iostream>
#include <memory>
#include <map>
#include <stack>
#include <variant>
#include <algorithm>
#include <sstream>

#include "debug_logger.h"



#define DEBUG_LOGGER_TRACE_LISP          // DEBUG_LOGGER("lisp ", logger_indent_lisp_t::indent)
#define DEBUG_LOGGER_LISP(...)           DEBUG_LOG("lisp ", logger_indent_lisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent; };

struct logger_indent_lisp_t   : logger_indent_t<logger_indent_lisp_t> { };



using namespace std::string_literals;

namespace lisp_interpreter {

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


  struct object_nil_t { };

  struct object_error_t {
    std::string msg;

    object_error_t(const std::string& msg) : msg(msg) { }
  };

  struct object_variable_t {
    std::string name;

    object_variable_t(const std::string& name) : name(name) { }
  };

  struct object_lambda_t;
  using object_lambda_sptr_t = std::shared_ptr<const object_lambda_t>;

  struct object_macro_t;
  using object_macro_sptr_t = std::shared_ptr<const object_macro_t>;

  struct object_list_t;
  using object_list_sptr_t = std::shared_ptr<const object_list_t>;

  enum class op_t {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    SCONCAT,

    GT,
    GTE,
    LT,
    LTE,
    EQ,
    NOEQ,

    DEF,
    SET,
    GET,
    CALL,
    QUOTE,
    TYPEOF,
    CONS,
    CAR,
    CDR,
    COND,
    PRINT,
    READ,
    EVAL,
    EVALIN,
    EVALSEQ,
    LAMBDA,
    MACRO,
    MACROEXPAND,
  };

  std::map<std::string, op_t> keywords = {
    { "+",             op_t::ADD },
    { "-",             op_t::SUB },
    { "*",             op_t::MUL },
    { "/",             op_t::DIV },
    { "mod",           op_t::MOD },
    { "%",             op_t::MOD },
    { "++",            op_t::SCONCAT },
    { ">",             op_t::GT },
    { ">=",            op_t::GTE },
    { "<",             op_t::LT },
    { "<=",            op_t::LTE },
    { "==",            op_t::EQ },
    { "/=",            op_t::NOEQ },
    { "!=",            op_t::NOEQ },
    { "def",           op_t::DEF },
    { "set!",          op_t::SET },
    { "get",           op_t::GET },
    { "call",          op_t::CALL },
    { "quote",         op_t::QUOTE },
    { "typeof",        op_t::TYPEOF },
    { "cons",          op_t::CONS },
    { "car",           op_t::CAR },
    { "cdr",           op_t::CDR },
    { "cond",          op_t::COND },
    { "print",         op_t::PRINT },
    { "read",          op_t::READ },
    { "eval",          op_t::EVAL },
    { "evalin",        op_t::EVALIN },
    { "evalseq",       op_t::EVALSEQ },
    { "lambda",        op_t::LAMBDA },
    { "macro",         op_t::MACRO },
    { "macroexpand",   op_t::MACROEXPAND },
  };

  using object_t = std::variant<
    object_nil_t,          // ()
    object_error_t,        // error
    bool,                  // bool
    int64_t,               // number
    double,                // number
    std::string,           // string
    op_t,                  // operator
    object_variable_t,     // symbols
    object_lambda_sptr_t,  // lambda
    object_macro_sptr_t,   // macro
    object_list_sptr_t     // list
    // object_map_sptr_t,  // map   // TODO
    // object_array_sptr_t // array // TODO
  >;

  struct env_t;
  using envs_t = std::shared_ptr<env_t>;

  struct object_lambda_t {
    object_t args;
    object_t body;
    envs_t   env;

    object_lambda_t(const object_t& args, const object_t& body, envs_t env)
        : args(args), body(body), env(env) { }
  };

  struct object_macro_t {
    object_t args;
    object_t body;

    object_macro_t(const object_t& args, const object_t& body)
        : args(args), body(body) { }
  };

  struct object_list_t {
    object_t head;
    object_t tail;

    object_list_t(const object_t& head, const object_t& tail) : head(head), tail(tail) { }
  };

  struct env_t : std::enable_shared_from_this<env_t> {
    using key_t = std::string;
    using val_t = object_t;

    std::map<key_t, val_t> frame;
    envs_t parent;

    env_t(envs_t parent = nullptr) : parent(parent) { }
    object_t defvar(const key_t& key, const val_t& val);
    object_t setvar(const key_t& key, const val_t& val);
    object_t getvar(const key_t& key);
  };

  struct context_t {
    std::stringstream stream;
    size_t eval_calls;
    // size_t stack_level_max;
    // size_t stack_level;

    context_t() : stream{}, eval_calls{} { }

    /*std::string show() {
      stream << std::endl;
      stream << "eval_calls: " << eval_calls << std::endl;
      std::string ret = stream.str();
      stream.clear();
      return ret;
    }*/
  };



  // FD

  std::string show(const object_t& object);
  object_t parse(const std::string& str);
  object_t eval(const object_t& object, envs_t env, context_t& ctx);



  // I N T E R F A C E

  object_t nil() {
    return object_nil_t();
  }

  object_t error(const std::string& msg) {
    return object_error_t(msg);
  }

  object_t atom_bool(bool value) {
    return value;
  }

  object_t atom_int(int64_t value) {
    return value;
  }

  object_t atom_double(double value) {
    return value;
  }

  object_t atom_string(const std::string& value) {
    return value;
  }

  object_t atom_operator(op_t value) {
    return value;
  }

  object_t atom_variable(const std::string& value) {
    return object_variable_t(value);
  }

  object_t lambda(const object_t& args, const object_t& body, envs_t env) {
    return std::make_shared<object_lambda_t>(args, body, env);
  }

  object_t macro(const object_t& args, const object_t& body) {
    return std::make_shared<object_macro_t>(args, body);
  }

  object_t list(const object_t& head, const object_t& tail) {
    return std::make_shared<object_list_t>(head, tail);
  }

  const object_error_t* as_atom_error(const object_t& object) {
    return std::get_if<object_error_t>(&object);
  }

  const bool* as_atom_bool(const object_t& object) {
    return std::get_if<bool>(&object);
  }

  const int64_t* as_atom_int(const object_t& object) {
    return std::get_if<int64_t>(&object);
  }

  const double* as_atom_double(const object_t& object) {
    return std::get_if<double>(&object);
  }

  const std::string* as_atom_string(const object_t& object) {
    return std::get_if<std::string>(&object);
  }

  const op_t* as_atom_operator(const object_t& object) {
    return std::get_if<op_t>(&object);
  }

  const object_variable_t* as_atom_variable(const object_t& object) {
    return std::get_if<object_variable_t>(&object);
  }

  const object_lambda_sptr_t* as_atom_lambda(const object_t& object) {
    return std::get_if<object_lambda_sptr_t>(&object);
  }

  const object_macro_sptr_t* as_atom_macro(const object_t& object) {
    return std::get_if<object_macro_sptr_t>(&object);
  }

  const object_list_sptr_t* as_atom_list(const object_t& object) {
    return std::get_if<object_list_sptr_t>(&object);
  }


  bool is_nil(const object_t& object) {
    return std::get_if<object_nil_t>(&object);
  }

  bool is_list(const object_t& object) {
    return std::get_if<object_list_sptr_t>(&object);
  }

  bool is_error(const object_t& object) {
    return std::get_if<object_error_t>(&object);
  }

  object_t env_t::defvar(const key_t& key, const val_t& val) {
    DEBUG_LOGGER_TRACE_LISP;
    frame[key] = val;
    return val;
  }

  object_t env_t::setvar(const key_t& key, const val_t& val) {
    DEBUG_LOGGER_TRACE_LISP;
    auto env = shared_from_this();
    while (env) {
      auto &fr = env->frame;
      auto it = fr.find(key);
      if (it != fr.end()) {
        it->second = val;
        return val;
      }
      env = env->parent;
    }
    return error("variable '" + key + "' not found");
  }

  object_t env_t::getvar(const key_t& key) {
    DEBUG_LOGGER_TRACE_LISP;
    auto env = shared_from_this();
    while (env) {
      auto &fr = env->frame;
      auto it = fr.find(key);
      if (it != fr.end()) {
        return it->second;
      }
      env = env->parent;
    }
    return error("variable '" + key + "' not found");
  }

  std::pair<object_t, object_t> decompose(const object_t& object) {
    DEBUG_LOGGER_TRACE_LISP;
    if (is_error(object))
      return {object, object};
    if (!is_list(object)) {
      auto ret = error("decompose: unexpected '" + show(object) + "', expected list in '" + show(object) + "'");
      return {ret, ret};
    }
    auto list = std::get<object_list_sptr_t>(object);
    return {list->head, list->tail};
  }

  object_t car(const object_t& object) {
    DEBUG_LOGGER_TRACE_LISP;
    return decompose(object).first;
  }

  object_t cdr(const object_t& object) {
    DEBUG_LOGGER_TRACE_LISP;
    return decompose(object).second;
  }

  object_t cons(const object_t& head, const object_t& tail) {
    DEBUG_LOGGER_TRACE_LISP;
    if (is_error(head)) return head;
    if (is_error(tail)) return tail;
    if (!is_list(tail) && !is_nil(tail))
      return error("cons: unexpected '" + show(tail) + "', expected list or nil in '" + show(tail) + "'");
    // assert(show(car(list(head, tail))) == show(head));
    // assert(show(cdr(list(head, tail))) == show(tail));
    return list(head, tail);
  }



  // L I B R A R Y

  object_t reverse(const object_t& object, bool recursive = true) {
    DEBUG_LOGGER_TRACE_LISP;
    if (!is_list(object))
      return object;
    auto list_r = nil();
    auto obj = object;
    while (is_list(obj)) {
      list_r = cons(recursive ? reverse(car(obj)) : car(obj), list_r);
      obj = cdr(obj);
    }
    return list_r;
  }

  std::string show_operator(op_t op) {
    switch (op) {
      case op_t::ADD:          return "+";
      case op_t::SUB:          return "-";
      case op_t::MUL:          return "*";
      case op_t::DIV:          return "/";
      case op_t::MOD:          return "%";
      case op_t::SCONCAT:      return "++";
      case op_t::GT:           return ">";
      case op_t::GTE:          return ">=";
      case op_t::LT:           return "<";
      case op_t::LTE:          return "<=";
      case op_t::EQ:           return "==";
      case op_t::NOEQ:         return "!=";
      case op_t::DEF:          return "def";
      case op_t::SET:          return "set";
      case op_t::GET:          return "get";
      case op_t::CALL:         return "call";
      case op_t::QUOTE:        return "quote";
      case op_t::TYPEOF:       return "typeof";
      case op_t::CONS:         return "cons";
      case op_t::CAR:          return "car";
      case op_t::CDR:          return "cdr";
      case op_t::COND:         return "cond";
      case op_t::PRINT:        return "print";
      case op_t::READ:         return "read";
      case op_t::EVAL:         return "eval";
      case op_t::EVALIN:       return "evalin";
      case op_t::EVALSEQ:      return "evalseq";
      case op_t::LAMBDA:       return "lambda";
      case op_t::MACRO:        return "macro";
      case op_t::MACROEXPAND:  return "macroexpand";
      default:                 return "O:U";
    }
  }

  std::string show(const object_t& object) {
    DEBUG_LOGGER_TRACE_LISP;
    std::string str;
    std::visit(overloaded {
        [&str] (object_nil_t) {
          str += "()";
        },
        [&str] (object_error_t error) { // Сюда не должны попасть
          str += "E:\""s + error.msg + "\"";
        },
        [&str] (bool v) {
          str += v ? "true" : "false";
        },
        [&str] (int64_t v) {
          str += std::to_string(v);
        },
        [&str] (double v) {
          str += std::to_string(v);
        },
        [&str] (const std::string& v) {
          str += "\"" + v + "\"";
        },
        [&str] (op_t v) {
          str += show_operator(v);
        },
        [&str] (object_variable_t v) {
          str += v.name;
        },
        [&str] (object_lambda_sptr_t v) {
          str += "(lamdba " + show(v->args) + " " + show(v->body) + ")";
        },
        [&str] (object_macro_sptr_t v) {
          str += "(macro " + show(v->args) + " " + show(v->body) + ")";
        },
        [&str, &object] (object_list_sptr_t) {
          str += "(";
          auto l = object;
          while (!is_nil(l)) {
            str += show(car(l));
            if (!is_nil(cdr(l))) str += " ";
            l = cdr(l);
          }
          str += ")";
        },
    }, object);
    return str;
  }

  object_t parse(const std::string& str) {
    DEBUG_LOGGER_TRACE_LISP;
    std::stack<object_t> stack;
    auto ret = nil();
    const char *it = str.c_str();
    const char *ite = str.c_str() + str.size();
    while (it != ite) {
      char c = *it;
      if (c == '(') {
        stack.emplace(nil());
      } else if (c == ')') {
        if (stack.empty()) return error("unexpected ')' on pos " + std::to_string(it - str.c_str()));

        auto tmp = stack.top();
        stack.pop();

        if (!stack.empty()) {
          auto object_n = cons(tmp, stack.top());
          stack.top().swap(object_n);
        } else {
          ret = cons(tmp, ret);
        }
      } else if (c == '"') {
        auto itf = std::find(it + 1, ite, '"');
        if (itf == ite) return error("parse: expected '\"' on pos " + std::to_string(it - str.c_str()));
        auto object_atom = atom_string(std::string(it + 1, itf));
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += (itf - it);
      } else if (std::isgraph(c)) {
        auto itf = std::find_if(it, ite, [](auto c) { return !isgraph(c) || c == '(' || c == ')'; });
        std::string value = std::string(it, itf);
        std::string value_lower = value;
        std::transform(value.begin(), value.end(), value_lower.begin(), ::tolower);
        object_t object_atom;
        if (auto itk = keywords.find(value_lower); itk != keywords.end()) {
          object_atom = atom_operator(itk->second);
        } else if (value_lower == "true") {
          object_atom = atom_bool(true);
        } else if (value_lower == "false") {
          object_atom = atom_bool(false);
        } else if ((std::isdigit(c) || c == '+' || c == '-') && value.find('.') == std::string::npos) {
          object_atom = atom_int(stoi(value));
        } else if (std::isdigit(c) || c == '+' || c == '-') {
          object_atom = atom_double(stod(value));
        } else {
          object_atom = atom_variable(value);
        }
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += (itf - it) - 1;
      }
      ++it;
    }
    if (!stack.empty()) return error("expected ')' on pos " + std::to_string(it - str.c_str()));
    if (is_nil(cdr(ret)) && is_list(car(ret))) ret = car(ret);
    return reverse(ret);
  }

  void for_each(const object_t& object, auto f) {
    auto l = object;
    while (!is_nil(l)) {
      auto p = decompose(l);
      if (!f(p.first)) break;
      l = p.second;
    }
  }



  object_t eval_quote(const object_t& head, const object_t& tail, envs_t, context_t&) {
    if (is_nil(tail)) return error("quote: expected argument in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("quote: wrong argument '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    return p.first;
  }

  object_t eval_cons(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("cons: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (is_nil(p.second)) return error("cons: expected argument (2) in '" + show(cons(head, tail)) + "'");
    auto h = eval(p.first, env, ctx);
    p = decompose(p.second);
    if (!is_nil(p.second)) return error("cons: wrong argument (3) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto t = eval(p.first, env, ctx);
    return cons(h, t);
  }

  object_t eval_car(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("car: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("car: wrong argument (2) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto t = eval(p.first, env, ctx);
    return car(t);
  }

  object_t eval_cdr(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("cdr: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("cdr: wrong argument (2) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto t = eval(p.first, env, ctx);
    return cdr(t);
  }

  object_t eval_cond(const object_t&, const object_t& tail, envs_t env, context_t& ctx) {
    auto ret = nil();
    for_each(tail, [&ret, &env, &ctx](const object_t& object) -> bool {
      auto p = decompose(object);
      auto condition = eval(p.first, env, ctx);
      if (auto b = as_atom_bool(condition); b) {
        if (*b) {
          ret = eval(car(p.second), env, ctx); // XXX
          return false;
        }
      } else {
        ret = error("cond: unexpected '" + show(condition)
            + "', expected bool expression in '" + show(p.first) + "'");
        return false;
      }
      return true;
    });
    return ret;
  }

  object_t eval_arithmetic(const auto& op, const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("eval_arithmetic: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    auto ret = eval(p.first, env, ctx);
    if (!as_atom_int(ret) && !as_atom_double(ret)) return error("eval_arithmetic: unexpected '" + show(ret) + "' in '" + show(cons(head, tail)) + "'");
    for_each(p.second, [&ret, &op, &head, &tail, &env, &ctx](const object_t& object) -> bool {
      auto curr = eval(object, env, ctx);
      if (as_atom_int(ret) && as_atom_int(curr)) {
        ret = op(*as_atom_int(ret), *as_atom_int(curr));
      } else if (as_atom_int(ret) && as_atom_double(curr)) {
        ret = op(static_cast<double>(*as_atom_int(ret)), *as_atom_double(curr));
      } else if (as_atom_double(ret) && as_atom_int(curr)) {
        ret = op(*as_atom_double(ret), static_cast<double>(*as_atom_int(curr)));
      } else {
        ret = error("eval_arithmetic: unexpected '" + show(curr) + "' in '" + show(cons(head, tail)) + "'");
        return false;
      }
      return true;
    });
    return ret;
  }

  object_t eval_concat(const auto& op, const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("eval_concat: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    auto ret = eval(p.first, env, ctx);
    for_each(p.second, [&ret, &op, &head, &tail, &env, &ctx](const object_t& object) -> bool {
      auto curr = eval(object, env, ctx);
      if (as_atom_string(ret) && as_atom_string(curr)) {
        ret = op(*as_atom_string(ret), *as_atom_string(curr));
      } else {
        ret = error("eval_concat: unexpected '" + show(curr) + "' in '" + show(cons(head, tail)) + "'");
        return false;
      }
      return true;
    });
    return ret;
  }

  object_t eval_cmp(const auto& op, const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("eval_cmp: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    auto acc = eval(p.first, env, ctx);
    auto ret = atom_bool(true);
    for_each(p.second, [&acc, &ret, &op, &head, &tail, &env, &ctx](const object_t& object) -> bool {
      auto curr = eval(object, env, ctx);
      bool b = true;
      if (as_atom_int(acc) && as_atom_int(curr)) {
        b = op(*as_atom_int(acc), *as_atom_int(curr));
      } else if (as_atom_double(acc) && as_atom_double(curr)) {
        b = op(*as_atom_double(acc), *as_atom_double(curr));
      } else if (as_atom_int(acc) && as_atom_double(curr)) {
        ret = op(static_cast<double>(*as_atom_int(acc)), *as_atom_double(curr));
      } else if (as_atom_double(acc) && as_atom_int(curr)) {
        ret = op(*as_atom_double(acc), static_cast<double>(*as_atom_int(curr)));
      } else if (as_atom_string(acc) && as_atom_string(curr)) {
        b = op(*as_atom_string(acc), *as_atom_string(curr));
      } else {
        ret = error("eval_cmp: unexpected '" + show(curr) + "' in '" + show(cons(head, tail)) + "'");
        return false;
      }
      if (!b) {
        ret = atom_bool(false);
        return false;
      }
      acc = curr;
      return true;
    });
    return ret;
  }

  object_t eval_typeof(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("typeof: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("typeof: wrong argument (2) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    std::string type;
    auto curr = eval(p.first, env, ctx);
    std::visit(overloaded {
      [&type] (object_nil_t)         { type = "nil"; },
      [&type] (object_error_t)       { type = "error"; },
      [&type] (bool)                 { type = "bool"; },
      [&type] (int64_t)              { type = "int"; },
      [&type] (double)               { type = "double"; },
      [&type] (const std::string&)   { type = "string"; },
      [&type] (op_t)                 { type = "operator"; },
      [&type] (object_variable_t)    { type = "variable"; },
      [&type] (object_lambda_sptr_t) { type = "lambda"; },
      [&type] (object_macro_sptr_t)  { type = "macro"; },
      [&type] (object_list_sptr_t)   { type = "list"; },
    }, curr);
    return atom_string(type);
  }

  object_t eval_eval(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("eval: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("eval: wrong argument (2) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    return eval(p.first, env, ctx);
  }

  object_t eval_evalseq(const object_t&, const object_t& tail, envs_t env, context_t& ctx) {
    auto ret = nil();
    for_each(tail, [&env, &ctx, &ret](const object_t& object) -> bool {
      auto curr = eval(object, env, ctx);
      ret = cons(curr, ret);
      return true;
    });
    return reverse(ret, false);
  }

  object_t eval_print(const object_t&, const object_t& tail, envs_t env, context_t& ctx) {
    auto ret = nil();
    for_each(tail, [&env, &ctx, &ret](const object_t& object) -> bool {
      auto curr = eval(object, env, ctx);
      ctx.stream << show(curr);
      ret = object;
      return true;
    });
    return ret;
  }

  object_t eval_lambda(const object_t& head, const object_t& tail, envs_t env, context_t&) {
    if (is_nil(tail)) return error("lambda: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (is_nil(p.second)) return error("lambda: expected argument (2) in '" + show(cons(head, tail)) + "'");
    auto args = p.first;
    p = decompose(p.second);
    if (!is_nil(p.second)) return error("lambda: wrong argument (3) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto body = p.first;
    return lambda(args, body, env);
  }

  object_t eval_macro(const object_t& head, const object_t& tail, envs_t, context_t&) {
    if (is_nil(tail)) return error("macro: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (is_nil(p.second)) return error("macro: expected argument (2) in '" + show(cons(head, tail)) + "'");
    auto args = p.first;
    p = decompose(p.second);
    if (!is_nil(p.second)) return error("macro: wrong argument (3) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto body = p.first;
    return macro(args, body);
  }

  object_t eval_def(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("def: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (is_nil(p.second)) return error("def: expected argument (2) in '" + show(cons(head, tail)) + "'");
    auto var = p.first;
    p = decompose(p.second);
    if (!is_nil(p.second)) return error("def: wrong argument (3) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto val = p.first;

    auto pvar = as_atom_variable(var);
    if (!pvar) return error("def: unexpected '" + show(var) + "', expected variable");
    val = eval(val, env, ctx);

    return env->defvar(pvar->name, val);
  }

  object_t eval_set(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("set: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (is_nil(p.second)) return error("set: expected argument (2) in '" + show(cons(head, tail)) + "'");
    auto var = p.first;
    p = decompose(p.second);
    if (!is_nil(p.second)) return error("set: wrong argument (3) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto val = p.first;

    auto pvar = as_atom_variable(var);
    if (!pvar) return error("set: unexpected '" + show(var) + "', expected variable");
    val = eval(val, env, ctx);

    return env->setvar(pvar->name, val);
  }

  object_t eval_get(const object_t& head, const object_t& tail, envs_t env, context_t&) {
    if (is_nil(tail)) return error("get: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!is_nil(p.second)) return error("get: wrong argument (2) '" + show(car(p.second))
          + "', too many arguments in '" + show(cons(head, tail)) + "'");
    auto pvar = as_atom_variable(p.first);
    if (!pvar) return error("def: unexpected '" + show(p.first) + "', expected variable");

    return env->getvar(pvar->name);
  }

  object_t eval_variable(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    if (is_nil(tail)) return error("eval_variable: expected argument (1) in '" + show(cons(head, tail)) + "'");
    auto p = decompose(tail);
    if (!as_atom_variable(p.first)) return error("eval_variable: expected variable (1) in '" + show(cons(head, tail)) + "'");
    auto var = *as_atom_variable(p.first);
    auto args = p.second;

    auto ofun = env->getvar(var.name);
    if (!as_atom_lambda(ofun) && !as_atom_macro(ofun)) {
      return error("eval_variable: unexpected '" + show(ofun) + "', expected lambda or macro");
    }

    bool is_lambda = as_atom_lambda(ofun);
    auto fargs = is_lambda
      ? ((*as_atom_lambda(ofun))->args)
      : ((*as_atom_macro(ofun))->args);
    auto env_args = is_lambda
      ? std::make_shared<env_t>((*as_atom_lambda(ofun))->env)
      : std::make_shared<env_t>();

    while (!is_nil(fargs)) {
      auto pf = decompose(fargs);
      auto pa = decompose(args);
      auto *parg = as_atom_variable(pf.first);
      if (!parg) return error("eval_variable: unexpected '" + show(pf.first) + "', expected variable");
      auto var = is_lambda
        ? eval(pa.first, env, ctx)
        : pa.first;
      env_args->defvar(parg->name, var);
      fargs = pf.second;
      args  = pa.second;
    }

    if (is_lambda)
      return eval(((*as_atom_lambda(ofun))->body), env_args, ctx);

    auto macroexpand = [](const object_t& object, envs_t env, auto f) -> object_t {
      object_t ret = object;
      std::visit(overloaded {
        [&ret, &env] (const object_variable_t& v) {
          auto obj = env->getvar(v.name);
          if (!is_error(obj)) ret = obj;
        },
        [&ret, &object, &env, &f] (object_list_sptr_t) {
          auto ret_local = nil();
          for_each(object, [&env, &f, &ret_local](const object_t& object) -> bool {
            auto curr = f(object, env, f);
            ret_local = cons(curr, ret_local);
            return true;
          });
          ret = reverse(ret_local, false);
        },
        [] (const auto&) {
          ;
        }
      }, object);
      return ret;
    };

    return macroexpand(((*as_atom_macro(ofun))->body), env_args, macroexpand);
  }

  object_t eval_list(const object_t& head, const object_t& tail, envs_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto ret = nil();
    std::visit(overloaded {
        [&ret, &head, &tail, env, &ctx] (op_t v) {
          switch (v) {
            case op_t::ADD: {
              ret = eval_arithmetic(std::plus<>(), head, tail, env, ctx);
              break;
            }
            case op_t::SUB: {
              ret = eval_arithmetic(std::minus<>(), head, tail, env, ctx);
              break;
            }
            case op_t::MUL: {
              ret = eval_arithmetic(std::multiplies<>(), head, tail, env, ctx);
              break;
            }
            case op_t::SCONCAT: {
              ret = eval_concat(std::plus<>(), head, tail, env, ctx);
              break;
            }
            case op_t::GT: {
              ret = eval_cmp(std::greater<>(), head, tail, env, ctx);
              break;
            }
            case op_t::GTE: {
              ret = eval_cmp(std::greater_equal<>(), head, tail, env, ctx);
              break;
            }
            case op_t::LT: {
              ret = eval_cmp(std::less<>(), head, tail, env, ctx);
              break;
            }
            case op_t::LTE: {
              ret = eval_cmp(std::less_equal<>(), head, tail, env, ctx);
              break;
            }
            case op_t::EQ: {
              ret = eval_cmp(std::equal_to<>(), head, tail, env, ctx);
              break;
            }
            case op_t::TYPEOF: {
              ret = eval_typeof(head, tail, env, ctx);
              break;
            }
            case op_t::EVAL: {
              ret = eval_eval(head, tail, env, ctx);
              break;
            }
            case op_t::EVALSEQ: {
              ret = eval_evalseq(head, tail, env, ctx);
              break;
            }
            case op_t::PRINT: {
              ret = eval_print(head, tail, env, ctx);
              break;
            }
            case op_t::QUOTE: {
              ret = eval_quote(head, tail, env, ctx);
              break;
            }
            case op_t::CONS: {
              ret = eval_cons(head, tail, env, ctx);
              break;
            }
            case op_t::CAR: {
              ret = eval_car(head, tail, env, ctx);
              break;
            }
            case op_t::CDR: {
              ret = eval_cdr(head, tail, env, ctx);
              break;
            }
            case op_t::COND: {
              ret = eval_cond(head, tail, env, ctx);
              break;
            }
            case op_t::LAMBDA: {
              ret = eval_lambda(head, tail, env, ctx);
              break;
            }
            case op_t::MACRO: {
              ret = eval_macro(head, tail, env, ctx);
              break;
            }
            case op_t::MACROEXPAND: {
              ret = eval_variable(head, tail, env, ctx);
              break;
            }
            case op_t::DEF: {
              ret = eval_def(head, tail, env, ctx);
              break;
            }
            case op_t::SET: {
              ret = eval_set(head, tail, env, ctx);
              break;
            }
            case op_t::GET: {
              ret = eval_get(head, tail, env, ctx);
              break;
            }
            default: {
              ret = error("eval_lisp: unexpected '" + show(head) + "' operator");
              break;
            }
          }
        },
        [&ret, &head, &tail, &env, &ctx] (const object_variable_t&) {
          ret = eval_variable(nil(), cons(head, tail), env, ctx);
        },
        [&ret, &head, &tail] (const auto&) {
          ret = error("eval_list: unexpected '" + show(head) + "' in '" + show(cons(head, tail)) + "'");
        },
    }, head);
    return ret;
  }

  object_t eval(const object_t& object, envs_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    if (is_error(object)) return object;
    ctx.eval_calls++;
    auto ret = nil();
    std::visit(overloaded {
        [&ret, &object] (object_nil_t) {
          ret = object;
        },
        [&ret, &object] (const object_error_t&) {
          ret = object;
        },
        [&ret, &object] (bool) {
          ret = object;
        },
        [&ret, &object] (int64_t) {
          ret = object;
        },
        [&ret, &object] (double) {
          ret = object;
        },
        [&ret, &object] (const std::string&) {
          ret = object;
        },
        [&ret, &object, &env] (const object_variable_t& v) {
          ret = env->getvar(v.name);
        },
        [&ret, &object, env, &ctx] (object_list_sptr_t) {
          auto p = decompose(object);
          ret = eval_list(p.first, p.second, env, ctx);
        },
        [&ret, &object] (const auto&) {
          ret = error("eval: unexpected '" + show(object) + "'");
        },
    }, object);
    return ret;
  }
}



