
all:
	g++ -std=c++2a lisp_interpreter.cpp main.cpp -o interpreter -fconcepts -O0 -g0 -Wall -Wextra -pedantic

