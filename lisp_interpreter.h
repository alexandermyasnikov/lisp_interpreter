
#include <iostream>
#include <memory>
#include <map>
#include <stack>
#include <variant>
#include <algorithm>
#include <sstream>
#include <fstream>

#include "debug_logger.h"



#define DEBUG_LOGGER_TRACE_LISP          // DEBUG_LOGGER("lisp ", logger_indent_lisp_t::indent)
#define DEBUG_LOGGER_LISP(...)           DEBUG_LOG("lisp ", logger_indent_lisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent; };

struct logger_indent_lisp_t   : logger_indent_t<logger_indent_lisp_t> { };



using namespace std::string_literals;

namespace lisp_interpreter {

  // TODO
  // lazy eval, eval flag
  // memoization
  // pure function
  // op -> string?
  // cmake
  // variant to struct

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


  struct object_nil_t { };
  using object_nil_sptr_t = std::shared_ptr<const object_nil_t>;

  struct object_lambda_t;
  using object_lambda_sptr_t = std::shared_ptr<const object_lambda_t>;

  struct object_macro_t;
  using object_macro_sptr_t = std::shared_ptr<const object_macro_t>;

  struct object_list_t;
  using object_list_sptr_t = std::shared_ptr<const object_list_t>;

  struct object_string_t {
    std::string value;

    object_string_t(const std::string& value) : value(value) { }
  };

  using object_string_sptr_t = std::shared_ptr<const object_string_t>;

  struct object_ident_t {
    std::string value;

    object_ident_t(const std::string& value) : value(value) { }
  };

  using object_ident_sptr_t = std::shared_ptr<const object_ident_t>;

  using object_t = std::variant<
    object_nil_sptr_t,      // nil
    bool,                   // bool
    int64_t,                // number
    double,                 // number
    object_string_sptr_t,   // string
    object_ident_sptr_t,    // ident
    object_list_sptr_t,     // list
    object_lambda_sptr_t,   // lambda
    object_macro_sptr_t     // macro
  >;

  using object_sptr_t = std::shared_ptr<const object_t>; // mutable in eval?


  struct error_t : std::runtime_error {
    error_t(const std::string& msg) : std::runtime_error(msg) { }
  };


  template <typename tkey_t, typename tval_t>
  struct env_base_t : std::enable_shared_from_this<env_base_t<tkey_t, tval_t>> {
    using env_t = env_base_t<tkey_t, tval_t>;

    std::map<tkey_t, tval_t>   frames;
    std::shared_ptr<env_t>     parent;

    env_base_t(std::shared_ptr<env_t> parent = nullptr) : parent(parent) { }

    tval_t defvar(const tkey_t& key, const tval_t& val) {
      DEBUG_LOGGER_TRACE_LISP;
      auto it = frames.find(key);
      if (it != frames.end()) throw error_t("env_base_t:defvar: value '" + key + "' is exists");
      frames[key] = val;
      return val;
    }

    tval_t getvar(const tkey_t& key) {
      DEBUG_LOGGER_TRACE_LISP;
      auto env = this->shared_from_this();
      while (env) {
        auto &frames = env->frames;
        auto it = frames.find(key);
        if (it != frames.end()) {
          return it->second;
        }
        env = env->parent;
      }
      throw error_t("env_base_t:getvar: value '" + key + "' is not exists");
    }
  };

  using env_t = env_base_t<std::string, object_sptr_t>;
  using env_sptr_t = std::shared_ptr<env_base_t<std::string, object_sptr_t>>;


  struct object_lambda_t {
    object_sptr_t   args;
    object_sptr_t   body;
    env_sptr_t      env;

    object_lambda_t(object_sptr_t args, object_sptr_t body, env_sptr_t env)
        : args(args), body(body), env(env) { }
  };

  struct object_macro_t {
    object_sptr_t   args;
    object_sptr_t   body;

    object_macro_t(object_sptr_t args, object_sptr_t body)
        : args(args), body(body) { }
  };

