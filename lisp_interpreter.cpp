
#include "lisp_interpreter.h"

namespace lisp_interpreter {

  object_sptr_t object_t::reverse(bool recursive) const {
    auto obj = self();
    if (!obj->as_list()) return obj;
    auto ret = nil();
    while (obj->as_list()) {
      auto p = obj->decompose();
      ret = (recursive ? p.first->reverse() : p.first)->cons(ret);
      obj = p.second;
    }
    return ret;
  }

  object_sptr_t object_t::eval_plus(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto x = p.first;
    p = p.second->decompose();
    auto y = p.first;
    if (!p.second->as_nil()) throw error_t("eval_plus: unexpected '" + p.second->show() + "'");

    auto ret = nil();
    auto op = std::plus<>();
    x = x->eval(env, ctx);
    y = y->eval(env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = string(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_plus: unexpected types in '" + t->show() + "'"); },
    }, x->value, y->value);
    return ret;
  }

  object_sptr_t object_t::eval_minus(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto x = p.first;
    p = p.second->decompose();
    auto y = p.first;
    if (!p.second->as_nil()) throw error_t("eval_minus: unexpected '" + p.second->show() + "'");

    auto ret = nil();
    auto op = std::minus<>();
    x = x->eval(env, ctx);
    y = y->eval(env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [t] (auto, auto) { throw error_t("eval_minus: unexpected types in '" + t->show() + "'"); },
    }, x->value, y->value);
    return ret;
  }

  object_sptr_t object_t::eval_multiplies(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto x = p.first;
    p = p.second->decompose();
    auto y = p.first;
    if (!p.second->as_nil()) throw error_t("eval_multiplies: unexpected '" + p.second->show() + "'");

    auto ret = nil();
    auto op = std::multiplies<>();
    x = x->eval(env, ctx);
    y = y->eval(env, ctx);
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [t] (auto, auto) { throw error_t("eval_multiplies: unexpected types in '" + t->show() + "'"); },
    }, x->value, y->value);
    return ret;
  }

  object_sptr_t object_t::eval_equal(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto x = p.first;
    p = p.second->decompose();
    auto y = p.first;
    if (!p.second->as_nil()) throw error_t("eval_equal: unexpected '" + p.second->show() + "'");

    auto ret = nil();
    auto op = std::equal_to<>();
    x = x->eval(env, ctx);
    y = y->eval(env, ctx);
    std::visit(overloaded {
      [&ret, &op] (bool    x, bool    y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = atom(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_equal: unexpected types in '" + t->show() + "'"); },
    }, x->value, y->value);
    return ret;
  }

  object_sptr_t object_t::eval_less(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    auto p = t->decompose();
    auto x = p.first;
    p = p.second->decompose();
    auto y = p.first;
    if (!p.second->as_nil()) throw error_t("eval_less: unexpected '" + p.second->show() + "'");

    auto ret = nil();
    auto op = std::less<>();
    x = x->eval(env, ctx);
    y = y->eval(env, ctx);
    DEBUG_LOGGER_LISP("x: %s", x->show().c_str());
    DEBUG_LOGGER_LISP("y: %s", y->show().c_str());
    std::visit(overloaded {
      [&ret, &op] (double  x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (int64_t x, double  y) { ret = atom(op(x, y)); },
      [&ret, &op] (double  x, int64_t y) { ret = atom(op(x, y)); },
      [&ret, &op] (object_string_sptr_t x, object_string_sptr_t y) { ret = atom(op(x->value, y->value)); },
      [t] (auto, auto) { throw error_t("eval_less: unexpected types in '" + t->show() + "'"); },
    }, x->value, y->value);
    return ret;
  }

  object_sptr_t object_t::eval_def(object_sptr_t, object_sptr_t t, env_sptr_t env_eval, env_sptr_t env_def, context_t& ctx, bool need_eval) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env_eval: %p", env_eval.get());
    DEBUG_LOGGER_LISP("env_def: %p", env_def.get());
    auto p = t->decompose();
    auto name = p.first;
    p = p.second->decompose();
    auto object = p.first;
    if (!p.second->as_nil()) throw error_t("eval_def: unexpected '" + p.second->show() + "'");

    auto sname = name->as_ident();
    if (!sname) throw error_t("eval_def: argument #1 is not ident");

    if (need_eval) object = object->eval(env_eval, ctx); // TODO lazy
    DEBUG_LOGGER_LISP("object: %s", object->show().c_str());

    return env_def->defvar((*sname)->value, object);
  }

  object_sptr_t object_t::eval_println(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    t->for_each([&env, &ctx](object_sptr_t object) -> bool {
      auto obj = object->eval(env, ctx);
      ctx.stream << obj->show();
      DEBUG_LOGGER_LISP("stream: %s", obj->show().c_str());
      return true;
    });
    ctx.stream << std::endl;
    return atom(true);
  }

  object_sptr_t object_t::eval_if(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    auto p = t->decompose();
    auto cond = p.first;
    p = p.second->decompose();
    auto consequent = p.first;
    p = p.second->decompose();
    auto alternative = p.first;
    if (!p.second->as_nil()) throw error_t("eval_if: unexpected '" + p.second->show() + "'");

    cond = cond->eval(env, ctx);
    auto cond_bool = cond->as_bool();
    if (!cond_bool) throw error_t("eval_if: argument #1 is not bool");

    auto ret = (*cond_bool ? consequent : alternative)->eval(env, ctx);
    DEBUG_LOGGER_LISP("ret: %s", ret->show().c_str());
    return ret;
  }

  object_sptr_t object_t::eval_quote(object_sptr_t, object_sptr_t t, env_sptr_t, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto tail = p.first;
    if (!p.second->as_nil()) throw error_t("eval_quote: unexpected '" + p.second->show() + "'");
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("ret: %s", tail->show().c_str());
    return tail;
  }

  object_sptr_t object_t::eval_eval(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto tail = p.first;
    if (!p.second->as_nil()) throw error_t("eval_eval: unexpected '" + p.second->show() + "'");
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("ret: %s", tail->show().c_str());
    return tail->eval(env, ctx);
  }

  object_sptr_t object_t::eval_cons(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto head = p.first;
    p = p.second->decompose();
    auto tail = p.first;
    if (!p.second->as_nil()) throw error_t("eval_cons: unexpected '" + p.second->show() + "'");

    head = head->eval(env, ctx);
    tail = tail->eval(env, ctx);
    return head->cons(tail);
  }

  object_sptr_t object_t::eval_head(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto l = p.first;
    if (!p.second->as_nil()) throw error_t("eval_head: unexpected '" + p.second->show() + "'");

    l = l->eval(env, ctx);
    return l->head();
  }

  object_sptr_t object_t::eval_tail(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto l = p.first;
    if (!p.second->as_nil()) throw error_t("eval_tail: unexpected '" + p.second->show() + "'");

    l = l->eval(env, ctx);
    return l->tail();
  }

  object_sptr_t object_t::eval_typeof(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto l = p.first;
    if (!p.second->as_nil()) throw error_t("eval_typeof: unexpected '" + p.second->show() + "'");

    l = l->eval(env, ctx);

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
    }, l->value);

    return string(ret);
  }

  object_sptr_t object_t::eval_lambda(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    auto p = t->decompose();
    auto args = p.first;
    p = p.second->decompose();
    auto body = p.first;
    if (!p.second->as_nil()) throw error_t("eval_lambda: unexpected '" + p.second->show() + "'");

    if (!args->as_list() && !args->as_nil()) throw error_t("eval_lambda: argument #1 is not list");

    DEBUG_LOGGER_LISP("args: %s", args->show().c_str());
    DEBUG_LOGGER_LISP("body: %s", body->show().c_str());
    DEBUG_LOGGER_LISP("env:  %p", env.get());
    return lambda(args, body, env);
  }

  object_sptr_t object_t::eval_macro(object_sptr_t, object_sptr_t t, env_sptr_t, context_t&) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    auto p = t->decompose();
    auto args = p.first;
    p = p.second->decompose();
    auto body = p.first;
    if (!p.second->as_nil()) throw error_t("eval_macro: unexpected '" + p.second->show() + "'");

    if (!args->as_list() && !args->as_nil()) throw error_t("eval_macro: argument #1 is not list");

    DEBUG_LOGGER_LISP("args: %s", args->show().c_str());
    DEBUG_LOGGER_LISP("body: %s", body->show().c_str());
    return macro(args, body);
  }

  object_sptr_t object_t::eval_load(object_sptr_t, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    auto p = t->decompose();
    auto name = p.first;
    if (!p.second->as_nil()) throw error_t("eval_load: unexpected '" + p.second->show() + "'");

    auto sname = name->as_string();
    if (!sname) throw error_t("eval_load: argument #1 is not string");

    std::ifstream ifs((*sname)->value);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    auto ret = parse(content);
    ret = ret->eval(env, ctx);
    return ret;
  }

  object_sptr_t object_t::eval_call_lambda(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("h: %s", h->show().c_str());
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());

    auto lambda = h->as_lambda();
    if (!lambda) throw error_t("eval_call_lambda: argument #0 is not lambda");

    auto env_lambda = std::make_shared<env_t>((*lambda)->env);

    DEBUG_LOGGER_LISP("env_lambda_origin: %p", (*lambda)->env.get());
    DEBUG_LOGGER_LISP("env_lambda: %p", env_lambda.get());
    DEBUG_LOGGER_LISP("args: %s", (*lambda)->args->show().c_str());
    DEBUG_LOGGER_LISP("body: %s", (*lambda)->body->show().c_str());

    (*lambda)->args->for_each([&t, &env, &env_lambda, &ctx](object_sptr_t object) -> bool {
      auto p = t->decompose();
      t = p.second;
      auto arg = object->cons(p.first->cons(nil()));
      auto val = eval_def(nil(), arg, env, env_lambda, ctx);
      DEBUG_LOGGER_LISP("arg: %s", arg->show().c_str());
      DEBUG_LOGGER_LISP("val: %s", val->show().c_str());
      return true;
    });

    auto ret = (*lambda)->body->eval(env_lambda, ctx);
    DEBUG_LOGGER_LISP("ret: %s", ret->show().c_str());
    return ret;
  }

  object_sptr_t object_t::eval_call_macro(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;

    auto macro = h->as_macro();
    if (!macro) throw error_t("eval_call_macro: argument #0 is not macro");

    auto env_macro = std::make_shared<env_t>();

    (*macro)->args->for_each([&t, &env, &env_macro, &ctx] (object_sptr_t object) -> bool {
      auto p = t->decompose();
      t = p.second;
      auto arg = object->cons(p.first->cons(nil()));
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
          object->for_each([env, f, &ret_local](object_sptr_t object) -> bool {
            auto curr = f(object, env, f);
            ret_local = curr->cons(ret_local);
            return true;
          });
          ret = ret_local->reverse(false);
        },
        [] (auto) {
          ;
        },
      }, object->value);
      return ret;
    };

    auto ret = macroexpand(((*macro)->body), env_macro, macroexpand);
    return ret->eval(env, ctx);
  }

  object_sptr_t object_t::eval_call(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("h: %s", h->show().c_str());
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());

    auto name = h->as_ident();
    if (!name) throw error_t("eval_call: argument #0 is not ident");

    auto obj = env->getvar((*name)->value);
    // obj = obj->eval(env, ctx);
    DEBUG_LOGGER_LISP("obj: %s", obj->show().c_str());

    if (obj->as_lambda()) {
      return eval_call_lambda(obj, t, env, ctx);
    } else if (obj->as_macro()){
      return eval_call_macro(obj, t, env, ctx);
    }

    return obj;
  }

  object_sptr_t object_t::eval_list(object_sptr_t h, object_sptr_t t, env_sptr_t env, context_t& ctx) {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("h: %s", h->show().c_str());
    DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    auto ret = nil();
    std::visit(overloaded {
      [&ret, h, t, env, &ctx] (object_ident_sptr_t v) {
        if (v->value == "__kernel_plus") {
          ret = eval_plus(h, t, env, ctx);
        } else if (v->value == "__kernel_minus") {
          ret = eval_minus(h, t, env, ctx);
        } else if (v->value == "__kernel_multiplies") {
          ret = eval_multiplies(h, t, env, ctx);
        } else if (v->value == "__kernel_equal") {
          ret = eval_equal(h, t, env, ctx);
        } else if (v->value == "__kernel_less") {
          ret = eval_less(h, t, env, ctx);
        } else if (v->value == "__kernel_println") {
          ret = eval_println(h, t, env, ctx);
        } else if (v->value == "__kernel_if") {
          ret = eval_if(h, t, env, ctx);
        } else if (v->value == "__kernel_eval") {
          ret = eval_eval(h, t, env, ctx);
        } else if (v->value == "__kernel_cons") {
          ret = eval_cons(h, t, env, ctx);
        } else if (v->value == "__kernel_head") {
          ret = eval_head(h, t, env, ctx);
        } else if (v->value == "__kernel_tail") {
          ret = eval_tail(h, t, env, ctx);
        } else if (v->value == "__kernel_typeof") {
          ret = eval_typeof(h, t, env, ctx);
        } else if (v->value == "__kernel_load") {
          ret = eval_load(h, t, env, ctx);
        } else if (v->value == "__kernel_def") {
          ret = eval_def(h, t, env, env, ctx);
        } else if (v->value == "__kernel_lambda") {
          ret = eval_lambda(h, t, env, ctx);
        } else if (v->value == "__kernel_macro") {
          ret = eval_macro(h, t, env, ctx);
        } else if (v->value == "__kernel_quote") {
          ret = eval_quote(h, t, env, ctx);
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
        DEBUG_LOGGER_LISP("h: %s", h->show().c_str());
        DEBUG_LOGGER_LISP("t: %s", t->show().c_str());
        DEBUG_LOGGER_LISP("env: %p", env.get());
        ret = h->eval(env, ctx);
        if (!t->as_nil()) {
          ret = t->eval(env, ctx);
        }
        DEBUG_LOGGER_LISP("ret: %s", ret->show().c_str());
      }
    }, h->value);
    DEBUG_LOGGER_LISP("ret: %s", ret->show().c_str());
    return ret;
  }

  object_sptr_t object_t::eval(env_sptr_t env, context_t& ctx) const {
    DEBUG_LOGGER_TRACE_LISP;
    DEBUG_LOGGER_LISP("self: %s", self()->show().c_str());
    DEBUG_LOGGER_LISP("env: %p", env.get());
    ctx.eval_calls++;
    auto ret = nil();
    std::visit(overloaded {
      [&ret, env, &ctx] (object_ident_sptr_t v) {
        DEBUG_LOGGER_LISP("eval: ident: %s", v->value.c_str());
        env->show();
        ret = env->getvar(v->value);
        // ret = ret->eval(env, ctx);
      },
      [&ret, env, &ctx, this] (object_list_sptr_t) {
        auto p = self()->decompose();
        ret = eval_list(p.first, p.second, env, ctx);
      },
      [&ret, this] (auto) {
        ret = self();
        DEBUG_LOGGER_LISP("ret = self: %s", ret->show().c_str());
      },
    }, value);
    DEBUG_LOGGER_LISP("ret: %s", ret->show().c_str());
    return ret;
  }

  std::string object_t::show() const {
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
        str += "(lambda " + v->args->show() + " " + v->body->show() + ")";
      },
      [&str] (object_macro_sptr_t v) {
        str += "(macro " + v->args->show() + " " + v->body->show() + ")";
      },
      [&str, this] (object_list_sptr_t) {
        str += '(';
        bool is_first = true;
        for_each([&str, &is_first] (object_sptr_t object) -> bool {
          if (!is_first) {
            str += " ";
          } else {
            is_first = false;
          }
          str += object->show();
          return true;
        });
        str += ')';
      },
      [&str] (auto) {
        str += "UNK";
      }
    }, value);
    return str;
  }

  object_sptr_t object_t::parse(const std::string& str) {
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

        stack.top() = l->cons(stack.top());
      } else if (*it == '-' && std::isdigit(*it)) {
        ;
      } else if (std::isdigit(*it) || (*it == '-' && it != ite && isdigit(*(it + 1)))) {
        auto it_origin = it;
        it = std::find_if_not(it + 1, ite, [](auto c){
            return std::isdigit(c) || c == '.'; });

        if (std::find(it_origin, it, '.') == it) { // int64_t
          int64_t value = stol(std::string(it_origin, it));
          stack.top() = atom(value)->cons(stack.top());
        } else { // double
          double value = stod(std::string(it_origin, it));
          stack.top() = atom(value)->cons(stack.top());
        }
      } else if (*it == '"') {
        auto it_origin = it;
        it = std::find(it + 1, ite, '"');
        if (it != ite) ++it;

        auto value = std::string(it_origin + 1, it - 1);
        stack.top() = string(value)->cons(stack.top());
      } else if (std::isgraph(*it)) {
        auto it_origin = it;
        it = std::find_if_not(it, ite, [](auto c){
            return std::isgraph(c) && c != '(' && c != ')' && c != '"'; });

        auto value = std::string(it_origin, it);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "true") {
          stack.top() = atom(true)->cons(stack.top());
        } else if (value == "false") {
          stack.top() = atom(false)->cons(stack.top());
        } else {
          stack.top() = ident(value)->cons(stack.top());
        }
      } else {
        it++;
      }
    }

    if (stack.size() != 1) throw error_t("parse: unexpected EOF");
    auto ret = stack.top();
    if (ret->as_list() && ret->tail()->as_nil()) ret = ret->head(); // XXX
    return ret->reverse();
  }

}

