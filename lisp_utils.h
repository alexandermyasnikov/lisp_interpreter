
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
      BOOL,
      DOUBLE,
      INTEGER,
      LAMBDA,
      MACRO,
      IDENT,
      END,
    };

    using attribute_t = std::variant<std::string, bool, int64_t, double>;
    using tokens_t = std::list<std::pair<token_t, attribute_t>>;



    struct tree_ident_t;
    using tree_ident_sptr_t = std::shared_ptr<tree_ident_t>;

    struct tree_lambda_t;
    using tree_lamda_sptr_t = std::shared_ptr<tree_lambda_t>;

    struct tree_macro_t;
    using tree_macro_sptr_t = std::shared_ptr<tree_macro_t>;

    struct tree_list_t;
    using tree_list_sptr_t = std::shared_ptr<tree_list_t>;

    using tree_t = std::variant<
      bool,
      double,
      int64_t,
      tree_ident_sptr_t,
      tree_lamda_sptr_t,
      tree_macro_sptr_t,
      tree_list_sptr_t
    >;

    struct tree_ident_t {
      std::string name;
    };

    struct tree_lambda_t {
      tree_list_sptr_t args;
      tree_t           body;
    };

    struct tree_macro_t {
      tree_list_sptr_t args;
      tree_t           body;
    };

    struct tree_list_t {
      std::list<tree_t> nodes;
    };



    struct object_t {
      using type_t = std::variant<
        bool,
        double,
        int64_t
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

    std::string show_tree(const tree_t& tree, int deep = 0) {
      std::string str;
      auto show_list = [this](tree_list_sptr_t value, int& deep) -> std::string {
        std::string str;
        str += "( ";
        deep += 2;
        bool is_first = true;
        bool simple_list = std::none_of(value->nodes.begin(), value->nodes.end(),
            [](auto& v) { return std::get_if<tree_lamda_sptr_t>(&v) || std::get_if<tree_list_sptr_t>(&v); });
        std::string indent = simple_list ? "" : "\n" + std::string(deep, ' ');

        for (const auto& node : value->nodes) {
          str += (is_first ? "" : indent) + show_tree(node, deep) + " ";
          is_first = false;
        }
        deep -= 2;
        str += ")";
        return str;
      };
      std::visit(overloaded {
        [&str, this] (bool value)                     { str = value ? "true" : "false"; },
        [&str, this] (int64_t value)                  { str = std::to_string(value); },
        [&str, this] (double value)                   { str = std::to_string(value); },
        [&str, this] (const tree_ident_sptr_t& value) { str = value->name; },
        [&str, &deep, this] (const tree_lamda_sptr_t& value) {
          tree_list_sptr_t tree_lambda = std::make_shared<tree_list_t>();
          tree_lambda->nodes.push_back(std::make_shared<tree_ident_t>(tree_ident_t{"lambda"}));
          tree_lambda->nodes.push_back(value->args);
          tree_lambda->nodes.push_back(value->body);
          str += show_tree(tree_lambda, deep);
        },
        [&str, &deep, show_list] (tree_list_sptr_t value)  {
          str += show_list(value, deep);
        },
        [&str] (auto) {
          str = "(unknown)";
        },
      }, tree);
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
          [](const std::string&) { return std::string{}; }
        }, {
          token_t::LP,
          std::regex("\\("),
          [](const std::string&) { return std::string{}; }
        }, {
          token_t::RP,
          std::regex("\\)"),
          [](const std::string&) { return std::string{}; }
        }, {
          token_t::BOOL,
          std::regex("true", std::regex_constants::icase),
          [](const std::string&) { return true; }
        }, {
          token_t::BOOL,
          std::regex("false", std::regex_constants::icase),
          [](const std::string&) { return false; }
        }, {
          token_t::DOUBLE,
          std::regex("[-+]?((\\d+\\.\\d*)|(\\d*\\.\\d+))"),
          [](const std::string& str) { return std::stod(str); }
        }, {
          token_t::INTEGER,
          std::regex("[-+]?\\d+"),
          [](const std::string& str) { return std::stol(str); }
        }, {
          token_t::LAMBDA,
          std::regex("lambda", std::regex_constants::icase),
          [](const std::string&) { return std::string{}; }
        }, {
          token_t::MACRO,
          std::regex("macro", std::regex_constants::icase),
          [](const std::string&) { return std::string{}; }
        }, {
          token_t::IDENT,
          std::regex("[\\w!#$%&*+-./:<=>?@_]+"),
          [](const std::string& str) { return str; }
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

      tree_t parse(const tokens_t& tokens) {
        DEBUG_LOGGER_TRACE_ULISP;
        tokens_t t = tokens;
        tree_t tree = parse_expr(t);
        match(t, token_t::END);
        return tree;
      }

      void match(tokens_t& tokens, token_t token) {
        if (tokens.front().first != token)
          throw std::runtime_error("match: unexpected token: " + std::to_string((int) tokens.front().first) + ", expected: " + std::to_string((int) token));
        tokens.pop_front();
      }

      tree_t parse_expr(tokens_t& tokens) {
        if (tokens.empty()) throw std::runtime_error("parse_expr: empty tokens");
        tree_t tree;
        auto kv = tokens.front();
        match(tokens, kv.first);
        switch (kv.first) {
          case token_t::BOOL:      { tree = std::get<bool>(kv.second); break; }
          case token_t::DOUBLE:    { tree = std::get<double>(kv.second); break; }
          case token_t::INTEGER:   { tree = std::get<int64_t>(kv.second); break; }
          case token_t::IDENT:     {
            tree = std::make_shared<tree_ident_t>(tree_ident_t{std::get<std::string>(kv.second)});
            break;
          }
          case token_t::LP: {
            if (tokens.front().first == token_t::LAMBDA) {
              match(tokens, token_t::LAMBDA);
              tree_lamda_sptr_t tree_lambda = std::make_shared<tree_lambda_t>();
              match(tokens, token_t::LP);
              tree_lambda->args = std::make_shared<tree_list_t>();
              while (tokens.front().first != token_t::RP) {
                tree_lambda->args->nodes.push_back(std::make_shared<tree_ident_t>(
                      tree_ident_t{std::get<std::string>(tokens.front().second)}));
                match(tokens, token_t::IDENT);
              }
              match(tokens, token_t::RP);
              tree_lambda->body = parse_expr(tokens);
              tree = tree_lambda;
            } else if (tokens.front().first == token_t::MACRO) {
              throw std::runtime_error("parse_expr: macro todo");
            } else {
              tree_list_sptr_t tree_list = std::make_shared<tree_list_t>();
              while (tokens.front().first != token_t::RP) {
                tree_list->nodes.push_back(parse_expr(tokens));
              }
              tree = tree_list;
            }
            match(tokens, token_t::RP);
            break;
          }
          default: throw std::runtime_error("parse_expr: unexpected token: " + std::to_string((int) tokens.front().first));
        }
        return tree;
      }
    };

    struct semantic_analyzer {

      /*semantic_tree_t parse(const syntax_tree_t& syntax_tree) {
        return {};
      }*/
    };

    struct code_generator { };

    void test() {
      DEBUG_LOGGER_TRACE_ULISP;
      std::string code = "( (def fib (lambda (x) ((def fib (lambda (a b x) (if (greater? x 0) (fib b (+ a b) (- x 1)) (b)))) (fib 1 1 x)))) (print -1 +2 +.3 -4.) ;comment 2\n (print 1 false) )";

      { // debug
        DEBUG_LOGGER_ULISP("code: '%s'", code.c_str());
      }

      auto tokens = lexical_analyzer_t().parse(code);

      { // debug
        for (const auto& token : tokens) {
          std::string str = show_attribute(token.second);
          DEBUG_LOGGER_ULISP("token: %d : '%s'", (int) token.first, str.c_str());
        }
      }

      tree_t tree = syntax_analyzer().parse(tokens);

      { // debug
        std::string str = show_tree(tree);
        DEBUG_LOGGER_ULISP("tree: '\n%s'", str.c_str());
      }

      // semantic_tree_t semantic_tree = semantic_analyzer().parse(syntax_tree);

      { // debug
        // std::string str = show_semantic_tree(semantic_tree_t);
        // DEBUG_LOGGER_ULISP("semantic_tree: '%s'", str.c_str());
      }

    }
  };

}

