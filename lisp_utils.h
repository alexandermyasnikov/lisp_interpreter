
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


    struct semantic_expr_stmt_t;
    struct semantic_lambda_stmt_t;
    struct semantic_def_stmt_t;

    struct semantic_ident_t {
      std::string name;
      size_t pointer;
    };

    struct semantic_atom_t {
      using atom_t = std::variant<
        lexeme_bool_t,
        lexeme_integer_t,
        lexeme_double_t,
        lexeme_string_t,
        std::shared_ptr<semantic_ident_t>>;
      atom_t atom;
    };

    struct semantic_if_stmt_t {
      std::shared_ptr<semantic_expr_stmt_t> test_expr;
      std::shared_ptr<semantic_expr_stmt_t> then_expr;
      std::shared_ptr<semantic_expr_stmt_t> else_expr;
    };

    struct semantic_fun_ident_stmt_t {
      using fun_ident_t = std::variant<
        std::shared_ptr<semantic_ident_t>,
        std::shared_ptr<semantic_lambda_stmt_t>>;
      fun_ident_t fun_ident;
    };

    struct semantic_call_stmt_t {
      std::shared_ptr<semantic_fun_ident_stmt_t>         fun_ident_stmt;
      std::vector<std::shared_ptr<semantic_expr_stmt_t>> expr_stmt;
    };

    struct semantic_expr_stmt_t {
      using expr_t = std::variant<
        std::shared_ptr<semantic_if_stmt_t>,
        std::shared_ptr<semantic_lambda_stmt_t>,
        std::shared_ptr<semantic_call_stmt_t>,
        std::shared_ptr<semantic_atom_t>>;
      expr_t expr;
    };

    struct semantic_body_stmt_t {
      using body_t = std::variant<
        std::shared_ptr<semantic_def_stmt_t>,
        std::shared_ptr<semantic_expr_stmt_t>>;
      body_t body;
    };

    struct semantic_lambda_stmt_t {
      std::vector<std::shared_ptr<semantic_ident_t>>   idents;
      std::vector<std::shared_ptr<semantic_body_stmt_t>> body_stmt;
    };

    struct semantic_def_stmt_t {
      std::shared_ptr<semantic_ident_t>       ident;
      std::shared_ptr<semantic_lambda_stmt_t> lambda_stmt;
    };

    struct semantic_tree_t {
      std::vector<std::shared_ptr<semantic_def_stmt_t>> def_stmt;

      // struct semantic_context {
      //   std::stack<std::string> name_space; // tmp
      // };

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



    static std::string show_semantic_atom(const semantic_atom_t& semantic_atom, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_atom: \n";
      indent += "  ";
      std::visit(overloaded {
        [&str, indent] (const lexeme_bool_t    &value) { str += indent + (value.value ? "true" : "false") + "\n"; },
        [&str, indent] (const lexeme_integer_t &value) { str += indent + std::to_string(value.value) + "\n"; },
        [&str, indent] (const lexeme_double_t  &value) { str += indent + std::to_string(value.value) + "\n"; },
        [&str, indent] (const lexeme_string_t  &value) { str += indent + "\"" + value.value + "\"" + "\n"; },
        [&str, deep]   (std::shared_ptr<semantic_ident_t> value) { str += show_semantic_ident(*value, deep + 2); },
        [&str, indent] (auto)                          { str += indent + "(unknown)" + "\n"; },
      }, semantic_atom.atom);
      return str;
    }

    static std::string show_semantic_fun_ident_stmt(const semantic_fun_ident_stmt_t& semantic_fun_ident_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_fun_ident_stmt: \n";

      std::visit(overloaded {
        [&str, deep] (std::shared_ptr<semantic_ident_t> value) {
          str += show_semantic_ident(*value, deep + 2);
        },
        [&str, deep] (std::shared_ptr<semantic_lambda_stmt_t> value) {
          str += show_semantic_lambda_stmt(*value, deep + 2);
        },
        [&str] (auto) { str = "(unknown)\n"; },
      }, semantic_fun_ident_stmt.fun_ident);
      return str;
    }

    static std::string show_semantic_call_stmt(const semantic_call_stmt_t& semantic_call_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_call_stmt: \n";
      str += indent + "  fun_ident_stmt: \n";
      str += show_semantic_fun_ident_stmt(*semantic_call_stmt.fun_ident_stmt, deep + 4);
      str += indent + "  args: \n";
      for (const auto& arg : semantic_call_stmt.expr_stmt) {
        str += show_semantic_expr_stmt(*arg, deep + 4);
      }
      return str;
    }

    static std::string show_semantic_if_stmt(const semantic_if_stmt_t& semantic_if_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_if_stmt: \n";
      str += indent + "  test_expr: \n";
      str += show_semantic_expr_stmt(*semantic_if_stmt.test_expr, deep + 4);
      str += indent + "  then_expr: \n";
      str += show_semantic_expr_stmt(*semantic_if_stmt.then_expr, deep + 4);
      str += indent + "  else_expr: \n";
      str += show_semantic_expr_stmt(*semantic_if_stmt.else_expr, deep + 4);
      return str;
    }

    static std::string show_semantic_expr_stmt(const semantic_expr_stmt_t& semantic_expr_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_expr_stmt: \n";

      std::visit(overloaded {
        [&str, deep] (std::shared_ptr<semantic_if_stmt_t> value) {
          str += show_semantic_if_stmt(*value, deep + 2);
        },
        [&str, deep] (std::shared_ptr<semantic_lambda_stmt_t> value) {
          str += show_semantic_lambda_stmt(*value, deep + 2);
        },
        [&str, deep] (std::shared_ptr<semantic_call_stmt_t> value) {
          str += show_semantic_call_stmt(*value, deep + 2);
        },
        [&str, deep] (std::shared_ptr<semantic_atom_t> value) {
          str += show_semantic_atom(*value, deep + 2);
        },
        [&str] (auto) { str = "(unknown)\n"; },
      }, semantic_expr_stmt.expr);
      return str;
    }

    static std::string show_semantic_body_stmt(const semantic_body_stmt_t& semantic_body_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_body_stmt: \n";

      std::visit(overloaded {
        [&str, deep] (std::shared_ptr<semantic_def_stmt_t> value) {
          str += show_semantic_def_stmt(*value, deep + 2);
        },
        [&str, deep] (std::shared_ptr<semantic_expr_stmt_t> value) {
          str += show_semantic_expr_stmt(*value, deep + 2);
        },
        [&str] (auto) { str = "(unknown)\n"; },
      }, semantic_body_stmt.body);
      return str;
    }

    static std::string show_semantic_ident(const semantic_ident_t& semantic_ident, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_ident: \n";
      str += indent + "  name:    " + semantic_ident.name + "\n";
      str += indent + "  pointer: " + std::to_string(semantic_ident.pointer) + "\n";
      str += indent + "  this:    " + std::to_string(reinterpret_cast<size_t>(&semantic_ident)) + "\n";
      return str;
    }

    static std::string show_semantic_lambda_stmt(const semantic_lambda_stmt_t& semantic_lambda_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_lambda_stmt: \n";
      for (const auto& arg : semantic_lambda_stmt.idents) {
        str += indent + "  arg: \n";
        str += show_semantic_ident(*arg, deep + 4);
      }
      for (const auto& body : semantic_lambda_stmt.body_stmt) {
        str += indent + "  body: \n";
        str += show_semantic_body_stmt(*body, deep + 4);
      }
      return str;
    }

    static std::string show_semantic_def_stmt(const semantic_def_stmt_t& semantic_def_stmt, size_t deep) {
      std::string indent = std::string(deep, ' ');
      std::string str = indent + "semantic_def_stmt: \n";
      str += indent + "  ident: \n";
      str += show_semantic_ident(*semantic_def_stmt.ident, deep + 4);
      str += indent + "  lambda_stmt: \n";
      str += show_semantic_lambda_stmt(*semantic_def_stmt.lambda_stmt, deep + 4);
      return str;
    }

    static std::string show_semantic_tree(const semantic_tree_t& semantic_tree) {
      std::string str = "semantic_tree: \n";
      size_t deep = 0;
      for (const auto& def_stmt : semantic_tree.def_stmt) {
        str += "  def_stmt: \n";
        str += show_semantic_def_stmt(*def_stmt, deep + 4);
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

    struct syntax_analyzer_t {

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

    struct semantic_analyzer_t {

      semantic_tree_t parse(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        return parse_program(syntax_tree);
      }

      std::shared_ptr<semantic_ident_t> parse_ident(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto name = std::get<lexeme_ident_t>(std::get<lexeme_t>(syntax_tree)).value;
          return std::make_shared<semantic_ident_t>(semantic_ident_t{name, {}});

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected ident");
        }
      };

      semantic_tree_t parse_program(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          semantic_tree_t semantic_tree;
          const auto& program = std::get<lexeme_list_sptr_t>(syntax_tree);

          for (const auto& def_stmt : program->list) {
            semantic_tree.def_stmt.push_back(parse_def_stmt(def_stmt));
          }

          return semantic_tree;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected program");
        }
      };

      std::shared_ptr<semantic_def_stmt_t> parse_def_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__def")
            throw std::exception();

          auto ident = parse_ident(args[1]);
          auto lambda = parse_lambda_stmt(args[2]);

          return std::make_shared<semantic_def_stmt_t>(semantic_def_stmt_t{ident, lambda});;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected def_stmt");
        }
      }

      std::shared_ptr<semantic_lambda_stmt_t> parse_lambda_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto semantic_lambda_stmt = std::make_shared<semantic_lambda_stmt_t>();
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__lambda")
            throw std::exception();

          const auto& lambda_args = std::get<lexeme_list_sptr_t>(args[1])->list;
          for (const auto& lambda_arg : lambda_args) {
            auto ident = parse_ident(lambda_arg);
            semantic_lambda_stmt->idents.push_back(ident);
          }

          const auto& lambda_body_list = std::get<lexeme_list_sptr_t>(args[2])->list;
          for (const auto& lambda_body : lambda_body_list) {
            semantic_lambda_stmt->body_stmt.push_back(parse_lambda_body_stmt(lambda_body));
          }

          return semantic_lambda_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected lambda_stmt");
        }
      }

      std::shared_ptr<semantic_body_stmt_t> parse_lambda_body_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto def_stmt = parse_def_stmt(syntax_tree);
            return std::make_shared<semantic_body_stmt_t>(semantic_body_stmt_t{def_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto expr_stmt = parse_expr_stmt(syntax_tree);
            return std::make_shared<semantic_body_stmt_t>(semantic_body_stmt_t{expr_stmt});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected lambda_body_stmt");
        }
      }

      std::shared_ptr<semantic_expr_stmt_t> parse_expr_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto if_stmt = parse_if_stmt(syntax_tree);
            return std::make_shared<semantic_expr_stmt_t>(semantic_expr_stmt_t{if_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto lambda_stmt = parse_lambda_stmt(syntax_tree);
            return std::make_shared<semantic_expr_stmt_t>(semantic_expr_stmt_t{lambda_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto call_stmt = parse_call_stmt(syntax_tree);
            return std::make_shared<semantic_expr_stmt_t>(semantic_expr_stmt_t{call_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto atom = parse_atom(syntax_tree);
            return std::make_shared<semantic_expr_stmt_t>(semantic_expr_stmt_t{atom});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected expr_stmt");
        }
      }

      std::shared_ptr<semantic_call_stmt_t> parse_call_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto semantic_call_stmt = std::make_shared<semantic_call_stmt_t>();
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() < 1)
            throw std::exception();

          semantic_call_stmt->fun_ident_stmt = parse_fun_ident_stmt(args[0]);

          for (size_t i = 1; i < args.size(); ++i) {
            auto expr = parse_expr_stmt(args[i]);
            semantic_call_stmt->expr_stmt.push_back(expr);
          }

          return semantic_call_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected call_stmt");
        }
      }

      std::shared_ptr<semantic_fun_ident_stmt_t> parse_fun_ident_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto ident = parse_ident(syntax_tree);
            return std::make_shared<semantic_fun_ident_stmt_t>(semantic_fun_ident_stmt_t{ident});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto lambda_stmt = parse_lambda_stmt(syntax_tree);
            return std::make_shared<semantic_fun_ident_stmt_t>(semantic_fun_ident_stmt_t{lambda_stmt});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected lambda_body_stmt");
        }
      }

      std::shared_ptr<semantic_if_stmt_t> parse_if_stmt(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree)->list;

          if (args.size() != 4)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__if")
            throw std::exception();

          auto test_expr = parse_expr_stmt(args[1]);
          auto then_expr = parse_expr_stmt(args[2]);
          auto else_expr = parse_expr_stmt(args[3]);

          return std::make_shared<semantic_if_stmt_t>(semantic_if_stmt_t{test_expr, then_expr, else_expr});;

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected if_stmt");
        }
      }

      std::shared_ptr<semantic_atom_t> parse_atom(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& atom = std::get<lexeme_t>(syntax_tree);
          if (std::get_if<lexeme_bool_t>(&atom)) {
            auto value = std::get<lexeme_bool_t>(atom);
            return std::make_shared<semantic_atom_t>(semantic_atom_t{value});;
          } else if (std::get_if<lexeme_integer_t>(&atom)) {
            auto value = std::get<lexeme_integer_t>(atom);
            return std::make_shared<semantic_atom_t>(semantic_atom_t{value});;
          } else if (std::get_if<lexeme_double_t>(&atom)) {
            auto value = std::get<lexeme_double_t>(atom);
            return std::make_shared<semantic_atom_t>(semantic_atom_t{value});;
          } else if (std::get_if<lexeme_string_t>(&atom)) {
            auto value = std::get<lexeme_string_t>(atom);
            return std::make_shared<semantic_atom_t>(semantic_atom_t{value});;
          } else if (std::get_if<lexeme_ident_t>(&atom)) {
            auto ident = parse_ident(syntax_tree);
            return std::make_shared<semantic_atom_t>(semantic_atom_t{ident});;
          } else {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected atom_stmt");
        }
      }
    };

    struct semantic_analyzer_advance_t {

      semantic_tree_t parse(const semantic_tree_t& semantic_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        return semantic_tree;
      }
    };

    struct code_generator_t {
    };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = R"LISP(
        (__def fib (__lambda (x)
          ((__def fib_inner (__lambda (a b x) ((__if (__greater? x 0) (fib_inner b (+ a b) (- x 1)) b))))
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

      auto syntax_tree = syntax_analyzer_t().parse(lexemes);

      { // debug
        std::string str = show_syntax_tree(syntax_tree);
        DEBUG_LOGGER_ULISP("syntax_tree: '\n%s\n'", str.c_str());
      }

      auto semantic_tree = semantic_analyzer_t().parse(syntax_tree);

      { // debug
        std::string str = show_semantic_tree(semantic_tree);
        DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

      /*auto*/ semantic_tree = semantic_analyzer_advance_t().parse(semantic_tree);

      { // debug
        std::string str = show_semantic_tree(semantic_tree);
        DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

    }
  };

}

