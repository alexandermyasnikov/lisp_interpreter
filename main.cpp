
#include <iostream>

#include "lisp_utils.h"



int main() {

  {
    lisp_utils::lisp_compiler_t lisp_compiler;
    lisp_compiler.test();
    std::cout << "end" << std::endl;
  }

  return 0;
}

