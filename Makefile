
all:
	g++ -std=c++2a lisp_interpreter.cpp main.cpp -o interpreter -fconcepts -O3 -g3 -Wall -Wextra -pedantic

