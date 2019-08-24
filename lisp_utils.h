
#include <iostream>
#include <list>
#include <regex>

#define DEBUG_LOGGER_TRACE_ULISP          DEBUG_LOGGER("lisp ", logger_indent_ulisp_t::indent)
#define DEBUG_LOGGER_ULISP(...)           DEBUG_LOG("lisp ", logger_indent_ulisp_t::indent, __VA_ARGS__)

struct logger_indent_ulisp_t   : logger_indent_t<logger_indent_ulisp_t> { };


namespace lisp_utils {

  using namespace std::string_literals;

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

    using lexemes_t = std::list<lexeme_t>;



    struct lexeme_list_t;
    using lexeme_list_sptr_t = std::shared_ptr<lexeme_list_t>;

    using syntax_tree_t = std::variant<
      lexeme_t,
      lexeme_list_sptr_t>;

    struct lexeme_list_t {
      std::vector<syntax_tree_t> list;
    };


    struct semantic_ident_t {
      std::string name;
      size_t pointer;
    };

    struct semantic_atom_t {
      /*std::variant<
        lexeme_bool_t,
        lexeme_integer_t,
        lexeme_double_t,
        lexeme_string_t,
        lexeme_ident_t>;*/
    };

    struct semantic_if_stmt_t {
    };

    struct semantic_fun_stmt_t {
    };

    struct semantic_expr_stmt_t {
    };

    struct semantic_lambda_stmt_t {
      std::vector<std::shared_ptr<semantic_ident_t>> args;
      std::vector<semantic_expr_stmt_t> body;
    };

    struct semantic_tree_t {
      std::map<std::shared_ptr<semantic_ident_t>, semantic_lambda_stmt_t> def_stmt;

      struct semantic_context {
        std::stack<std::string> name_space; // tmp
      };

    };



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
        [&str] (lexeme_list_sptr_t value) {
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


    static std::string show_semantic_expr_stmt(const semantic_expr_stmt_t& semantic_expr_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_expr_stmt: \n";
      // str += indent + "  name: " + semantic_ident.name + "\n";
      return str;
    }

    static std::string show_semantic_ident(const semantic_ident_t& semantic_ident, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_ident: \n";
      str += indent + "  name: " + semantic_ident.name + "\n";
      str += indent + "  pointer: " + std::to_string(semantic_ident.pointer) + "\n";
      return str;
    }

    static std::string show_semantic_lambda_stmt(const semantic_lambda_stmt_t& semantic_lambda_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_lambda_stmt: \n";
      for (const auto& arg : semantic_lambda_stmt.args) {
        str += indent + "  arg: \n";
        str += show_semantic_ident(*arg, deep + 4);
      }
      for (const auto& body : semantic_lambda_stmt.body) {
        str += indent + "  body: \n";
        str += show_semantic_expr_stmt(body, deep + 4);
      }
      return str;
    }

    static std::string show_semantic_tree(const semantic_tree_t& semantic_tree) {
      std::string str = "semantic_tree: \n";
      size_t deep = 2;
      for (const auto& def_stm : semantic_tree.def_stmt) {
        str += show_semantic_ident(*def_stm.first, deep);
        str += show_semantic_lambda_stmt(def_stm.second, deep);
      }
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

      // program       : def_stmt*
      // def_stmt      : LP __def IDENT lambda_stmt RP
      // lambda_stmt:  : LP __lambda LP IDENT* RP LP lambda_body* RP RP
      // lambda_body   : def_stmt | expr
      // expr          : atom | fun_stmt | if_stmt
      // fun_stmt      : LP IDENT expr* RP
      // if_stmt       : LP __if expr expr expr RP
      // atom          : BOOL | INT | DOUBLE | STRING | IDENT | lambda_stmt

      // using syntax_tree_t = std::variant<lexeme_t, lexeme_list_sptr_t>; 
      // struct lexeme_list_t { std::vector<syntax_tree_t> list; };

      semantic_tree_t parse(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        return parse_program(syntax_tree);
      }

      semantic_tree_t parse_program(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_tree_t semantic_tree;

          const auto& program = std::get<lexeme_list_sptr_t>(syntax_tree);

          for (const auto& def_stmt : program->list) {
            parse_def_stmt(def_stmt, semantic_tree);
          }

          return semantic_tree;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected program");
        }
      };

      void parse_def_stmt(const syntax_tree_t& syntax_tree, semantic_tree_t& semantic_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__def")
            throw std::exception();

          const auto& fun_name = std::get<lexeme_ident_t>(std::get<lexeme_t>(args[1])).value;
          const auto lambda = parse_lambda_stmt(args[2], semantic_tree);

          auto ident = std::make_shared<semantic_ident_t>(semantic_ident_t{fun_name, {}});
          semantic_tree.def_stmt[ident] = lambda;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected def_stmt");
        }
      }

