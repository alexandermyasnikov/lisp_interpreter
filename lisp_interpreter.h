
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
#define DEBUG_LOGGER_LISP(...)           // DEBUG_LOG("lisp ", logger_indent_lisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent; };

struct logger_indent_lisp_t   : logger_indent_t<logger_indent_lisp_t> { };



using namespace std::string_literals;

namespace lisp_interpreter {

  // TODO
  // eval -> defvar -> getvar
  // memoization
  // pure function
  // cmake

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


  struct object_t;
  using object_sptr_t = std::shared_ptr<const object_t>;


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
      DEBUG_LOGGER_LISP("env: defvar: %p   %s   %s", this, key.c_str(), val->show().c_str());
      return val;
    }

    tval_t getvar(const tkey_t& key) const {
      DEBUG_LOGGER_TRACE_LISP;
      auto env = this->shared_from_this();
      while (env) {
        auto &frames = env->frames;
        auto it = frames.find(key);
        if (it != frames.end()) {
          DEBUG_LOGGER_LISP("env: getvar: %p   %s   %s", this, key.c_str(), it->second->show().c_str());
          return it->second;
        }
        env = env->parent;
      }
      throw error_t("env_base_t:getvar: value '" + key + "' is not exists");
    }

    void show() const {
      DEBUG_LOGGER_TRACE_LISP;
      std::string ret;
      auto env = this->shared_from_this();
      while (env) {
        auto &frames = env->frames;
        for (const auto& kv : frames) {
          DEBUG_LOGGER_LISP("env: %p \t key: %s \t val: %s", env.get(), kv.first.c_str(), kv.second->show().c_str());
        }
        env = env->parent;
      }
    }
  };

  using env_t = env_base_t<std::string, object_sptr_t>;
  using env_sptr_t = std::shared_ptr<env_t>;


  struct context_t {
    std::stringstream stream;
    size_t eval_calls;
    // size_t stack_level_max;
    // size_t stack_level;

    context_t() : stream{}, eval_calls{} { }
  };


  struct object_t : std::enable_shared_from_this<object_t> {

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

    using variant_t = std::variant<
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

    object_t(const variant_t& value) : value(value) { }

   private:

    static object_sptr_t atom(const variant_t& value) {
      return std::make_shared<object_t>(value);
    }

    static object_sptr_t nil() {
      static auto nil = std::make_shared<object_nil_t>();
      static auto object = std::make_shared<object_t>(nil);
      return object;
    }

    static object_sptr_t string(const std::string& str) {
      auto l = std::make_shared<object_string_t>(str);
      variant_t v = l;
      return std::make_shared<object_t>(v);
    }

    static object_sptr_t ident(const std::string& str) {
      auto l = std::make_shared<object_ident_t>(str);
      return std::make_shared<object_t>(l);
    }

    static object_sptr_t list(object_sptr_t head, object_sptr_t tail) {
      auto l = std::make_shared<object_list_t>(head, tail);
      return std::make_shared<object_t>(l);
    }

    static object_sptr_t lambda(object_sptr_t args, object_sptr_t body, env_sptr_t env) {
      auto l = std::make_shared<object_lambda_t>(args, body, env);
      return std::make_shared<object_t>(l);
    }

    static object_sptr_t macro(object_sptr_t args, object_sptr_t body) {
      auto l = std::make_shared<object_macro_t>(args, body);
      return std::make_shared<object_t>(l);
    }

    const bool* as_bool() const {
      return std::get_if<bool>(&value);
    }

    const object_nil_sptr_t* as_nil() const {
      return std::get_if<object_nil_sptr_t>(&value);
    }

    const object_string_sptr_t* as_string() const {
      return std::get_if<object_string_sptr_t>(&value);
    }

    const object_ident_sptr_t* as_ident() const {
      return std::get_if<object_ident_sptr_t>(&value);
    }

    const object_list_sptr_t* as_list() const {
      return std::get_if<object_list_sptr_t>(&value);
    }

    const object_lambda_sptr_t* as_lambda() const {
      return std::get_if<object_lambda_sptr_t>(&value);
    }

    const object_macro_sptr_t* as_macro() const {
      return std::get_if<object_macro_sptr_t>(&value);
    }

    object_sptr_t self() const {
      return shared_from_this();
    }

    std::pair<object_sptr_t, object_sptr_t> decompose() const {
      auto list = as_list();
      if (!list) throw error_t("decompose: object '" + show() + "' is not list");
      return {(*list)->head, (*list)->tail};
    }

    object_sptr_t head() const {
      return decompose().first;
    }

    object_sptr_t tail() const {
      return decompose().second;
    }

    object_sptr_t cons(object_sptr_t tail) const {
      if (!tail->as_list() && !tail->as_nil()) throw error_t("cons: tail is not list");
      return list(self(), tail);
    }

    void for_each(auto f) const {
      auto object = self();
      while (object->as_list()) {
        auto p = object->decompose();
        if (!f(p.first)) break;
        object = p.second;
      }
    }

    object_sptr_t reverse(bool recursive = true) const;

    static object_sptr_t eval_plus       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_minus      (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_multiplies (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_equal      (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_less       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_println    (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_if         (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_quote      (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_eval       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_cons       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_head       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_tail       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_typeof     (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_lambda     (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_macro      (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_load       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_call       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_list       (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_def        (object_sptr_t, object_sptr_t, env_sptr_t, env_sptr_t, context_t&, bool need_eval = true);
    static object_sptr_t eval_call_lambda(object_sptr_t, object_sptr_t, env_sptr_t, context_t&);
    static object_sptr_t eval_call_macro (object_sptr_t, object_sptr_t, env_sptr_t, context_t&);

    variant_t value;

   public:

    object_sptr_t eval(env_sptr_t env, context_t& ctx) const;
    std::string show() const;

    static object_sptr_t parse(const std::string& str);

  };

}