  struct object_list_t {
    object_sptr_t   head;
    object_sptr_t   tail;

    object_list_t(object_sptr_t head, object_sptr_t tail) : head(head), tail(tail) { }
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

  std::string show(object_sptr_t object);
  object_sptr_t parse(const std::string& str);
  object_sptr_t eval(object_sptr_t object, env_sptr_t env, context_t& ctx);



  // I N T E R F A C E

  object_sptr_t atom(const object_t& object) {
    return std::make_shared<object_t>(object);
  }

  const bool* as_bool(object_sptr_t object) {
    return std::get_if<bool>(object.get());
  }

  object_sptr_t nil() {
    static auto nil = std::make_shared<object_nil_t>();
    static auto object = std::make_shared<object_t>(nil);
    return object;
  }

  const object_nil_sptr_t* as_nil(object_sptr_t object) {
    return std::get_if<object_nil_sptr_t>(object.get());
  }

  object_sptr_t string(const std::string& str) {
    auto l = std::make_shared<object_string_t>(str);
    return atom(l);
  }

  const object_string_sptr_t* as_string(object_sptr_t object) {
    return std::get_if<object_string_sptr_t>(object.get());
  }

  object_sptr_t ident(const std::string& str) {
    auto l = std::make_shared<object_ident_t>(str);
    return atom(l);
  }

  const object_ident_sptr_t* as_ident(object_sptr_t object) {
    return std::get_if<object_ident_sptr_t>(object.get());
  }

  object_sptr_t list(object_sptr_t head, object_sptr_t tail) {
    auto l = std::make_shared<object_list_t>(head, tail);
    return atom(l);
  }

  const object_list_sptr_t* as_list(object_sptr_t object) {
    return std::get_if<object_list_sptr_t>(object.get());
  }

  object_sptr_t lambda(object_sptr_t args, object_sptr_t body, env_sptr_t env) {
    auto l = std::make_shared<object_lambda_t>(args, body, env);
    return atom(l);
  }

  const object_lambda_sptr_t* as_lambda(object_sptr_t object) {
    return std::get_if<object_lambda_sptr_t>(object.get());
  }

  object_sptr_t macro(object_sptr_t args, object_sptr_t body) {
    auto l = std::make_shared<object_macro_t>(args, body);
    return atom(l);
  }

  const object_macro_sptr_t* as_macro(object_sptr_t object) {
    return std::get_if<object_macro_sptr_t>(object.get());
  }

  std::pair<object_sptr_t, object_sptr_t> decompose(object_sptr_t object) {
    DEBUG_LOGGER_TRACE_LISP;
    auto list = as_list(object);
    if (!list) throw error_t("decompose: object '" + show(object) + "' is not list");
    return {(*list)->head, (*list)->tail};
  }

  object_sptr_t head(object_sptr_t object) {
    DEBUG_LOGGER_TRACE_LISP;
    return decompose(object).first;
  }

  object_sptr_t tail(object_sptr_t object) {
    DEBUG_LOGGER_TRACE_LISP;
    return decompose(object).second;
  }

  object_sptr_t cons(object_sptr_t head, object_sptr_t tail ) {
    DEBUG_LOGGER_TRACE_LISP;
    if (!as_list(tail) && !as_nil(tail)) throw error_t("cons: tail is not list");
    return list(head, tail);
  }



  // L I B R A R Y

  object_sptr_t reverse(object_sptr_t object, bool recursive = true) {
    DEBUG_LOGGER_TRACE_LISP;
    if (!as_list(object)) return object;
    auto ret = nil();
    while (as_list(object)) {
      auto p = decompose(object);
      ret = cons(recursive ? reverse(p.first) : p.first, ret);
      object = p.second;
    }
    return ret;
  }

  void for_each(object_sptr_t object, auto f) {
    DEBUG_LOGGER_TRACE_LISP;
    while (as_list(object)) {
      auto p = decompose(object);
      if (!f(p.first)) break;
      object = p.second;
    }
  }

