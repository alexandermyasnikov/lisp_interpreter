
#include <iostream>
#include <list>
#include <regex>

#define DEBUG_LOGGER_TRACE_ULISP          DEBUG_LOGGER("lisp ", logger_indent_ulisp_t::indent)
#define DEBUG_LOGGER_ULISP(...)           DEBUG_LOG("lisp ", logger_indent_ulisp_t::indent, __VA_ARGS__)

struct logger_indent_ulisp_t   : logger_indent_t<logger_indent_ulisp_t> { };

namespace lisp_utils {

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;



  struct object_t {
    // sizeof(bool) = 1
    // sizeof(int64_t) = 8
    // sizeof(double) = 8
    // list: <ptr, ptr>
    // lambda: ptr
    // macro: ptr?
    // string
    // struct
    // map
    // set
    size_t type  : 1 * 8,
            count : 4 * 8,
            size  : 3 * 8; // size_t*
    void* data;
  };

  using register_t = uint64_t;
  using stackv_t = uint64_t;

  using stack_t = std::deque<stackv_t>;

  using text_t = std::deque<uint8_t>;

  // static_block  { data }
  // dynamic_block { std::shared_ptr<void*> } -> { data }
  // block: <uint64_t: ptr, uint64_t: type + count>

  struct state_t {
    register_t rip;
    register_t rbp;
    register_t rsp;
    register_t rax; // object_t
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
    // stats_t stat;

    void init() { }
    bool progress() { return false; }
  };



  struct lisp_compiler_t {

    struct token_empty_t   { };
    struct token_lp_t      { };
    struct token_rp_t      { };
    struct token_bool_t    { bool        value; };
    struct token_integer_t { int64_t     value; };
    struct token_double_t  { double      value; };
    struct token_string_t  { std::string value; };
    struct token_ident_t   { std::string value; };
    struct token_list_t;

    using token_t = std::variant<
      token_empty_t,
      token_lp_t,
      token_rp_t,
      std::shared_ptr<token_bool_t>,
      std::shared_ptr<token_integer_t>,
      std::shared_ptr<token_double_t>,
      std::shared_ptr<token_string_t>,
      std::shared_ptr<token_ident_t>,
      std::shared_ptr<token_list_t>>;

    using tokens_t = std::list<token_t>;
    using ast_t = std::shared_ptr<token_t>; // abstract syntax tree

    struct token_list_t {
      std::list<std::shared_ptr<token_t>> list;
    };

    // program       : stmt*
    // stmt          : BOOL | INT | DOUBLE | STRING | IDENT | list
    // list          : LP stmt* RP

    // program       : def_stmt*
    // def_stmt      : LP __def IDENT lambda_stmt RP
    // lambda_stmt:  : LP __lambda LP IDENT* RP LP lambda_body* RP RP
    // lambda_body   : def_stmt | expr
    // if_stmt       : LP __if expr expr expr RP
    // fun_stmt      : LP IDENT expr* RP
    // expr          : atom | fun_stmt | if_stmt
    // atom          : BOOL | INT | DOUBLE | STRING | IDENT | lambda_stmt

    static std::string show_token(const token_t& token) {
      std::string str;
      std::visit(overloaded {
        [&str] (token_empty_t)                              { str = "(empty)"; },
        [&str] (token_lp_t)                                 { str = "("; },
        [&str] (token_rp_t)                                 { str = ")"; },
        [&str] (std::shared_ptr<token_bool_t>      value)   { str = value->value ? "true" : "false"; },
        [&str] (std::shared_ptr<token_integer_t>   value)   { str = std::to_string(value->value); },
        [&str] (std::shared_ptr<token_double_t>    value)   { str = std::to_string(value->value); },
        [&str] (std::shared_ptr<token_string_t>    value)   { str = "\"" + value->value + "\""; },
        [&str] (std::shared_ptr<token_ident_t>     value)   { str = value->value; },
        [&str] (std::shared_ptr<token_list_t>      value)   { str = "( "; for (const auto& t : value->list) str += show_token(*t) + " "; str += ")"; },
        [&str] (auto)                                       { str = "(unknown)"; },
      }, token);
      return str;
    }

    std::string show_ast(ast_t ast) {
      token_t token = *ast;
      std::string ret = show_token(token);
      return ret;
    }



    struct lexical_analyzer_t {

      struct rule_t {
        std::regex   regex;
        std::function<token_t (const std::string&)> get_token;
      };

      using rules_t = std::list<rule_t>;

