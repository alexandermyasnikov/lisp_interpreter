
#include <deque>
#include <regex>
#include <vector>
#include <iomanip>
#include <variant>
#include <iostream>

#include "debug_logger.h"

#define DEBUG_LOGGER_TRACE_ULISP          DEBUG_LOGGER("lisp ", logger_indent_ulisp_t::indent)
#define DEBUG_LOGGER_ULISP(...)           DEBUG_LOG("lisp ", logger_indent_ulisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent = 0; };

struct logger_indent_ulisp_t : logger_indent_t<logger_indent_ulisp_t> { };

enum instruction_t : uint8_t {
  UNK,
  LOAD,
  SAVE,
  LLOAD,
  LSAVE,
  ALLOC,
  FREE,
  DROP,
  JUMP,
  CALL,
  RET,
  GETSP,
  GETBP,

  FUNC,
  INTEGER,
  FUNCTION,
  VARIABLE,

  // LOAD  : A    -> M[A]
  // SAVE  : A, x ->             M[A] := x
  // LLOAD : A    -> M[BP+A]     M[SP] := M[BP+A]
  // LSAVE : A, V ->             M[BP+A] := V
  //
  // ALLOC : N -> A     A := MALLOC(N)
  // FREE  : A ->       FREE(A)
  //
  // DROP : x    ->
  //
  // JUMP : b, A ->          if b THEN PC := A
  //
  // CALL : A -> PC            M[SP] <-> PC
  // RET  : P0, ..., Pn ->     PC := RA; SP := RB
  //
  // GETBP :   -> BP     M[SP] := BP
  // GETSP :   -> SP     M[SP] := SP
};

enum sys_call_t : uint8_t {
  PRINT,
};



struct reader_t {

  void read(void*& data, size_t len) {
  }

  void save_pos(size_t len) {
  }
};



struct stream_t {
  std::vector<uint8_t> __stream;
  size_t               __pos;
  reader_t             __reader;

  stream_t() : __stream(100), __pos(0) { }
  stream_t(const stream_t& stream) = default;

  void* get_forward_pos(size_t len) {
    if (__pos + len > __stream.size())
      throw std::runtime_error("get_forward_pos: invalid pos");
    return &__stream.at(__pos);
  }

  void save_forward_pos(size_t len) {
    __pos += len;
  }

  void* get_backward_pos(size_t len) {
    if (len > __pos)
      throw std::runtime_error("get_backward_pos: invalid pos");
    return &__stream.at(__pos);
  }

  void save_backward_pos(size_t len) {
    __pos -= len;
  }

  std::string hex(size_t a = std::string::npos, size_t b = std::string::npos) {
    if (a == std::string::npos && b == std::string::npos) {
      a = 0;
      b = __stream.size();
    }
    a = std::min(a, __stream.size());
    b = a + std::min(b, __stream.size() - a);
    std::stringstream ss;
    ss << "{ ";
    // for (const auto b : std::deque(stream.begin() + a, stream.begin() + b)) {
    for (const auto b : __stream) {
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(b) << " ";
    }
    ss << "} " << /*std::dec << (b - a) <<*/ "s";
    return ss.str();
  }
};

struct ttype_t {
  std::string name;
  size_t size;
  std::function<void()> ctor;
  std::function<void()> dtor;
};

struct type_t {
  virtual std::string name() = 0;
  virtual size_t size() = 0;
  virtual void ctor(stream_t& stream, size_t pos = -1) = 0;
  virtual void dtor(stream_t& stream, size_t pos = -1) = 0;
  virtual void read(stream_t& stream, size_t pos = -1) = 0;
  virtual void write(stream_t& stream, size_t pos = -1) = 0;
};

using type_sptr_t = std::shared_ptr<type_t>;

struct type_int_t : type_t {
  std::string name() override { return "i"; }
  size_t size() override { return 4; }
  void ctor(stream_t& stream, size_t pos = -1) override { DEBUG_LOGGER_ULISP("ctor"); }
  void dtor(stream_t& stream, size_t pos = -1) override { DEBUG_LOGGER_ULISP("ctor"); }
};


struct stream_utils_t {

  template <typename tvalue_t>
  void static write_forward_pod(stream_t& stream, const tvalue_t& src) {
    {
      size_t len = sizeof(src);
      void* dst = stream.get_forward_pos(sizeof(src));
      memcpy(dst, &src, len);
      stream.save_forward_pos(len);
    }
  }

  template <typename tvalue_t>
  void static read_forward_pod(stream_t& stream, tvalue_t& dst) {
    {
      size_t len = sizeof(dst);
      void* src = stream.get_forward_pos(sizeof(dst));
      memcpy(&dst, src, len);
      stream.save_forward_pos(len);
    }
  }