  std::string show(object_sptr_t object) {
    DEBUG_LOGGER_TRACE_LISP;
    std::string str;
    std::visit(overloaded {
        [&str] (object_nil_sptr_t) {
          str += "()";
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
        [&str] (object_string_sptr_t v) {
          str += "\"" + v->value + "\"";
        },
        [&str] (object_ident_sptr_t v) {
          str += v->value;
        },
        [&str] (object_lambda_sptr_t v) {
          str += "(lambda " + show(v->args) + " " + show(v->body) + ")";
        },
        [&str] (object_macro_sptr_t v) {
          str += "(macro " + show(v->args) + " " + show(v->body) + ")";
        },
        [&str, &object] (object_list_sptr_t) {
          str += '(';
          bool is_first = true;
          for_each(object, [&str, &is_first](object_sptr_t object) -> bool {
            if (!is_first) {
              str += " ";
            } else {
              is_first = false;
            }
            str += show(object);
            return true;
          });
          str += ')';
        },
        [&str] (auto) {
          str += "UNK";
        }
    }, *object);
    return str;
  }

  object_sptr_t parse(const std::string& str) {
    DEBUG_LOGGER_TRACE_LISP;
    std::stack<object_sptr_t> stack;
    auto it = str.begin();
    auto ite = str.end();
    stack.emplace(nil());

    while (it != ite) {
      if (*it == '(') {
        it++;
        stack.emplace(nil());
      } else if (*it == ';') {
        it = std::find(it, ite, '\n');
      } else if (*it == ')') {
        it++;
        if (stack.empty()) throw error_t("unexpected ')' on pos " + std::to_string(ite - it));

        auto l = stack.top();
        stack.pop();

        if (stack.empty()) throw error_t("unexpected ')' on pos " + std::to_string(ite - it));

        auto object_n = cons(l, stack.top());
        stack.top().swap(object_n);

      } else if (*it == '-' && std::isdigit(*it)) {
        ;
      } else if (std::isdigit(*it) || (*it == '-' && it != ite && isdigit(*(it + 1)))) {
        auto it_origin = it;
        it = std::find_if_not(it + 1, ite, [](auto c){
            return std::isdigit(c) || c == '.'; });

        if (std::find(it_origin, it, '.') == it) { // int64_t
          auto value = stol(std::string(it_origin, it));
          auto obj= cons(atom(value), stack.top());
          stack.top().swap(obj);
        } else { // double
          auto value = stod(std::string(it_origin, it));
          auto obj= cons(atom(value), stack.top());
          stack.top().swap(obj);
        }
      } else if (*it == '"') {
        auto it_origin = it;
        it = std::find(it + 1, ite, '"');
        if (it != ite) ++it;

        auto value = std::string(it_origin + 1, it - 1);
        auto obj= cons(string(value), stack.top());
        stack.top().swap(obj);
      } else if (std::isgraph(*it)) {
        auto it_origin = it;
        it = std::find_if_not(it, ite, [](auto c){
            return std::isgraph(c) && c != '(' && c != ')' && c != '"'; });

        auto value = std::string(it_origin, it);
        auto obj= cons(ident(value), stack.top());
        stack.top().swap(obj);
      } else {
        it++;
      }
    }

    if (stack.size() != 1) throw error_t("parse: unexpected EOF");
    auto ret = stack.top();
    if (as_list(ret) && as_nil(tail(ret))) ret = head(ret); // XXX
    return reverse(ret);
  }

  object_sptr_t eval_plus(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto x = p.first;
    p = decompose(p.second);
    auto y = p.first;
    if (!as_nil(p.second)) throw error_t("eval_plus: unexpected '" + show(p.second) + "'");

    auto ret = nil();
    auto op = std::plus<>();
    x = eval(x, env, ctx);
    y = eval(y, env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = string(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_plus: unexpected types in '" + show(t) + "'"); },
    }, *x, *y);
    return ret;
  }

  object_sptr_t eval_minus(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto x = p.first;
    p = decompose(p.second);
    auto y = p.first;
    if (!as_nil(p.second)) throw error_t("eval_minus: unexpected '" + show(p.second) + "'");

    auto ret = nil();
    auto op = std::minus<>();
    x = eval(x, env, ctx);
    y = eval(y, env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [t] (auto, auto) { throw error_t("eval_minus: unexpected types in '" + show(t) + "'"); },
    }, *x, *y);
    return ret;
  }

