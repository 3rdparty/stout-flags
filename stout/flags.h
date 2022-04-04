#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "absl/time/time.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/text_format.h"
#include "stout/v1/flag.pb.h"
#include "stout/v1/positional_argument.pb.h"
#include "stout/v1/subcommand.pb.h"

////////////////////////////////////////////////////////////////////////

namespace stout::flags {

////////////////////////////////////////////////////////////////////////

// Forward declaration.
template <typename Flags>
class ParserBuilder;

class Parser {
 public:
  // Returns a builder for a parser based on the specifid flags; see
  // 'ParserBuilder' for more details on what can be modified from the
  // default parser.
  template <typename Flags>
  static ParserBuilder<Flags> Builder(Flags* flags);

  // Parse flags from 'argc' and 'argv' and modify 'argc' and 'argv'
  // with what ever flags or arguments were not parsed.
  void Parse(int* argc, const char*** argv);

 private:
  template <typename Flags>
  friend class ParserBuilder;

  Parser()
    : standard_flags_(new stout::v1::StandardFlags()) {
    // Add all the "standard flags" first, e.g., --help.
    AddAllOrExit(standard_flags_.get());
  }

  // Helper that adds all the flags and their descriptors.
  void AddAllOrExit(google::protobuf::Message* message);

  // Try to fill messages_ and fields_ helpers depending on the extension
  // that users specify (stout::v1::flag or stout::v1::argument are allowed).
  template <typename T>
  void TryFillFieldAndMessageHelpers(
      const T* field_extension,
      const google::protobuf::FieldDescriptor* field,
      google::protobuf::Message* message) {
    /* clang-format off */
    if (!std::is_same_v<const stout::v1::Flag*, decltype(field_extension)> &&
        !std::is_same_v<const stout::v1::Argument*,
                        decltype(field_extension)>){
      std::cerr << "Expected 'const stout::v1::Flag*' " 
                << "or 'const stout::v1::Argument*' type" << std::endl;
      exit(1);
    }
    /* clang-format on */
    if (field_extension->names().empty()) {
      std::cerr
          << "Missing at least one name in 'names' for field '"
          << field->full_name() << "'"
          << std::endl;
      std::exit(1);
    }

    if (field_extension->help().empty()) {
      std::cerr
          << "Missing 'help' for field '"
          << field->full_name() << "'"
          << std::endl;
      std::exit(1);
    }

    for (const auto& name : field_extension->names()) {
      AddOrExit(name, field, message);
    }

    for (const auto& name : field_extension->deprecated_names()) {
      AddOrExit(name, field, message);
    }
  }

  // Helper that adds a flag and it's descriptor.
  void AddOrExit(
      const std::string& name,
      const google::protobuf::FieldDescriptor* field,
      google::protobuf::Message* message);

  // Check if 'arg' passed as an argument is the name of the subcommand.
  // If so the succeeding arguments will be parsed as subcommand flags
  // untill will be encountered another subcommand or the end of the
  // arguments.
  google::protobuf::Message* TryParseSubcommand(const std::string& arg);

  // Helper that returns field pointer if current argument is a positional
  // argument.
  const google::protobuf::FieldDescriptor* GetFieldForPositionalArgument(
      const std::string& arg);

  // Helper for parsing a normalized form of flags.
  void Parse(
      const std::multimap<std::string, std::optional<std::string>>& values);

  // Helper for parsing positional arguments.
  void TryParsePositionalArguments();

  void FillTopLevelHelpNode(google::protobuf::Message* message);

  struct SubcommandHelp;

  void FillHelpNode(
      const google::protobuf::Descriptor* descriptor,
      std::vector<SubcommandHelp>& subcommands,
      std::size_t indent);

  // Fill help_nodes_ for the message.
  void FillHelpNodes();

