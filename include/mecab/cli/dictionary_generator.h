#ifndef __MECAB_DICTIONARY_GENERATOR_H__
#define __MECAB_DICTIONARY_GENERATOR_H__

#include <iostream>
#include <string>

#include "mecab/char_property.h"
#include "mecab/common.h"
#include "mecab/context_id.h"
#include "mecab/dictionary.h"
#include "mecab/dictionary_rewriter.h"
#include "mecab/feature_index.h"
#include "mecab/learner_node.h"
#include "mecab/mmap.h"

// used in mecab/_C/cli/dictionary_generator.cc

namespace MeCab {

inline void copy(const char* src, const char* dst) {
  std::cout << "copying " << src << " to " << dst << std::endl;
  Mmap<char> mmap;
  CHECK_DIE(mmap.open(src));
  std::ofstream ofs(dst, std::ios::binary | std::ios::out);
  CHECK_DIE(ofs) << "permission denied: " << dst;
  ofs.write(reinterpret_cast<char*>(mmap.begin()), mmap.size());
  ofs.close();
}

class DictionaryGenerator {
 private:
  static void gencid_bos(const std::string& bos_feature, DictionaryRewriter* rewrite, ContextID* cid) {
    std::string ufeature, lfeature, rfeature;
    rewrite->rewrite2(bos_feature, &ufeature, &lfeature, &rfeature);
    cid->addBOS(lfeature.c_str(), rfeature.c_str());
  }

  static void gencid(const char* filename, DictionaryRewriter* rewrite, ContextID* cid) {
    std::ifstream ifs(filename);
    CHECK_DIE(ifs) << "no such file or directory: " << filename;
    scoped_fixed_array<char, BUF_SIZE> line;
    std::cout << "reading " << filename << " ... " << std::flush;
    size_t num = 0;
    std::string feature, ufeature, lfeature, rfeature;
    char* col[8];
    while (ifs.getline(line.get(), line.size())) {
      const size_t n = tokenizeCSV(line.get(), col, 5);
      CHECK_DIE(n == 5) << "format error: " << line.get();
      feature = col[4];
      rewrite->rewrite2(feature, &ufeature, &lfeature, &rfeature);
      cid->add(lfeature.c_str(), rfeature.c_str());
      ++num;
    }
    std::cout << num << std::endl;
    ifs.close();
  }

  static bool genmatrix(const char* filename, const ContextID& cid, DecoderFeatureIndex* fi, int factor) {
    std::ofstream ofs(filename);
    CHECK_DIE(ofs) << "permission denied: " << filename;

    LearnerPath path;
    LearnerNode rnode;
    LearnerNode lnode;
    rnode.stat = lnode.stat = MECAB_NOR_NODE;
    rnode.rpath = &path;
    lnode.lpath = &path;
    path.lnode = &lnode;
    path.rnode = &rnode;

    const std::map<std::string, int>& left = cid.left_ids();
    const std::map<std::string, int>& right = cid.right_ids();

    CHECK_DIE(left.size() > 0) << "left id size is empty";
    CHECK_DIE(right.size() > 0) << "right id size is empty";

    ofs << right.size() << ' ' << left.size() << std::endl;

    size_t l = 0;
    for (std::map<std::string, int>::const_iterator rit = right.begin(); rit != right.end(); ++rit) {
      ++l;
      progress_bar("emitting matrix      ", l + 1, right.size());
      for (std::map<std::string, int>::const_iterator lit = left.begin(); lit != left.end(); ++lit) {
        path.rnode->wcost = 0;
        fi->buildBigramFeature(&path, rit->first.c_str(), lit->first.c_str());
        fi->calcCost(&path);
        ofs << rit->second << ' ' << lit->second << ' ' << tocost(path.cost, factor) << '\n';
      }
    }

    return true;
  }

  static void gendic(const char* ifile,
                     const char* ofile,
                     const CharProperty& property,
                     DictionaryRewriter* rewrite,
                     const ContextID& cid,
                     DecoderFeatureIndex* fi,
                     bool unk,
                     int factor) {
    std::ifstream ifs(ifile);
    CHECK_DIE(ifs) << "no such file or directory: " << ifile;

    std::ofstream ofs(ofile);
    CHECK_DIE(ofs) << "permission denied: " << ofile;

    std::cout << "emitting " << ofile << " ... " << std::flush;

    LearnerPath path;
    LearnerNode rnode;
    LearnerNode lnode;
    rnode.stat = lnode.stat = MECAB_NOR_NODE;
    rnode.rpath = &path;
    lnode.lpath = &path;
    path.lnode = &lnode;
    path.rnode = &rnode;

    scoped_fixed_array<char, BUF_SIZE> line;
    char* col[8];
    size_t num = 0;

    while (ifs.getline(line.get(), line.size())) {
      const size_t n = tokenizeCSV(line.get(), col, 5);
      CHECK_DIE(n == 5) << "format error: " << line.get();

      std::string w = std::string(col[0]);
      const std::string feature = std::string(col[4]);

      std::string ufeature, lfeature, rfeature;
      rewrite->rewrite2(feature, &ufeature, &lfeature, &rfeature);
      const int lid = cid.lid(lfeature.c_str());
      const int rid = cid.rid(rfeature.c_str());

      CHECK_DIE(lid > 0) << "CID is not found for " << lfeature;
      CHECK_DIE(rid > 0) << "CID is not found for " << rfeature;

      if (unk) {
        const int c = property.id(w.c_str());
        CHECK_DIE(c >= 0) << "unknown property [" << w << "]";
        path.rnode->char_type = static_cast<unsigned char>(c);
      } else {
        size_t mblen = 0;
        const CharInfo cinfo = property.getCharInfo(w.c_str(), w.c_str() + w.size(), &mblen);
        path.rnode->char_type = cinfo.default_type;
      }

      fi->buildUnigramFeature(&path, ufeature.c_str());
      fi->calcCost(&rnode);
      CHECK_DIE(escape_csv_element(&w)) << "invalid character found: " << w;

      ofs << w << ',' << lid << ',' << rid << ',' << tocost(rnode.wcost, factor) << ',' << feature << '\n';
      ++num;
    }

    std::cout << num << std::endl;
  }