  size_t static write_varint(size_t pos, stream_t& stream, uint64_t value) {
    size_t ret{};
    if (value < 252) {
      auto val = static_cast<uint8_t>(value);
      // ret += stream.write(pos, &val, sizeof(val));
    } else {
      throw std::runtime_error("write_varint: todo");
    }
    return ret;
  }
};



struct fun_t {
  std::string name;
  ttype_t type_ret;
  std::vector<std::pair<ttype_t, std::string>> args;
  std::vector<std::pair<ttype_t, std::string>> local_vars;
};



struct analyzer_t {
  struct lexeme_empty_t   { };
  struct lexeme_integer_t { int32_t     value; };
  struct lexeme_ident_t   { std::string value; };
  struct lexeme_string_t  { std::string value; };

  using lexeme_t = std::variant<
    lexeme_empty_t,
    lexeme_integer_t,
    lexeme_ident_t,
    lexeme_string_t>;

  struct rule_t {
    std::regex   regex;
    std::function<lexeme_t (const std::string&)> get_lexeme;
  };

  using rules_t = std::vector<rule_t>;

  static inline rules_t rules = {
    {
      std::regex("\\s+|;.*?\n"),
      [](const std::string&) { return lexeme_empty_t{}; }
    }, {
      std::regex("[-+]?\\d+"),
        [](const std::string& str) { return lexeme_integer_t{std::stoi(str)}; }
    }, {
      std::regex("[\\w\\d_\\.]+"),
        [](const std::string& str) { return lexeme_ident_t{str}; }
    }, {
      std::regex("\".*?\""),
        [](const std::string& str) { return lexeme_string_t{std::string(str.begin() + 1, str.end() - 2)}; }
    }
  };

  static stream_t parse(const std::string& str) {
    DEBUG_LOGGER_TRACE_ULISP;
    std::vector<lexeme_t> lexemes;

    {
      std::string s = str;
      std::smatch m;
      while (!s.empty()) {
        for (const auto& rule : rules) {
          if (std::regex_search(s, m, rule.regex, std::regex_constants::match_continuous)) {
            lexeme_t lexeme = rule.get_lexeme(m.str());
            if (!std::get_if<lexeme_empty_t>(&lexeme))
              lexemes.push_back(lexeme);
            s = m.suffix().str();
            break;
          }
        }
        if (m.empty()) {
          DEBUG_LOGGER_ULISP("WARN: unexpected: '%s'", s.c_str());
          s.erase(s.begin());
        } else if (!m.empty() && !m.length()) {
          DEBUG_LOGGER_ULISP("WARN: empty regex");
          s.erase(s.begin());
        }
      }
    }

    stream_t data;
    std::map<std::string, uint32_t> labels;
    std::shared_ptr<fun_t> fun_curr;

    std::vector<ttype_t> types = {
      {
        "int",
        4,
        []() { },
        []() { },
      },
    };

    {
      size_t pos = 0;
      size_t i = 0;

      while (i < lexemes.size()) {
        std::string lexeme_curr;
        lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        if (lexeme_curr!= "FUNC") throw std::runtime_error("expected FUNC");

        const auto& fname = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        // const auto& ret_type = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        DEBUG_LOGGER_ULISP("FUNC %s", fname.c_str());

        lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i++)).value;

        DEBUG_LOGGER_ULISP("ARG");

        while (true) {
          if (std::get<lexeme_ident_t>(lexemes.at(i)).value == "VAR") break;

          const auto& arg  = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          const auto& type = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          DEBUG_LOGGER_ULISP("  %s %s", arg.c_str(), type.c_str());
        }

        lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i++)).value;

        DEBUG_LOGGER_ULISP("VAR");

        while (true) {
          if (std::get<lexeme_ident_t>(lexemes.at(i)).value == "BEGIN") break;

          const auto& arg  = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          const auto& type = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          DEBUG_LOGGER_ULISP("  %s %s", arg.c_str(), type.c_str());
        }

        lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        DEBUG_LOGGER_ULISP("BEGIN");

        while (lexeme_curr != "END") {
          const auto& instruction = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          if (instruction == "INTEGER") {
            const auto& value = std::get<lexeme_integer_t>(lexemes.at(i++)).value;
            stream_utils_t::write_forward_pod(data, static_cast<uint8_t>(INTEGER));
            stream_utils_t::write_forward_pod(data, static_cast<int32_t>(value));
            DEBUG_LOGGER_ULISP("  INTEGER %d", value);
          } else if (instruction == "FUNCTION") {
            const auto& value = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
            DEBUG_LOGGER_ULISP("  FUNCTION %s", value.c_str());
          } else if (instruction == "VARIABLE") {
            const auto& value = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
            DEBUG_LOGGER_ULISP("  VARIABLE %s", value.c_str());
          } else if (instruction == "CALL") {
            DEBUG_LOGGER_ULISP("  CALL");
          } else if (instruction == "ADDI") {
            DEBUG_LOGGER_ULISP("  ADDI");
          } else {
            throw std::runtime_error("unknown instruction: " + instruction);
          }
          lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i)).value.c_str();
        }

        lexeme_curr = std::get<lexeme_ident_t>(lexemes.at(i++)).value.c_str();
        if (lexeme_curr!= "END") throw std::runtime_error("expected END");
        DEBUG_LOGGER_ULISP("END");
        DEBUG_LOGGER_ULISP("");

      }
    }

    {
      /*for (const auto& [label, pos] : idents) {
        auto val = static_cast<uint32_t>(labels[label]);
        data.write(pos, &val, sizeof(val));
      }*/
    }

    return data;
  }
};

