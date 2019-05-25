
#include <iostream>
#include <memory>
#include <map>
#include <stack>
#include <variant>
#include <optional>
#include <algorithm>

#define PR        std::cout << __FUNCTION__ << '\t' << __LINE__ << std::endl
#define PRM(msg)  std::cout << __FUNCTION__ << '\t' << __LINE__ << '\t' << msg << std::endl
#define assert(x) if (!(x)) PRM("ASSERT " #x)



using namespace std::string_literals;

namespace lisp_interpreter {

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  struct object_undefined_t;
  struct object_error_t;
  struct object_atom_keyword_t;
  struct object_atom_bool_t;
  struct object_atom_double_t;
  struct object_atom_string_t;
  struct object_atom_variable_t;
  struct object_list_t;

  using object_undefined_sptr_t       = std::shared_ptr<const object_undefined_t>;
  using object_error_sptr_t           = std::shared_ptr<const object_error_t>;
  using object_atom_keyword_sptr_t    = std::shared_ptr<const object_atom_keyword_t>;
  using object_atom_bool_sptr_t       = std::shared_ptr<const object_atom_bool_t>;
  using object_atom_double_sptr_t     = std::shared_ptr<const object_atom_double_t>;
  using object_atom_string_sptr_t     = std::shared_ptr<const object_atom_string_t>;
  using object_atom_variable_sptr_t   = std::shared_ptr<const object_atom_variable_t>;
  using object_list_sptr_t            = std::shared_ptr<const object_list_t>;

  using object_t = std::variant<
    object_undefined_sptr_t,
    object_error_sptr_t,
    object_atom_keyword_sptr_t,
    object_atom_double_sptr_t,
    object_atom_string_sptr_t,
    object_atom_variable_sptr_t,
    object_list_sptr_t>;

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


  struct object_undefined_t { };

  struct object_error_t {
    std::string msg;

    object_error_t(const std::string& msg) : msg(msg) { }
  };

  struct object_list_t {
    object_t head;
    object_t tail;

    object_list_t(object_t head, object_t tail) : head(head), tail(tail) { }
  };

  struct object_atom_keyword_t {
    op_t value;
    object_atom_keyword_t(op_t value) : value(value) { }
  };

  struct object_atom_bool_t {
    bool value;
    object_atom_bool_t(bool value) : value(value) { }
  };

  struct object_atom_double_t {
    double value;
    object_atom_double_t(double value) : value(value) { }
  };

  struct object_atom_string_t {
    std::string value;
    object_atom_string_t(const std::string& value) : value(value) { }
  };

  struct object_atom_variable_t {
    std::string value;
    object_atom_variable_t(const std::string& value) : value(value) { }
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
    { "=",             op_t::EQ },
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



  // I N T E R F A C E

  object_t undefined() { // private
    static auto undefined = std::make_shared<object_undefined_t>();
    return undefined;
  }

  object_t error(const std::string& msg) {
    return std::make_shared<object_error_t>(msg);
  }

  object_t atom_keyword(op_t value) {
    return std::make_shared<object_atom_keyword_t>(value);
  }

  object_t atom_double(double value) {
    return std::make_shared<object_atom_double_t>(value);
  }

  object_t atom_string(const std::string& value) {
    return std::make_shared<object_atom_string_t>(value);
  }

  object_t atom_variable(const std::string& value) {
    return std::make_shared<object_atom_variable_t>(value);
  }

  object_t list() {
    return std::make_shared<object_list_t>(undefined(), undefined());
  }

  /*bool is_atom(object_t object) {
    return std::get_if<object_atom_sptr_t>(&object);
  }*/

  bool is_list(object_t object) {
    return std::get_if<object_list_sptr_t>(&object);
  }

  bool is_undefined(object_t object) {
    return std::get_if<object_undefined_sptr_t>(&object);
  }

  bool is_error(object_t object) {
    return std::get_if<object_error_sptr_t>(&object);
  }

  bool is_null(object_t object) {
    if (!is_list(object)) return false;
    auto list = std::get<object_list_sptr_t>(object);
    return is_undefined(list->head) && is_undefined(list->tail);
  }

