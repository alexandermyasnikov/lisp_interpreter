
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


  // FD

  std::string show_struct(object_t object);
  std::string show(object_t object);
  object_t parse(const std::string& str);



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

  object_t list(object_t head, object_t tail) {
    return std::make_shared<object_list_t>(head, tail);
  }

  bool is_nil(object_t object) {
    return std::get_if<object_nil_t>(&object);
  }

  bool is_list(object_t object) {
    return std::get_if<object_list_sptr_t>(&object);
  }

  bool is_error(object_t object) {
    return std::get_if<object_error_t>(&object);
  }

  std::pair<object_t, object_t> pair(object_t object) {
    if (is_error(object)) {
      return {object, object};
    } else if (!is_list(object)) {
      auto ret = error("pair: object '" + (show_struct(object)) + "' is not list");
      return {ret, ret};
    }
    auto list = std::get<object_list_sptr_t>(object);
    return {list->head, list->tail};
  }

  object_t car(object_t object) {
    return pair(object).first;
  }

  object_t cdr(object_t object) {
    return pair(object).second;
  }

  object_t cons(object_t head, object_t tail) {
    if (is_error(head))
      return head;
    else if (is_error(tail))
      return tail;
    else if (!is_list(tail) && !is_nil(tail))
      return error("cons: object '" + (show_struct(tail)) + "' is not list and not nil");
    assert(show_struct(car(list(head, tail))) == show_struct(head));
    assert(show_struct(cdr(list(head, tail))) == show_struct(tail));
    return list(head, tail);
  }



  // L I B R A R Y

  object_t reverse(object_t object) {
    if (!is_list(object))
      return object;
    auto list_r = nil();
    while (is_list(object)) {
      list_r = cons(reverse(car(object)), list_r);
      object = cdr(object);
    }
    return list_r;
  }

  std::string show(object_t object) {
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
          str += std::to_string(static_cast<int>(v)); // TODO
        },
        [&str] (object_variable_t v) {
          str += v.name;
        },
        [&str, &object] (object_list_sptr_t) {
          auto p = pair(object);
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

  std::string show_struct(object_t object) {
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
          str += "O:" + std::to_string(static_cast<int>(v)) + " "; // TODO
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
        if (auto itk = keywords.find(value_lower); itk != keywords.end()) { // operator
          object_atom = atom_operator(itk->second);
        } else if (value_lower == "true") {                                 // bool true
          object_atom = atom_bool(true);
        } else if (value_lower == "false") {                                // bool false
          object_atom = atom_bool(false);
        } else if (std::all_of(value.begin(), value.end(), isdigit)) {      // int
          object_atom = atom_int(stoi(value));
        } else if (value.find('.') != std::string::npos) {                  // double
          object_atom = atom_double(stod(value));
        } else {                                                            // variable
          object_atom = atom_variable(value);
        }
        if (stack.empty()) {
          ret = cons(object_atom, ret);
        } else {
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
        }
        it += (itf - it) - 1;
      } else {
      }
      ++it;
    }
    if (!stack.empty()) return error("expected ')' on pos " + std::to_string(it - str.c_str()));
    if (is_nil(cdr(ret)) && is_list(car(ret))) ret = car(ret);
    return reverse(ret);
  }

  object_t eval_op(object_t op, object_t acc, object_t tail) {
    // TODO
    return op;
  }

  object_t eval(object_t object) { // TODO
    object_t ret = object;
    std::visit(overloaded {
        [&ret] (object_list_sptr_t) {
          // auto op = car(ret);
          // auto tail = cdr(ret);
          // ret = eval_op(op, undefined(), tail);
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
    auto l = atom_bool(true);
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(show(l) == R"LISP(true)LISP");
    assert(show_struct(l) == R"LISP(B:true )LISP");
    assert(show_struct(car(l)) == R"LISP(E:"pair: object 'B:true ' is not list" )LISP");
    assert(show_struct(cdr(l)) == R"LISP(E:"pair: object 'B:true ' is not list" )LISP");
  }

  {
    auto l = nil();
    assert(is_nil(l));
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(show(l) == R"LISP(())LISP");
    assert(show_struct(l) == R"LISP(nil )LISP");
    assert(show_struct(car(l)) == R"LISP(E:"pair: object 'nil ' is not list" )LISP");
    assert(show_struct(cdr(l)) == R"LISP(E:"pair: object 'nil ' is not list" )LISP");
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
    std::string str = R"LISP(  ab)LISP";
    auto l = parse(str);
    std::cout << "input: '" << str << "'" << std::endl;
    std::cout << "output: '" << show(l) << "'" << std::endl;
    std::cout << "output: '" << show_struct(l) << "'" << std::endl;
  }

  return 0;
}

