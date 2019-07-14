
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
        str = "(__kernel_load \"standart.lispam\")";
      } else if (str == "") {
        break;
      }

      try {
        object_sptr_t l;
        {
          LOG_DURATION(ctx.time_parse);
          l = object_t::parse(str);
        }
        std::cout << "input: \t" << l->show() << std::endl;
        {
          ctx.time_eval = 159;
          LOG_DURATION(ctx.time_eval);
          l = l->eval(env, ctx);
        }
        std::cout << "result: \t" << l->show() << std::endl;
      } catch (const std::exception& e) {
        std::cout << "exception: \t" << e.what() << std::endl;
      } catch (...) {
        std::cout << "exception" << std::endl;
      }

      std::cout << "eval_calls: \t" << ctx.eval_calls << std::endl;
      std::cout << "time_parse: \t" << ctx.time_parse << " ms" << std::endl;
      std::cout << "time_eval: \t" << ctx.time_eval << " ms" << std::endl;
      std::cout << "stream: \t" << ctx.stream.str() << std::endl;
    }
  }

  return 0;
}