struct interpreter_t {
  stream_t data;
  stream_t stack;

  stream_t in;
  stream_t out;

  interpreter_t(const stream_t& data) : data(data) { }

  void run() {
    DEBUG_LOGGER_TRACE_ULISP;

    // registers:
    uint32_t ip = 0;
    uint32_t bp = 0;
    uint32_t sp = 0;
    bool run = true;

    bp = sp;

    DEBUG_LOGGER_ULISP("data: %s", data.hex().c_str());

    data.__pos = {}; // XXX

    while (run) {
      DEBUG_LOGGER_ULISP("");
      DEBUG_LOGGER_ULISP("data: ... %s ...", data.hex(ip, ip + 100).c_str());
      DEBUG_LOGGER_ULISP("stack: %s ...", stack.hex(0, sp).c_str());
      DEBUG_LOGGER_ULISP("ip: %x", ip);
      DEBUG_LOGGER_ULISP("bp: %x", bp);
      DEBUG_LOGGER_ULISP("sp: %x", sp);

      uint8_t cmd;
      stream_utils_t::read_forward_pod(data, cmd);
      DEBUG_LOGGER_ULISP("cmd: %hhx", cmd);
      ip += sizeof(cmd);

      switch(cmd) {
        case SAVE: {
          DEBUG_LOGGER_ULISP("SAVE:");
          uint32_t op1{};
          uint32_t op2{};
          // sp -= stack.read(sp - sizeof(op1), &op1, sizeof(op1));
          // sp -= stack.read(sp - sizeof(op2), &op2, sizeof(op2));
          DEBUG_LOGGER_ULISP("  op1: %x", op1);
          DEBUG_LOGGER_ULISP("  op2: %x", op2);
          // SAVE  : A, x ->             M[A] := x
          break;
        }
        case INTEGER: {
          DEBUG_LOGGER_ULISP("INTEGER:");
          int32_t op1{};
          stream_utils_t::read_forward_pod(data, op1);
          stream_utils_t::write_forward_pod(stack, static_cast<int32_t>(op1));
          stream_utils_t::write_forward_pod(stack, static_cast<uint8_t>(INTEGER));
          DEBUG_LOGGER_ULISP("  op1: %d   %x", op1, op1);
          break;
        }
        case RET: {
          DEBUG_LOGGER_ULISP("RET:");
          if (!bp) run = false;
          // uint32_t op1{};
          // ip += data.read(ip, &op1, sizeof(op1));
          // sp += stack.write(sp, &op1, sizeof(op1));
          // DEBUG_LOGGER_ULISP("  op1: %x", op1);
          break;
        }
        default: throw std::runtime_error("unknown instruction: " + std::to_string(cmd));
      }
    }
  }
};


/*

; int
; double
; char
; pointer ...
; struct ...

; type person_t struct
;   id: int
;   age: int
; end
; 
; type pint_t pointer int
; 
; type array_t struct
;   data: pint_t
;   size: int
; end
; 
; type string_t struct
;   data :pint_t
;   size :int
;   cap  :int
; end

function sum :int
arg
  x :int
  y :int
var
  ret :int
begin
  LLOAD x
  LLOAD y
  ADDI
end

*/

// person.name -> &person + person_offset(name)

int main() {
  std::string code = R"ASM(
    FUNC main
    ARG
    VAR
    BEGIN
      INTEGER 10
      INTEGER 12
      FUNCTION sum
      CALL
      INTEGER 14
      FUNCTION sum
      CALL
    END

    FUNC sum
    ARG
      x int
      y int
    VAR
      ret int
    BEGIN
      VARIABLE x
      VARIABLE y
      ADDI
    END
    )ASM";

  auto data = analyzer_t::parse(code);

  interpreter_t interpreter(data);
  interpreter.run();

  std::cout << "in:  " << interpreter.in.hex() << std::endl;
  std::cout << "out: " << interpreter.out.hex() << std::endl;

  return 0;
}

