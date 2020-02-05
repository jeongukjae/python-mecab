#ifndef __MECAB_UTILS_STRING_UTILS_H__
#define __MECAB_UTILS_STRING_UTILS_H__

#include <cstring>
#include <string>

namespace MeCab {
void inline dtoa(double val, char* s) {
  std::sprintf(s, "%-16f", val);
  char* p = s;
  for (; *p != ' '; ++p) {
  }
  *p = '\0';
  return;
}

template <class T>
inline void itoa(T val, char* s) {
  char* t;
  T mod;

  if (val < 0) {
    *s++ = '-';
    val = -val;
  }
  t = s;

  while (val) {
    mod = val % 10;
    *t++ = static_cast<char>(mod) + '0';
    val /= 10;
  }

  if (s == t)
    *t++ = '0';
  *t = '\0';
  std::reverse(s, t);

  return;
}

template <class T>
inline void uitoa(T val, char* s) {
  char* t;
  T mod;
  t = s;
  while (val) {
    mod = val % 10;
    *t++ = static_cast<char>(mod) + '0';
    val /= 10;
  }

  if (s == t)
    *t++ = '0';
  *t = '\0';
  std::reverse(s, t);
  return;
}

inline std::string create_filename(const std::string& path, const std::string& file) {
  std::string s = path;
  if (s.size() && s[s.size() - 1] != '/')
    s += '/';
  s += file;
  return s;
}

inline std::string remove_filename(std::string path) {
  int len = static_cast<int>(path.size()) - 1;
  for (; len >= 0 && path.at(len) != '/'; --len)
    ;

  if (len > 0)
    return path.substr(0, len);

  return ".";
}

inline std::string remove_pathname(std::string path) {
  int len = static_cast<int>(path.size()) - 1;
  for (; len >= 0 && path.at(len) != '/'; --len)
    ;

  if (len > 0)
    return path.substr(len + 1, path.size() - len);

  return ".";
}

inline std::string replace_string(std::string string, const std::string& source, const std::string& destination) {
  const std::string::size_type pos = string.find(source);
  std::string result(string);
  if (pos != std::string::npos) {
    result.replace(pos, source.size(), destination);
  }
  return result;
}

inline std::string to_lower(std::string text) {
  std::string result{text};
  for (size_t i = 0; i < text.size(); ++i) {
    char c = result[i];
    if ((c >= 'A') && (c <= 'Z'))
      result[i] = c + 'a' - 'A';
  }
  return result;
}

enum { UTF8, UTF16, UTF16LE, UTF16BE, ASCII };

inline int decode_charset(const char* charset) {
  std::string tmp = charset;
  tmp = to_lower(tmp);
  if (tmp == "utf8" || tmp == "utf_8" || tmp == "utf-8")
    return UTF8;
  else if (tmp == "utf16" || tmp == "utf_16" || tmp == "utf-16")
    return UTF16;
  else if (tmp == "utf16be" || tmp == "utf_16be" || tmp == "utf-16be")
    return UTF16BE;
  else if (tmp == "utf16le" || tmp == "utf_16le" || tmp == "utf-16le")
    return UTF16LE;
  else if (tmp == "ascii")
    return ASCII;

  return UTF8;  // default is UTF8
}

inline char getEscapedChar(const char p) {
  switch (p) {
    case '0':
      return '\0';
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case 's':
      return ' ';
    case '\\':
      return '\\';
    default:
      break;
  }

  return '\0';  // never be here
}

inline void tokenize(std::string str, std::string delimiter, std::vector<std::string>& tokenizedResult) {
  size_t pos = 0;
  std::string token;
  std::string str_(str);

  while ((pos = str_.find(delimiter)) != std::string::npos) {
    token = str_.substr(0, pos);
    tokenizedResult.emplace_back(token);
    str_ = str_.substr(pos + delimiter.length());
  }

  tokenizedResult.emplace_back(str_);
}

template <class Iterator>
inline size_t tokenizeCSV(char* str, Iterator out, size_t max) {
  char* eos = str + std::strlen(str);
  char* start = 0;
  char* end = 0;
  size_t n = 0;

  for (; str < eos; ++str) {
    // skip white spaces
    while (*str == ' ' || *str == '\t')
      ++str;
    if (*str == '"') {
      start = ++str;
      end = start;
      for (; str < eos; ++str) {
        if (*str == '"') {
          str++;
          if (*str != '"')
            break;
        }
        *end++ = *str;
      }
      str = std::find(str, eos, ',');
    } else {
      start = str;
      str = std::find(str, eos, ',');
      end = str;
    }
    if (max-- > 1)
      *end = '\0';
    *out++ = start;
    ++n;
    if (max == 0)
      break;
  }

  return n;
}

template <class Iterator>
inline size_t tokenize(char* str, const char* del, Iterator out, size_t max) {
  char* stre = str + std::strlen(str);
  const char* dele = del + std::strlen(del);
  size_t size = 0;

  while (size < max) {
    char* n = std::find_first_of(str, stre, del, dele);
    *n = '\0';
    *out++ = str;
    ++size;
    if (n == stre)
      break;
    str = n + 1;
  }

  return size;
}

// continus run of space is regarded as one space
template <class Iterator>
inline size_t tokenize2(char* str, const char* del, Iterator out, size_t max) {
  char* stre = str + std::strlen(str);
  const char* dele = del + std::strlen(del);
  size_t size = 0;

  while (size < max) {
    char* n = std::find_first_of(str, stre, del, dele);
    *n = '\0';
    if (*str != '\0') {
      *out++ = str;
      ++size;
    }
    if (n == stre)
      break;
    str = n + 1;
  }

  return size;
}

inline bool escape_csv_element(std::string* w) {
  if (w->find(',') != std::string::npos || w->find('"') != std::string::npos) {
    std::string tmp = "\"";
    for (size_t j = 0; j < w->size(); j++) {
      if ((*w)[j] == '"')
        tmp += '"';
      tmp += (*w)[j];
    }
    tmp += '"';
    *w = tmp;
  }
  return true;
}

inline int progress_bar(const char* message, size_t current, size_t total) {
  static char bar[] = "###########################################";
  static int scale = sizeof(bar) - 1;
  static int prev = 0;

  int cur_percentage = static_cast<int>(100.0 * current / total);
  int bar_len = static_cast<int>(1.0 * current * scale / total);

  if (prev != cur_percentage) {
    printf("%s: %3d%% |%.*s%*s| ", message, cur_percentage, bar_len, bar, scale - bar_len, "");
    if (cur_percentage == 100)
      printf("\n");
    else
      printf("\r");
    fflush(stdout);
  }

  prev = cur_percentage;

  return 1;
}

}  // namespace MeCab

#endif  // __MECAB_UTILS_STRING_UTILS_H__