 public:
  static int run(int argc, char** argv) {
    static const std::vector<MeCab::Option> long_options{
        {"dicdir", 'd', ".", "DIR", "set DIR as dicdir(default \".\" )"},
        {"outdir", 'o', ".", "DIR", "set DIR as output dir"},
        {"model", 'm', "", "FILE", "use FILE as model file"}};

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

    ContextID cid;
    DecoderFeatureIndex fi;
    DictionaryRewriter rewrite;

    const std::string dicdir = param.get<std::string>("dicdir");
    const std::string outdir = param.get<std::string>("outdir");
    const std::string model = param.get<std::string>("model");

#define DCONF(file) create_filename(dicdir, std::string(file)).c_str()
#define OCONF(file) create_filename(outdir, std::string(file)).c_str()

    CHECK_DIE(param.parseFile(DCONF(DICRC))) << "no such file or directory: " << DCONF(DICRC);

    std::string charset;
    {
      Dictionary dic;
      CHECK_DIE(dic.open(DCONF(SYS_DIC_FILE), "r"));
      charset = dic.charset();
      CHECK_DIE(!charset.empty());
    }

    CharProperty property;
    property.open(create_filename(param.get<std::string>("dicdir"), CHAR_PROPERTY_FILE));
    property.set_charset(charset.c_str());

    const std::string bos = param.get<std::string>("bos-feature");
    const int factor = param.get<int>("cost-factor");

    std::vector<std::string> dic;
    get_all_csvs_in_directory(dicdir.c_str(), dic);

    {
      CHECK_DIE(dicdir != outdir) << "output directory = dictionary directory! "
                                     "Please specify different directory.";
      CHECK_DIE(!outdir.empty()) << "output directory is empty";
      CHECK_DIE(!model.empty()) << "model file is empty";
      CHECK_DIE(fi.open(param)) << "cannot open feature index";
      CHECK_DIE(factor > 0) << "cost factor needs to be positive value";
      CHECK_DIE(!bos.empty()) << "bos-feature is empty";
      CHECK_DIE(dic.size()) << "no dictionary is found in " << dicdir;
      CHECK_DIE(rewrite.open(DCONF(REWRITE_FILE)));
    }

    gencid_bos(bos, &rewrite, &cid);
    gencid(DCONF(UNK_DEF_FILE), &rewrite, &cid);

    for (std::vector<std::string>::const_iterator it = dic.begin(); it != dic.end(); ++it) {
      gencid(it->c_str(), &rewrite, &cid);
    }

    std::cout << "emitting " << OCONF(LEFT_ID_FILE) << "/ " << OCONF(RIGHT_ID_FILE) << std::endl;

    cid.build();
    cid.save(OCONF(LEFT_ID_FILE), OCONF(RIGHT_ID_FILE));

    gendic(DCONF(UNK_DEF_FILE), OCONF(UNK_DEF_FILE), property, &rewrite, cid, &fi, true, factor);

    for (std::vector<std::string>::const_iterator it = dic.begin(); it != dic.end(); ++it) {
      std::string file = *it;
      file = remove_pathname(file);
      gendic(it->c_str(), OCONF(file.c_str()), property, &rewrite, cid, &fi, false, factor);
    }

    genmatrix(OCONF(MATRIX_DEF_FILE), cid, &fi, factor);

    copy(DCONF(CHAR_PROPERTY_DEF_FILE), OCONF(CHAR_PROPERTY_DEF_FILE));
    copy(DCONF(REWRITE_FILE), OCONF(REWRITE_FILE));
    copy(DCONF(DICRC), OCONF(DICRC));
    copy(DCONF(FEATURE_FILE), OCONF(FEATURE_FILE));
    copy(model.c_str(), OCONF(MODEL_DEF_FILE));

#undef OCONF
#undef DCONF

    std::cout << "\ndone!" << std::endl;

    return 0;
  }
};
}  // namespace MeCab

#endif  // __MECAB_DICTIONARY_GENERATOR_H__
