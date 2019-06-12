#!/usr/bin/expect -f
set timeout -1
spawn ./a.out

# expect "lisp $ "
# send -- "(1 2 3)\n"

# expect "lisp $ "
# send -- "(+ 1 2)\n"

# expect "lisp $ "
# send -- "((+ 1 2))\n"

expect "lisp $ "
send -- "(def a 10) \n"

expect "lisp $ "
send -- "(get a) \n"
send -- "(evalseq (def a 10) (set! a 11) (+ 1 2) (get a)) \n"

expect "lisp $ "
send -- "(evalseq (def f (lambda (x) (cond ((> x 0) (* x (call f (- x 1)))) (true 1)))) (call f 5)) \n"

expect "lisp $ "
send -- "(evalseq (def f (lambda (x y z) (+ x y z))) (call f 1 (* 4 5) 10)) \n"

send -- "\n"
expect eof