  // is_null

  object_t car(object_t object) {
    if (is_error(object)) return object;
    if (!is_list(object)) return error("car: object is not list");
    auto list = std::get<object_list_sptr_t>(object);
    if (is_undefined(list->head)) return error("car: object->head is undefined");
    return list->head;
  }

  object_t cdr(object_t object) {
    if (is_error(object)) return object;
    if (!is_list(object)) return error("cdr: expected list");
    auto list = std::get<object_list_sptr_t>(object);
    if (is_undefined(list->head) && is_undefined(list->tail)) return error("cdr: object->tail is undefined");
    return list->tail;
  }

  object_t cons(object_t head, object_t tail) {
    if (is_error(head)) return head;
    if (is_error(tail)) return tail;
    if (!is_list(tail)) return error("cons: tail is not list");
    if (is_error(car(tail))) {
      if (is_error(cdr(tail))) {
        return std::make_shared<object_list_t>(head, undefined());
      } else {
        return cons(head, cdr(tail));
      }
    }
    return std::make_shared<object_list_t>(head, tail);
  }



  // L I B R A R Y

  object_t reverse(object_t object) {
    if (!is_list(object))
      return object;
    auto list_r = list();
    while (is_list(object) && !is_error(car(object))) {
      list_r = cons(reverse(car(object)), list_r);
      object = cdr(object);
    }
    return list_r;
  }

  std::string show_list(object_t object) { // private
    std::string str;
    std::visit(overloaded { // TODO
        [&str] (object_atom_string_sptr_t atom) {
          str += atom->value + " ";
        },
        [&str, &object] (object_list_sptr_t) {
          auto head = car(object);
          str += is_list(head) ? "( " + show_list(head) + ") " : show_list(head);
          str += show_list(cdr(object));
        },
        [] (auto) { },
    }, object);
    return str;
  }

  std::string show(object_t object) {
    if (is_error(object))
      return std::get<object_error_sptr_t>(object)->msg + " ";
    return is_list(object) ? "( " + show_list(object) + ") " : show_list(object);
  }

  std::string show_stucture(object_t object) {
    std::string str;
    std::visit(overloaded {
        [&str] (object_undefined_sptr_t) {
          str += "U ";
        },
        [&str] (object_error_sptr_t) {
          str += "E ";
        },
        [&str] (object_atom_string_sptr_t atom) { // TODO
          str += atom->value + " ";
        },
        [&str, &object] (object_list_sptr_t) {
          auto head = is_error(car(object)) ? undefined() : car(object); // XXX сомнительно
          auto tail = is_error(cdr(object)) ? undefined() : cdr(object);
          str += "LIST ( " + show_stucture(head) + ") ( " + show_stucture(tail) + ") ";
        },
        [&str] (auto) {
        },
    }, object);
    return str;
  }

  object_t parse(const std::string& str) {
    std::stack<object_t> stack;
    object_t ret = list();
    const char *it = str.c_str();
    const char *ite = str.c_str() + str.size();
    while (it != ite) {
      char c = *it;
      if (c == '(') {
        stack.emplace(list());
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
      /*} else if (auto itf = keywords.find("+"); itf != keywords.end()) {
        auto object_atom = std::make_shared<object_atom_keyword_t>(itf->first);
        // std::cout << "keyword: '" << object_atom->name << "'" << std::endl;
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += itf->first.size() - 1;
      */} else if (std::isalnum(c)) {
        auto itf = std::find_if(it, ite, [](auto c) { return !isalnum(c); });
        std::string value = std::string(it, itf);
        std::string value_lower = value;
        std::transform(value.begin(), value.end(), value_lower.begin(), ::tolower);

        object_t object_atom;
        if (auto itk = keywords.find(value_lower); itk != keywords.end()) { // keyword
          auto object_atom = atom_keyword(itk->second);
        } else { // variable
          auto object_atom = atom_variable(value);
        }
        // std::cout << "variable: '" << object_atom->name << "'" << std::endl;
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += (itf - it) - 1;
      } else if (c == '"') {
        auto itf = std::find(it + 1, ite, '"');
        if (itf == ite) return error("parse: expected '\"' on pos " + std::to_string(it - str.c_str()));
        auto object_atom = std::make_shared<object_atom_string_t>(std::string(it, itf + 1));
        // std::cout << "string: '" << object_atom->name << "'" << std::endl;
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += (itf - it);
      }
      ++it;
    }
    if (!stack.empty()) return error("expected ')' on pos " + std::to_string(it - str.c_str()));
    if (is_error(cdr(ret))) ret = car(ret); // Если список один, то дополнительные скобки не нужны.
    return reverse(ret);
  }

