
#include <iostream>
#include <memory>
#include <map>
#include <stack>
#include <variant>
#include <algorithm>
#include <sstream>

#include "lisp_interpreter.h"

#define PRM(msg)  std::cout << __FUNCTION__ << ':' << __LINE__ << '\t' << msg << std::endl
#define assert(x) if (!(x)) PRM("ASSERT " #x)



int main() {
  using namespace lisp_interpreter;

  // R E P L
  {
    auto env = std::make_shared<env_t>();
    std::string str;
    while (true) {
      context_t ctx;
      std::cout << "lisp $ ";
      std::getline(std::cin, str);

      if (str == ":l") {
        env = std::make_shared<env_t>();
        str = "(:load \"standart.lispam\")";
      } else if (str == "") {
        break;
      }

      try {
        auto l = parse(str);
        std::cout << "input: \t" << show(l) << std::endl;
        l = eval(l, env, ctx);
        std::cout << "result: \t" << show(l) << std::endl;
      } catch (const std::exception& e) {
        std::cout << "exception: \t" << e.what() << std::endl;
      } catch (...) {
        std::cout << "exception" << std::endl;
      }

      std::cout << "eval_calls: \t" << ctx.eval_calls << std::endl;
      std::cout << "stream: \t" << ctx.stream.str() << std::endl;
    }
  }

  return 0;
}

