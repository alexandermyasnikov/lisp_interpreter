
#include <iostream>
#include <memory>
#include <vector>
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
  struct object_atom_t;
  struct object_list_t;

  using object_undefined_sptr_t = std::shared_ptr<const object_undefined_t>;
  using object_error_sptr_t = std::shared_ptr<const object_error_t>;
  using object_atom_sptr_t = std::shared_ptr<const object_atom_t>;
  using object_list_sptr_t = std::shared_ptr<const object_list_t>;

  using object_t = std::variant<
    object_undefined_sptr_t,
    object_error_sptr_t,
    object_atom_sptr_t,
    object_list_sptr_t>;

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

  struct object_atom_t {
    std::string name;

    object_atom_t(const std::string& name) : name(name) { }
  };

  std::vector<std::string> keywords = {
    "+",
    "-",
    "*",
    "/",
  };




  // I N T E R F A C E

  object_t undefined() { // private
    static auto undefined = std::make_shared<object_undefined_t>();
    return undefined;
  }

  object_t error(const std::string& msg) {
    return std::make_shared<object_error_t>(msg);
  }

  object_t atom(const std::string& name) {
    return std::make_shared<object_atom_t>(name);
  }

  object_t list() {
    return std::make_shared<object_list_t>(undefined(), undefined());
  }

  bool is_atom(object_t object) {
    return std::get_if<object_atom_sptr_t>(&object);
  }

  bool is_list(object_t object) {
    return std::get_if<object_list_sptr_t>(&object);
  }

  bool is_undefined(object_t object) {
    return std::get_if<object_undefined_sptr_t>(&object);
  }

  bool is_error(object_t object) {
    return std::get_if<object_error_sptr_t>(&object);
  }

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
    if (is_undefined(list->tail)) return error("cdr: object->tail is undefined");
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



  struct shower_t {
    std::string show(object_t object) {
      if (is_error(object))
        return std::get<object_error_sptr_t>(object)->msg + " ";
      return is_list(object) ? "( " + show_list(object) + ") " : show_list(object);
    }

    std::string show_list(object_t object) {
      std::string str;
      std::visit(overloaded {
          [&str] (object_atom_sptr_t atom) {
            str += atom->name + " ";
          },
          [&str, &object, this] (object_list_sptr_t) {
            auto head = car(object);
            str += is_list(head) ? "( " + show_list(head) + ") " : show_list(head);
            str += show_list(cdr(object));
          },
          [] (auto arg) { },
      }, object);
      return str;
    }

    std::string show_n(object_t object) {
      std::string str;
      std::visit(overloaded {
          [&str] (object_undefined_sptr_t) {
            str += "U ";
          },
          [&str] (object_error_sptr_t) {
            str += "E ";
          },
          [&str] (object_atom_sptr_t atom) {
            str += atom->name + " ";
          },
          [&str, &object, this] (object_list_sptr_t) {
            auto head = is_error(car(object)) ? undefined() : car(object); // XXX сомнительно
            auto tail = is_error(cdr(object)) ? undefined() : cdr(object);
            str += "LIST ( " + show_n(head) + ") ( " + show_n(tail) + ") ";
          },
      }, object);
      return str;
    }
  };



  struct parser_t {
    std::stack<object_t> stack;
    object_t object = list();

    object_t parse(const std::string& str) {
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
            object = cons(tmp, object);
          }
        } else if (auto itf = std::find_if(keywords.begin(), keywords.end(),
              [&](const std::string& keyword) {
                  return 0 == keyword.compare(0, keyword.size(), it,
                      0, std::min(keyword.size(), (size_t) (ite - it))); });
                  itf != keywords.end()) {
          auto object_atom = std::make_shared<object_atom_t>(*itf);
          // std::cout << "keyword: '" << object_atom->name << "'" << std::endl;
          if (stack.empty()) {
            object = cons(object_atom, object);
          } else {
            auto object_n = cons(object_atom, stack.top());
            stack.top().swap(object_n);
          }
          it += itf->size() - 1;
        } else if (std::isalnum(c)) {
          auto itf = std::find_if(it, ite, [](char c) { return !isalnum(c); });
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf));
          // std::cout << "variable: '" << object_atom->name << "'" << std::endl;
          if (stack.empty()) {
            object = cons(object_atom, object);
          } else {
            auto object_n = cons(object_atom, stack.top());
            stack.top().swap(object_n);
          }
          it += (itf - it) - 1;
        } else if (c == '"') {
          auto itf = std::find(it + 1, ite, '"');
          if (itf == ite) return error("parse: expected '\"' on pos " + std::to_string(it - str.c_str()));
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf + 1));
          // std::cout << "string: '" << object_atom->name << "'" << std::endl;
          if (stack.empty()) {
            object = cons(object_atom, object);
          } else {
            auto object_n = cons(object_atom, stack.top());
            stack.top().swap(object_n);
          }
          it += (itf - it);
        }
        ++it;
      }
      if (!stack.empty()) return error("expected ')' on pos " + std::to_string(it - str.c_str()));
      if (is_error(cdr(object))) object = car(object); // Если список один, то дополнительные скобки не нужны.
      return reverse(object);
    }
  };
}

