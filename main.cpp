
#include <iostream>
#include <memory>
#include <map>
#include <stack>
#include <variant>
#include <algorithm>

#define PR        std::cout << __PRETTY_FUNCTION__ << '\t' << __LINE__ << std::endl
#define PRM(msg)  std::cout << __FUNCTION__ << ':' << __LINE__ << '\t' << msg << std::endl
#define assert(x) if (!(x)) PRM("ASSERT " #x)



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
    // context TODO

    object_variable_t(const std::string& name) : name(name) { }
  };

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
    { "quote",         op_t::QUOTE },
    { "typeof",        op_t::TYPEOF },
    { "cons",          op_t::CONS },
    { "car",           op_t::CAR },
    { "cdr",           op_t::CDR },
    { "cond",          op_t::COND },
    { "print",         op_t::PRINT },
    { "read",          op_t::READ },
    { "eval",          op_t::EVAL },
    { "eval-in",       op_t::EVALIN },
    { "lambda",        op_t::LAMBDA },
    { "macro",         op_t::MACRO },
    { "macroexpand",   op_t::MACROEXPAND },
  };

  using object_t = std::variant<
    object_nil_t,          // ()
    object_error_t,        // error
    bool,                  // atom
    int,                   // atom
    double,                // atom
    std::string,           // atom
    op_t,                  // atom
    object_variable_t,     // atom
    object_list_sptr_t     // list
  >;

  struct object_list_t {
    object_t head;
    object_t tail;

    object_list_t(object_t head, object_t tail) : head(head), tail(tail) { }
  };

  struct env_t;
  using envs_t = std::shared_ptr<env_t>;

  struct env_t : std::enable_shared_from_this<env_t> {
    using key_t = std::string;
    using val_t = object_t;

    std::map<key_t, val_t> frame;
    envs_t parent;

    env_t(envs_t parent = nullptr) : parent(parent) { }
    object_t setvar(const key_t& key, const val_t& val);
    object_t getvar(const key_t& key);
    object_t defvar(const key_t& key, const val_t& val);
  };



  // FD

  std::string show_struct(const object_t& object);
  std::string show(const object_t& object);
  object_t parse(const std::string& str);
  object_t eval(const object_t& object);



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

  object_t atom_int(int value) {
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

  object_t list(const object_t& head, const object_t& tail) {
    return std::make_shared<object_list_t>(head, tail);
  }

  const object_error_t* as_atom_error(const object_t& object) {
    return std::get_if<object_error_t>(&object);
  }

  const bool* as_atom_bool(const object_t& object) {
    return std::get_if<bool>(&object);
  }

  const int* as_atom_int(const object_t& object) {
    return std::get_if<int>(&object);
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

  object_t env_t::setvar(const key_t& key, const val_t& val) {
    auto env = shared_from_this();
    while (env) {
      auto fr = env->frame;
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
    auto env = shared_from_this();
    while (env) {
      auto fr = env->frame;
      auto it = fr.find(key);
      if (it != fr.end()) {
        return it->second;
      }
      env = env->parent;
    }
    return error("variable '" + key + "' not found");
  }

  object_t env_t::defvar(const key_t& key, const val_t& val) {
    frame[key] = val;
    return val;
  }

  std::pair<object_t, object_t> decompose(const object_t& object) {
    if (is_error(object))
      return {object, object};
    if (!is_list(object)) {
      auto ret = error("decompose: unexpected '" + show(object) + "', expected list");
      return {ret, ret};
    }
    auto list = std::get<object_list_sptr_t>(object);
    return {list->head, list->tail};
  }

  object_t car(const object_t& object) {
    return decompose(object).first;
  }

  object_t cdr(const object_t& object) {
    return decompose(object).second;
  }

  object_t cons(const object_t& head, const object_t& tail) {
    if (is_error(head))
      return head;
    if (is_error(tail))
      return tail;
    if (!is_list(tail) && !is_nil(tail))
      return error("cons: unexpected '" + show(tail) + "', expected list or nil");
    assert(show_struct(car(list(head, tail))) == show_struct(head));
    assert(show_struct(cdr(list(head, tail))) == show_struct(tail));
    return list(head, tail);
  }



  // L I B R A R Y

  object_t reverse(const object_t& object, bool recursive = true) {
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
      case op_t::LAMBDA:       return "lambda";
      case op_t::MACRO:        return "macro";
      case op_t::MACROEXPAND:  return "macroexpand";
      default: return "O:U";
    }
  }

  std::string show(const object_t& object) {
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
        [&str] (int v) {
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
        [&str, &object] (object_list_sptr_t) {
          auto p = decompose(object);
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

  std::string show_struct(const object_t& object) {
    std::string str;
    std::visit(overloaded {
        [&str] (object_nil_t) {
          str += "nil ";
        },
        [&str] (object_error_t error) {
          str += "E:\""s + error.msg + "\" ";
        },
        [&str] (bool v) {
          str += "B:"s + (v ? "true" : "false") + " ";
        },
        [&str] (int v) {
          str += "I:" + std::to_string(v) + " ";
        },
        [&str] (double v) {
          str += "D:" + std::to_string(v) + " ";
        },
        [&str] (const std::string& v) {
          str += "S:\"" + v + "\" ";
        },
        [&str] (op_t v) {
          str += "O:" + show_operator(v) + " ";
        },
        [&str] (object_variable_t v) {
          str += "V:" + v.name + " ";
        },
        [&str, &object] (object_list_sptr_t) {
          str += "LIST ( " + show_struct(car(object)) + ") ( " + show_struct(cdr(object)) + ") ";
        },
    }, object);
    return str;
  }

  object_t parse(const std::string& str) {
    std::stack<object_t> stack;
    object_t ret = nil();
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

  object_t op_arithmetic(const object_t& a, const object_t& b, auto f) {
    if (is_error(a)) return a;
    if (is_error(b)) return b;
    auto ai = as_atom_int(a);
    auto bi = as_atom_int(b);
    auto ad = as_atom_double(a);
    auto bd = as_atom_double(b);
    if (ai && bi) return f(*ai, *bi);
    if (ai && bd) return f(*ai, *bd);
    if (ad && bi) return f(*ad, *bi);
    if (ad && bd) return f(*ad, *bd);
    return (ai || ad)
      ? error("op_add: unexpected '" + show(b) + "', expected int or double")
      : error("op_add: unexpected '" + show(a) + "', expected int or double");
  }

  object_t op_int(const object_t& a, const object_t& b, auto f) {
    if (is_error(a)) return a;
    if (is_error(b)) return b;
    auto ai = as_atom_int(a);
    auto bi = as_atom_int(b);
    if (!ai) return error("op_int: unexpected '" + show(a) + "', expected int");
    if (!bi) return error("op_int: unexpected '" + show(b) + "', expected int");
    return f(*ai, *bi);
  }

  object_t op_string(const object_t& a, const object_t& b, auto f) {
    if (is_error(a)) return a;
    if (is_error(b)) return b;
    auto as = as_atom_string(a);
    auto bs = as_atom_string(b);
    if (!as) return error("op_string: unexpected '" + show(a) + "', expected string");
    if (!bs) return error("op_string: unexpected '" + show(b) + "', expected string");
    return f(*as, *bs);
  }

  object_t op_bool(const object_t& a, const object_t& b, auto f) {
    if (is_error(a)) return a;
    if (is_error(b)) return b;
    auto ab = as_atom_bool(a);
    auto bb = as_atom_bool(b);
    if (!ab) return error("op_bool: unexpected '" + show(a) + "', expected bool");
    if (!bb) return error("op_bool: unexpected '" + show(b) + "', expected bool");
    return f(*ab, *bb);
  }

  object_t eval_op(const object_t& object_op, const object_t& tail) {
    object_t ret = nil();
    object_t prev = nil();

    if (is_list(object_op)) {
      auto res = eval(object_op);
      return !is_nil(tail) ? eval(tail) : res;
    }

    auto op = as_atom_operator(object_op);
    if (!op) return error("eval_op: unexpected '" + show(object_op) + "', expected operator");
    if (is_nil(tail)) return error("unexpected nil, expected operand after '" + show(object_op) + "'");
    auto t = tail;
    while (!is_nil(t)) {
      auto p = decompose(t);
      auto curr = eval(p.first);
      switch (*op) {
        case op_t::ADD: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_arithmetic(ret, curr, [](auto a, auto b) -> object_t { return a + b; });
          break;
        }
        case op_t::SUB: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_arithmetic(ret, curr, [](auto a, auto b) -> object_t { return a - b; });
          break;
        }
        case op_t::MUL: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_arithmetic(ret, curr, [](auto a, auto b) -> object_t { return a * b; });
          break;
        }
        case op_t::DIV: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_arithmetic(ret, curr, [](auto a, auto b) -> object_t { return b != 0 ? a / b : error("division by zero"); });
          break;
        }
        case op_t::MOD: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_int(ret, curr, [](auto a, auto b) -> object_t { return a % b; });
          break;
        }
        case op_t::SCONCAT: {
          ret = is_nil(ret)
            ? (is_nil(curr) ? error("unexpected nil, expected operand") : curr)
            : op_string(ret, curr, [](auto a, auto b) -> object_t { return a + b; });
          break;
        }
        case op_t::GT:
        case op_t::GTE:
        case op_t::LT:
        case op_t::LTE:
        case op_t::EQ:
        case op_t::NOEQ: {
          if (is_nil(ret)) { ret = atom_bool(true); }
          if (is_nil(prev)) { prev = curr; break; }

          object_t r;
          switch (*op) {
            case op_t::GT:   r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a >  b; }); break;
            case op_t::GTE:  r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a >= b; }); break;
            case op_t::LT:   r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a <  b; }); break;
            case op_t::LTE:  r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a <= b; }); break;
            case op_t::EQ:   r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a == b; }); break;
            case op_t::NOEQ: r = op_arithmetic(prev, curr, [](auto a, auto b) -> object_t { return a != b; }); break;
            default: r = error("unexpected bool operator"); break;
          }

          ret = op_bool(ret, r, [](auto a, auto b) -> object_t { return a && b; });
          prev = curr;
          break;
        }
        case op_t::DEF: {
          if (is_nil(prev)) { prev = curr; break; }
          auto var = as_atom_variable(prev);
          if (!var) return error("eval_op: unexpected '" + show(prev) + "', expected variable");
          auto env = std::make_shared<env_t>(); // TODO
          return env->defvar(var->name, curr);
        }
        case op_t::SET: {
          if (is_nil(prev)) { prev = curr; break; }
          auto var = as_atom_variable(prev);
          if (!var) return error("eval_op: unexpected '" + show(prev) + "', expected variable");
          auto env = std::make_shared<env_t>(); // TODO
          return env->setvar(var->name, curr);
        }
        case op_t::GET: {
          auto var = as_atom_variable(curr);
          if (!var) return error("eval_op: unexpected '" + show(prev) + "', expected variable");
          auto env = std::make_shared<env_t>(); // TODO
          return env->getvar(var->name);
        }
        case op_t::QUOTE: {
          return tail;
          break;
        }
        case op_t::TYPEOF: {
          std::string type;
          std::visit(overloaded {
              [&type] (object_nil_t)         { type = "nil"; },
              [&type] (object_error_t)       { type = "error"; },
              [&type] (bool)                 { type = "bool"; },
              [&type] (int)                  { type = "int"; },
              [&type] (double)               { type = "double"; },
              [&type] (const std::string&)   { type = "string"; },
              [&type] (op_t)                 { type = "operator"; },
              [&type] (object_variable_t)    { type = "variable"; },
              [&type] (object_list_sptr_t)   { type = "list"; },
          }, curr);
          return atom_string(type);
        }
        case op_t::CONS: {
          ret = cons(curr, ret);
          break;
        }
        case op_t::CAR: {
          return car(curr);
          break;
        }
        case op_t::CDR: {
          return cdr(curr);
          break;
        }
        // case op_t::COND: { }
        case op_t::PRINT: {
          std::cout << show(curr) << std::endl;
          break;
        }
        // case op_t::READ: { }
        // case op_t::EVAL: { }
        // case op_t::EVALIN { }
        // case op_t::LAMBDA: { }
        // case op_t::MACRO: { }
        // case op_t::MACROEXPAND: { }
        default: return error("unexpected '" + show(object_op) + "' operator");
      }
      t = p.second;
    }
    if (*op == op_t::CONS) ret = reverse(ret, false);
    return ret;
  }

  object_t eval(const object_t& object) { // TODO
    if (is_error(object)) return object;
    object_t ret = object;
    std::visit(overloaded {
        [&ret] (object_error_t) { // TODO
          ;
        },
        [&ret] (object_list_sptr_t) {
          auto p = decompose(ret);
          ret = eval_op(p.first, p.second);
        },
        [&ret] (auto) {
          ;
        },
    }, object);
    return ret;
  }
}

