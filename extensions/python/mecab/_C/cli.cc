#include "cli.h"

#include "mecab/cost_trainer.h"
#include "mecab/dictionary_compiler.h"
#include "mecab/dictionary_generator.h"
#include "mecab/eval.h"
#include "mecab/utils.h"
#include "mecab/utils/param.h"
#include "mecab/utils/scoped_ptr.h"
#include "mecab/utils/stream_wrapper.h"

const std::vector<MeCab::Option> mecab_options{
    {"rcfile", 'r', "", "FILE", "use FILE as resource file"},
    {"dicdir", 'd', "", "DIR", "set DIR  as a system dicdir"},
    {"userdic", 'u', "", "FILE", "use FILE as a user dictionary"},
    {"lattice-level", 'l', "0", "INT", "lattice information level (DEPRECATED)"},
    {"dictionary-info", 'D', "", "", "show dictionary information and exit"},
    {"output-format-type", 'O', "", "TYPE", "set output format type (wakati,none,...)"},
    {"all-morphs", 'a', "", "", "output all morphs(default false)"},
    {"nbest", 'N', "1", "INT", "output N best results (default 1)"},
    {"partial", 'p', "", "", "partial parsing mode (default false)"},
    {"marginal", 'm', "", "", "output marginal probability (default false)"},
    {"max-grouping-size", 'M', "24", "INT", "maximum grouping size for unknown words (default 24)"},
    {"node-format", 'F', "%m\\t%H\\n", "STR", "use STR as the user-defined node format"},
    {"unk-format", 'U', "%m\\t%H\\n", "STR", "use STR as the user-defined unknown node format"},
    {"bos-format", 'B', "", "STR", "use STR as the user-defined beginning-of-sentence format"},
    {"eos-format", 'E', "EOS\\n", "STR", "use STR as the user-defined end-of-sentence format"},
    {"eon-format", 'S', "", "STR", "use STR as the user-defined end-of-NBest format"},
    {"unk-feature", 'x', "", "STR", "use STR as the feature for unknown word"},
    {"input-buffer-size", 'b', "", "INT", "set input buffer size (default 8192)"},
    {"dump-config", 'P', "", "", "dump MeCab parameters"},
    {"allocate-sentence", 'C', "", "", "allocate new memory for input sentence"},
    {"theta", 't', "0.75", "FLOAT", "set temparature parameter theta (default 0.75)"},
    {"cost-factor", 'c', "700", "INT", "set cost factor (default 700)"},
    {"output", 'o', "", "FILE", "set the output file name"}};

int mecab_main(int argc, char** argv) {
  MeCab::Param param;
  if (!param.parse(argc, argv, mecab_options)) {
    return EXIT_FAILURE;
  }

  if (param.get<bool>("help")) {
    std::cout << param.getHelpMessage() << std::endl;
    return EXIT_SUCCESS;
  }

  if (param.get<bool>("version")) {
    std::cout << param.getVersionMessage() << std::endl;
    return EXIT_SUCCESS;
  }

  if (!load_dictionary_resource(&param)) {
    return EXIT_SUCCESS;
  }

  if (param.get<int>("lattice-level") >= 1) {
    std::cerr << "lattice-level is DEPERCATED. "
              << "use --marginal or --nbest." << std::endl;
  }

  MeCab::scoped_ptr<MeCab::Model> model(MeCab::Model::create(param));
  if (model.get() == NULL) {
    return EXIT_FAILURE;
  }

  std::string ofilename = param.get<std::string>("output");
  if (ofilename.empty()) {
    ofilename = "-";
  }

  const int nbest = param.get<int>("nbest");
  CHECK_DIE(nbest > 0 && nbest <= NBEST_MAX) << "invalid N value";

  MeCab::ostream_wrapper ofs(ofilename.c_str());
  CHECK_DIE(*ofs) << "no such file or directory: " << ofilename;

  if (param.get<bool>("dump-config")) {
    param.dumpConfig(*ofs);
    return EXIT_FAILURE;
  }

  if (param.get<bool>("dictionary-info")) {
    for (const MeCab::DictionaryInfo* d = model->dictionary_info(); d; d = d->next) {
      *ofs << "filename:\t" << d->filename << std::endl;
      *ofs << "version:\t" << d->version << std::endl;
      *ofs << "charset:\t" << d->charset << std::endl;
      *ofs << "type:\t" << d->type << std::endl;
      *ofs << "size:\t" << d->size << std::endl;
      *ofs << "left size:\t" << d->lsize << std::endl;
      *ofs << "right size:\t" << d->rsize << std::endl;
      *ofs << std::endl;
    }
    return EXIT_FAILURE;
  }

  const std::vector<std::string>& rest_ = param.getRestParameters();
  std::vector<std::string> rest = rest_;

  if (rest.empty()) {
    rest.push_back("-");
  }

  size_t ibufsize =
      std::min(MAX_INPUT_BUFFER_SIZE, std::max(param.get<int>("input-buffer-size"), MIN_INPUT_BUFFER_SIZE));

  const bool partial = param.get<bool>("partial");
  if (partial) {
    ibufsize *= 8;
  }

  MeCab::scoped_array<char> ibuf_data(new char[ibufsize]);
  char* ibuf = ibuf_data.get();

  MeCab::scoped_ptr<MeCab::Tagger> tagger(MeCab::Tagger::create(model.get()));

  CHECK_DIE(tagger.get()) << "cannot create tagger";

  for (size_t i = 0; i < rest.size(); ++i) {
    MeCab::istream_wrapper ifs(rest[i].c_str());
    CHECK_DIE(*ifs) << "no such file or directory: " << rest[i];

    while (true) {
      if (!partial) {
        ifs->getline(ibuf, ibufsize);
      } else {
        std::string sentence;
        MeCab::scoped_fixed_array<char, BUF_SIZE> line;
        for (;;) {
          if (!ifs->getline(line.get(), line.size())) {
            ifs->clear(std::ios::eofbit | std::ios::badbit);
            break;
          }
          sentence += line.get();
          sentence += '\n';
          if (std::strcmp(line.get(), "EOS") == 0 || line[0] == '\0') {
            break;
          }
        }
        std::strncpy(ibuf, sentence.c_str(), ibufsize);
      }
      if (ifs->eof() && !ibuf[0]) {
        return false;
      }
      if (ifs->fail()) {
        std::cerr << "input-buffer overflow. "
                  << "The line is split. use -b #SIZE option." << std::endl;
        ifs->clear();
      }
      const char* r = (nbest >= 2) ? tagger->parseNBest(nbest, ibuf) : tagger->parse(ibuf);
      CHECK_DIE(r) << tagger->what();
      *ofs << r << std::flush;
    }
  }

  return EXIT_SUCCESS;
}

// exports
int mecab_system_eval(int argc, char** argv) {
  return MeCab::Eval::eval(argc, argv);
}

int mecab_test_gen(int argc, char** argv) {
  return MeCab::TestSentenceGenerator::run(argc, argv);
}

int mecab_dict_gen(int argc, char** argv) {
  return MeCab::DictionaryGenerator::run(argc, argv);
}

int mecab_dict_index(int argc, char** argv) {
  return MeCab::DictionaryComplier::run(argc, argv);
}

int mecab_cost_train(int argc, char** argv) {
  return MeCab::Learner::run(argc, argv);
}
