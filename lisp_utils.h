
#include <iostream>
#include <list>
#include <regex>

#define DEBUG_LOGGER_TRACE_ULISP          DEBUG_LOGGER("lisp ", logger_indent_ulisp_t::indent)
#define DEBUG_LOGGER_ULISP(...)           DEBUG_LOG("lisp ", logger_indent_ulisp_t::indent, __VA_ARGS__)

struct logger_indent_ulisp_t   : logger_indent_t<logger_indent_ulisp_t> { };

namespace lisp_utils {

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;



  struct object_t { };

  using register_t = uint64_t;
  using stackv_t = uint64_t;

  using stack_t = std::deque<stackv_t>;

  using text_t = std::deque<uint8_t>;

  // static_block  { data }
  // dynamic_block { std::shared_ptr<void*> } -> { data }

  struct state_t {
    register_t rip; 
    register_t rbp; 
    register_t rsp; 
    register_t rax; 
  };

  struct insruction_t {
    static inline uint8_t push = 0x01;
    static inline uint8_t pop  = 0x02;
    static inline uint8_t move = 0x03;
    static inline uint8_t ret  = 0x04;
    static inline uint8_t jmp  = 0x05;
    static inline uint8_t call = 0x06;
    static inline uint8_t int0 = 0x07;
    static inline uint8_t add  = 0x08;
    static inline uint8_t sub  = 0x09;
    static inline uint8_t mul  = 0x0a;
    static inline uint8_t div  = 0x0b;
    static inline uint8_t cmp  = 0x0c;
    static inline uint8_t dcmp = 0x0d;
  };

  struct machine_t {
    text_t  text;
    stack_t stack;
    state_t state;

    void init() { }
    bool step() { return false; }
  };



  struct lisp_compiler_t {

    enum class token_t {
      EMPTY,
      LP,
      RP,
      DOUBLE,
      INTEGER,
      TRUE,
      FALSE,
      LAMBDA,
      MACRO,
      NAME,
      END,
    };

    using attribute_t = std::variant<std::string, bool, int64_t, double>;
    using tokens_t = std::list<std::pair<token_t, attribute_t>>;



    struct syntax_tree_list_t;
    using syntax_tree_list_sptr_t = std::shared_ptr<syntax_tree_list_t>;

    using syntax_tree_t = std::variant<attribute_t, syntax_tree_list_sptr_t>;

    struct syntax_tree_list_t {
      std::list<syntax_tree_t> nodes;
    };



    struct object_t {
      using type_t = std::variant<
        bool,
        int64_t,
        double
        >;
    };

    std::string show_attribute(const attribute_t& attribute) {
      std::string str;
      std::visit(overloaded {
        [&str] (const std::string& value)    { str = value; },
        [&str] (bool value)                  { str = value ? "true" : "false"; },
        [&str] (double value)                { str = std::to_string(value); },
        [&str] (int64_t value)               { str = std::to_string(value); },
        [&str] (auto)                        { str = "(unknown)"; },
      }, attribute);
      return str;
    }

    std::string show_syntax_tree(const syntax_tree_t& syntax_tree) {
      std::string str;
      std::visit(overloaded {
        [&str, this] (const attribute_t& value) {
          str = show_attribute(value);
        },
        [&str, this] (syntax_tree_list_sptr_t value) {
          str += "( ";
          for (const auto& node : value->nodes)
            str += show_syntax_tree(node) + " ";
          str += ")";
        },
        [&str] (auto) {
          str = "(unknown)";
        },
      }, syntax_tree);
      return str;
    }



    struct lexical_analyzer_t {

      struct rule_t {
        token_t      token;
        std::regex   regex;
        std::function<attribute_t (const std::string&)> get_attribute;
      };

      using rules_t = std::list<rule_t>;

