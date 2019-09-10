
// TODO
// def -> deffun
// (let ... in ...)
// static typification
// memoization
// optimization: inline, tail_recursion

Все константы и функции вынесены.
Аргументы функции - указатели на данные.


( def
  filter
  ( lambda
    ( f l )
    ( ( def $1 (lambda () ( iter f l ( ) ) ) )
      ( def $0 (lambda () ( reverse $1 ) ) )
      ( def
        iter
        ( lambda                                                 ; + context
          ( f l res )
          ( ( def $11 (lambda () ( id res ) ) )                  ; context.idents[11]
            ( def $10 (lambda () ( id l ) ) )                    ; context.idents[10]
            ( def $9  (lambda () ( id f ) ) )                    ; context.idents[9]
            ( def $8  (lambda () ( head $10 ) ) )                ; context.idents[8]
            ( def $7  (lambda () ( cons $8 $11 ) ) )             ; context.idents[7]
            ( def $6  (lambda () ( head $10 ) ) )                ; context.idents[6]
            ( def $5  (lambda () ( $9 $6 ) ) )                   ; context.idents[5]
            ( def $4  (lambda () ( if $5 $7 $11 ) ) )            ; context.idents[4]
            ( def $3  (lambda () ( tail $10 ) ) )                ; context.idents[3]
            ( def $2  (lambda () ( iter $9 $3 $4 ) ) )           ; context.idents[2]
            ( def $1  (lambda () ( nil? $10 ) ) )                ; context.idents[1]
            ( def $0  (lambda () ( if $1 $11 $2 ) ) )            ; context.idents[0]
            $0 ) ) )                                             ; rax
      $0 ) ) )


grammar simple:
  program         : stmt*
  stmt            : BOOL | INT | DOUBLE | STRING | IDENT | list
  list            : LP stmt* RP


grammar:
  program_stmt    : def_stmt*
  def_stmt        : LP __def ident lambda_stmt RP
  lambda_stmt     : LP __lambda LP ident* RP LP body_stmt* RP RP
  body_stmt       : def_stmt | expr_stmt
  expr_stmt       : if_stmt | lambda_stmt | call_stmt | atom
  call_stmt       : LP fun_ident_stmt expr_stmt* RP
  fun_ident_stmt  : ident | lambda_stmt
  if_stmt         : LP __if expr_stmt expr_stmt expr_stmt RP
  atom            : ident | const_value
  ident           : IDENT
  const_value     : BOOL | INT | DOUBLE | STRING


(__def fib (__lambda (x)
  ((__def fib_inner (__lambda (a b x) ((__if (__greater? x 0) (fib_inner b (+ a b) (- x 1)) b))))
  (fib_inner 1 1 x))))


( def
  filter
  ( lambda
    ( f l )
    ( ( def
        iter
        ( lambda
          ( f l res )
          ( ( if
              ( nil? l )
              res
              ( iter
                f
                ( tail l )
                ( if
                  ( f
                    ( head l ) )
                  ( cons
                    ( head l )
                    res )
                  res ) ) ) ) ) )
      ( reverse
        ( iter
          f
          l
          ( ) ) ) ) ) )

stack:
|---------|
| rbp old |
|---------|
| rsp     |
|---------|


|--------------|---------|----------------------|----------------------------|
| label        | pointer | command              | comment                    |
|--------------|---------|----------------------|----------------------------|
| filter       | 0       |                      |                            |
|              |         | ...                  |                            |
|--------------|---------|----------------------|----------------------------|
| filter::iter | 100     | push rbp             |                            |
|              |         | mov  rbp rsp         |                            |
|              |         | ...                  |                            |
|              |         | ret                  |                            |
|--------------|---------|----------------------|----------------------------|
| plus         | 200     | push rbp             |                            |
|              |         | mov  rbp rsp         |                            |
|              |         | add  rsp 3*S         | Память под ret, agr1, arg0 |
|              |         | mov  rbp+1*S rbp-3*S |                            |
|              |         | mov  rbp+2*S rbp-2*S |                            |
|              |         | syscall 1            | result in rax              |
|              |         | ret                  |                            |
|--------------|---------|----------------------|----------------------------|

$1:
  push rbp
  mov  rbp rsp
  sub  rsp 0
    add $2 $3   -> rax
  mov  rsp rbp
  pop  rbp
  ret
$2:
  ...




registers?:
flags? - for test - branch
pc     - program counter
exp    - current expression
val    - result
env    - current env
unev   - args in lambda
arg1-8 - eval(args) in lambda
cont?  - label
proc?  - current lambda name

state_t:
  exp:    object_t
  proc:   name
  args:   mutable, internal, std::vector <object_t, env_t> -> <object_t, nullptr>
  idents: mutable, external, std::map <name, object_t, env_t> -> <name, object_t, nullptr>
  val:    object_t