      static inline rules_t rules = {
        {
          std::regex("\\s+|;.*?\n"),
          [](const std::string&) { return token_empty_t{}; }
        }, {
          std::regex("\\("),
          [](const std::string&) { return token_lp_t{}; }
        }, {
          std::regex("\\)"),
          [](const std::string&) { return token_rp_t{}; }
        }, {
          std::regex("true", std::regex_constants::icase),
          [](const std::string&) { return std::make_shared<token_bool_t>(token_bool_t{true}); }
        }, {
          std::regex("false", std::regex_constants::icase),
          [](const std::string&) { return std::make_shared<token_bool_t>(token_bool_t{false}); }
        }, {
          std::regex("[-+]?((\\d+\\.\\d*)|(\\d*\\.\\d+))"),
          [](const std::string& str) { return std::make_shared<token_double_t>(token_double_t{std::stod(str)}); }
        }, {
          std::regex("\".*?\""),
          [](const std::string& str) { return std::make_shared<token_string_t>(
              token_string_t{str.substr(1, str.size() - 2)}); }
        }, {
          std::regex("[-+]?\\d+"),
          [](const std::string& str) { return std::make_shared<token_integer_t>(token_integer_t{std::stol(str)}); }
        }, {
          std::regex("[\\w!#$%&*+-./:<=>?@_]+"),
          [](const std::string& str) { return std::make_shared<token_ident_t>(token_ident_t{str}); }
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
              token_t token = rule.get_token(m.str());
              if (!std::get_if<token_empty_t>(&token))
                tokens.push_back(token);
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
        return tokens;
      }
    };

    struct syntax_analyzer {

      ast_t parse(tokens_t tokens) {
        DEBUG_LOGGER_TRACE_ULISP;
        std::stack<std::shared_ptr<token_list_t>> stack;
        stack.push(std::make_shared<token_list_t>());
        for (const auto& token : tokens) {
          if (std::get_if<token_lp_t>(&token)) {
            stack.push(std::make_shared<token_list_t>());
          } else if (std::get_if<token_rp_t>(&token)) {
            auto top = std::make_shared<token_t>(stack.top());
            stack.pop();
            stack.top()->list.push_back(top);
          } else {
            stack.top()->list.push_back(std::make_shared<token_t>(token));
          }
        }
        if (stack.size() != 1) throw std::runtime_error("syntax_analyzer: parse error");
        auto ret = std::make_shared<token_t>(stack.top());
        return ret;
      }
    };

    struct semantic_analyzer {

      struct production_t;
      using productions_t = std::map<std::string, production_t>;

      struct production_t {
        std::function<bool (const productions_t& productions, const tokens_t&)> parse;
      };

      // stmt          : def_stmt*
      // def_stmt      : LP __def IDENT lambda_stmt RP
      // lambda_stmt:  : LP __lambda LP IDENT* RP LP lambda_body* RP RP
      // lambda_body   : def_stmt | expr
      // if_stmt       : LP __if expr expr expr RP
      // fun_stmt      : LP IDENT expr* RP
      // expr          : atom | fun_stmt | if_stmt
      // atom          : BOOL | INT | DOUBLE | STRING | IDENT | lambda_stmt

      struct program_stmt_t;
      static inline productions_t productions = {
        {
          "stmt", {
            [](const productions_t& productions, const tokens_t& tokens) {
              while (productions.at("def_stmt").parse(productions, tokens)) { }
              return true;
            },
          },
        }
      };

      ast_t parse(ast_t ast) {
        DEBUG_LOGGER_TRACE_ULISP;
        /*tokens_t tokens = tokens_orig;
        productions.at("stmt").parse(productions, tokens);
        if (!tokens.empty()) {
          DEBUG_LOGGER_ULISP("WARN: unexpected: '%s'", show_token(tokens.front()).c_str());
          DEBUG_LOGGER_ULISP("WARN: tokens.size(): %zd", tokens.size());
        }
        return tokens;*/
        return ast;
      }
    };

    struct code_generator { };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = R"LISP(
        (def fib (lambda (x)
          ((def fib (lambda (a b x) (if (greater? x 0) (fib b (+ a b) (- x 1)) (b))))
          (fib 1 1 x))))
      )LISP";

      { // debug
        DEBUG_LOGGER_ULISP("code: '%s'", code.c_str());
      }

      auto tokens = lexical_analyzer_t().parse(code);

      { // debug
        std::string str;
        for (const auto& token : tokens) {
          str += show_token(token) + " ";
        }
        DEBUG_LOGGER_ULISP("tokens: '\n%s'", str.c_str());
      }

      ast_t syntax_tree = syntax_analyzer().parse(tokens);

      { // debug
        std::string str = show_ast(syntax_tree);
        DEBUG_LOGGER_ULISP("syntax_tree: '\n%s\n'", str.c_str());
      }

      auto semantic_tree = semantic_analyzer().parse(syntax_tree);

      { // debug
        std::string str = show_ast(semantic_tree);
        DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

    }
  };

}