  template <typename T>
  std::pair<std::string, std::string> GetHelpInfoFromField(
      const T* field_extension,
      const google::protobuf::FieldDescriptor* field) {
    /* clang-format off */
    if (!std::is_same_v<const stout::v1::Flag*, decltype(field_extension)> &&
        !std::is_same_v<const stout::v1::Argument*,
                        decltype(field_extension)> &&
        !std::is_same_v<const stout::v1::Subcommand*,
                        decltype(field_extension)>){
      std::cerr << "Expected 'const stout::v1::Flag*' " 
                << "or 'const stout::v1::Argument*' type"
                << "or 'const stout::v1::Subcommand*' type"
                << std::endl;
      exit(1);
    }
    /* clang-format on */

    std::string flag_name;

    const bool boolean =
        field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL;

    std::string prefix = (boolean) ? "--(no-)" : "--";
    std::string postfix{""};

    if (field->options().HasExtension(stout::v1::argument)
        || field->options().HasExtension(stout::v1::subcommand)) {
      prefix = "";
    }
    if (field->options().HasExtension(stout::v1::flag) && !boolean) {
      postfix += "=...";
    }

    for (int i = 0; i < field_extension->names().size(); ++i) {
      if (!i) {
        flag_name += prefix + field_extension->names()[i] + postfix;
      } else {
        flag_name += ", " + prefix + field_extension->names()[i] + postfix;
      }
    }

    return std::pair<std::string, std::string>{
        flag_name,
        field_extension->help()};
  }

  // Helper that prints out help for the flags for this parser.
  void PrintHelp();

  // NOTE: need to heap allocate with a 'std::unique_ptr' here because
  // we 'std::move()' the parser but also store a pointer to
  // 'standard_flags_' so that pointer must be stable. This allocation
  // will not likely be in the fast path but if it is this design
  // should be reconsidered.
  std::unique_ptr<stout::v1::StandardFlags> standard_flags_;

  // Map from flag name to the field descriptor for the flag.
  std::map<std::string, const google::protobuf::FieldDescriptor*> fields_;

  // Map from field pointer to the value of a positional argument.
  std::map<
      const google::protobuf::FieldDescriptor*,
      std::string>
      pos_arg_fields_;

  // Map from the field descriptor to the 'google::protobuf::Message'
  // that we will use to update the flag value via reflection.
  std::map<
      const google::protobuf::FieldDescriptor*,
      google::protobuf::Message*>
      messages_;

  // Map from message descriptors to functions that overload the
  // default parsing for that descriptor.
  std::map<
      const google::protobuf::Descriptor*,
      std::function<
          std::optional<std::string>(
              const std::string&,
              google::protobuf::Message*)>>
      overload_parsing_;

  // Map from help string to function that we use to validate the
  // parsed flags.
  std::map<std::string, std::function<bool()>> validate_;

  // Name of the program that we extracted from 'argv'.
  std::string program_name_;

  // Helper struct for storing the parsed "name" and normalized
  // protobuf "text" for a flag. This is used for handling possible
  // duplicates.
  struct Parsed {
    std::string name;
    std::string text;
  };

  struct SubcommandHelp {
    std::size_t indent;
    const google::protobuf::FieldDescriptor* field;
  };

  // Helper struct for printing the message's help.
  struct HelpNode {
    // Descriptor for the current level subcommand.
    const google::protobuf::Descriptor* descriptor;

    // Vector for containing subcommand descriptor's chain.
    std::vector<SubcommandHelp> subcommands;
  };

  // Vector for containing all recursive nodes of the message
  // for printing help.
  std::vector<HelpNode> help_nodes_;

  // Map from field descriptor to the helper struct 'Parsed' for
  // capturing what flags or positional arguments have already
  // been parsed.
  std::map<const google::protobuf::FieldDescriptor*, Parsed> parsed_;

  // Optional for including all environment variables for parsing
  // with specific prefix.
  std::optional<std::string> environment_variable_prefix_;

  // Helper for saving the current message for which subcommand flags are
  // being parsed.
  std::optional<google::protobuf::Message*> cur_message_;

  // Helper for saving the previous message for which subcommand flags are
  // being parsed. It's always top message at the beginning.
  std::optional<google::protobuf::Message*> previous_message_;

  // Helper for avoiding duplicate subcommands.
  std::set<std::string> subcommands_;

