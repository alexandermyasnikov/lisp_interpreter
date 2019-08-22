
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

    // program       : stmt*
    // stmt          : BOOL | INT | DOUBLE | STRING | IDENT | list
    // list          : LP stmt* RP

    struct lexeme_empty_t   { };
    struct lexeme_lp_t      { };
    struct lexeme_rp_t      { };
    struct lexeme_bool_t    { bool        value; };
    struct lexeme_integer_t { int64_t     value; };
    struct lexeme_double_t  { double      value; };
    struct lexeme_string_t  { std::string value; };
    struct lexeme_ident_t   { std::string value; };

    using lexeme_t = std::variant<
      lexeme_empty_t,
      lexeme_lp_t,
      lexeme_rp_t,
      lexeme_bool_t,
      lexeme_integer_t,
      lexeme_double_t,
      lexeme_string_t,
      lexeme_ident_t>;

    struct lexeme_list_t;

    using syntax_tree_t = std::variant<
      lexeme_t,
      std::shared_ptr<lexeme_list_t>>;

    struct lexeme_list_t {
      std::vector<syntax_tree_t> list;
    };

    using lexemes_t = std::list<lexeme_t>;



    struct semantic_def_stmt_t {
    };

    struct semantic_program_t {
      std::vector<semantic_def_stmt_t> def_stmt;
    };

    using semantic_tree_t = semantic_program_t;

    // program       : def_stmt*
    // def_stmt      : LP __def IDENT lambda_stmt RP
    // lambda_stmt:  : LP __lambda LP IDENT* RP LP lambda_body* RP RP
    // lambda_body   : def_stmt | expr
    // if_stmt       : LP __if expr expr expr RP
    // fun_stmt      : LP IDENT expr* RP
    // expr          : atom | fun_stmt | if_stmt
    // atom          : BOOL | INT | DOUBLE | STRING | IDENT | lambda_stmt



    static std::string show_lexeme(const lexeme_t& lexeme) {
      std::string str;
      std::visit(overloaded {
        [&str] (lexeme_empty_t)                { str = "(empty)"; },
        [&str] (lexeme_lp_t)                   { str = "("; },
        [&str] (lexeme_rp_t)                   { str = ")"; },
        [&str] (const lexeme_bool_t    &value) { str = value.value ? "true" : "false"; },
        [&str] (const lexeme_integer_t &value) { str = std::to_string(value.value); },
        [&str] (const lexeme_double_t  &value) { str = std::to_string(value.value); },
        [&str] (const lexeme_string_t  &value) { str = "\"" + value.value + "\""; },
        [&str] (const lexeme_ident_t   &value) { str = value.value; },
        [&str] (auto)                          { str = "(unknown)"; },
      }, lexeme);
      return str;
    }

    static std::string show_syntax_tree(const syntax_tree_t& syntax_tree) {
      std::string str;
      std::visit(overloaded {
        [&str] (std::shared_ptr<lexeme_list_t> value) {
          str += "( ";
          for (const auto& v : value->list)
            str += show_syntax_tree(v) + " ";
          str += ")";
        },
        [&str] (const lexeme_t& value) { str = show_lexeme(value); },
        [&str] (auto)                  { str = "(unknown)"; },
      }, syntax_tree);
      return str;
    }



    struct lexical_analyzer_t {

      struct rule_t {
        std::regex   regex;
        std::function<lexeme_t (const std::string&)> get_lexeme;
      };

      using rules_t = std::list<rule_t>;

      static inline rules_t rules = {
        {
          std::regex("\\s+|;.*?\n"),
          [](const std::string&) { return lexeme_empty_t{}; }
        }, {
          std::regex("\\("),
          [](const std::string&) { return lexeme_lp_t{}; }
        }, {
          std::regex("\\)"),
          [](const std::string&) { return lexeme_rp_t{}; }
        }, {
          std::regex("true", std::regex_constants::icase),
          [](const std::string&) { return lexeme_bool_t{true}; }
        }, {
          std::regex("false", std::regex_constants::icase),
          [](const std::string&) { return lexeme_bool_t{false}; }
        }, {
          std::regex("[-+]?((\\d+\\.\\d*)|(\\d*\\.\\d+))"),
          [](const std::string& str) { return lexeme_double_t{std::stod(str)}; }
        }, {
          std::regex("\".*?\""),
          [](const std::string& str) { return lexeme_string_t{str.substr(1, str.size() - 2)}; }
        }, {
          std::regex("[-+]?\\d+"),
          [](const std::string& str) { return lexeme_integer_t{std::stol(str)}; }
        }, {
          std::regex("[\\w!#$%&*+-./:<=>?@_]+"),
          [](const std::string& str) { return lexeme_ident_t{str}; }
        }
      };

      lexemes_t parse(const std::string& str) {
        DEBUG_LOGGER_TRACE_ULISP;
        lexemes_t lexemes;
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
        return lexemes;
      }
    };

    struct syntax_analyzer {

      syntax_tree_t parse(const lexemes_t& lexemes) {
        DEBUG_LOGGER_TRACE_ULISP;
        std::stack<lexeme_list_t> stack;
        stack.push(lexeme_list_t{});
        for (const auto& lexeme : lexemes) {
          if (std::get_if<lexeme_lp_t>(&lexeme)) {
            stack.push(lexeme_list_t{});
          } else if (std::get_if<lexeme_rp_t>(&lexeme)) {
            auto top = stack.top();
            stack.pop();
            stack.top().list.push_back(std::make_shared<lexeme_list_t>(std::move(top)));
          } else {
            stack.top().list.push_back(lexeme);
          }
        }
        if (stack.size() != 1) throw std::runtime_error("syntax_analyzer: parse error");
        return std::make_shared<lexeme_list_t>(std::move(stack.top()));
      }
    };

    struct semantic_analyzer {

      struct semantic_context {
        std::map<std::string, semantic_def_stmt_t> functions; // XXX
        std::stack<std::string> name_space; // tmp
      };

      semantic_tree_t parse_program(semantic_tree_t semantic_tree, semantic_context& ctx) {
        DEBUG_LOGGER_TRACE_ULISP;
        /*auto program_if = *ast;
        if (!std::get_if<std::shared_ptr<token_list_t>>(&program_if))
          throw std::runtime_error("semantic_analyzer: expected program");
        auto program = std::get<std::shared_ptr<token_list_t>>(program_if);
        for (const auto def_stmt : program->list) {
          parse_def_stmt(def_stmt, ctx);
        }
        return ast;*/
        return semantic_tree;
      };

      /*ast_t parse_def_stmt(ast_t ast, semantic_context& ctx) {
        DEBUG_LOGGER_TRACE_ULISP;
        auto def_stmt_if = *ast;

        if (!std::get_if<std::shared_ptr<token_list_t>>(&def_stmt_if))
          throw std::runtime_error("semantic_analyzer: expected def_stmt");
        std::vector<ast_t> args = std::get<std::shared_ptr<token_list_t>>(def_stmt_if)->list;

        if (args.size() != 3)
          throw std::runtime_error("semantic_analyzer: expected def_stmt");

        if (!std::get_if<std::shared_ptr<token_ident_t>>(args[0].get())
            || std::get<std::shared_ptr<token_ident_t>>(*args[0])->value != "__def")
          throw std::runtime_error("semantic_analyzer: expected def_stmt");

        if (!std::get_if<std::shared_ptr<token_ident_t>>(args[0].get()))
          throw std::runtime_error("semantic_analyzer: expected def_stmt");
        auto fun_name = std::get<std::shared_ptr<token_ident_t>>(*args[1])->value;

        return ast;
      };*/

      semantic_tree_t parse(syntax_tree_t syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        semantic_context ctx;
        // auto ret = parse_program(ast, ctx);
        return {};
      }
    };

    struct code_generator {
    };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = R"LISP(
        (__def fib (__lambda (x)
          ((__def fib (__lambda (a b x) (__if (__greater? x 0) (fib b (+ a b) (- x 1)) (b))))
          (fib 1 1 x))))
      )LISP";

      { // debug
        DEBUG_LOGGER_ULISP("code: '%s'", code.c_str());
      }

      auto lexemes = lexical_analyzer_t().parse(code);

      { // debug
        std::string str;
        for (const auto& lexeme : lexemes) {
          str += show_lexeme(lexeme) + " ";
        }
        DEBUG_LOGGER_ULISP("lexemes: '\n%s'", str.c_str());
      }

      auto syntax_tree = syntax_analyzer().parse(lexemes);

      { // debug
        std::string str = show_syntax_tree(syntax_tree);
        DEBUG_LOGGER_ULISP("syntax_tree: '\n%s\n'", str.c_str());
      }

      // auto semantic_tree = semantic_analyzer().parse(syntax_tree);

      { // debug
        // std::string str = show_ast(semantic_tree);
        // DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

    }
  };

}

