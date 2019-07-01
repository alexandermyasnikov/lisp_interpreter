
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



  // T E S T

  auto env = std::make_shared<env_t>();

  {
    context_t ctx;
    // std::cout << "output: '" << show(parse(R"LISP((+ 1 2) (+ 3 4))LISP")) << "'" << std::endl;
    // std::cout << "output: '" << show(eval(parse(R"LISP((+ 1 2) (+ 3 4) 5)LISP"), env, ctx)) << "'" << std::endl;
    // std::cout << "output: '" << show(eval(parse(R"LISP(:load 'demo.lispam)LISP"), env, ctx)) << "'" << std::endl;
    // std::cout << "output: '" << show(parse(R"LISP((1 2 3))LISP")) << "'" << std::endl;
    // eval(parse(R"LISP((def a 1) (print a))LISP"));
  }



  /*{
    context_t ctx;

    assert(show(eval(parse(R"LISP((+ (+ 5 5 1) 1 2 3 (+ 10)))LISP"), env, ctx)) == R"LISP(27)LISP");
    assert(show(eval(parse(R"LISP((+ (+ 5 5 1) 1 2.0 3 (+ 10)))LISP"), env, ctx)) == R"LISP(27.000000)LISP");

    assert(show(eval(parse(R"LISP((< (+ 0 1) 2.0 3 (+ 10)))LISP"), env, ctx)) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((< (+ 0 1) 2 3 (+ 10)))LISP"), env, ctx)) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((< "a" (++ "a" "b") "c"))LISP"), env, ctx)) == R"LISP(true)LISP");
    assert(show(eval(parse(R"LISP((< "a" (++ "a") "c"))LISP"), env, ctx)) == R"LISP(false)LISP");

    assert(show(eval(parse(R"LISP((quote (+ 1 2)))LISP"), env, ctx)) == R"LISP((+ 1 2))LISP");
    assert(show(eval(parse(R"LISP((quote (+ 1 2)))LISP"), env, ctx)) == R"LISP((+ 1 2))LISP");
    assert(show(eval(parse(R"LISP((quote))LISP"), env, ctx))
        == R"LISP(E:"quote: expected argument in '(quote)'")LISP");
    assert(show(eval(parse(R"LISP((quote (1) (2)))LISP"), env, ctx))
        == R"LISP(E:"quote: wrong argument '(2)', too many arguments in '(quote (1) (2))'")LISP");

    assert(show(eval(parse(R"LISP((cons 10 (quote (11))))LISP"), env, ctx)) == R"LISP((10 11))LISP");
    assert(show(eval(parse(R"LISP((cons 10 11))LISP"), env, ctx))
        == R"LISP(E:"cons: unexpected '11', expected list or nil in '11'")LISP");
    assert(show(eval(parse(R"LISP((cons))LISP"), env, ctx))
        == R"LISP(E:"cons: expected argument (1) in '(cons)'")LISP");
    assert(show(eval(parse(R"LISP((cons 10))LISP"), env, ctx))
        == R"LISP(E:"cons: expected argument (2) in '(cons 10)'")LISP");
    assert(show(eval(parse(R"LISP((cons 10 (11) (12)))LISP"), env, ctx))
        == R"LISP(E:"cons: wrong argument (3) '(12)', too many arguments in '(cons 10 (11) (12))'")LISP");

    assert(show(eval(parse(R"LISP((car (quote (11))))LISP"), env, ctx)) == R"LISP(11)LISP");
    assert(show(eval(parse(R"LISP((car (quote (11 12))))LISP"), env, ctx)) == R"LISP(11)LISP");
    assert(show(eval(parse(R"LISP((car))LISP"), env, ctx))
        == R"LISP(E:"car: expected argument (1) in '(car)'")LISP");
    assert(show(eval(parse(R"LISP((car 1 2))LISP"), env, ctx))
        == R"LISP(E:"car: wrong argument (2) '2', too many arguments in '(car 1 2)'")LISP");

    assert(show(eval(parse(R"LISP((cdr (quote (11))))LISP"), env, ctx)) == R"LISP(())LISP");
    assert(show(eval(parse(R"LISP((cdr (quote (11 12))))LISP"), env, ctx)) == R"LISP((12))LISP");
    assert(show(eval(parse(R"LISP((cdr))LISP"), env, ctx))
        == R"LISP(E:"cdr: expected argument (1) in '(cdr)'")LISP");
    assert(show(eval(parse(R"LISP((cdr 1 2))LISP"), env, ctx))
        == R"LISP(E:"cdr: wrong argument (2) '2', too many arguments in '(cdr 1 2)'")LISP");

    assert(show(eval(parse(R"LISP((typeof (+ 1 2)))LISP"), env, ctx)) == R"LISP("int")LISP");
    assert(show(eval(parse(R"LISP((typeof (quote (+ 1 2))))LISP"), env, ctx)) == R"LISP("list")LISP");

    assert(show(eval(parse(R"LISP((cond (false (eval "a")) (true (eval "b")) (true (eval "c"))))LISP"), env, ctx)) == R"LISP("b")LISP");

    assert(show(eval(parse(R"LISP((evalseq (def a 10) (set! a 11) (+ 1 2) (get a)))LISP"), env, ctx))
        == R"LISP((10 11 3 11))LISP");
    assert(show(eval(parse(R"LISP((evalseq (def f (lambda (x) (cond ((> x 0) (* x (f (- x 1)))) (true 1)))) (f 5)))LISP"), env, ctx))
        == R"LISP(((lamdba (x) (cond ((> x 0) (* x (f (- x 1)))) (true 1))) 120))LISP");
    assert(show(eval(parse(R"LISP((evalseq (def f (lambda (x y z) (+ x y z))) (f 1 (* 4 5) 10)))LISP"), env, ctx))
        == R"LISP(((lamdba (x y z) (+ x y z)) 31))LISP");

    assert(show(eval(parse(R"LISP(evalseq (def msum (macro (x y z) (+ x y z))) (macroexpand msum (+ 0 1 2) x 3))LISP"), env, ctx))
        == R"LISP(((macro (x y z) (+ x y z)) (+ (+ 0 1 2) x 3)))LISP");
  }*/



  // R E P L
  {
    std::string str;
    while (true) {
      try {
        std::cout << "lisp $ ";

        static bool init = true;
        if (init) {
          init = false;
          str = "(:load 'demo.lispam)";
        } else {
          std::getline(std::cin, str);
        }

        if (str == "") break;
        context_t ctx;
        auto l = parse(str);
        std::cout << "input: \t" << show(l) << std::endl;
        l = eval(l, env, ctx);
        std::cout << "result: \t" << show(l) << std::endl;
        std::cout << "eval_calls: \t" << ctx.eval_calls << std::endl;
        std::cout << "stream: \t" << ctx.stream.str() << std::endl;
      } catch (const std::exception& e) {
        std::cout << "exception: \t" << e.what() << std::endl;
      } catch (...) {
        std::cout << "exception" << std::endl;
      }
    }
  }

  return 0;
}

