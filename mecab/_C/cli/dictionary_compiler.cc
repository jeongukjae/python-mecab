#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "mecab.h"
#include "mecab/char_property.h"
#include "mecab/connector.h"
#include "mecab/dictionary.h"
#include "mecab/dictionary_rewriter.h"
#include "mecab/feature_index.h"
#include "mecab/param.h"

#include "cli.h"

namespace MeCab {

class DictionaryComplier {
 public:
  static int run(int argc, char** argv) {
    const std::vector<MeCab::Option> long_options{
        {"dicdir", 'd', ".", "DIR", "set DIR as dic dir (default \".\")"},
        {"outdir", 'o', ".", "DIR", "set DIR as output dir (default \".\")"},
        {"model", 'm', "", "FILE", "use FILE as model file"},
        {"userdic", 'u', "", "FILE", "build user dictionary"},
        {"assign-user-dictionary-costs", 'a', "", "", "only assign costs/ids to user dictionary"},
        {"build-unknown", 'U', "", "", "build parameters for unknown words"},
        {"build-model", 'M', "", "", "build model file"},
        {"build-charcategory", 'C', "", "", "build character category maps"},
        {"build-sysdic", 's', "", "", "build system dictionary"},
        {"build-matrix", 'm', "", "", "build connection matrix"},
        {"charset", 'c', MECAB_DEFAULT_CHARSET, "ENC",
         "make charset of binary dictionary ENC (default " MECAB_DEFAULT_CHARSET ")"},
        {"charset", 't', MECAB_DEFAULT_CHARSET, "ENC", "alias of -c"},
        {"dictionary-charset", 'f', MECAB_DEFAULT_CHARSET, "ENC",
         "assume charset of input CSVs as ENC (default " MECAB_DEFAULT_CHARSET ")"},
        {
            "wakati",
            'w',
            "",
            "",
            "build wakati-gaki only dictionary",
        },
        {"posid", 'p', "", "", "assign Part-of-speech id"},
        {"node-format", 'F', "", "STR", "use STR as the user defined node format"}};

    Param param;

    if (!param.parse(argc, argv, long_options)) {
      std::cout << "\n\n" << COPYRIGHT << "\ntry '--help' for more information." << std::endl;
      return -1;
    }

    if (param.get<bool>("help")) {
      std::cout << param.getHelpMessage() << std::endl;
      return 0;
    }
    if (param.get<bool>("version")) {
      std::cout << param.getVersionMessage() << std::endl;
      return 0;
    }

    const std::string dicdir = param.get<std::string>("dicdir");
    const std::string outdir = param.get<std::string>("outdir");
    bool opt_unknown = param.get<bool>("build-unknown");
    bool opt_matrix = param.get<bool>("build-matrix");
    bool opt_charcategory = param.get<bool>("build-charcategory");
    bool opt_sysdic = param.get<bool>("build-sysdic");
    bool opt_model = param.get<bool>("build-model");
    bool opt_assign_user_dictionary_costs = param.get<bool>("assign-user-dictionary-costs");
    const std::string userdic = param.get<std::string>("userdic");

#define DCONF(file) create_filename(dicdir, std::string(file)).c_str()
#define OCONF(file) create_filename(outdir, std::string(file)).c_str()

    CHECK_DIE(param.parseFile(DCONF(DICRC))) << "no such file or directory: " << DCONF(DICRC);

    std::vector<std::string> dic;
    if (userdic.empty()) {
      enum_csv_dictionaries(dicdir.c_str(), &dic);
    } else {
      dic = param.getRestParameters();
    }

    if (!userdic.empty()) {
      CHECK_DIE(dic.size()) << "no dictionaries are specified";
      param.set("type", std::to_string(MECAB_USR_DIC));
      if (opt_assign_user_dictionary_costs) {
        Dictionary::assignUserDictionaryCosts(param, dic, userdic.c_str());
      } else {
        Dictionary::compile(param, dic, userdic.c_str());
      }
    } else {
      if (!opt_unknown && !opt_matrix && !opt_charcategory && !opt_sysdic && !opt_model) {
        opt_unknown = opt_matrix = opt_charcategory = opt_sysdic = opt_model = true;
      }

      if (opt_charcategory || opt_unknown) {
        CharProperty::compile(DCONF(CHAR_PROPERTY_DEF_FILE), DCONF(UNK_DEF_FILE), OCONF(CHAR_PROPERTY_FILE));
      }

      if (opt_unknown) {
        std::vector<std::string> tmp;
        tmp.push_back(DCONF(UNK_DEF_FILE));
        param.set("type", std::to_string(MECAB_UNK_DIC));
        Dictionary::compile(param, tmp, OCONF(UNK_DIC_FILE));
      }

      if (opt_model) {
        if (file_exists(DCONF(MODEL_DEF_FILE))) {
          FeatureIndex::compile(param, DCONF(MODEL_DEF_FILE), OCONF(MODEL_FILE));
        } else {
          std::cout << DCONF(MODEL_DEF_FILE) << " is not found. skipped." << std::endl;
        }
      }

      if (opt_sysdic) {
        CHECK_DIE(dic.size()) << "no dictionaries are specified";
        param.set("type", std::to_string(MECAB_SYS_DIC));
        Dictionary::compile(param, dic, OCONF(SYS_DIC_FILE));
      }

      if (opt_matrix) {
        Connector::compile(DCONF(MATRIX_DEF_FILE), OCONF(MATRIX_FILE));
      }
    }

    std::cout << "\ndone!" << std::endl;

    return 0;
  }
};

#undef DCONF
#undef OCONF
}  // namespace MeCab

// export functions
int mecab_dict_index(int argc, char** argv) {
  return MeCab::DictionaryComplier::run(argc, argv);
}