  // Current index of the positional argument to parse. Will be always
  // increased when a new positional argument arrives.
  std::size_t cur_index_pos_arg_ = 1;

  const std::size_t step_ = 5;

  // This flag will be set to true when the first subcommand was encountered.
  bool parsing_subcommand_flags_ = false;
};

////////////////////////////////////////////////////////////////////////

template <typename Flags>
class ParserBuilder {
 public:
  ParserBuilder(Flags* flags)
    : flags_(flags) {
    parser_.AddAllOrExit(flags_);
    parser_.FillTopLevelHelpNode(flags_);
    parser_.FillHelpNodes();
  }

  // Overloads the parsing of the specified type 'T' with the
  // specified function. Note that 'T' must be a protobuf.
  template <typename T, typename F>
  ParserBuilder& OverloadParsing(F&& f) {
    if (!TryOverloadParsing<T>(std::forward<F>(f))) {
      std::cerr
          << "Encountered more than one overload parsing for "
          << T().GetDescriptor()->full_name()
          << std::endl;
      std::exit(1);
    }

    return *this;
  }

  // Adds a validation function to be invoked that will print out the
  // specified help message if validation fails.
  template <typename F>
  ParserBuilder& Validate(std::string&& help, F&& f) {
    parser_.validate_.emplace(
        std::move(help),
        [f = std::forward<F>(f), flags = flags_]() {
          return f(*flags);
        });

    return *this;
  }

  // Returns a parser.
  Parser Build() {
    // Try to overload parsing of 'google.protobuf.Duration' flag
    // fields and ignore if already overloaded.
    TryOverloadParsing<google::protobuf::Duration>(
        [](const std::string& value, auto* duration) {
          absl::Duration d;
          std::string error;
          if (!absl::AbslParseFlag(value, &d, &error)) {
            return std::optional<std::string>(error);
          } else {
            duration->set_seconds(
                absl::IDivDuration(d, absl::Seconds(1), &d));
            duration->set_nanos(
                absl::IDivDuration(d, absl::Nanoseconds(1), &d));
            return std::optional<std::string>();
          }
        });

    parser_.cur_message_ = flags_;
    parser_.previous_message_ = parser_.cur_message_;
    return std::move(parser_);
  }

  // Enable parsing environment variable.
  ParserBuilder& IncludeEnvironmentVariablesWithPrefix(
      const std::string& prefix) {
    if (parser_.environment_variable_prefix_) {
      std::cerr << "Redundant environment variable prefix '"
                << prefix << "'; already have '"
                << parser_.environment_variable_prefix_.value() << "'"
                << std::endl;
      std::exit(1);
    }
    parser_.environment_variable_prefix_ = prefix;
    return *this;
  }

 private:
  template <typename, typename = void>
  struct HasGetDescriptor : std::false_type {};

  template <typename T>
  struct HasGetDescriptor<
      T,
      std::void_t<decltype(std::declval<T>().GetDescriptor())>>
    : std::true_type {};

  template <typename T, typename F>
  bool TryOverloadParsing(F&& f) {
    static_assert(
        HasGetDescriptor<T>::value,
        "can only overload parsing of message types "
        "(not primitives or 'string')");

    const auto* descriptor = T().GetDescriptor();

    if (parser_.overload_parsing_.count(descriptor) == 0) {
      parser_.overload_parsing_[descriptor] =
          [f = std::forward<F>(f)](
              const std::string& value,
              google::protobuf::Message* message) {
            return f(value, dynamic_cast<T*>(message));
          };
      return true;
    } else {
      return false;
    }
  }

 private:
  Flags* flags_ = nullptr;
  Parser parser_;
};

////////////////////////////////////////////////////////////////////////

// Defined after 'ParserBuilder' to deal with circular dependency.
template <typename Flags>
ParserBuilder<Flags> Parser::Builder(Flags* flags) {
  return ParserBuilder<Flags>(flags);
}

////////////////////////////////////////////////////////////////////////

} // namespace stout::flags

////////////////////////////////////////////////////////////////////////
