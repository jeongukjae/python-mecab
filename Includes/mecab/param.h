#ifndef _MECAB_PARAM_H_
#define _MECAB_PARAM_H_

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "mecab/common.h"
#include "mecab/scoped_ptr.h"
#include "mecab/string_buffer.h"
#include "mecab/utils.h"

namespace MeCab {

struct Option {
  const char* name;
  char short_name;
  const char* default_value;
  const char* arg_description;
  const char* description;
};

namespace {
template <class Target, class Source>
Target lexical_cast(Source arg) {
  std::stringstream interpreter;
  Target result;
  if (!(interpreter << arg) || !(interpreter >> result) || !(interpreter >> std::ws).eof()) {
    MeCab::scoped_ptr<Target> r(new Target());  // return default value
    return *r;
  }
  return result;
}

template <>
std::string lexical_cast(std::string arg) {
  return arg;
}

void init_param(std::string* help, std::string* version, const std::string& system_name, const Option* opts) {
  *help = std::string(COPYRIGHT) + "\nUsage: " + system_name + " [options] files\n";

  *version = std::string(PACKAGE) + " of " + VERSION + '\n';

  size_t max = 0;
  for (size_t i = 0; opts[i].name; ++i) {
    size_t l = 1 + std::strlen(opts[i].name);
    if (opts[i].arg_description)
      l += (1 + std::strlen(opts[i].arg_description));
    max = std::max(l, max);
  }

  for (size_t i = 0; opts[i].name; ++i) {
    size_t l = std::strlen(opts[i].name);
    if (opts[i].arg_description)
      l += (1 + std::strlen(opts[i].arg_description));
    *help += " -";
    *help += opts[i].short_name;
    *help += ", --";
    *help += opts[i].name;
    if (opts[i].arg_description) {
      *help += '=';
      *help += opts[i].arg_description;
    }
    for (; l <= max; l++)
      *help += ' ';
    *help += opts[i].description;
    *help += '\n';
  }

  *help += '\n';
  return;
}

enum ArgError { UNRECOGNIZED, REQUIRE_ARG, NO_ARG };

bool printArgError(const ArgError error, const char* option) {
  switch (error) {
    case UNRECOGNIZED:
      CHECK_FALSE(false) << "unrecognized option `" << option << "`";
      break;
    case REQUIRE_ARG:
      CHECK_FALSE(false) << "`" << option << "` requires an argument";
      break;
    case NO_ARG:
      CHECK_FALSE(false) << "`" << option << "` doesn't allow an argument";
      break;
  }
  return false;
}

}  // namespace

class Param {
 private:
  std::map<std::string, std::string> conf_;
  std::vector<std::string> rest_;
  std::string system_name_;
  std::string help_;
  std::string version_;

 public:
  bool open(int argc, char** argv, const Option* opts) {
    int ind = 0;

    if (argc <= 0) {
      system_name_ = "unknown";
      return true;  // this is not error
    }

    system_name_ = std::string(argv[0]);

    init_param(&help_, &version_, system_name_, opts);

    for (size_t i = 0; opts[i].name; ++i) {
      if (opts[i].default_value)
        set<std::string>(opts[i].name, opts[i].default_value);
    }

    for (ind = 1; ind < argc; ind++) {
      if (argv[ind][0] == '-') {
        // long options
        if (argv[ind][1] == '-') {
          char* s;
          for (s = &argv[ind][2]; *s != '\0' && *s != '='; s++)
            ;
          size_t len = (size_t)(s - &argv[ind][2]);
          if (len == 0)
            return true;  // stop the scanning

          bool hit = false;
          size_t i = 0;
          for (i = 0; opts[i].name; ++i) {
            size_t nlen = std::strlen(opts[i].name);
            if (nlen == len && std::strncmp(&argv[ind][2], opts[i].name, len) == 0) {
              hit = true;
              break;
            }
          }

          if (!hit)
            return printArgError(UNRECOGNIZED, argv[ind]);

          if (opts[i].arg_description) {
            if (*s == '=') {
              set<std::string>(opts[i].name, s + 1);
            } else {
              if (argc == (ind + 1))
                return printArgError(REQUIRE_ARG, argv[ind]);
              set<std::string>(opts[i].name, argv[++ind]);
            }
          } else {
            if (*s == '=')
              return printArgError(NO_ARG, argv[ind]);
            set<int>(opts[i].name, 1);
          }

          // short options
        } else if (argv[ind][1] != '\0') {
          size_t i = 0;
          bool hit = false;
          for (i = 0; opts[i].name; ++i) {
            if (opts[i].short_name == argv[ind][1]) {
              hit = true;
              break;
            }
          }

          if (!hit)
            return printArgError(UNRECOGNIZED, argv[ind]);

          if (opts[i].arg_description) {
            if (argv[ind][2] != '\0') {
              set<std::string>(opts[i].name, &argv[ind][2]);
            } else {
              if (argc == (ind + 1))
                return printArgError(REQUIRE_ARG, argv[ind]);
              set<std::string>(opts[i].name, argv[++ind]);
            }
          } else {
            if (argv[ind][2] != '\0')
              return printArgError(NO_ARG, argv[ind]);
            set<int>(opts[i].name, 1);
          }
        }
      } else {
        rest_.push_back(std::string(argv[ind]));  // others
      }
    }

    return true;
  }