evaluator_t:
  exp:    object_t
  proc:   std::string
  args:   std::vector<evaluator_t, env_t>
  val:    object_t

lambda, macro:
args: $0, $1, ..., count
idents: { object_t, env_t }
body: object_t($0, $1, ..., idents...)

ident:
  name:  std::string
  ptr:   uint64_t

object_t:
  variant:
    nil
    bool
    uint64_t
    double
    list
    ident
    lambda
    macro

env_t -> stack.size

parse: string -> { object_t }
prepare: (lambda ...) -> { object_t }
prepare: (macro ...) -> { object_t }
prepare: args: x, y, ... -> args: $0, $1, ...
eval: { object_t } -> value


(def sum (lambda (x y) (+ x y)))
(+ (sum 1 2) 3)

(def sum (macro ($0 $1) (+ $0 $1)))
(def sum2 (macro ($0) (sum 10 $0)))



stack:
  std::deque<std::variant<uint64_t, std::shared_ptr<object_t>>>

registers:
  rip - instruction pointer
  rbp - base pointer
  rsp - stack pointer
  rax - result

symbol_table:
  std::map<std::string, pointer>
  mangling names

instructions:
  push   <val>
  pop    <val>
  mov    <dst> <src>
  ret
  jump   <ptr>
  eval   <val>
  param  <val>
  call   <ptr>
  ...
  iplus         <int> <int>   +d
  iminus        <int> <int>   +d
  imultuplies   <int> <int>   +d
  idivides      <int> <int>   +d
  iless         <int> <int>   +d
  iequal        <int> <int>   +d
  print         <val>
  // if         <bool> <val> <val>
  cons          <val> <list>
  head          <list>
  tail          <list>
  is_int        <val>
  is_double     <val>
  is_list       <val>
  is_string     <val>
  fcall         <string>
  def           <string> <val>
  try           <val> <val>
  ...

segments:
  stack
  heap
  text
  dynsym
  comments

instructions example:
  myfun:
    ...
    push ip
    push 2   -> args_count
    push 1
    push 2
    call <sum>   -> push ip?
    ...

  sum:
    push rbp
    mov  rbp rsp
    sub  rsp <localsize>
    ...
    mov  rsp rbp
    pop  rbp
    ret

library example:
  (export
    filter
    foldl
    foldr
    fib)

interpreter example:
  (import standart.lispam standart)
  (import mylib.lispam mylib)
  (print (fib 10))


new 07.24:
  lisp types:
    bool
    int
    double
    string
    list
    lambda
    macro
    ident

  object_t:
    // static_block  { data }
    // dynamic_block { std::shared_ptr<void*> } -> { data }
    std::shared_ptr<obj_t>

  stack_t:
    // std::deque<uint64_t>
    std::deque<object_t>

  instruction_t:
    push   <val>
    pop    <val>
    mov    <dst> <src>
    ret
    jump   <ptr>
    call   <ptr>
    int    <num>   // free malloc print fcall? def

    add
    sub
    mul
    div
    cmp
    dcmp
    ...

shared_ptr<T>:
  T* value
  size_t* count


gen code:
  -> quadruple (op, arg1, arg2, result)

function:
  * decompose args
  * args is array lambda
  * body is array [lambda] + return?
  * context
    * lambda create context
    * syscall using current context
  (fib (+ 1 (+ 2 3)))
    -> (def $1 (+ 1 (+ 2 3))) (fib $1)
    -> (def $2 (+ 2 3)) (def $1 (+ 1 $2)) (fib $1)
  (fib (+ 1 (+ 2 3)))
    -> $1 := () -> (+ 2 3)
    -> $2 := () -> (+ 1 $1)
    -> fib $2

standart.lispam:
  [ functions ]


new 08.16:
  program:
    [ block ]

  block:
    parent block
    (params)
    [ commands ]

  ident:
    name: string
    type
    value

  type:
    [ attribute ]
    [ value ]
    [ operation ]
  ex: number, double, bool, char, string, pointer, enum, struct, array, vector, union, set, list, map

  // Отделить описание типа от данных для добавления пользовательских

new 08.31:
  type:
    uint64_t   sizeof(type) == 8
    double     sizeof(type) == 8
    все другие строются поверх uint64_t, в том числе reinterpret_cast

  operation:
    add    : int
    sub    : int
    mult   : int
    dev    : int
    dereference operator
    reference
    call
    ret
    cp
    jmp


