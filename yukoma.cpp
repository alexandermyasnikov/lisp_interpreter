
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

enum : uint8_t {
  UNK,
  LABEL,
  PUSH,
  PUSHL,
  ADD,
  CALL,
  RET,
  INT,
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

  std::string hex() {
    std::stringstream ss;
    ss << "{ ";
    for (const auto b : stream) {
      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(b) << " ";
    }
    ss << "}";
    return ss.str();
  }
};



// grammar:   command*
// command:   IDENT ":" | IDENT IDENT*

struct analyzer_t {
  struct lexeme_empty_t   { };
  struct lexeme_integer_t { int         value; };
  struct lexeme_ident_t   { std::string value; };

  using lexeme_t = std::variant<
    lexeme_empty_t,
    lexeme_integer_t,
    lexeme_ident_t>;

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
      std::regex("[\\w\\d_]+"),
        [](const std::string& str) { return lexeme_ident_t{str}; }
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
        } else if (instruction == "PUSHF") {
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
          throw std::runtime_error("unknown instruction");
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

  interpreter_t(const stream_t& data) : data(data) { }

  void run() {
    DEBUG_LOGGER_TRACE_ULISP;

    // uint8_t& rip = stack[0];
    // uint8_t& rbp = stack[1];
    // rip = 100;
    // rbp = sizeof(rip) + sizeof(rbp);

    // registers:
    uint64_t rip = 100;
    uint64_t rbp = 0;
    uint64_t rsp = 0;
    // stack.write(0, &rip, sizeof(rip));
    // stack.write(0, &rbp, sizeof(rbp));

    DEBUG_LOGGER_ULISP("data: %s", data.hex().c_str());

    while (rip) {
      DEBUG_LOGGER_ULISP("stack: %s", stack.hex().c_str());
      DEBUG_LOGGER_ULISP("rip: %x %d", rip, rip);
      DEBUG_LOGGER_ULISP("rbp: %x", rbp);
      uint8_t cmd;
      // stack.read(rip, &cmd, sizeof(cmd));
      data.read(rip, &cmd, sizeof(cmd));
      DEBUG_LOGGER_ULISP("cmd: %hhx", cmd);
      rip += sizeof(cmd);

      switch(cmd) {
        case PUSH: {
          uint64_t op1;
          data.read(rip, &op1, sizeof(op1));
          rip += sizeof(op1);
          stack.write(rsp, &op1, sizeof(op1));
          rsp += sizeof(op1);
          DEBUG_LOGGER_ULISP("PUSH: %x", op1);
          break;
        }
        case ADD: {
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
        }
        default: throw std::runtime_error("unknown instruction");
      }
      // break;
    }
  }
};

static std::vector<uint8_t> test_data = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  PUSH, 0x05, 0x00, 0x00, 0x00, /*arg1*/
  PUSH, 0x02, 0x00, 0x00, 0x00, /*arg2*/
  PUSH, 0x03, 0x00, 0x00, 0x00, /*func_pointer*/
  // RET,
};

/*

stack:
  10
*/

// PUSH <value>
// APUSH <src> <value>
// RPUSH <src> <value>
// COPY <src> <dst>
// flag: value | absolute_ptr | relative_ptr

int main() {
  std::string code = R"ASM(
    LABEL main
      PUSH 5
      PUSH 2
      PUSHF sum
      RET

    LABEL sum
      PUSH 1
      INT
      RET
    )ASM";

  auto data = analyzer_t::parse(code);

  interpreter_t interpreter(data);
  interpreter.run();

  return 0;
}