int main() {
  using namespace lisp_interpreter;



  // T E S T

  {
    auto a = atom("a");
    assert(is_atom(a));
    assert(shower_t().show(a) == R"LISP(a )LISP");
    assert(shower_t().show_n(a) == R"LISP(a )LISP");
  }

  {
    auto l = list();
    assert(is_error(car(l)));
    assert(is_error(cdr(l)));
    assert(shower_t().show_n(l) == R"LISP(LIST ( U ) ( U ) )LISP");
    assert(shower_t().show(l) == R"LISP(( ) )LISP");
  }

  {
    auto l = cons(atom("a"), list());
    assert(is_atom(car(l)));
    assert(is_error(cdr(l)));
    assert(shower_t().show_n(l) == R"LISP(LIST ( a ) ( U ) )LISP");
    assert(shower_t().show(l) == R"LISP(( a ) )LISP");
  }

  {
    auto l = cons(atom("a"), cons(atom("b"), list()));
    assert(is_atom(car(l)));
    assert(is_list(cdr(l)));
    assert(shower_t().show_n(l) == R"LISP(LIST ( a ) ( LIST ( b ) ( U ) ) )LISP");
    assert(shower_t().show(l) == R"LISP(( a b ) )LISP");
  }

  {
    auto l = cons(list(), cons(atom("b"), list()));
    assert(is_list(car(l)));
    assert(is_list(cdr(l)));
    assert(shower_t().show_n(l) == R"LISP(LIST ( LIST ( U ) ( U ) ) ( LIST ( b ) ( U ) ) )LISP");
    assert(shower_t().show(l) == R"LISP(( ( ) b ) )LISP");
  }

  {
    auto expr = cons(
        atom("*"),
        cons(
          atom("2"),
          cons(
            cons(
              atom("+"),
              cons(
                atom("3"),
                cons(
                  atom("4"),
                  list()))),
            list())));

    assert(shower_t().show(expr) == "( * 2 ( + 3 4 ) ) ");
  }

  {
    std::string str = R"LISP((1 2 3) )LISP";
    auto l = parser_t().parse(str);
    assert(shower_t().show_n(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(shower_t().show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(1 2 3 )LISP";
    auto l = parser_t().parse(str);
    assert(shower_t().show_n(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(shower_t().show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(1 2 3 )LISP";
    auto l = parser_t().parse(str);
    assert(shower_t().show_n(l) == R"LISP(LIST ( 1 ) ( LIST ( 2 ) ( LIST ( 3 ) ( U ) ) ) )LISP");
    assert(shower_t().show(l) == R"LISP(( 1 2 3 ) )LISP");
  }

  {
    std::string str = R"LISP(())LISP";
    auto l = parser_t().parse(str);
    assert(shower_t().show_n(l) == R"LISP(LIST ( U ) ( U ) )LISP");
    assert(shower_t().show(l) == R"LISP(( ) )LISP");
  }

  {
    std::string str = R"LISP(( 1 2 3 )LISP";
    auto l = parser_t().parse(str);
    assert(is_error(l));
  }

  {
    std::string str = R"LISP(1 (2 3 )LISP";
    auto l = parser_t().parse(str);
    assert(is_error(l));
  }

  {
    std::string str = R"LISP(2 () )LISP";
    std::cout << "input: '" << str << "'" << std::endl;
    auto object = parser_t().parse(str);
    std::cout << "output: '" << shower_t().show(object) << "'" << std::endl;
    std::cout << "output: '" << shower_t().show_n(object) << "'" << std::endl;
  }

  return 0;
}

