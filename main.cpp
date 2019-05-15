
#include <iostream>
#include <memory>
#include <vector>
#include <stack>
#include <variant>
#include <algorithm>

#define PR        std::cout << __FUNCTION__ << '\t' << __LINE__ << std::endl
#define PRM(msg)  std::cout << __FUNCTION__ << '\t' << __LINE__ << '\t' << msg << std::endl
#define assert(x) if (!(x)) PRM("ASSERT " #x)



using namespace std::string_literals;

namespace lisp_interpreter {

  // syntax
  // list:  LIST head tail | LIST Undefined Undefined
  // head:  list | atom
  // tail:  list | atom
  // atom:  keyword: digit+ | digit+.digit+ | char[char|digit|-]*

  // syntax. example
  // () -> LIST Undefined Undefined
  // (1) -> LIST 1 (LIST Undefined Undefined)
  // (1 2) -> LIST 1 (LIST 2 (LIST Undefined Undefined))
  // (1 (2)) -> LIST 1 (LIST (LIST 2 (LIST Undefined Undefined)) (LIST Undefined Undefined))

  struct object_undefined_t;
  struct object_atom_t;
  struct object_list_t;

  using object_undefined_sptr_t = std::shared_ptr<const object_undefined_t>;
  using object_atom_sptr_t = std::shared_ptr<const object_atom_t>;
  using object_list_sptr_t = std::shared_ptr<const object_list_t>;

  using object_t = std::variant<
    object_undefined_sptr_t,
    object_atom_sptr_t,
    object_list_sptr_t>;

  struct object_undefined_t { };

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




  // I N T E R F A T E

  object_t new_atom(const std::string& name) {
    return std::make_shared<object_atom_t>(name);
  }

  object_t new_undefined() {
    static auto undefined = std::make_shared<object_undefined_t>();
    return undefined;
  }

  object_t new_list() {
    return std::make_shared<object_list_t>(new_undefined(), new_undefined());
  }

  bool is_undefined(object_t object) {
    return std::get_if<object_undefined_sptr_t>(&object);
  }

  bool is_atom(object_t object) {
    return std::get_if<object_atom_sptr_t>(&object);
  }

  bool is_list(object_t object) {
    return std::get_if<object_list_sptr_t>(&object);
  }

  object_t car(object_t object) {
    if (!is_list(object)) return new_undefined();
    auto list = std::get<object_list_sptr_t>(object);
    if (is_undefined(list->head)) return new_undefined();
    return list->head;
  }

  object_t cdr(object_t object) {
    if (!is_list(object)) return new_undefined();
    auto list = std::get<object_list_sptr_t>(object);
    if (is_undefined(list->tail)) return new_undefined();
    return list->tail;
  }

  object_t cons(object_t head, object_t tail) {
    return std::make_shared<object_list_t>(head, tail);
  }



  struct shower_t {
    std::string show(object_t object) {
      return "(" + show_list(object) + ") ";
    }
    std::string show_list(object_t object) {
      std::string str;
      std::get_if<object_list_sptr_t>(&object);
      if (is_undefined(object)) {
        ;
      } else if (is_atom(object)) {
        str += std::get<object_atom_sptr_t>(object)->name + " ";
      } else if (is_list(object)) {
        auto head = car(object);
        str += is_list(head) ? show(head) : show_list(head);
        str += show_list(cdr(object));
      }
      return str;
    }
  };



  struct parser_t {
    std::stack<object_t> stack;
    object_t object = new_undefined();

    object_t parse(const std::string& str) {
      const char *it = str.c_str();
      const char *ite = str.c_str() + str.size();
      while (it != ite) {
        char c = *it;
        if (c == '(') {
          // std::cout << "push list" << std::endl;
          stack.emplace(new_list());
        } else if (c == ')') {
          if (stack.empty()) throw std::runtime_error("unexpected ')'");
          // std::cout << "pop list" << std::endl;

          auto list = stack.top();
          auto list_r = new_list();
          while (is_list(list)) {
            list_r = cons(car(list), (list_r)); // reverse
            list = cdr(list);
          }
          stack.top().swap(list_r);

          if (stack.size() == 1) { // save object
            if (is_undefined(object))
              object = stack.top();
            else
              object = cons(object, stack.top());
          }

          auto tmp = stack.top();
          stack.pop();

          if (!stack.empty()) {
            auto object_n = cons(tmp, stack.top());
            stack.top().swap(object_n);
          }

        } else if (std::isspace(c)) {
          ;
        } else if (auto itf = std::find_if(keywords.begin(), keywords.end(),
              [&](const std::string& keyword) {
                  return 0 == keyword.compare(0, keyword.size(), it,
                      0, std::min(keyword.size(), (size_t) (ite - it))); });
                  itf != keywords.end()) {
          auto object_atom = std::make_shared<object_atom_t>(*itf);
          // std::cout << "keyword: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += itf->size() - 1;
        } else if (std::isalnum(c)) {
          auto itf = std::find_if(it, ite, [](char c) { return !isalnum(c); });
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf));
          // std::cout << "variable: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += (itf - it) - 1;
        } else if (c == '"') {
          auto itf = std::find(it + 1, ite, '"');
          if (itf == ite) throw std::runtime_error("parse: expected '\"'");
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf + 1));
          // std::cout << "string: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += (itf - it);
        }
        ++it;
      }
      if (!stack.empty()) throw std::runtime_error("expected ')'");
      return object;
    }
  };
}

int main() {
  using namespace lisp_interpreter;



  // T E S T S

  {
    auto l = new_list();
    assert(is_list(l));
    assert(is_undefined(car(l)));
    assert(is_undefined(cdr(l)));
  }
  {
    auto u = new_undefined();
    auto l = cons(new_atom("1"), u);
    assert(is_list(l));
    assert(!is_undefined(car(l)));
    assert(is_undefined(cdr(l)));
  }
  {
    auto expr = cons(
        new_atom("*"),
        cons(
          new_atom("2"),
          cons(
            cons(
              new_atom("+"),
              cons(
                new_atom("3"),
                cons(
                  new_atom("4"),
                  new_undefined()))),
            new_undefined())));

    assert(shower_t().show(expr) == "(* 2 (+ 3 4 ) ) ");
  }
  {
    std::string str = R"LISP((1 29 abc (  + 1   "2" 34 ("a" "b" "c")) 56 78 ))LISP";
    auto object = parser_t().parse(str);
    assert(shower_t().show(object) == R"LISP((1 29 abc (+ 1 "2" 34 ("a" "b" "c" ) ) 56 78 ) )LISP");
  }

  {
    std::string str = R"LISP((1 29 abc (  + 1   "2" 34 ("a" "b" "c")) 56 78 ))LISP";
    std::cout << "input: " << str << std::endl;
    auto object = parser_t().parse(str);
    std::cout << "output: " << shower_t().show(object) << std::endl;
  }

  return 0;
}

