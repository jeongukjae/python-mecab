#ifndef _MECAB_NEW_PARAM_H_
#define _MECAB_NEW_PARAM_H_

#include <algorithm>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "mecab/common.h"

namespace MeCab {

struct Option {
  std::string optionName;
  char shortOption;
  std::string defaultValue;
  std::string argName;
  std::string description;

  Option& operator=(Option const o) {
    optionName = o.optionName;
    shortOption = o.shortOption;
    argName = o.argName;
    description = o.description;

    return *this;
  }
};

static std::vector<Option> defaultOptions{{"help", 'h', "", "", "print help message and exit"},
                                          {"version", 'v', "", "", "print version and exit"}};

namespace {

inline size_t getLengthOfOption(Option option) {
  if (option.argName.empty()) {
    // +1 : space between displayname and description
    return option.optionName.length() + 1;
  }
  // +1
  // * space between displayname and description
  // * '=' character
  return option.optionName.length() + option.argName.length() + 2;
}

template <class Target>
Target getDefaultValue() {
  return (Target)NULL;
}

template <>
std::string getDefaultValue() {
  return "";
}

template <class Target>
Target castString(std::string arg) {
  std::stringstream interpreter;
  Target result;
  if (!(interpreter << arg) || !(interpreter >> result) || !(interpreter >> std::ws).eof())
    return getDefaultValue<Target>();
  return result;
}

template <>
std::string castString(std::string arg) {
  return arg;
}

enum ArgError { UNRECOGNIZED, REQUIRE_ARG, NO_ARG };

bool printArgError(const ArgError error, const std::string option) {
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
  std::string commandName;
  std::string helpMessage;
  std::string versionMessage;

  std::vector<std::string> restParameters;
  std::unordered_map<std::string, std::string> configurations;

  void constructHelpAndVersionMessage(const std::vector<Option> options) {
    std::vector<size_t> lengthOfOptions;
    std::transform(options.begin(), options.end(), std::back_inserter(lengthOfOptions), getLengthOfOption);
    size_t maxDisplayLength = *std::max_element(lengthOfOptions.begin(), lengthOfOptions.end());

    helpMessage = std::string(COPYRIGHT) + "\nUsage: " + commandName + " [options] files\n";
    versionMessage = std::string(PACKAGE) + " of " + VERSION + '\n';

    // append options to help message
    for (const auto& option : options) {
      size_t length = 0;

      helpMessage += " -";
      helpMessage.append(1, option.shortOption);
      helpMessage += ", --" + option.optionName;
      length += option.optionName.length();
      if (!option.argName.empty()) {
        helpMessage += "=" + option.argName;
        length += option.argName.length() + 1;
      }

      helpMessage.append(maxDisplayLength - length, ' ');
      helpMessage += option.description + "\n";
    }
  }

  void setDefaultValue(const std::vector<Option> options) {
    for (const auto& option : options)
      if (!option.defaultValue.empty())
        configurations[option.optionName] = option.defaultValue;
  }

 public:
  explicit Param() {}
  ~Param() {}

  bool parse(std::vector<std::string> arguments, std::vector<Option> options) {
    std::unordered_map<std::string, Option*> longOptionMap;
    std::unordered_map<char, Option*> shortOptionMap;

    options.insert(options.end(), defaultOptions.begin(), defaultOptions.end());
    if (arguments.size() == 0) {
      // parse nothing
      // this is not a bug
      commandName = "unknown";
      return true;
    }

    commandName = std::string(arguments.at(0));
    constructHelpAndVersionMessage(options);
    setDefaultValue(options);

    for (auto& option : options) {
      longOptionMap[option.optionName] = &option;
      shortOptionMap[option.shortOption] = &option;
    }

    Option* optionForNextArg = nullptr;

    for (auto& argument : arguments) {
      if (*arguments.begin() == argument || argument.size() == 0)
        continue;

      if (optionForNextArg != nullptr) {
        configurations[optionForNextArg->optionName] = argument;
        optionForNextArg = nullptr;
        continue;
      }

      if (argument.rfind("--", 0) == 0) {
        size_t end = argument.find_first_of("=");
        if (end == std::string::npos)
          end = argument.length();

        std::string key = argument.substr(2, end - 2);
        auto iterator = longOptionMap.find(key);
        if (iterator == longOptionMap.end())
          return printArgError(UNRECOGNIZED, argument);

        Option selected = *iterator->second;
        if (selected.argName.empty()) {
          // this argument does not need arguments, but passed
          if (argument.length() != selected.optionName.length() + 2)
            return printArgError(NO_ARG, argument);

          configurations[selected.optionName] = "1";
        } else if (argument.length() == selected.optionName.length() + 2) {
          optionForNextArg = &selected;
        } else {
          auto value = argument.substr(selected.optionName.length() + 3);
          configurations[selected.optionName] = value;
        }
      } else if (argument.rfind("-", 0) == 0) {
        char shortArg = argument.at(1);

        auto iterator = shortOptionMap.find(shortArg);
        if (iterator == shortOptionMap.end())
          return printArgError(UNRECOGNIZED, argument);

        Option selected = *iterator->second;
        if (selected.argName.empty()) {
          // this argument does not need arguments, but passed
          if (argument.length() != 2)
            return printArgError(NO_ARG, argument);

          configurations[selected.optionName] = "1";
        } else if (argument.length() == 2) {
          // -a some-arg
          optionForNextArg = &selected;
        } else if (argument.at(2) == '=') {
          // -a=some-arg
          auto value = argument.substr(3);
          configurations[selected.optionName] = value;
        } else {
          auto value = argument.substr(2);
          configurations[selected.optionName] = value;
        }
      } else {
        restParameters.push_back(argument);
      }
    }

    if (optionForNextArg)
      return printArgError(REQUIRE_ARG, optionForNextArg->optionName);

    return true;
  }

  bool parse(int argc, char** const argv, std::vector<Option> options) {
    std::vector<std::string> arguments;
    arguments.reserve(argc);

    for (int i = 0; i < argc; ++i)
      arguments.push_back(std::string(argv[i]));

    return parse(arguments, options);
  }

  bool parse(const char* arg, const std::vector<Option> options) {
    std::string argumentString = std::string(PACKAGE) + " " + arg;
    std::vector<std::string> arguments;

    auto first = argumentString.begin();
    auto second = argumentString.begin();
    auto third = argumentString.begin();
    auto last = argumentString.end();

    for (; second != last && first != last; first = third) {
      second = std::find_if(first, last, std::iswspace);
      third = std::find_if_not(second, last, std::iswspace);

      if (first != second)
        arguments.push_back(std::string(first, second));
    }

    return parse(arguments, options);
  }

  bool parseFile(std::string filename) {
    std::ifstream ifs(filename);

    CHECK_FALSE(!ifs.fail()) << "cannot open file: " << filename;

    std::string line;
    while (std::getline(ifs, line)) {
      if (!line.size() || line[0] == ';' || line[0] == '#')
        continue;

      size_t pos = line.find_first_of('=');
      CHECK_FALSE(pos != std::string::npos) << "format error: " << line;

      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      key.erase(key.find_last_not_of(" \t\v") + 1);
      value.erase(0, value.find_first_not_of(" \t\v"));

      // do not override
      set(key, value, false);
    }

    return true;
  }

  void set(const std::string key, const std::string value, bool override = true) {
    auto found = configurations.find(key);
    if (override || (found == configurations.end()) || (found->second.empty()))
      configurations[key] = value;
  }

  template <class Target>
  Target get(std::string key) const {
    auto iterator = configurations.find(key);
    if (iterator == configurations.end())
      return getDefaultValue<Target>();
    return castString<Target>(iterator->second);
  }

  void dumpConfig(std::ostream& os = std::cout) const {
    for (auto it = configurations.begin(); it != configurations.end(); ++it) {
      os << it->first << ": " << it->second << std::endl;
    }
  }

  const std::vector<std::string>& getRestParameters() const { return restParameters; }

  const std::string getCommandName() const { return commandName; }
  const std::string getHelpMessage() const { return helpMessage; }
  const std::string getVersionMessage() const { return versionMessage; }

  void clear() {
    configurations.clear();
    restParameters.clear();
  }
};
}  // namespace MeCab
#endif  // _MECAB_NEW_PARAM_H_