      static inline rules_t rules = {
        {
          token_t::EMPTY,
          std::regex("\\s+|;.*\n"),
          [](const auto&) { return std::string{}; }
        }, {
          token_t::LP,
          std::regex("\\("),
          [](const auto&) { return std::string("("); }
        }, {
          token_t::RP,
          std::regex("\\)"),
          [](const auto&) { return std::string(")"); }
        }, {
          token_t::DOUBLE,
          std::regex("[-+]?((\\d+\\.\\d*)|(\\d*\\.\\d+))"),
          [](const auto& str) { return std::stod(str); }
        }, {
          token_t::INTEGER,
          std::regex("[-+]?\\d+"),
          [](const auto& str) { return std::stol(str); }
        }, {
          token_t::TRUE,
          std::regex("true", std::regex_constants::icase),
          [](const auto&) { return true; }
        }, {
          token_t::FALSE,
          std::regex("false", std::regex_constants::icase),
          [](const auto&) { return false; }
        }, {
          token_t::LAMBDA,
          std::regex("lambda", std::regex_constants::icase),
          [](const auto&) { return std::string("lambda"); }
        }, {
          token_t::MACRO,
          std::regex("macro", std::regex_constants::icase),
          [](const auto&) { return std::string("macro"); }
        }, {
          token_t::NAME,
          std::regex("[\\w!#$%&*+-./:<=>?@_]+"),
          [](const auto& str) { return str; }
        }
      };

      tokens_t parse(const std::string& str) {
        DEBUG_LOGGER_TRACE_ULISP;
        tokens_t tokens;
        std::string s = str;
        std::smatch m;
        while (!s.empty()) {
          for (const auto& rule : rules) {
            if (std::regex_search(s, m, rule.regex, std::regex_constants::match_continuous)) {
              if (rule.token != token_t::EMPTY)
                tokens.push_back({rule.token, rule.get_attribute(m.str())});
              DEBUG_LOGGER_ULISP("found: '%s'", m.str().c_str());
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
        tokens.push_back({token_t::END, std::string{}});
        return tokens;
      }
    };

    struct syntax_analyzer {

      syntax_tree_t parse(const tokens_t& tokens) {
        DEBUG_LOGGER_TRACE_ULISP;
        tokens_t t = tokens;
        syntax_tree_t syntax_tree = parse_expr(t);
        match(t, token_t::END);
        return syntax_tree;
      }

      void match(tokens_t& tokens, token_t token) {
        DEBUG_LOGGER_ULISP("match : %d", (int) tokens.front().first);
        DEBUG_LOGGER_TRACE_ULISP;
        if (tokens.front().first != token)
          throw std::runtime_error("match: unexpected token: " + std::to_string((int) tokens.front().first) + ", expected: " + std::to_string((int) token));
        tokens.pop_front();
      }

      syntax_tree_t parse_expr(tokens_t& tokens) {
        DEBUG_LOGGER_TRACE_ULISP;
        if (tokens.empty()) throw std::runtime_error("parse_expr: empty tokens");
        syntax_tree_t syntax_tree;
        auto kv = tokens.front();
        switch (kv.first) {
          case token_t::DOUBLE:
          case token_t::INTEGER:
          case token_t::TRUE:
          case token_t::LAMBDA:
          case token_t::MACRO:
          case token_t::NAME: {
            match(tokens, kv.first);
            syntax_tree = kv.second;
            break;
          }
          case token_t::LP: {
            match(tokens, token_t::LP);
            syntax_tree_list_sptr_t syntax_tree_list = std::make_shared<syntax_tree_list_t>();
            while (tokens.front().first != token_t::RP) {
              syntax_tree_list->nodes.push_back(parse_expr(tokens));
            }
            match(tokens, token_t::RP);
            syntax_tree = syntax_tree_list;
            break;
          }
          default: throw std::runtime_error("parse_expr: unexpected token: " + std::to_string((int) tokens.front().first));
        }
        return syntax_tree;
      }
    };

    struct semantic_analyzer { };

    struct code_generator { };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = "( (def fib (lambda (x) ((def fib (lambda (a b x) (if (greater? x 0) (fib b (+ a b) (- x 1)) (b)))) (fib 1 1 x)))) (print -1 +2 +.3 -4.) ;comment 2\n (print 1) )";
      auto tokens = lexical_analyzer_t().parse(code);

      { // debug
        for (const auto& token : tokens) {
          std::string str = show_attribute(token.second);
          DEBUG_LOGGER_ULISP("token: %d : '%s'", (int) token.first, str.c_str());
        }
      }

      syntax_tree_t syntax_tree = syntax_analyzer().parse(tokens);

      { // debug
        std::string str = show_syntax_tree(syntax_tree);
        DEBUG_LOGGER_ULISP("syntax_tree: '%s'", str.c_str());
      }

    }
  };

}