  bool open(const char* arg, const Option* opts) {
    scoped_fixed_array<char, BUF_SIZE> str;
    std::strncpy(str.get(), arg, str.size());
    char* ptr[64];
    unsigned int size = 1;
    ptr[0] = const_cast<char*>(PACKAGE);

    for (char* p = str.get(); *p;) {
      while (isspace(*p))
        *p++ = '\0';
      if (*p == '\0')
        break;
      ptr[size++] = p;
      if (size == sizeof(ptr))
        break;
      while (*p && !isspace(*p))
        p++;
    }

    return open(size, ptr, opts);
  }

  bool load(const char* filename) {
    std::ifstream ifs(filename);

    CHECK_FALSE(ifs) << "no such file or directory: " << filename;

    std::string line;
    while (std::getline(ifs, line)) {
      if (!line.size() || (line.size() && (line[0] == ';' || line[0] == '#')))
        continue;

      size_t pos = line.find('=');
      CHECK_FALSE(pos != std::string::npos) << "format error: " << line;

      size_t s1, s2;
      for (s1 = pos + 1; s1 < line.size() && isspace(line[s1]); s1++)
        ;
      for (s2 = pos - 1; static_cast<long>(s2) >= 0 && isspace(line[s2]); s2--)
        ;
      const std::string value = line.substr(s1, line.size() - s1);
      const std::string key = line.substr(0, s2 + 1);
      set<std::string>(key.c_str(), value, false);
    }

    return true;
  }

  void clear() {
    conf_.clear();
    rest_.clear();
  }

  const std::vector<std::string>& rest_args() const { return rest_; }

  std::string getProgramName() const { return system_name_; }
  std::string getHelpMessage() const { return help_; }
  std::string getVersion() const { return version_; }
  int printVersion() const {
    if (get<bool>("help")) {
      std::cout << help_ << std::endl;
      return 0;
    }

    if (get<bool>("version")) {
      std::cout << version_ << std::endl;
      return 0;
    }

    return 1;
  }

  template <class T>
  T get(const char* key) const {
    std::map<std::string, std::string>::const_iterator it = conf_.find(key);
    if (it == conf_.end()) {
      scoped_ptr<T> r(new T());
      return *r;
    }
    return lexical_cast<T, std::string>(it->second);
  }

  template <class T>
  void set(const char* key, const T& value, bool rewrite = true) {
    std::string key2 = std::string(key);
    if (rewrite || (!rewrite && conf_.find(key2) == conf_.end()))
      conf_[key2] = lexical_cast<std::string, T>(value);
  }

  void dump_config(std::ostream* os) const {
    for (std::map<std::string, std::string>::const_iterator it = conf_.begin(); it != conf_.end(); ++it) {
      *os << it->first << ": " << it->second << std::endl;
    }
  }

  explicit Param() {}
  virtual ~Param() {}
};
}  // namespace MeCab

#endif  // _MECAB_PARAM_H_
