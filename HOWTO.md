
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
  program       : stmt*
  stmt          : BOOL | INT | DOUBLE | STRING | IDENT | list
  list          : LP stmt* RP


grammar:
  program       : def_stmt*
  def_stmt      : LP __def IDENT lambda_stmt RP
  lambda_stmt:  : LP __lambda LP IDENT* RP LP lambda_body* RP RP
  lambda_body   : def_stmt | expr
  expr          : atom | fun_stmt | if_stmt | lambda_stmt
  fun_stmt      : LP IDENT expr* RP
  if_stmt       : LP __if expr expr expr RP
  atom          : BOOL | INT | DOUBLE | STRING | IDENT


(def fib (lambda (x)
  ((def fib (lambda (a b x) (if (greater? x 0) (fib b (+ a b) (- x 1)) (b))))
  (fib 1 1 x))))


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



(__def fib (__lambda (x)
  ((__def fib (__lambda (a b x) (if (__greater? x 0) (fib b (+ a b) (- x 1)) (b))))
  (fib 1 1 x))))

fib :
  call:
    fun: fib::fib
    args: 1 1 $0

fib::fib :
  if_condition:
    call:
      fun: greater?
      args:
        expr:
          $2
        expr:
          0
  then_body:
    call:
      fun: fib::fib
      args:
        expr:
          $1
        expr:
          call:
            fun: +
            args:
              expr:
                $0
              expr:
                $1
        expr:
          call:
            fun: -
            args:
              expr:
                $2
              expr:
                1
  else_body:
    expr:
      $1


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