(__def fib (__lambda (x)
  ((__def fib_inner (__lambda (a b x) ((__if (__greater? x 0) (fib_inner b (+ a b) (- x 1)) b))))
  (fib_inner 1 1 x))))

  program_stmt_t: {
    def_stmt_t: {
      ident_t: {
        name:    fib
        pointer: 0
        this:    94683237526432
      }
      lambda_stmt_t: {
        ident_t: {
          name:    x
          pointer: 0
          this:    94683237528944
        }
        body_stmt_t: {
          def_stmt_t: {
            ident_t: {
              name:    fib_inner
              pointer: 0
              this:    94683237529008
            }
            lambda_stmt_t: {
              ident_t: {
                name:    a
                pointer: 0
                this:    94683237529072
              }
              ident_t: {
                name:    b
                pointer: 0
                this:    94683237529136
              }
              ident_t: {
                name:    x
                pointer: 0
                this:    94683237529200
              }
              body_stmt_t: {
                expr_stmt_t: {
                  if_stmt_t: {
                    expr_stmt_t: {
                      call_stmt_t: {
                        fun_ident_stmt_t: {
                          ident_t: {
                            name:    __greater?
                            pointer: 0
                            this:    94683237529488
                          }
                        }
                        expr_stmt_t: {
                          atom_stmt_t: {
                            ident_t: {
                              name:    x
                              pointer: 0
                              this:    94683237529712
                            }
                          }
                        }
                        expr_stmt_t: {
                          atom_stmt_t: {
                            const_value_t: {
                              int64_t: 0
                            }
                          }
                        }
                      }
                    }
                    expr_stmt_t: {
                      call_stmt_t: {
                        fun_ident_stmt_t: {
                          ident_t: {
                            name:    fib_inner
                            pointer: 0
                            this:    94683237530048
                          }
                        }
                        expr_stmt_t: {
                          atom_stmt_t: {
                            ident_t: {
                              name:    b
                              pointer: 0
                              this:    94683237530160
                            }
                          }
                        }
                        expr_stmt_t: {
                          call_stmt_t: {
                            fun_ident_stmt_t: {
                              ident_t: {
                                name:    +
                                pointer: 0
                                this:    94683237530384
                              }
                            }
                            expr_stmt_t: {
                              atom_stmt_t: {
                                ident_t: {
                                  name:    a
                                  pointer: 0
                                  this:    94683237530496
                                }
                              }
                            }
                            expr_stmt_t: {
                              atom_stmt_t: {
                                ident_t: {
                                  name:    b
                                  pointer: 0
                                  this:    94683237530688
                                }
                              }
                            }
                          }
                        }
                        expr_stmt_t: {
                          call_stmt_t: {
                            fun_ident_stmt_t: {
                              ident_t: {
                                name:    -
                                pointer: 0
                                this:    94683237531056
                              }
                            }
                            expr_stmt_t: {
                              atom_stmt_t: {
                                ident_t: {
                                  name:    x
                                  pointer: 0
                                  this:    94683237531168
                                }
                              }
                            }
                            expr_stmt_t: {
                              atom_stmt_t: {
                                const_value_t: {
                                  int64_t: 1
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                    expr_stmt_t: {
                      atom_stmt_t: {
                        ident_t: {
                          name:    b
                          pointer: 0
                          this:    94683237531664
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        body_stmt_t: {
          expr_stmt_t: {
            call_stmt_t: {
              fun_ident_stmt_t: {
                ident_t: {
                  name:    fib_inner
                  pointer: 0
                  this:    94683237532176
                }
              }
              expr_stmt_t: {
                atom_stmt_t: {
                  const_value_t: {
                    int64_t: 1
                  }
                }
              }
              expr_stmt_t: {
                atom_stmt_t: {
                  const_value_t: {
                    int64_t: 1
                  }
                }
              }
              expr_stmt_t: {
                atom_stmt_t: {
                  ident_t: {
                    name:    x
                    pointer: 0
                    this:    94683237532688
                  }
                }
              }
            }
          }
        }
      }
    }
  }


defun:
  ptr: push rbp
       mov  rbp rsp
       ...
       ret


void fibr( x ) {
  $1 = greater?(x, 0);
  $2 = if ($1) {
    $4 = x - 2;
    $3 = x - 1;
    $2 = fibr($3);
    $1 = fibr($4);
    plus($2, $1);
  } else {
    1
  }
  return $2;
}


sum: \ x y z -> x + y + z
max: \ x y   -> if x > y then x else y
f:   \ x     -> sum(1 max(x 3) 4)
main: \ -> f(5)

main:
  push 5
  push 1
  push <f>
  call
  ret

f:
  push 1
  push x
  push 3
  push 2
  push <max>
  call
  push 4
  call sum
  ret

max:
  push x
  push y
  >
  M1
  BF
  x
  M2
  BRL
  y    :M1
  ret  :M2

sum:
  push x
  push y
  +
  push z
  +
  ret

stack:
  10



sum:  \ x y z -> x + y + z
max:  \ x y   -> if x > y then x else y
f:    \ x     -> sum(1 max(x 3) 4)
main: \ -> f(5)






BF:
    add    : int
    sub    : int
    mult   : int
    dev    : int
    dadd    : double
    dsub    : double
    dmult   : double
    ddev    : double
    dereference operator
    reference
    call    : push <rip>   cp xxx <rip>
    ret     :
    cp
    jmp





