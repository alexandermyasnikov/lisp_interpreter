
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
  LABEL,
  STRING,
  PUSH,
  PUSH1,
  PUSHL,
  ADD,
  CALL,
  RET,
  INT,
};

enum sys_call_t : uint8_t {
  PRINT,
};



struct stream_t {
  std::deque<uint8_t> stream;

  void write(size_t pos, const void* data, size_t len) {
    if (stream.size() < pos + len) {
      stream.resize(pos + len);
    }
    std::copy(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + len, stream.begin() + pos);
  }

  void read(size_t pos, void* data, size_t len) {
    if (stream.size() < pos + len) {
      throw std::runtime_error("could not read");
    }
    std::copy(stream.begin() + pos, stream.begin() + pos + len, static_cast<uint8_t*>(data));
  }

  void resize(size_t size) {
    stream.resize(size);
  }

  size_t size() {
    return stream.size();
  }

  std::string hex() {
    std::stringstream ss;
    ss << "{ ";
    for (const auto b : stream) {
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(b) << " ";
    }
    ss << "} " << std::dec << stream.size() << "s";
    return ss.str();
  }
};



// grammar:   command*
// command:   IDENT ":" | IDENT IDENT*

struct analyzer_t {
  struct lexeme_empty_t   { };
  struct lexeme_integer_t { int         value; };
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
    std::vector<std::pair<std::string, uint32_t>> idents;

    {
      size_t pos = 0;
      for (size_t i{}; i < lexemes.size(); ++i) {
        const auto& instruction = std::get<lexeme_ident_t>(lexemes.at(i)).value;
        if (instruction == "LABEL") {
          const auto& label = std::get<lexeme_ident_t>(lexemes.at(++i)).value;
          DEBUG_LOGGER_ULISP("LABEL %s", label.c_str());
          labels[label] = pos;
        } else if (instruction == ".string") {
          const auto& value = std::get<lexeme_string_t>(lexemes.at(++i)).value;
          DEBUG_LOGGER_ULISP(".string \"%s\"", value.c_str());
          {
            data.write(pos, value.data(), value.size() + 1);
            pos += sizeof(value.size() + 1);
          }
        } else if (instruction == "PUSH") {
          const auto& value = std::get<lexeme_integer_t>(lexemes.at(++i)).value;
          DEBUG_LOGGER_ULISP("PUSH %d", value);
          {
            auto cmd = static_cast<uint8_t>(PUSH);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
            auto val = static_cast<uint32_t>(value);
            data.write(pos, &val, sizeof(val));
            pos += sizeof(val);
          }
        } else if (instruction == "PUSH1") {
          const auto& value = std::get<lexeme_integer_t>(lexemes.at(++i)).value;
          DEBUG_LOGGER_ULISP("PUSH1 %d", value);
          {
            auto cmd = static_cast<uint8_t>(PUSH1);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
            auto val = static_cast<uint8_t>(value);
            data.write(pos, &val, sizeof(val));
            pos += sizeof(val);
          }
        } else if (instruction == "PUSHL") {
          const auto& value = std::get<lexeme_ident_t>(lexemes.at(++i)).value;
          DEBUG_LOGGER_ULISP("PUSHL %s", value.c_str());
          {
            auto cmd = static_cast<uint8_t>(PUSHL);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
            idents.push_back({value, pos});
            auto val = static_cast<uint32_t>(0);
            data.write(pos, &val, sizeof(val));
            pos += sizeof(val);
          }
        } else if (instruction == "INT") {
          DEBUG_LOGGER_ULISP("INT");
          {
            auto cmd = static_cast<uint8_t>(INT);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
          }
        } else if (instruction == "CALL") {
          DEBUG_LOGGER_ULISP("CALL");
          {
            auto cmd = static_cast<uint8_t>(CALL);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
          }
        } else if (instruction == "RET") {
          DEBUG_LOGGER_ULISP("RET");
          {
            auto cmd = static_cast<uint8_t>(RET);
            data.write(pos, &cmd, sizeof(cmd));
            pos += sizeof(cmd);
          }
        } else {
          throw std::runtime_error("unknown instruction 2");
        }
      }
    }

