
#include <iostream>
#include <variant>
#include <vector>

#include "debug_logger.h"



#define DEBUG_LOGGER_TRACE_LISP          // DEBUG_LOGGER("lisp ", logger_indent_lisp_t::indent)
#define DEBUG_LOGGER_LISP(...)           // DEBUG_LOG("lisp ", logger_indent_lisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent = 0; };

struct logger_indent_lisp_t   : logger_indent_t<logger_indent_lisp_t> { };



using namespace std::string_literals;

namespace lisp_interpreter {

}