      semantic_lambda_stmt_t parse_lambda_stmt(const syntax_tree_t& syntax_tree, semantic_tree_t& semantic_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_lambda_stmt_t semantic_lambda_stmt;

          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__lambda")
            throw std::exception();

          const auto& lambda_args = std::get<lexeme_list_sptr_t>(args[1])->list;
          for (const auto& lambda_arg : lambda_args) {
            const auto arg_name = std::get<lexeme_ident_t>(std::get<lexeme_t>(lambda_arg)).value;
            auto ident = std::make_shared<semantic_ident_t>(semantic_ident_t{arg_name, {}});
            semantic_lambda_stmt.args.push_back(ident);
          }

          const auto& lambda_body_list = std::get<lexeme_list_sptr_t>(args[2])->list;
          for (const auto& lambda_body : lambda_body_list) {
            parse_lambda_body_stmt(lambda_body, semantic_lambda_stmt, semantic_tree);
          }

          return semantic_lambda_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected lambda_stmt");
        }
      }

      void parse_lambda_body_stmt(const syntax_tree_t& syntax_tree,
          semantic_lambda_stmt_t& semantic_lambda_stmt,
          semantic_tree_t& semantic_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            parse_def_stmt(syntax_tree, semantic_tree);
          } catch (const std::exception& e) {
            ;
          }

          auto expr = parse_expr_stmt(syntax_tree);
          semantic_lambda_stmt.body.push_back(expr);

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected lambda_body_stmt");
        }
      }

      semantic_expr_stmt_t parse_expr_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            const auto atom = parse_atom(syntax_tree);
            return {};
          } catch (const std::exception& e) {
            ;
          }

          try {
            const auto fun_stmt = parse_fun_stmt(syntax_tree);
            return {};
          } catch (const std::exception& e) {
            ;
          }

          parse_if_stmt(syntax_tree);
          return {};

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected expr_stmt");
        }
      }

      semantic_fun_stmt_t parse_fun_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_fun_stmt_t semantic_fun_stmt;

          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() < 1)
            throw std::exception();

          const auto& fun_name = std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value;

          for (size_t i = 1; i < args.size(); ++i) {
            const auto expr = parse_expr_stmt(args[i]);
          }

          return semantic_fun_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected fun_stmt");
        }
      }

      semantic_if_stmt_t parse_if_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_if_stmt_t semantic_if_stmt;

          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 4)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__if")
            throw std::exception();

          const auto expr_if   = parse_expr_stmt(args[1]);
          const auto expr_then = parse_expr_stmt(args[2]);
          const auto expr_else = parse_expr_stmt(args[3]);

          return semantic_if_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected if_stmt");
        }
      }

      semantic_atom_t parse_atom(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_atom_t semantic_atom;

          const auto& atom = std::get<lexeme_t>(syntax_tree);

          return semantic_atom;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected atom_stmt");
        }
      }
    };

    struct code_generator {
    };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = R"LISP(
        (__def fib (__lambda (x)
          ((__def_inner fib_inner (__lambda (a b x) (__if (__greater? x 0) (fib_inner b (+ a b) (- x 1)) (b))))
          (fib_inner 1 1 x))))
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

      auto semantic_tree = semantic_analyzer().parse(syntax_tree);

      { // debug
        std::string str = show_semantic_tree(semantic_tree);
        DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

    }
  };

}