    {
      for (const auto& [label, pos] : idents) {
        auto val = static_cast<uint32_t>(labels[label]);
        data.write(pos, &val, sizeof(val));
      }
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

    stack.write(sp, &ip, sizeof(ip));
    sp += sizeof(ip);
    stack.write(sp, &bp, sizeof(bp));
    sp += sizeof(bp);
    bp = sp;

    DEBUG_LOGGER_ULISP("data: %s", data.hex().c_str());

    while (true) {
      DEBUG_LOGGER_ULISP("");
      DEBUG_LOGGER_ULISP("stack: %s", stack.hex().c_str());
      DEBUG_LOGGER_ULISP("ip: %x", ip);
      DEBUG_LOGGER_ULISP("bp: %x", bp);
      DEBUG_LOGGER_ULISP("sp: %x", sp);

      uint8_t cmd;
      data.read(ip, &cmd, sizeof(cmd));
      DEBUG_LOGGER_ULISP("cmd: %hhx", cmd);
      ip += sizeof(cmd);

      switch(cmd) {
        case PUSHL:
        case PUSH: {
          uint32_t op1;
          data.read(ip, &op1, sizeof(op1));
          ip += sizeof(op1);
          stack.write(sp, &op1, sizeof(op1));
          sp += sizeof(op1);
          DEBUG_LOGGER_ULISP("PUSHX: %x", op1);
          break;
        }
        case PUSH1: {
          uint8_t op1;
          data.read(ip, &op1, sizeof(op1));
          ip += sizeof(op1);
          stack.write(sp, &op1, sizeof(op1));
          sp += sizeof(op1);
          DEBUG_LOGGER_ULISP("PUSH1: %hhx", op1);
          break;
        }
        case CALL: {
          uint32_t op1;
          stack.read(sp - sizeof(op1), &op1, sizeof(op1));

          stack.write(sp, &ip, sizeof(ip));
          sp += sizeof(ip);
          ip = op1;

          stack.write(sp, &bp, sizeof(bp));
          sp += sizeof(bp);
          bp = sp;
          DEBUG_LOGGER_ULISP("CALL");
          break;
        }
        case RET: {
          uint32_t ip_orig;
          uint32_t bp_orig;
          stack.resize(bp);
          sp = bp;
          stack.read(sp - sizeof(bp_orig), &bp_orig, sizeof(bp_orig));
          sp -= sizeof(bp_orig);
          stack.read(sp - sizeof(ip_orig), &ip_orig, sizeof(ip_orig));
          sp -= sizeof(ip_orig);
          stack.resize(sp);
          ip = ip_orig;
          bp = bp_orig;
          DEBUG_LOGGER_ULISP("RET");
          break;
        }
        case INT: {
          uint32_t op1;
          stack.read(sp - sizeof(op1), &op1, sizeof(op1));
          sp -= sizeof(op1);

          switch (op1) {
            case PRINT: {
              uint8_t op1;
              stack.read(sp - sizeof(op1), &op1, sizeof(op1));
              sp -= sizeof(op1);
              out.write(out.size(), &op1, sizeof(op1));
              break;
            }
            default : {
              break;
            }
          }
          DEBUG_LOGGER_ULISP("INT");
          break;
        }
        /*case ADD: {
          uint64_t op1;
          uint64_t op2;
          stack.read(rsp - sizeof(op2), &op2, sizeof(op2));
          rsp -= sizeof(op2);
          stack.read(rsp - sizeof(op1), &op1, sizeof(op1));
          rsp -= sizeof(op1);
          DEBUG_LOGGER_ULISP("ADD: %x %x", op1, op2);
          uint64_t ret = op1 + op2;
          stack.write(rsp, &ret, sizeof(ret));
          break;
        }*/
        default: throw std::runtime_error("unknown instruction 1");
      }

      if (!ip) break;
    }
  }
};



int main() {
  std::string code = R"ASM(
    LABEL main
      PUSH 5
      PUSH 2
      PUSHL sum
      CALL
      PUSHL test_print
      CALL
      RET

    LABEL hello_str
      .string "Hello, World!"

    LABEL mul
      PUSH 21
      INT
      RET

    LABEL sum
      PUSH 20
      INT
      RET

    LABEL test_print
      PUSH1 72              ; 'H'
      PUSHL __print_char
      CALL
      PUSH1 105             ; 'i'
      PUSHL __print_char
      CALL
      PUSH1 33              ; '!'
      PUSHL __print_char
      CALL
      RET

    LABEL __print_char
      PUSH 0                ; syscall PRINT
      INT
      RET

    )ASM";

  auto data = analyzer_t::parse(code);

  interpreter_t interpreter(data);
  interpreter.run();

  std::cout << "in:  " << interpreter.in.hex() << std::endl;
  std::cout << "out: " << interpreter.out.hex() << std::endl;

  return 0;
}

