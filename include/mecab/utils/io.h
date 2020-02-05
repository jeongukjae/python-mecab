#ifndef __MECAB_UTILS_IO_H__
#define __MECAB_UTILS_IO_H__

#include <dirent.h>

#include "mecab/param.h"
#include "mecab/utils/string_utils.h"

namespace MeCab {

inline bool file_exists(const char* filename) {
  std::ifstream ifs(filename);
  if (!ifs) {
    return false;
  }
  return true;
}

inline bool load_dictionary_resource(Param* param) {
  std::string rcfile = param->get<std::string>("rcfile");

  if (rcfile.empty()) {
    const char* homedir = getenv("HOME");
    if (homedir) {
      const std::string s = create_filename(std::string(homedir), ".mecabrc");
      std::ifstream ifs(s.c_str());
      if (ifs) {
        rcfile = s;
      }
    }
  }

  if (rcfile.empty()) {
    const char* rcenv = getenv("MECABRC");
    if (rcenv) {
      rcfile = rcenv;
    }
  }

  if (rcfile.empty()) {
    rcfile = MECAB_DEFAULT_RC;
  }

  if (!param->parseFile(rcfile.c_str())) {
    return false;
  }

  std::string dicdir = param->get<std::string>("dicdir");
  if (dicdir.empty()) {
    dicdir = ".";  // current
  }
  std::string rcpath = remove_filename(rcfile);
  dicdir = replace_string(dicdir, "$(rcpath)", rcpath);
  param->set("dicdir", dicdir, true);
  std::string dicrc = create_filename(dicdir, DICRC);

  return param->parseFile(dicrc);
}

inline void enum_csv_dictionaries(const char* path, std::vector<std::string>* dics) {
  dics->clear();

  DIR* dir = opendir(path);
  CHECK_DIE(dir) << "no such directory: " << path;

  for (struct dirent* dp = readdir(dir); dp; dp = readdir(dir)) {
    const std::string tmp = dp->d_name;
    if (tmp.size() >= 5) {
      std::string ext = tmp.substr(tmp.size() - 4, 4);
      if (to_lower(ext) == ".csv") {
        dics->push_back(create_filename(path, tmp));
      }
    }
  }
  closedir(dir);
}

}  // namespace MeCab

#endif  // __MECAB_UTILS_IO_H__
