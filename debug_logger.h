
#include <cstdarg>
#include <chrono>



#define DEBUG_LOGGER(name, indent)       debug_logger_t debug_logger(indent, name, __FILE__, __FUNCTION__, __LINE__)
#define DEBUG_LOG(name, indent, ...)     debug_logger_t::log(name, indent, __LINE__, __VA_ARGS__)

#define LOG_DURATION(time)               log_duration_t(time);



class debug_logger_t {
 public:
  debug_logger_t(int& indent, const char* name, const char* file, const char* function, const int line)
      : indent(indent), name(name), file(file), function(function), line(line) {
    fprintf(stderr, "%s %d    %*s#%d --> %s\n", name, indent / 2, indent, "", line, function);
    fflush(stderr);
    indent += 2;
    time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  ~debug_logger_t( ) {
    indent -= 2;

    time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - time;
    fprintf(stderr, "%s %d %c  %*s# <-- %s %ldms\n", name, indent / 2, std::uncaught_exceptions() ? '*' : ' ', indent, "", function, time);
    fflush(stderr);
  }

  static void log(const char* name, int indent, int line, const char* format, ...) {
    fprintf(stderr, "%s %d    %*s#%d    ", name, indent / 2, indent, "", line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
  }

 private:
  int&         indent;
  const  char* name;
  const  char* file; // TODO
  const  char* function;
  const  int   line;
  uint64_t     time;
};

struct log_duration_t {
  log_duration_t(uint64_t& time) : time(time), start(std::chrono::steady_clock::now()) {
  }

  ~log_duration_t() {
    auto finish = std::chrono::steady_clock::now();
    auto dur = finish - start;
    time = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
  }

 private:
  uint64_t& time;
  std::chrono::steady_clock::time_point start;
};
