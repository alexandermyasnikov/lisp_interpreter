
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

  struct object_t {
    virtual ~object_t() { }
  };

  using object_sptr_t      = std::shared_ptr<object_t>;

  // struct object_null_t     : object_t { };
  struct object_int_t      : object_t { int value; };
  struct object_double_t   : object_t { double value; };
  struct object_string_t   : object_t { std::string value; };
  struct object_variable_t : object_t { std::string name; };
  struct object_list_t     : object_t { object_sptr_t head; object_sptr_t tail; };
  struct object_keyword_t  : object_t { int id; };

  using object_list_sptr_t = std::shared_ptr<object_list_t>;

  std::vector<std::string> keywords = {
    "+",
    "-",
    "*",
    "/",
  };


  object_sptr_t car(const object_sptr_t& object) { // car (список) -> S-выражение.
    auto object_list = std::dynamic_pointer_cast<object_list_t>(object);
    if (!object_list) throw std::runtime_error("expected list");
    return object_list->head;
  }

  object_sptr_t cdr(const object_sptr_t& object) { // cdr (список) -> список
    auto object_list = std::dynamic_pointer_cast<object_list_t>(object);
    if (!object_list) throw std::runtime_error("expected list");
    return object_list->tail;
  }

  object_sptr_t cons(const object_sptr_t& head, const object_sptr_t& tail) { // cons (s-выражение, список) -> список
    auto tail_list = std::dynamic_pointer_cast<object_list_t>(tail);
    if (!tail_list) throw std::runtime_error("expected list");

    auto object_list = std::make_shared<object_list_t>();
    object_list->head = head;
    object_list->tail = tail;
    return object_list;
  }


  struct parser_t {
    std::stack<object_sptr_t> stack;
    object_sptr_t object = nullptr;

    object_sptr_t parse(const std::string& str) {
      const char *it = str.c_str();
      const char *ite = str.c_str() + str.size();
      int value;
      while (it != ite) {
        char c = *it;
        if (c == '(') {
          std::cout << "push list" << std::endl;
          stack.emplace(std::make_shared<object_list_t>());
          if (stack.size() == 1) {
            if (!object)
              object = stack.top();
            else
              object = cons(object, stack.top());;
          }
        } else if (c == ')') {
          if (stack.empty()) throw std::runtime_error("unexpected ')'");
          std::cout << "pop list" << std::endl;
          stack.pop();
        } else if (std::isspace(c)) {
          ;
        } else if (auto itf = std::find_if(keywords.begin(), keywords.end(),
              [&](const std::string& keyword) {
                  return 0 == keyword.compare(0, keyword.size(), it,
                      0, std::min(keyword.size(), (size_t) (ite - it))); });
                  itf != keywords.end()) {
          std::cout << "keyword: '" << *itf << "'" << std::endl;
          it += itf->size() - 1;
        } else if (auto [p, ec] = std::from_chars(it, ite, value); ec == std::errc()) {
          auto object_int = std::make_shared<object_int_t>();
          object_int->value = value;
          std::cout << "digit: '" << value << "'" << std::endl;
          it += (p - it) - 1;
          if (!object)
            object = std::make_shared<object_list_t>();
          object = cons(object_int, object); // XXX
        } else if (std::isalpha(c)) {
          auto object_variable = std::make_shared<object_variable_t>();
          auto itf = std::find_if(it, ite, [](char c) { return !isalnum(c) && c != '-'; });
          object_variable->name = std::string(it, itf);
          std::cout << "variable: '" << object_variable->name << "'" << std::endl;
          it += (itf - it) - 1;
        } else if (c == '"') {
          auto object_string = std::make_shared<object_string_t>();
          auto itf = std::find(it + 1, ite, '"');
          object_string->value = std::string(it + 1, itf);
          std::cout << "string: '" << object_string->value << "'" << std::endl;
          it += (itf - it);
        } else {
          throw std::runtime_error("parse: unknown char: '"s + c + "'");
        }
        ++it;
      }
      std::cout << "stack.size(): " << stack.size() << std::endl;
      if (!stack.empty()) throw std::runtime_error("expected ')'");
      return object;
    }
  };

  struct shower_t {
    void show(object_sptr_t object) {
      if (!object) return;

      if (auto object_list = std::dynamic_pointer_cast<object_list_t>(object); object_list) {
        // std::cout << "(" << std::endl;
        show(object_list->head);
        show(object_list->tail);
        // std::cout << ")" << std::endl;
      } else if (auto object_int = std::dynamic_pointer_cast<object_int_t>(object); object_int) {
        std::cout << object_int->value << std::endl;
      } else if (auto object_double = std::dynamic_pointer_cast<object_double_t>(object); object_double) {
        std::cout << object_double->value << std::endl;
      } else if (auto object_string = std::dynamic_pointer_cast<object_string_t>(object); object_string) {
        std::cout << "\"" << object_string->value << "\"" << std::endl;
      } else if (auto object_variable = std::dynamic_pointer_cast<object_variable_t>(object); object_variable) {
        std::cout << object_variable->name << std::endl;
      } else if (auto object_keyword = std::dynamic_pointer_cast<object_keyword_t>(object); object_keyword) {
        std::cout << keywords.at(object_keyword->id) << std::endl;
      } else {
          throw std::runtime_error("show: unknown type");
      }
    }
  };



  /*inline static const std::vector<object_sptr_t> objects {
    // std::make_shared<object_null_t>(),
    // std::make_shared<object_double_t>(),
    // std::make_shared<object_string_t>(),
    std::make_shared<object_list_t>(),
  };*/

  /*static std::string_view parse(const std::string_view& str) {
    auto it = str.cbegin();
    auto ite = str.cend();
    auto s = str;
    while (it != ite) {
      for (const auto& o : objects) {
        auto r = o->parse(str);
        if (r.second) {
          it = r.first.cbegin() + 1;
          break;
        }
      }


      ++it;
    }
    return str;
  }*/ 
}

int main() {
  using namespace lisp_interpreter;

  std::string str = R"LISP(1 29 abc (  + 1   "2" 34) 56 78 )LISP";
  std::cout << "input: " << str << std::endl;

  auto object = parser_t().parse(str);
  shower_t().show(object);

  std::cout << "end." << std::endl;

  return 0;
}

