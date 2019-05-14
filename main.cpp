
#include <iostream>
#include <string_view>
#include <memory>
#include <vector>
#include <stack>
#include <charconv>
#include <algorithm>

#define PR       std::cout << __FUNCTION__ << '\t' << __LINE__ << std::endl
#define PRM(msg) std::cout << __FUNCTION__ << '\t' << __LINE__ << '\t' << msg << std::endl



using namespace std::string_literals;

namespace lisp_interpreter {

  // syntax
  // list:  LIST head tail | LIST Undefined Undefined
  // head:  list | atom
  // tail:  list
  // atom:  keyword: digit+ | digit+.digit+ | char[char|digit|-]*

  // syntax. example
  // () -> LIST Undefined Undefined
  // (1) -> LIST 1 (LIST Undefined Undefined)
  // (1 2) -> LIST 1 (LIST 2 (LIST Undefined Undefined))
  // (1 (2)) -> LIST 1 (LIST (LIST 2 (LIST Undefined Undefined)) (LIST Undefined Undefined))

  struct object_t {
    using sptr_t = std::shared_ptr<object_t>;
    virtual ~object_t() { }
  };

  struct object_list_t : object_t {
    using sptr_t = std::shared_ptr<object_list_t>;

    object_t::sptr_t        head;
    object_list_t::sptr_t   tail;

    object_list_t(object_t::sptr_t head, object_list_t::sptr_t tail) : head(head), tail(tail) { }
    object_list_t(object_t::sptr_t head) : head(head), tail(nullptr) { }
    object_list_t() : head(nullptr), tail(nullptr) { }
  };

  struct object_atom_t : object_t {
    using sptr_t = std::shared_ptr<object_atom_t>;

    std::string name;

    object_atom_t(const std::string& name) : name(name) { }
  };

  std::vector<std::string> keywords = {
    "+",
    "-",
    "*",
    "/",
  };



  bool is_null_list(const object_t::sptr_t& object) {
    auto object_list = std::dynamic_pointer_cast<object_list_t>(object);
    if (!object_list) return true;
    if (!object_list->head) return true;
    return false;
  }

  // car (список) -> S-выражение.
  object_t::sptr_t car(const object_t::sptr_t& object) {
    auto object_list = std::dynamic_pointer_cast<object_list_t>(object);
    if (!object_list) throw std::runtime_error("car: expected list");
    if (!object_list->head) throw std::runtime_error("car: head not exist");
    return object_list->head;
  }

  // cdr (список) -> список
  object_t::sptr_t cdr(const object_t::sptr_t& object) {
    auto object_list = std::dynamic_pointer_cast<object_list_t>(object);
    if (!object_list) throw std::runtime_error("cdr: expected list");
    if (!object_list->head) throw std::runtime_error("cdr: tail not exist");
    return object_list->tail;
  }

  // cons (s-выражение, список) -> список
  object_t::sptr_t cons(const object_t::sptr_t& head, const object_t::sptr_t& tail) {
    auto tail_list = std::dynamic_pointer_cast<object_list_t>(tail);
    if (!tail_list) throw std::runtime_error("cons: expected list");

    auto object_list = std::make_shared<object_list_t>(head, tail_list);
    return object_list;
  }



  struct shower_t {
    void show(object_t::sptr_t object) {
      std::cout << "(";
      show_list(object); // XXX некрасиво
      std::cout << ")";
    }
    void show_list(object_t::sptr_t object) {
      if (!object) {
        return;
      }

      if (auto object_list = std::dynamic_pointer_cast<object_list_t>(object); object_list) {
        if (auto object_list_head = std::dynamic_pointer_cast<object_list_t>(object_list->head); object_list_head) {
          std::cout << "(";
          show_list(object_list->head);
          std::cout << ") ";
        } else {
          show_list(object_list->head);
        }
        show_list(object_list->tail);
      } else if (auto object_atom = std::dynamic_pointer_cast<object_atom_t>(object); object_atom) {
        std::cout << object_atom->name << " ";
      } else {
          throw std::runtime_error("show_list: unknown type");
      }
    }
  };



  struct parser_t {
    std::stack<object_t::sptr_t> stack;
    object_t::sptr_t object = nullptr;

    object_t::sptr_t parse(const std::string& str) {
      const char *it = str.c_str();
      const char *ite = str.c_str() + str.size();
      int value;
      while (it != ite) {
        char c = *it;
        if (c == '(') {
          std::cout << "push list" << std::endl;
          stack.emplace(std::make_shared<object_list_t>());
        } else if (c == ')') {
          if (stack.empty()) throw std::runtime_error("unexpected ')'");
          std::cout << "pop list" << std::endl;

          if (stack.size() == 1) {
            if (!object)
              object = stack.top();
            else
              object = cons(object, stack.top());
          }

          auto tmp = stack.top();
          stack.pop();

          if (!stack.empty()) { // XXX иначе кинуть исключение
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
          std::cout << "keyword: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += itf->size() - 1;
        } else if (std::isalnum(c)) {
          auto itf = std::find_if(it, ite, [](char c) { return !isalnum(c); });
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf));
          std::cout << "variable: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += (itf - it) - 1;
        } else if (c == '"') {
          auto itf = std::find(it + 1, ite, '"');
          if (itf == ite) throw std::runtime_error("parse: expected '\"'");
          auto object_atom = std::make_shared<object_atom_t>(std::string(it, itf + 1));
          std::cout << "string: '" << object_atom->name << "'" << std::endl;
          auto object_n = cons(object_atom, stack.top());
          stack.top().swap(object_n);
          it += (itf - it);
        } else {
          throw std::runtime_error("parse: unknown char: '"s + c + "'");
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

  {
    auto l_ex1 = std::make_shared<object_list_t>(
        std::make_shared<object_atom_t>("*"),
        std::make_shared<object_list_t>(
          std::make_shared<object_atom_t>("2"),
          std::make_shared<object_list_t>(
            std::make_shared<object_list_t>(
              std::make_shared<object_atom_t>("+"),
              std::make_shared<object_list_t>(
                std::make_shared<object_atom_t>("3"),
                std::make_shared<object_list_t>(
                  std::make_shared<object_atom_t>("4")
                  )
                )
              )
            )
          )
        );

    auto l_ex2 = std::make_shared<object_atom_t>("1");

    auto l_ex3 = std::make_shared<object_list_t>(
        std::make_shared<object_atom_t>("1")
        );

    auto l_ex4 = std::make_shared<object_list_t>(
        std::make_shared<object_list_t>(
          std::make_shared<object_atom_t>("2")
          )
        );

    // shower_t().show(l_ex1);
    // std::cout << std::endl;
  }

  {
    std::string str = R"LISP((1 29 abc (  + 1   "2" 34) 56 78 ))LISP";
    std::cout << "input: " << str << std::endl;

    auto object = parser_t().parse(str);

    shower_t().show(object);
    std::cout << std::endl;
  }

  return 0;
}

