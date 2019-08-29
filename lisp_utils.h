
#include <iostream>
#include <variant>
#include <vector>
#include <regex>
#include <list>
#include "debug_logger.h"

#define DEBUG_LOGGER_TRACE_ULISP          DEBUG_LOGGER("lisp ", logger_indent_ulisp_t::indent)
#define DEBUG_LOGGER_ULISP(...)           DEBUG_LOG("lisp ", logger_indent_ulisp_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent = 0; };

struct logger_indent_ulisp_t : logger_indent_t<logger_indent_ulisp_t> { };



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

    struct lexical_analyzer_t {

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

      static lexemes_t parse(const std::string& str) {
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
    };

    struct syntax_analyzer_lisp_t {

      using lexeme_t = lexical_analyzer_t::lexeme_t;
      using lexemes_t = lexical_analyzer_t::lexemes_t;

      struct lexeme_list_t;
      using lexeme_list_sptr_t = std::shared_ptr<lexeme_list_t>;

      using syntax_tree_lisp_t = std::variant<
        lexeme_t,
        lexeme_list_sptr_t>;

      struct lexeme_list_t {
        std::vector<syntax_tree_lisp_t> list;
      };

      static syntax_tree_lisp_t parse(const lexemes_t& lexemes) {
        DEBUG_LOGGER_TRACE_ULISP;
        std::stack<lexeme_list_t> stack;
        stack.push(lexeme_list_t{});
        for (const auto& lexeme : lexemes) {
          if (std::get_if<lexical_analyzer_t::lexeme_lp_t>(&lexeme)) {
            stack.push(lexeme_list_t{});
          } else if (std::get_if<lexical_analyzer_t::lexeme_rp_t>(&lexeme)) {
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

      static std::string show_syntax_lisp_tree(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;
        std::string str;
        std::visit(overloaded {
            [&str] (lexeme_list_sptr_t value) {
              str += "( ";
              for (const auto& v : value->list)
              str += show_syntax_lisp_tree(v) + " ";
              str += ")";
            },
            [&str] (const lexeme_t& value) { str = lexical_analyzer_t::show_lexeme(value); },
            [&str] (auto)                  { str = "(unknown)"; },
        }, syntax_tree_lisp);
        return str;
      }
    };

    struct syntax_analyzer_t {

      struct program_stmt_t;
      struct def_stmt_t;
      struct lambda_stmt_t;
      struct body_stmt_t;
      struct expr_stmt_t;
      struct call_stmt_t;
      struct fun_ident_stmt_t;
      struct if_stmt_t;
      struct atom_stmt_t;
      struct ident_t;
      struct const_value_t;

      using syntax_tree_t = std::variant<
        std::shared_ptr<program_stmt_t>,
        std::shared_ptr<def_stmt_t>,
        std::shared_ptr<lambda_stmt_t>,
        std::shared_ptr<body_stmt_t>,
        std::shared_ptr<expr_stmt_t>,
        std::shared_ptr<call_stmt_t>,
        std::shared_ptr<fun_ident_stmt_t>,
        std::shared_ptr<if_stmt_t>,
        std::shared_ptr<atom_stmt_t>,
        std::shared_ptr<ident_t>,
        std::shared_ptr<const_value_t>>;

      struct program_stmt_t {
        std::vector<std::shared_ptr<def_stmt_t>> defs;
      };

      struct def_stmt_t {
        std::shared_ptr<ident_t>       name;
        std::shared_ptr<lambda_stmt_t> fun;
      };

      struct lambda_stmt_t {
        std::vector<std::shared_ptr<ident_t>>       args;
        std::vector<std::shared_ptr<body_stmt_t>> bodies;
      };

      struct body_stmt_t {
        using body_t = std::variant<
          std::shared_ptr<def_stmt_t>,
          std::shared_ptr<expr_stmt_t>>;
        body_t body;
      };

      struct expr_stmt_t {
        using expr_t = std::variant<
          std::shared_ptr<if_stmt_t>,
          std::shared_ptr<lambda_stmt_t>,
          std::shared_ptr<call_stmt_t>,
          std::shared_ptr<atom_stmt_t>>;
        expr_t expr;
      };

      struct call_stmt_t {
        std::shared_ptr<fun_ident_stmt_t>         fun;
        std::vector<std::shared_ptr<expr_stmt_t>> args;
      };

      struct fun_ident_stmt_t {
        using fun_t = std::variant<
          std::shared_ptr<ident_t>,
          std::shared_ptr<lambda_stmt_t>>;
        fun_t fun;
      };

      struct if_stmt_t {
        std::shared_ptr<expr_stmt_t> test_expr;
        std::shared_ptr<expr_stmt_t> then_expr;
        std::shared_ptr<expr_stmt_t> else_expr;
      };

      struct atom_stmt_t {
        using atom_t = std::variant<
          std::shared_ptr<ident_t>,
          std::shared_ptr<const_value_t>>;
        atom_t atom;
      };

      struct ident_t {
        std::string name;
        size_t pointer;
      };

      struct const_value_t {
        using type_t = std::variant<
          bool,
          int64_t,
          double,
          std::string>;
        type_t const_value;
      };

      using syntax_tree_lisp_t = syntax_analyzer_lisp_t::syntax_tree_lisp_t;
      using lexeme_list_sptr_t = syntax_analyzer_lisp_t::lexeme_list_sptr_t;
      using lexeme_ident_t     = lexical_analyzer_t::lexeme_ident_t;
      using lexeme_t           = lexical_analyzer_t::lexeme_t;

      syntax_tree_t parse(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;
        return {parse_program(syntax_tree_lisp)};
      }

      std::shared_ptr<ident_t> parse_ident(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto name = std::get<lexeme_ident_t>(std::get<lexeme_t>(syntax_tree_lisp)).value;
          return std::make_shared<ident_t>(ident_t{name, {}});

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected ident");
        }
      };

      std::shared_ptr<program_stmt_t> parse_program(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto program_stmt = std::make_shared<program_stmt_t>();
          const auto& program = std::get<lexeme_list_sptr_t>(syntax_tree_lisp);

          for (const auto& def_stmt : program->list) {
            program_stmt->defs.push_back(parse_def_stmt(def_stmt));
          }

          return program_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected program");
        }
      };

      std::shared_ptr<def_stmt_t> parse_def_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree_lisp)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__def")
            throw std::exception();

          auto name = parse_ident(args[1]);
          auto fun = parse_lambda_stmt(args[2]);

          return std::make_shared<def_stmt_t>(def_stmt_t{name, fun});;

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected def_stmt");
        }
      }

      std::shared_ptr<lambda_stmt_t> parse_lambda_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto lambda_stmt = std::make_shared<lambda_stmt_t>();
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree_lisp)->list;

          if (args.size() != 3)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__lambda")
            throw std::exception();

          const auto& lambda_args = std::get<lexeme_list_sptr_t>(args[1])->list;
          for (const auto& arg : lambda_args) {
            lambda_stmt->args.push_back(parse_ident(arg));
          }

          const auto& bodies = std::get<lexeme_list_sptr_t>(args[2])->list;
          for (const auto& body : bodies) {
            lambda_stmt->bodies.push_back(parse_body_stmt(body));
          }

          return lambda_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_antic_analyzer: expected lambda_stmt");
        }
      }

      std::shared_ptr<body_stmt_t> parse_body_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto def_stmt = parse_def_stmt(syntax_tree_lisp);
            return std::make_shared<body_stmt_t>(body_stmt_t{def_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto expr_stmt = parse_expr_stmt(syntax_tree_lisp);
            return std::make_shared<body_stmt_t>(body_stmt_t{expr_stmt});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected lambda_body_stmt");
        }
      }

      std::shared_ptr<expr_stmt_t> parse_expr_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto if_stmt = parse_if_stmt(syntax_tree_lisp);
            return std::make_shared<expr_stmt_t>(expr_stmt_t{if_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto lambda_stmt = parse_lambda_stmt(syntax_tree_lisp);
            return std::make_shared<expr_stmt_t>(expr_stmt_t{lambda_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto call_stmt = parse_call_stmt(syntax_tree_lisp);
            return std::make_shared<expr_stmt_t>(expr_stmt_t{call_stmt});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto atom = parse_atom(syntax_tree_lisp);
            return std::make_shared<expr_stmt_t>(expr_stmt_t{atom});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected expr_stmt");
        }
      }

      std::shared_ptr<call_stmt_t> parse_call_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          auto call_stmt = std::make_shared<call_stmt_t>();
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree_lisp)->list;

          if (args.size() < 1)
            throw std::exception();

          call_stmt->fun = parse_fun_ident_stmt(args[0]);

          for (size_t i = 1; i < args.size(); ++i) {
            call_stmt->args.push_back(parse_expr_stmt(args[i]));
          }

          return call_stmt;

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected call_stmt");
        }
      }

      std::shared_ptr<fun_ident_stmt_t> parse_fun_ident_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto ident = parse_ident(syntax_tree_lisp);
            return std::make_shared<fun_ident_stmt_t>(fun_ident_stmt_t{ident});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto lambda_stmt = parse_lambda_stmt(syntax_tree_lisp);
            return std::make_shared<fun_ident_stmt_t>(fun_ident_stmt_t{lambda_stmt});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected lambda_body_stmt");
        }
      }

      std::shared_ptr<if_stmt_t> parse_if_stmt(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          const auto& args = std::get<lexeme_list_sptr_t>(syntax_tree_lisp)->list;

          if (args.size() != 4)
            throw std::exception();

          if (std::get<lexeme_ident_t>(std::get<lexeme_t>(args[0])).value != "__if")
            throw std::exception();

          auto test_expr = parse_expr_stmt(args[1]);
          auto then_expr = parse_expr_stmt(args[2]);
          auto else_expr = parse_expr_stmt(args[3]);

          return std::make_shared<if_stmt_t>(if_stmt_t{test_expr, then_expr, else_expr});;

        } catch (const std::exception& e) {
          throw std::runtime_error("syntax_analyzer: expected if_stmt");
        }
      }

      std::shared_ptr<atom_stmt_t> parse_atom(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        try {
          try {
            auto ident = parse_ident(syntax_tree_lisp);
            return std::make_shared<atom_stmt_t>(atom_stmt_t{ident});;
          } catch (const std::exception& e) {
            ;
          }

          try {
            auto const_value = parse_const_value(syntax_tree_lisp);
            return std::make_shared<atom_stmt_t>(atom_stmt_t{const_value});;
          } catch (const std::exception& e) {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected atom_stmt");
        }
      }

      std::shared_ptr<const_value_t> parse_const_value(const syntax_tree_lisp_t& syntax_tree_lisp) {
        DEBUG_LOGGER_TRACE_ULISP;

        using lexeme_bool_t      = lexical_analyzer_t::lexeme_bool_t;
        using lexeme_integer_t   = lexical_analyzer_t::lexeme_integer_t;
        using lexeme_double_t    = lexical_analyzer_t::lexeme_double_t;
        using lexeme_string_t    = lexical_analyzer_t::lexeme_string_t;

        try {

          const auto& atom = std::get<lexeme_t>(syntax_tree_lisp);
          if (std::get_if<lexeme_bool_t>(&atom)) {
            auto value = std::get<lexeme_bool_t>(atom).value;
            return std::make_shared<const_value_t>(const_value_t{value});;
          } else if (std::get_if<lexeme_integer_t>(&atom)) {
            auto value = std::get<lexeme_integer_t>(atom).value;
            return std::make_shared<const_value_t>(const_value_t{value});;
          } else if (std::get_if<lexeme_double_t>(&atom)) {
            auto value = std::get<lexeme_double_t>(atom).value;
            return std::make_shared<const_value_t>(const_value_t{value});;
          } else if (std::get_if<lexeme_string_t>(&atom)) {
            auto value = std::get<lexeme_string_t>(atom).value;
            return std::make_shared<const_value_t>(const_value_t{value});;
          } else {
            throw std::exception();
          }

        } catch (const std::exception& e) {
          throw std::runtime_error("semantic_analyzer: expected atom_stmt");
        }
      }

      static void for_each_syntax_tree(const syntax_tree_t& syntax_tree, auto e, auto l) {
        DEBUG_LOGGER_TRACE_ULISP;
        std::visit(overloaded {
            [&syntax_tree, e, l] (std::shared_ptr<program_stmt_t> program_stmt) {
              if (e(syntax_tree)) {
                for (const auto& def : program_stmt->defs) {
                  for_each_syntax_tree(syntax_tree_t{def}, e, l);
                }
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<def_stmt_t> def_stmt) {
              if (e(syntax_tree)) {
                for_each_syntax_tree(syntax_tree_t{def_stmt->name}, e, l);
                for_each_syntax_tree(syntax_tree_t{def_stmt->fun}, e, l);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<lambda_stmt_t> lambda_stmt) {
              if (e(syntax_tree)) {
                for (const auto& arg : lambda_stmt->args) {
                  for_each_syntax_tree(syntax_tree_t{arg}, e, l);
                }
                for (const auto& body : lambda_stmt->bodies) {
                  for_each_syntax_tree(syntax_tree_t{body}, e, l);
                }
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<body_stmt_t> body_stmt) {
              if (e(syntax_tree)) {
                std::visit(overloaded {
                    [&syntax_tree, e, l] (auto stmt) { for_each_syntax_tree(syntax_tree_t{stmt}, e, l); },
                }, body_stmt->body);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<expr_stmt_t> expr_stmt) {
              if (e(syntax_tree)) {
                std::visit(overloaded {
                    [&syntax_tree, e, l] (auto stmt) { for_each_syntax_tree(syntax_tree_t{stmt}, e, l); },
                }, expr_stmt->expr);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<call_stmt_t> call_stmt) {
              if (e(syntax_tree)) {
                for_each_syntax_tree(syntax_tree_t{call_stmt->fun}, e, l);
                for (const auto& arg : call_stmt->args) {
                  for_each_syntax_tree(syntax_tree_t{arg}, e, l);
                }
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<fun_ident_stmt_t> fun_ident_stmt) {
              if (e(syntax_tree)) {
                std::visit(overloaded {
                    [&syntax_tree, e, l] (auto stmt) { for_each_syntax_tree(syntax_tree_t{stmt}, e, l); },
                }, fun_ident_stmt->fun);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<if_stmt_t> if_stmt) {
              if (e(syntax_tree)) {
                for_each_syntax_tree(syntax_tree_t{if_stmt->test_expr}, e, l);
                for_each_syntax_tree(syntax_tree_t{if_stmt->then_expr}, e, l);
                for_each_syntax_tree(syntax_tree_t{if_stmt->else_expr}, e, l);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<atom_stmt_t> atom_stmt) {
              if (e(syntax_tree)) {
                std::visit(overloaded {
                    [&syntax_tree, e, l] (auto stmt) { for_each_syntax_tree(syntax_tree_t{stmt}, e, l); },
                }, atom_stmt->atom);
              }
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<ident_t> /*ident*/) {
              e(syntax_tree);
              l(syntax_tree);
            }, [&syntax_tree, e, l] (std::shared_ptr<const_value_t> /*const_value*/) {
              e(syntax_tree);
              l(syntax_tree);
            }, [&syntax_tree, e, l] (auto) {
              throw std::runtime_error("for_each_syntax_tree: unknown type");
            },
        }, syntax_tree);
      }

      static std::string show_syntax_tree(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        size_t deep = 0;
        std::string str;

        auto e = [&str, &deep](const syntax_tree_t& syntax_tree) -> bool {
          bool ret = true;
          std::visit(overloaded {
            [&str, &deep] (std::shared_ptr<program_stmt_t> /*program_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "program_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<def_stmt_t> /*def_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "def_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<lambda_stmt_t> /*lambda_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "lambda_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<body_stmt_t> /*body_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "body_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<expr_stmt_t> /*expr_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "expr_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<call_stmt_t> /*call_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "call_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<fun_ident_stmt_t> /*fun_ident_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "fun_ident_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<if_stmt_t> /*if_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "if_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<atom_stmt_t> /*atom_stmt*/) {
              deep += 2;
              str += std::string(deep, ' ') + "atom_stmt_t: { \n";
            },
            [&str, &deep] (std::shared_ptr<ident_t> ident) {
              deep += 2;
              str += std::string(deep, ' ') + "ident_t: { \n";
              str += std::string(deep, ' ') + "  name:    " + ident->name + "\n";
              str += std::string(deep, ' ') + "  pointer: " + std::to_string(ident->pointer) + "\n";
              str += std::string(deep, ' ') + "  this:    " +
                  std::to_string(reinterpret_cast<size_t>(ident.get())) + "\n";
            },
            [&str, &deep] (std::shared_ptr<const_value_t> const_value) {
              deep += 2;
              std::visit(overloaded {
                  [&str, deep] (bool value) {
                    str += std::string(deep + 2, ' ') + "bool: " + (value ? "true" : "else") + "\n";
                  }, [&str, deep] (int64_t value) {
                    str += std::string(deep + 2, ' ') + "int64_t: " + std::to_string(value) + "\n";
                  }, [&str, deep] (double  value) {
                    str += std::string(deep + 2, ' ') + "double: " + std::to_string(value) + "\n";
                  }, [&str, deep] (std::string& value) {
                    str += std::string(deep + 2, ' ') + "string: " + "\"" + value + "\"" + "\n";
                  },
              }, const_value->const_value);
            },
            [] (auto) {
              throw std::runtime_error("show_syntax_tree: unknown type");
            }
          }, syntax_tree);
          return ret;
        };

        auto l = [&str, &deep](const syntax_tree_t& syntax_tree) {
          std::visit(overloaded {
            [&str, &deep] (auto) {
              str += std::string(deep, ' ') + "} \n";
              deep -= 2;
            }
          }, syntax_tree);
        };

        for_each_syntax_tree(syntax_tree, e, l);

        return str;
      }
    };

    struct semantic_analyzer_t {

      using syntax_tree_t = syntax_analyzer_t::syntax_tree_t; // TODO xxx_analyzer_t -> namespace
      using ident_t = syntax_analyzer_t::ident_t;

      static syntax_tree_t parse(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;
        auto tree = traversal_linking_idents(syntax_tree);
        return tree;
      }

      static syntax_tree_t traversal_linking_idents(const syntax_tree_t& syntax_tree) {
        DEBUG_LOGGER_TRACE_ULISP;

        struct ident_table_t {
          std::vector<std::shared_ptr<ident_t>> idents;
        };

        return syntax_tree;
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

      auto lexemes = lexical_analyzer_t::parse(code);

      { // debug
        std::string str;
        for (const auto& lexeme : lexemes) {
          str += lexical_analyzer_t::show_lexeme(lexeme) + " ";
        }
        DEBUG_LOGGER_ULISP("lexemes: '\n%s'", str.c_str());
      }

      auto syntax_tree_lisp = syntax_analyzer_lisp_t::parse(lexemes);

      { // debug
        std::string str = syntax_analyzer_lisp_t::show_syntax_lisp_tree(syntax_tree_lisp);
        DEBUG_LOGGER_ULISP("syntax_tree_lisp: '\n%s\n'", str.c_str());
      }

      auto syntax_tree = syntax_analyzer_t().parse(syntax_tree_lisp);

      { // debug
        std::string str = syntax_analyzer_t::show_syntax_tree(syntax_tree);
        DEBUG_LOGGER_ULISP("syntax_tree: '\n%s\n'", str.c_str());
      }

      /*auto*/ syntax_tree = semantic_analyzer_t::parse(syntax_tree);

      { // debug
        std::string str = syntax_analyzer_t::show_syntax_tree(syntax_tree);
        DEBUG_LOGGER_ULISP("semantic_tree: '\n%s\n'", str.c_str());
      }

    }
  };

}