  object_sptr_t eval_multiplies(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto x = p.first;
    p = decompose(p.second);
    auto y = p.first;
    if (!as_nil(p.second)) throw error_t("eval_multiplies: unexpected '" + show(p.second) + "'");

    auto ret = nil();
    auto op = std::multiplies<>();
    x = eval(x, env, ctx);
    y = eval(y, env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [t] (auto, auto) { throw error_t("eval_multiplies: unexpected types in '" + show(t) + "'"); },
    }, *x, *y);
    return ret;
  }

  object_sptr_t eval_equal(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto x = p.first;
    p = decompose(p.second);
    auto y = p.first;
    if (!as_nil(p.second)) throw error_t("eval_equal: unexpected '" + show(p.second) + "'");

    auto ret = nil();
    auto op = std::equal_to<>();
    x = eval(x, env, ctx);
    y = eval(y, env, ctx);
    std::visit(overloaded {
      [&ret, &op] (bool    x, bool    y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = atom(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_equal: unexpected types in '" + show(t) + "'"); },
    }, *x, *y);
    return ret;
  }

  object_sptr_t eval_less(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto x = p.first;
    p = decompose(p.second);
    auto y = p.first;
    if (!as_nil(p.second)) throw error_t("eval_less: unexpected '" + show(p.second) + "'");

    auto ret = nil();
    auto op = std::less<>();
    x = eval(x, env, ctx);
    y = eval(y, env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = atom(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_less: unexpected types in '" + show(t) + "'"); },
    }, *x, *y);
    return ret;
  }

  object_sptr_t eval_def(object_sptr_t, object_sptr_t t, env_sptr_t env_eval, env_sptr_t env_def, context_t& ctx, bool need_eval = true) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto name = p.first;
    p = decompose(p.second);
    auto object = p.first;
    if (!as_nil(p.second)) throw error_t("eval_def: unexpected '" + show(p.second) + "'");

    auto sname = as_ident(name);
    if (!sname) throw error_t("eval_def: argument #1 is not ident");

    if (need_eval) object = eval(object, env_eval, ctx);

    return env_def->defvar((*sname)->value, object);
  }

  object_sptr_t eval_println(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    for_each(t, [&env, &ctx](object_sptr_t object) -> bool {
      auto obj = eval(object, env, ctx);
      ctx.stream << show(obj);
      return true;
    });
    ctx.stream << std::endl;
    return atom(true);
  }

  object_sptr_t eval_if(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto cond = p.first;
    p = decompose(p.second);
    auto consequent = p.first;
    p = decompose(p.second);
    auto alternative = p.first;
    if (!as_nil(p.second)) throw error_t("eval_if: unexpected '" + show(p.second) + "'");

    cond = eval(cond, env, ctx);
    auto cond_bool = as_bool(cond);
    if (!cond_bool) throw error_t("eval_if: argument #1 is not bool");

    return eval(*cond_bool ? consequent : alternative, env, ctx);
  }

  object_sptr_t eval_quote(object_sptr_t, object_sptr_t t, env_sptr_t, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto tail = p.first;
    if (!as_nil(p.second)) throw error_t("eval_quote: unexpected '" + show(p.second) + "'");
    return tail;
  }

  object_sptr_t eval_cons(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto head = p.first;
    p = decompose(p.second);
    auto tail = p.first;
    if (!as_nil(p.second)) throw error_t("eval_cons: unexpected '" + show(p.second) + "'");

    head = eval(head, env, ctx);
    tail = eval(tail, env, ctx);
    return cons(head, tail);
  }

  object_sptr_t eval_head(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto l = p.first;
    if (!as_nil(p.second)) throw error_t("eval_head: unexpected '" + show(p.second) + "'");

    l = eval(l, env, ctx);
    return head(l);
  }

  object_sptr_t eval_tail(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto l = p.first;
    if (!as_nil(p.second)) throw error_t("eval_tail: unexpected '" + show(p.second) + "'");

    l = eval(l, env, ctx);
    return tail(l);
  }

  object_sptr_t eval_typeof(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto l = p.first;
    if (!as_nil(p.second)) throw error_t("eval_typeof: unexpected '" + show(p.second) + "'");

    l = eval(l, env, ctx);

    std::string ret;
    std::visit(overloaded {
      [&ret] (object_nil_sptr_t)     { ret = "nil"; },
      [&ret] (bool)                  { ret = "bool"; },
      [&ret] (int64_t)               { ret = "int"; },
      [&ret] (double)                { ret = "double"; },
      [&ret] (object_string_sptr_t)  { ret = "string"; },
      [&ret] (object_ident_sptr_t)   { ret = "ident"; },
      [&ret] (object_list_sptr_t)    { ret = "list"; },
      [&ret] (object_lambda_sptr_t)  { ret = "lambda"; },
      [&ret] (object_macro_sptr_t)   { ret = "macro"; },
      [&ret] (auto)                  { ret = "unknown"; },
    }, *l);

    return string(ret);
  }

  object_sptr_t eval_lambda(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto args = p.first;
    p = decompose(p.second);
    auto body = p.first;
    if (!as_nil(p.second)) throw error_t("eval_lambda: unexpected '" + show(p.second) + "'");

    if (!as_list(args) && !as_nil(args)) throw error_t("eval_lambda: argument #1 is not list");

    return lambda(args, body, env);
  }

  object_sptr_t eval_macro(object_sptr_t, object_sptr_t t, env_sptr_t, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto args = p.first;
    p = decompose(p.second);
    auto body = p.first;
    if (!as_nil(p.second)) throw error_t("eval_macro: unexpected '" + show(p.second) + "'");

    if (!as_list(args) && !as_nil(args)) throw error_t("eval_macro: argument #1 is not list");

    return macro(args, body);
  }

  object_sptr_t eval_load(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = decompose(t);
    auto name = p.first;
    if (!as_nil(p.second)) throw error_t("eval_load: unexpected '" + show(p.second) + "'");

    auto sname = as_string(name);
    if (!sname) throw error_t("eval_load: argument #1 is not string");

    std::ifstream ifs((*sname)->value);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    auto ret = parse(content);
    ret = eval(ret, env, ctx);
    return ret;
  }

  object_sptr_t eval_call_lambda(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;

    auto lambda = as_lambda(h);
    if (!lambda) throw error_t("eval_call_lambda: argument #0 is not lambda");

    auto env_lambda = std::make_shared<env_t>((*lambda)->env);

    for_each((*lambda)->args, [&t, &env, &env_lambda, &ctx](object_sptr_t object) -> bool {
      auto p = decompose(t);
      t = p.second;
      auto arg = cons(object, cons(p.first, nil()));
      auto val = eval_def(nil(), arg, env, env_lambda, ctx);
      return true;
    });

    auto ret = eval((*lambda)->body, env_lambda, ctx);
    return ret;
  }

  object_sptr_t eval_call_macro(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;

    auto macro = as_macro(h);
    if (!macro) throw error_t("eval_call_macro: argument #0 is not macro");

    auto env_macro = std::make_shared<env_t>();

    for_each((*macro)->args, [&t, &env, &env_macro, &ctx](object_sptr_t object) -> bool {
      auto p = decompose(t);
      t = p.second;
      auto arg = cons(object, cons(p.first, nil()));
      auto val = eval_def(nil(), arg, env, env_macro, ctx, false);
      return true;
    });

    auto macroexpand = [](object_sptr_t object, env_sptr_t env, auto f) -> object_sptr_t {
      auto ret = object;
      std::visit(overloaded {
        [&ret, env] (object_ident_sptr_t v) {
          try {
            ret = env->getvar(v->value);
          } catch (const error_t&) {
            ;
          }
        },
        [&ret, object, env, f] (object_list_sptr_t) {
          auto ret_local = nil();
          for_each(object, [env, f, &ret_local](object_sptr_t object) -> bool {
            auto curr = f(object, env, f);
            ret_local = cons(curr, ret_local);
            return true;
          });
          ret = reverse(ret_local, false);
        },
        [] (auto) {
          ;
        },
      }, *object);
      return ret;
    };

    return macroexpand(((*macro)->body), env_macro, macroexpand);
  }

  object_sptr_t eval_call(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;

    auto name = as_ident(h);
    if (!name) throw error_t("eval_call: argument #0 is not ident");

    auto obj = env->getvar((*name)->value);

    if (as_lambda(obj)) {
      return eval_call_lambda(obj, t, env, ctx);
    } else if (as_macro(obj)){
      return eval_call_macro(obj, t, env, ctx);
    }

    return obj;
  }

  object_sptr_t eval_list(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto ret = nil();
    std::visit(overloaded {
      [&ret, h, t, env, &ctx] (object_ident_sptr_t v) {
        if (v->value == "+") {
          ret = eval_plus(h, t, env, ctx);
        } else if (v->value == "-") {
           ret = eval_minus(h, t, env, ctx);
        } else if (v->value == "*") {
          ret = eval_multiplies(h, t, env, ctx);
        } else if (v->value == "=") {
          ret = eval_equal(h, t, env, ctx);
        } else if (v->value == "<") {
          ret = eval_less(h, t, env, ctx);
        } else if (v->value == "def") {
          ret = eval_def(h, t, env, env, ctx);
        } else if (v->value == "println") {
          ret = eval_println(h, t, env, ctx);
        } else if (v->value == "if") {
          ret = eval_if(h, t, env, ctx);
        } else if (v->value == "quote") {
          ret = eval_quote(h, t, env, ctx);
        } else if (v->value == "cons") {
          ret = eval_cons(h, t, env, ctx);
        } else if (v->value == "head") {
          ret = eval_head(h, t, env, ctx);
        } else if (v->value == "tail") {
          ret = eval_tail(h, t, env, ctx);
        } else if (v->value == "typeof") {
          ret = eval_typeof(h, t, env, ctx);
        } else if (v->value == "lambda") {
          ret = eval_lambda(h, t, env, ctx);
        } else if (v->value == "macro") {
          ret = eval_macro(h, t, env, ctx);
        } else if (v->value == ":load") {
          ret = eval_load(h, t, env, ctx);
        } else {
          ret = eval_call(h, t, env, ctx);
        }
      },
      [&ret, h, t, env, &ctx] (object_lambda_sptr_t) {
        ret = eval_call_lambda(h, t, env, ctx);
      },
      [&ret, h, t, env, &ctx] (object_macro_sptr_t) {
        ret = eval_call_macro(h, t, env, ctx);
      },
      [&ret, h, t, env, &ctx] (auto) {
        ret = eval(h, env, ctx);

        if (!as_nil(t)) {
          ret = eval(t, env, ctx);
        }
      }
    }, *h);
    return ret;
  }

  object_sptr_t eval(object_sptr_t object, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("object: '%s'", show(object).c_str());
    ctx.eval_calls++;
    auto ret = nil();
    std::visit(overloaded {
      [&ret, object, env, &ctx] (object_ident_sptr_t v) {
        ret = env->getvar(v->value);
        ret = eval(ret, env, ctx);
      },
      [&ret, object, env, &ctx] (object_list_sptr_t) {
        auto p = decompose(object);
        ret = eval_list(p.first, p.second, env, ctx);
      },
      [&ret, object] (auto) {
        ret = object;
      },
    }, *object);
    DEBUG_LOGGER_LISP("ret: '%s'", show(object).c_str());
    return ret;
  }
}



