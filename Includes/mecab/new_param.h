#ifndef _MECAB_NEW_PARAM_H_
#define _MECAB_NEW_PARAM_H_

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "mecab/common.h"

namespace MeCab {

struct Option {
  std::string optionName;
  char shortOption;
  std::string argName;
  std::string description;
  std::string defaultValue;

  Option& operator=(Option const o) {
    optionName = o.optionName;
    shortOption = o.shortOption;
    argName = o.argName;
    description = o.description;

    return *this;
  }
};

std::vector<Option> defaultOptions{{"help", 'h', "", "print help message and exit", ""},
                                   {"version", 'v', "", "print version and exit", ""}};

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
Target castString(std::string arg) {
  std::stringstream interpreter;
  Target result;
  if (!(interpreter << arg) || !(interpreter >> result) || !(interpreter >> std::ws).eof())
    return (Target) nullptr;
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

    for (auto& argument : arguments) {
      if (argument.size() == 0)
        continue;

      if (argument.rfind("--", 0) == 0) {
        size_t end = argument.find_first_of("=");
        if (end == std::string::npos)
          end = argument.length();

        std::string key = argument.substr(2, end);
        auto iterator = longOptionMap.find(key);
        if (iterator == longOptionMap.end())
          return printArgError(UNRECOGNIZED, argument);

        Option selected = *iterator->second;
        if (selected.argName.empty()) {
          // this argument does not need arguments, but passed
          if (argument.length() != selected.optionName.length() + 2)
            return printArgError(NO_ARG, argument);

          configurations[selected.optionName] = "1";
        } else {
          // TODO
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
        } else {
          // TODO
        }
      } else {
        restParameters.push_back(argument);
      }
    }

    return true;
  }

  bool parse(int argc, char** const argv, std::vector<Option> options) {
    std::vector<std::string> arguments;
    arguments.reserve(argc);

    for (int i = 0; i < argc; ++i)
      arguments.push_back(std::string(argv[i]));

    return parse(arguments, options);
  }

  void set(const std::string key, const std::string value, bool override = true) {
    if (override || (configurations.find(key) == configurations.end()))
      configurations[key] = value;
  }

  template <class Target>
  Target get(std::string key) const {
    auto iterator = configurations.find(key);
    if (iterator == configurations.end())
      return (Target) nullptr;
    return castString<Target>(iterator->second);
  }

  const std::string getCommandName() const { return commandName; }
  const std::string getHelpMessage() const { return helpMessage; }
  const std::string getVersionMessage() const { return versionMessage; }
};
}  // namespace MeCab
#endif  // _MECAB_NEW_PARAM_H_