int main() {
  using namespace lisp_interpreter;



  // T E S T

  auto env = std::make_shared<env_t>(); // TODO

  {
    auto l = atom_bool(true);
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(show(l) == R"LISP(true)LISP");
    assert(show_struct(l) == R"LISP(B:true )LISP");
    assert(show_struct(car(l)) == R"LISP(E:"decompose: unexpected 'true', expected list" )LISP");
    assert(show_struct(cdr(l)) == R"LISP(E:"decompose: unexpected 'true', expected list" )LISP");
    assert(show(car(l)) == R"LISP(E:"decompose: unexpected 'true', expected list")LISP");
    assert(show(cdr(l)) == R"LISP(E:"decompose: unexpected 'true', expected list")LISP");
  }

  {
    auto l = nil();
    assert(is_nil(l));
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(show(l) == R"LISP(())LISP");
    assert(show_struct(l) == R"LISP(nil )LISP");
    assert(show(car(l)) == R"LISP(E:"decompose: unexpected '()', expected list")LISP");
    assert(show(cdr(l)) == R"LISP(E:"decompose: unexpected '()', expected list")LISP");
  }

  {
    auto l = cons(atom_bool(false), nil());
    assert(show(l) == R"LISP((false))LISP");
    assert(show_struct(l) == R"LISP(LIST ( B:false ) ( nil ) )LISP");
    assert(show_struct(car(l)) == R"LISP(B:false )LISP");
    assert(show_struct(cdr(l)) == R"LISP(nil )LISP");
  }

  {
    auto l = cons(
        atom_bool(true),
        cons(
          nil(),
          cons(
            atom_int(15),
            cons(
              atom_double(1.23),
              cons(
                atom_string("my string"),
                cons(
                  atom_variable("my_int1"),
                  nil()))))));
    assert(show(l) == R"LISP((true () 15 1.230000 "my string" my_int1))LISP");
    assert(show_struct(l) == R"LISP(LIST ( B:true ) ( LIST ( nil ) ( LIST ( I:15 ) ( LIST ( D:1.230000 ) ( LIST ( S:"my string" ) ( LIST ( V:my_int1 ) ( nil ) ) ) ) ) ) )LISP");
  }

  {
    std::string str = R"LISP((1 (2.3 var2) "my str"))LISP";
    auto l = parse(str);
    assert(show(l) == R"LISP((1 (2.300000 var2) "my str"))LISP");
    assert(show_struct(l) == R"LISP(LIST ( I:1 ) ( LIST ( LIST ( D:2.300000 ) ( LIST ( V:var2 ) ( nil ) ) ) ( LIST ( S:"my str" ) ( nil ) ) ) )LISP");
  }

  {
    std::string str = R"LISP(1 (2.3 var2) "my str")LISP";
    auto l = parse(str);
    assert(show(l) == R"LISP((1 (2.300000 var2) "my str"))LISP");
    assert(show_struct(l) == R"LISP(LIST ( I:1 ) ( LIST ( LIST ( D:2.300000 ) ( LIST ( V:var2 ) ( nil ) ) ) ( LIST ( S:"my str" ) ( nil ) ) ) )LISP");
  }

  {
    std::string str = R"LISP(abc)LISP";
    auto l = parse(str);
    assert(show(l) == R"LISP((abc))LISP");
    assert(show_struct(l) == R"LISP(LIST ( V:abc ) ( nil ) )LISP");
  }

  {
    std::string str = R"LISP(  a b (0 (1) 2 ((3) 4) 5 6 () 7))LISP";
    auto l = parse(str);
    assert(show(l) == R"LISP((a b (0 (1) 2 ((3) 4) 5 6 () 7)))LISP");
  }

  {
    assert(show(eval(parse(R"LISP((+ 10 1 2 3))LISP"))) == R"LISP(16)LISP");
    assert(show(eval(parse(R"LISP((- 10 1 2 3))LISP"))) == R"LISP(4)LISP");
    assert(show(eval(parse(R"LISP((- 10 1.0 2 3))LISP"))) == R"LISP(4.000000)LISP");
    assert(show(eval(parse(R"LISP((* 10 1 2 3))LISP"))) == R"LISP(60)LISP");
    assert(show(eval(parse(R"LISP((/ 10 1 2 3))LISP"))) == R"LISP(1)LISP");
    assert(show(eval(parse(R"LISP((/ 10.0 1 2 3))LISP"))) == R"LISP(1.666667)LISP");
    assert(show(eval(parse(R"LISP((% 100 3 3))LISP"))) == R"LISP(1)LISP");
    assert(show(eval(parse(R"LISP((++ "a" "b" "c"))LISP"))) == R"LISP("abc")LISP");
  }

  {
    assert(show(eval(parse(R"LISP((> 2 1))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((> 2 1 0))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((> 2 1 0 5))LISP"))) == R"LISP(false)LISP");
    assert(show(eval(parse(R"LISP((> 2))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((> 2.0 5))LISP"))) == R"LISP(false)LISP");
    assert(show(eval(parse(R"LISP((> 9 8 7 -2 -3 (+ 1 -9)))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((> 9 8 7 -3 -3 (+ 1 -9)))LISP"))) == R"LISP(false)LISP");
    assert(show(eval(parse(R"LISP((>= 9 8 7 -3 -3 (+ 1 -9)))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((>= 2.0 2))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((>= 2 2.0))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((< 1 2 6))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((< 1 2 6 6))LISP"))) == R"LISP(false)LISP");
    assert(show(eval(parse(R"LISP((<= 1 2 6 6))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((== (+ 1 2) 3 (- 10 7) 3.))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((== (+ 1 2) 3 (- 10 7) 3. 1))LISP"))) == R"LISP(false)LISP");
    assert(show(eval(parse(R"LISP((!= 1 2 6))LISP"))) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((!= 1 2 2))LISP"))) == R"LISP(false)LISP");
  }

  {
    assert(show(eval(parse(R"LISP((quote (+ 1 2) 3 4))LISP"))) == R"LISP(((+ 1 2) 3 4))LISP");
    assert(show(eval(parse(R"LISP((typeof (+ 1 2)))LISP"))) == R"LISP("int")LISP");
    assert(show(eval(parse(R"LISP((typeof (quote (+ 1 2))))LISP"))) == R"LISP("list")LISP");
    assert(show(eval(parse(R"LISP((cons 0 1 (+ 1 1) (quote 3 4)))LISP"))) == R"LISP((0 1 2 (3 4)))LISP");
    assert(show(eval(parse(R"LISP(car (cons 0 1 (+ 1 1) (quote 3 4)))LISP"))) == R"LISP(0)LISP");
    assert(show(eval(parse(R"LISP(cdr (cons 0 1 (+ 1 1) (quote 3 4)))LISP"))) == R"LISP((1 2 (3 4)))LISP");
    assert(show(eval(parse(R"LISP((+ 1 2)(* 10 2)(+ 5 2))LISP"))) == R"LISP(7)LISP");
  }

  {
    std::cout << "output: '" << show(eval(parse(R"LISP((def a (quote 1 2 3)) (get a))LISP"))) << "'" << std::endl;
    // eval(parse(R"LISP((def a 1) (print a))LISP"));
  }

  return 0;
}