  object_t eval_op(object_t op, object_t acc, object_t tail) {
    return op;
  }

  object_t eval(object_t object) {
    object_t ret = object;
    std::visit(overloaded {
        [&ret] (object_list_sptr_t) {
          auto op = car(ret);
          auto tail = cdr(ret);
          ret = eval_op(op, undefined(), tail);
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

  {
    auto a = atom_string("a");
    // assert(is_atom(a));
    assert(show(a) == R"LISP(a )LISP");
    assert(show_stucture(a) == R"LISP(a )LISP");
  }

  {
    auto l = list();
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(show_stucture(l) == R"LISP(LIST ( U ) ( U ) )LISP");
    assert(show(l) == R"LISP(( ) )LISP");
  }

  {
    auto l = cons(atom_string("a"), list());
    // assert(is_atom(car(l)));
    assert(is_error(cdr(l)));
    assert(show_stucture(l) == R"LISP(LIST ( a ) ( U ) )LISP");
    assert(show(l) == R"LISP(( a ) )LISP");
  }

  {
    auto l = cons(atom_string("a"), cons(atom_string("b"), list()));
    // assert(is_atom(car(l)));
    assert(is_list(cdr(l)));
    assert(show_stucture(l) == R"LISP(LIST ( a ) ( LIST ( b ) ( U ) ) )LISP");
    assert(show(l) == R"LISP(( a b ) )LISP");
  }

  {
    auto l = cons(list(), cons(atom_string("b"), list()));
    assert(is_list(car(l)));
    assert(is_list(cdr(l)));
    assert(show_stucture(l) == R"LISP(LIST ( LIST ( U ) ( U ) ) ( LIST ( b ) ( U ) ) )LISP");
    assert(show(l) == R"LISP(( ( ) b ) )LISP");
  }

  {
    auto expr = cons(
        atom_string("*"),
        cons(
          atom_string("2"),
          cons(
            cons(
              atom_string("+"),
              cons(
                atom_string("3"),
                cons(
                  atom_string("4"),
                  list()))),
            list())));

    assert(show(expr) == "( * 2 ( + 3 4 ) ) ");
  }

  {
    std::string str = R"LISP((1 2 3) )LISP";
    auto l = parse(str);
    assert(show_stucture(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(1 2 3 )LISP";
    auto l = parse(str);
    assert(show_stucture(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(1 2 3 )LISP";
    auto l = parse(str);
    assert(show_stucture(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(())LISP";
    auto l = parse(str);
    assert(show_stucture(l) == R"LISP(LIST ( U ) ( U ) )LISP");
    assert(show(l) == R"LISP(( ) )LISP");
  }

  {
    std::string str = R"LISP(( 1 2 3 )LISP";
    auto l = parse(str);
    assert(is_error(l));
  }

  {
    std::string str = R"LISP(1 (2 3 )LISP";
    auto l = parse(str);
    assert(is_error(l));
  }

  {
    std::string str = R"LISP((+ 1 2 3 var "str" (CONS 1 2 3)))LISP";
    std::cout << "input: '" << str << "'" << std::endl;
    auto object = eval(parse(str));
    std::cout << "output: '" << show(object) << "'" << std::endl;
    std::cout << "output: '" << show_stucture(object) << "'" << std::endl;
  }

  return 0;
}

