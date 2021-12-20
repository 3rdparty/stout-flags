#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "absl/time/time.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/text_format.h"
#include "stout/v1/flag.pb.h"

namespace stout {
namespace flags {

// Forward declaration.
template <typename Flags>
class ParserBuilder;

class Parser {
 public:
  template <typename Flags>
  static ParserBuilder<Flags> Builder(Flags* flags);

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
  void AddAllOrExit(google::protobuf::Message* message) {
    const auto* descriptor = message->GetDescriptor();

    for (size_t i = 0; i < descriptor->field_count(); i++) {
      const auto* field = descriptor->field(i);

      const auto& flag = field->options().GetExtension(stout::v1::flag);

      if (flag.names().empty()) {
        std::cerr
            << "Missing at least one flag name in 'names' for field '"
            << field->full_name() << "'"
            << std::endl;
        std::exit(1);
      }

      if (flag.help().empty()) {
        std::cerr
            << "Missing flag 'help' for field '" << field->full_name() << "'"
            << std::endl;
        std::exit(1);
      }

      for (const auto& name : flag.names()) {
        AddOrExit(name, field, message);
      }

      for (const auto& name : flag.deprecated_names()) {
        AddOrExit(name, field, message);
      }
    }
  }

  // Helper that adds a flag and it's descriptor.
  void AddOrExit(
      const std::string& name,
      const google::protobuf::FieldDescriptor* field,
      google::protobuf::Message* message) {
    auto [_, inserted] = fields_.emplace(name, field);

    if (!inserted) {
      std::cerr
          << "Encountered duplicate flag name '" << name << "' "
          << "for field '" << field->full_name() << "'"
          << std::endl;
      std::exit(1);
    }

    messages_.emplace(field, message);
  }

  void Parse(
      const std::multimap<std::string, std::optional<std::string>>& values);

  void PrintHelp();

  // NOTE: need to heap allocate with a 'std::unique_ptr' here because
  // we 'std::move()' the parser but also store a pointer to
  // 'standard_flags_' so that pointer must be stable. This allocation
  // will not likely be in the fast path but if it is this design
  // should be reconsidered.
  std::unique_ptr<stout::v1::StandardFlags> standard_flags_;

  google::protobuf::Message* message_;

  std::map<std::string, const google::protobuf::FieldDescriptor*> fields_;

  std::map<
      const google::protobuf::FieldDescriptor*,
      google::protobuf::Message*>
      messages_;

  std::map<
      const google::protobuf::Descriptor*,
      std::function<
          std::optional<std::string>(
              const std::string&,
              google::protobuf::Message*)>>
      overload_parsing_;

  std::map<
      std::string,
      std::function<
          bool(google::protobuf::Message*)>>
      validate_;

  std::string program_name_;

  // Helper struct for storing the parsed "name" and normalized
  // protobuf "text" for a flag. This is used for handling possible
  // duplicates.
  struct Parsed {
    std::string name;
    std::string text;
  };

  std::map<const google::protobuf::FieldDescriptor*, Parsed> parsed_;
};

template <typename Flags>
class ParserBuilder {
 public:
  ParserBuilder(google::protobuf::Message* message) {
    parser_.message_ = message;
    parser_.AddAllOrExit(parser_.message_);
  }

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

  template <typename F>
  ParserBuilder& Validate(std::string&& help, F&& f) {
    parser_.validate_.emplace(
        std::move(help),
        [f = std::forward<F>(f)](google::protobuf::Message* message) {
          return f(*dynamic_cast<Flags*>(message));
        });

    return *this;
  }

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

    return std::move(parser_);
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

  Parser parser_;
};

template <typename Flags>
ParserBuilder<Flags> Parser::Builder(Flags* flags) {
  return ParserBuilder<Flags>(flags);
}

} // namespace flags
} // namespace stout
