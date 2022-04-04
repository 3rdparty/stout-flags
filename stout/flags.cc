#include "stout/flags.h"

#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"

////////////////////////////////////////////////////////////////////////

// We need this global variable for parsing environment varables.
// See 'Parser::Parse()' for more details.
extern char** environ;

////////////////////////////////////////////////////////////////////////

namespace stout::flags {

////////////////////////////////////////////////////////////////////////

void Parser::AddAllOrExit(google::protobuf::Message* message) {
  const auto* descriptor = message->GetDescriptor();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const auto* field = descriptor->field(i);

    // We need this descriptor for the subcommand's logic.
    const google::protobuf::OneofDescriptor* real_oneof_field =
        field->real_containing_oneof();

    // Check if the current field of the message is in 'oneof'.
    if (real_oneof_field != nullptr) {
      // Check if the name of 'oneof' is 'subcommand'. If no -> exit with
      // an error.
      if (real_oneof_field->name() != "subcommand") {
        std::cerr << "'oneof' field must have 'subcommand' name. "
                  << "Other names are illegal"
                  << std::endl;
        std::exit(1);
      } else {
        // Subcommands must have stout.v1.subcommand extension.
        if (!field->options().HasExtension(stout::v1::subcommand)) {
          std::cerr << "Every field of the 'oneof subcommand' must "
                       "have (stout.v1.subcommand) extension"
                    << std::endl;
          std::exit(1);
        } else {
          // Check for missing 'names' and 'help'
          // in (stout.v1.subcommand) extension.
          const auto& subcommand =
              field->options().GetExtension(stout::v1::subcommand);

          if (subcommand.names().empty()) {
            std::cerr
                << "Missing at least one name in 'names' for field '"
                << field->full_name() << "'"
                << std::endl;
            std::exit(1);
          }

          if (subcommand.help().empty()) {
            std::cerr
                << "Missing 'help' for field '"
                << field->full_name() << "'"
                << std::endl;
            std::exit(1);
          }
        }
      }
    } else {
      if (field->options().HasExtension(stout::v1::subcommand)) {
        std::cerr << "(stout.v1.subcommand) extension should be inside only"
                  << " a 'oneof subcommand' field" << std::endl;
        std::exit(1);
      }

      if (field->options().HasExtension(stout::v1::flag)) {
        const auto& flag = field->options().GetExtension(stout::v1::flag);
        TryFillFieldAndMessageHelpers(&flag, field, message);
      }

      if (field->options().HasExtension(stout::v1::argument)) {
        if (field->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
          std::cerr << "Field '" << field->full_name()
                    << "' with 'stout::v1::argument' extension "
                    << "must have string type" << std::endl;
          std::exit(1);
        }

        const auto& argument =
            field->options().GetExtension(stout::v1::argument);

        CHECK_GE(argument.position(), 1u)
            << "Field '" << field->full_name()
            << "' should have option "
            << "'position' greater or "
            << "equal than 1";

        TryFillFieldAndMessageHelpers(&argument, field, message);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::AddOrExit(
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

////////////////////////////////////////////////////////////////////////

google::protobuf::Message* Parser::TryParseSubcommand(const std::string& arg) {
  CHECK(cur_message_);

  if (const google::protobuf::FieldDescriptor* subcommand_field =
          cur_message_
              .value()
              ->GetDescriptor()
              ->FindFieldByName(arg);
      subcommand_field == nullptr) {
    // There is no any field with the name 'arg'.
    return nullptr;
  } else {
    if (const google::protobuf::OneofDescriptor* real_oneof =
            subcommand_field->real_containing_oneof();
        real_oneof == nullptr) {
      return nullptr;
    } else {
      if (real_oneof->name() != "subcommand") {
        return nullptr;
      } else {
        return cur_message_
            .value()
            ->GetReflection()
            ->MutableMessage(
                cur_message_.value(),
                subcommand_field);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

const google::protobuf::FieldDescriptor* Parser::GetFieldForPositionalArgument(
    const std::string& arg) {
  const google::protobuf::Descriptor* descriptor =
      cur_message_.value()->GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field =
        descriptor->field(i);
    if (field->options().HasExtension(stout::v1::argument)) {
      const stout::v1::Argument& argument =
          field->options().GetExtension(stout::v1::argument);
      if (argument.position() == cur_index_pos_arg_) {
        return field;
      }
    }
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////

void Parser::Parse(int* argc, const char*** argv) {
  // Grab the program name from argv, without removing it.
  program_name_ = *argc > 0
      ? std::filesystem::path((*argv)[0]).filename().string()
      : "";

  // Keep the arguments that are not being processed as flags.
  std::vector<const char*> args;

  std::multimap<std::string, std::optional<std::string>> values;

  // Read flags from the command line.
  for (int i = 1; i < *argc; i++) {
    std::string arg((*argv)[i]);

    // Strip both leading and trailing whitespace.
    absl::StripAsciiWhitespace(&arg);

    // Stop parsing flags after '--' is encountered.
    if (arg == "--") {
      // Save the rest of the arguments.
      for (int j = i + 1; j < *argc; j++) {
        args.push_back((*argv)[j]);
      }
      break;
    }

    // Skip anything that doesn't look like a flag.
    if (arg.find("--") != 0) {
      if (subcommands_.count(arg) > 0) {
        std::cerr << "Encountered duplicate subcommand '"
                  << arg << "'." << std::endl;
        std::exit(1);
      }
      if (previous_message_
                  .value()
                  ->GetDescriptor()
                  ->FindFieldByName(arg)
              != nullptr
          && parsing_subcommand_flags_) {
        std::cerr << "You have already set oneof 'subcommand'"
                  << " field for the message '"
                  << previous_message_.value()->GetTypeName()
                  << "'" << std::endl;
        std::exit(1);
      }
      // It could be probably subcommand.
      if (google::protobuf::Message* subcommand_message =
              TryParseSubcommand(arg);
          subcommand_message != nullptr) {
        parsing_subcommand_flags_ = true;
        previous_message_ = cur_message_;
        cur_message_ = subcommand_message;
        subcommands_.insert(arg);
        AddAllOrExit(*cur_message_);
        continue;
      }
      if (const auto* field = GetFieldForPositionalArgument(arg);
          field != nullptr) {
        pos_arg_fields_.emplace(field, arg);
        ++cur_index_pos_arg_;
        continue;
      }
      args.push_back((*argv)[i]);
      continue;
    }

    std::string name;
    std::optional<std::string> value;

    size_t eq = arg.find_first_of('=');
    if (eq == std::string::npos && arg.find("--no-") == 0) { // --no-name
      name = arg.substr(2);
    } else if (eq == std::string::npos) { // --name
      name = arg.substr(2);
    } else { // --name=value
      name = arg.substr(2, eq - 2);
      value = arg.substr(eq + 1);
    }

    values.emplace(name, value);
  }

  // Parse environment variables if environment_variable_prefix_
  // contains the specific prefix of variables we want to parse.
  if (environment_variable_prefix_) {
    // Run through all environment variables and select those which has
    // prefix (environment_variable_prefix_) in the name. Then we grab
    // correct name and the value and store this pair in `values`.
    // We use global variable `environ` to have access to the environment
    // variables to be able to select all necessary variables for
    // parsing according to the included prefix (see class `Parser`
    // in `stout/flags.h` which has `environment_variable_prefix_`
    // variable of type `std::optional<std::string>`).
    for (char** env = environ; *env != nullptr; ++env) {
      if (!absl::StrContains(*env, *environment_variable_prefix_)) {
        continue;
      }

      // By default all environment variables are as follows:
      //    name=value
      //  Hence we're splitting by '=' to correctly get the name and value.
      const std::vector<std::string> name_value =
          absl::StrSplit(*env, absl::MaxSplits('=', 1));

      CHECK_EQ(name_value.size(), 2u)
          << "Expecting all environment variables to have '=' delimiter";

      // Grab the name and the value.
      absl::string_view name{name_value[0]};
      std::string value{name_value[1]};

      if (absl::ConsumePrefix(
              &name,
              environment_variable_prefix_.value() + "_")) {
        // It's possible that users can set variables with upper cases.
        // So we should be sure that names we pass for parsing have
        // only lower-cases symbols.
        values.emplace(absl::AsciiStrToLower(name), value);
      }
    }
  }

  // Parse flags.
  Parse(values);

  // Parse positional arguments.
  TryParsePositionalArguments();

  // Update 'argc' and 'argv' if we successfully loaded the flags.
  CHECK_LE(args.size(), (size_t) *argc);
  int i = 1; // Start at '1' to skip argv[0].
  for (const char* arg : args) {
    (*argv)[i++] = arg;
  }

  *argc = i;

  // Now null terminate the array. Note that we'll "leak" the
  // arguments that were processed here but it's not like they would
  // have gotten deleted in normal operations anyway.
  (*argv)[i++] = nullptr;
}

////////////////////////////////////////////////////////////////////////

void Parser::TryParsePositionalArguments() {
  for (const auto& [field, value] : pos_arg_fields_) {
    // Normalize value.
    std::string normalized_value = "'" + absl::CEscape(value) + "'";

    // Parse the value using an error collector that aggregates the
    // error for us to print out later.
    struct ErrorCollector : public google::protobuf::io::ErrorCollector {
      void AddError(
          int /* line */,
          int /* column */,
          const std::string& message) override {
        error += message;
      }

      void AddWarning(
          int line,
          int column,
          const std::string& message) override {
        // For now we treat all warnings as errors.
        AddError(line, column, message);
      }

      std::string error;
    } error_collector;

    google::protobuf::TextFormat::Parser text_format_parser;
    text_format_parser.RecordErrorsTo(&error_collector);

    if (!text_format_parser.ParseFieldValueFromString(
            normalized_value,
            field,
            messages_[field])) {
      std::cerr << "Failed to parse positional argument '" << value
                << "' from normalized value '" << normalized_value
                << "' due to protobuf text-format parser error(s): "
                << error_collector.error << std::endl;
    } else {
      // Successfully parsed!
      parsed_.emplace(field, Parsed{value, normalized_value});
    }
  }

  std::set<std::string> errors;

  // Ensure required positional arguments are present.
  for (const auto& [_, field] : fields_) {
    const stout::v1::Argument& argument =
        field->options().GetExtension(stout::v1::argument);
    if (!parsed_.count(field) && argument.required()) {
      CHECK(!argument.names().empty());
      std::string names;
      for (int i = 0; i < argument.names().size(); i++) {
        if (i == 1) {
          names += " (aka ";
        } else if (i > 1) {
          names += ", ";
        }
        names += "'" + argument.names().at(i) + "'";
      }
      if (argument.names().size() > 1) {
        names += ")";
      }
      errors.insert(
          "Positional argument " + names + " not parsed but required");
    }
  }

  if (!errors.empty()) {
    std::cerr
        << program_name_ << ": "
        << "Failed while parsing positional arguments:"
        << std::endl
        << std::endl;
    for (const auto& error : errors) {
      std::cerr << "* " << error
                << std::endl
                << std::endl;
    }
    std::exit(1);
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::Parse(
    const std::multimap<std::string, std::optional<std::string>>& values) {
  // NOTE: using a set for errors to avoid duplicates which may
  // happen, e.g., when we have duplicate flags that are unknown or
  // duplicate boolean flags that conflict, etc.
  std::set<std::string> errors;

  for (const auto& [name, value] : values) {
    bool is_negated = absl::StartsWith(name, "no-");
    std::string non_negated_name = !is_negated ? name : name.substr(3);

    auto iterator = fields_.find(non_negated_name);

    if (iterator == fields_.end()) {
      errors.insert(
          "Encountered unknown flag '" + non_negated_name + "'"
          + (!is_negated ? "" : " via '" + name + "'"));
      continue;
    }

    const auto* field = iterator->second;

    // Need to normalize 'value' into protobuf text-format which
    // doesn't have a concept of "no-" prefix or non-empty booleans.
    std::optional<std::string> text;

    const bool boolean =
        field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL;

    if (boolean) {
      if (!value) {
        text = is_negated ? "false" : "true";
      } else if (is_negated) {
        // Boolean flags with "no-" prefix must have an empty value.
        errors.insert(
            "Encountered negated boolean flag '" + name
            + "' with an unexpected value '" + value.value() + "'");
        continue;
      } else {
        text = value.value();
      }
    } else if (is_negated) {
      // Non-boolean flags cannot have "no-" prefix.
      errors.insert(
          "Failed to parse non-boolean flag '"
          + non_negated_name + "' via '" + name + "'");
      continue;
    } else if (!value.has_value() || value.value().empty()) {
      // Non-boolean flags must have a non-empty value.
      errors.insert(
          "Failed to parse non-boolean flag '" + non_negated_name
          + "': missing value");
      continue;
    } else {
      if (field->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
        text = value.value();
      } else {
        text = "'" + absl::CEscape(value.value()) + "'";
      }
    }

    CHECK(text);

    // Check if the field is a duplicate.
    const bool duplicate = parsed_.count(field) > 0;

    if (duplicate) {
      if (boolean) {
        // Only boolean flags can be duplicated and if/when they are
        // they can not conflict with one another.
        if (text != parsed_[field].text) {
          errors.insert(
              "Encountered duplicate boolean flag '" + non_negated_name + "' "
              + (parsed_[field].name != non_negated_name
                     ? "with flag aliased as '" + parsed_[field].name + "' "
                     : "")
              + "that has a conflicting value");
          continue;
        }
      } else {
        // Only boolean flags can be duplicated.
        errors.insert(
            "Encountered duplicate flag '" + non_negated_name + "'"
            + (parsed_[field].name != non_negated_name
                   ? " with flag aliased as '" + parsed_[field].name + "'"
                   : ""));
        continue;
      }
    }

    // Parse the value using an overloaded parser if provided.
    if (overload_parsing_.count(field->message_type()) > 0) {
      CHECK(messages_.count(field) != 0);
      auto* message = messages_[field];
      std::optional<std::string> error =
          overload_parsing_[field->message_type()](
              text.value(),
              message->GetReflection()->MutableMessage(message, field));

      if (error) {
        errors.insert(
            "Failed to parse flag '" + non_negated_name
            + "' from normalized value '" + text.value()
            + "' due to overloaded parsing error: " + error.value());
      } else {
        // Successfully parsed!
        parsed_[field] = {non_negated_name, text.value()};
      }
    } else {
      // Parse the value using an error collector that aggregates the
      // error for us to print out later.
      struct ErrorCollector : public google::protobuf::io::ErrorCollector {
        void AddError(
            int /* line */,
            int /* column */,
            const std::string& message) override {
          error += message;
        }

        void AddWarning(
            int line,
            int column,
            const std::string& message) override {
          // For now we treat all warnings as errors.
          AddError(line, column, message);
        }

        std::string error;
      } error_collector;

      google::protobuf::TextFormat::Parser text_format_parser;
      text_format_parser.RecordErrorsTo(&error_collector);
      if (!text_format_parser.ParseFieldValueFromString(
              text.value(),
              field,
              messages_[field])) {
        errors.insert(
            "Failed to parse flag '" + non_negated_name
            + "' from normalized value '" + text.value()
            + "' due to protobuf text-format parser error(s): "
            + error_collector.error);
      } else {
        // Successfully parsed!
        parsed_[field] = {non_negated_name, text.value()};
      }
    }
  }

  // Print out help if requested.
  if (standard_flags_->help()) {
    PrintHelp();
    std::exit(0);
  }

  // Ensure required flags are present.
  for (const auto& [_, field] : fields_) {
    const auto& flag = field->options().GetExtension(stout::v1::flag);
    if (flag.required() && parsed_.count(field) == 0) {
      CHECK(!flag.names().empty());
      std::string names;
      for (int i = 0; i < flag.names().size(); i++) {
        if (i == 1) {
          names += " (aka ";
        } else if (i > 1) {
          names += ", ";
        }
        names += "'" + flag.names().at(i) + "'";
      }
      if (flag.names().size() > 1) {
        names += ")";
      }
      errors.insert(
          "Flag " + names + " not parsed but required");
    }
  }

  // Perform validations.
  for (auto& [error, f] : validate_) {
    if (!f()) {
      errors.insert(error);
    }
  }

  if (!errors.empty()) {
    std::cerr
        << program_name_ << ": "
        << "Failed while parsing and validating flags:"
        << std::endl
        << std::endl;
    for (const auto& error : errors) {
      std::cerr << "* " << error
                << std::endl
                << std::endl;
    }
    std::exit(1);
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::FillTopLevelHelpNode(google::protobuf::Message* message) {
  const google::protobuf::Descriptor* descriptor =
      message->GetDescriptor();

  help_nodes_.emplace_back(
      HelpNode{
          descriptor,
          std::vector<SubcommandHelp>{}});

  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (field->real_containing_oneof()) {
      CHECK(help_nodes_.size() == 1u);
      help_nodes_[0].subcommands.emplace_back(SubcommandHelp{0u, field});
    }
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::FillHelpNode(
    const google::protobuf::Descriptor* descriptor,
    std::vector<SubcommandHelp>& subcommands,
    std::size_t indent) {
  CHECK(descriptor);

  for (int i = 0; i < descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);

    if (field->real_containing_oneof()) {
      const auto iterator = std::find_if(
          subcommands.begin(),
          subcommands.end(),
          [&field](const auto& node) {
            return field == node.field;
          });
      if (iterator == subcommands.end()) {
        subcommands.emplace_back(SubcommandHelp{indent + step_, field});
        FillHelpNode(field->message_type(), subcommands, step_);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::FillHelpNodes() {
  for (const auto& node : help_nodes_[0].subcommands) {
    help_nodes_.emplace_back(
        HelpNode{
            node.field->message_type(),
            std::vector<SubcommandHelp>{}});
    FillHelpNode(
        node.field->message_type(),
        help_nodes_[help_nodes_.size() - 1].subcommands,
        0u);
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::PrintHelp() {
  std::string help = "Usage:\n\n";

  std::string general_help{program_name_ + " [...]"};

  std::size_t indent = 0;

  std::vector<std::string> subcommands_with_cur_indent;

  // Helper for avoiding duplicate names for general help.
  std::set<std::string> general_help_names;

  bool again = false;

  // Prepare firstly general help information.
  for (std::size_t i = 0; i < help_nodes_.size(); ++i) {
    for (std::size_t j = 0; j < help_nodes_[i].subcommands.size(); ++j) {
      if (indent == help_nodes_[i].subcommands[j].indent) {
        again = true;
        subcommands_with_cur_indent.push_back(
            help_nodes_[i].subcommands[j].field->name());
      }
    }

    if (i == help_nodes_.size() - 1) {
      for (std::size_t k = 0; k < subcommands_with_cur_indent.size(); ++k) {
        bool duplicate =
            general_help_names.count(subcommands_with_cur_indent[k]) >= 1;
        if (subcommands_with_cur_indent.size() == 1) {
          if (duplicate) {
            continue;
          }
          general_help +=
              " {" + subcommands_with_cur_indent[k] + "} [...]";
        } else if (k == subcommands_with_cur_indent.size() - 1) {
          if (duplicate) {
            general_help += "} [...]";
            continue;
          }
          general_help +=
              "|" + subcommands_with_cur_indent[k] + "} [...]";
        } else if (k == 0) {
          if (duplicate) {
            general_help += "{";
            continue;
          }
          general_help +=
              " {" + subcommands_with_cur_indent[k];
        } else {
          if (duplicate) {
            continue;
          }
          general_help +=
              "|" + subcommands_with_cur_indent[k];
        }
        general_help_names.insert(subcommands_with_cur_indent[k]);
      }
      subcommands_with_cur_indent.clear();
    }

    if (again && i == help_nodes_.size() - 1) {
      again = false;
      i = -1;
      indent += step_;
    }
  }

  help += general_help + "\n\n";
  help += "[...] - flags or positional arguments\n\n";

  if (help_nodes_.size() > 1) {
    help += "{...|...} - subcommands\n\n";
    help +=
        "NOTE: subcommands must follow in correct order.\n"
        "REMEMBER, only one subcommand from the list {...}\n"
        "can be set at a time!\n"
        "Check more specific information about the\n"
        "subcommands below.\n\n";
  }

  // Prepare specific information about flags, positional arguments and
  // subcommands.
  indent = 0;

  // Print standard flags help.
  const auto* standart_descriptor = standard_flags_->GetDescriptor();
  for (int k = 0; k < standart_descriptor->field_count(); ++k) {
    const auto* standart_field = standart_descriptor->field(k);

    const auto& flag =
        standart_field->options().GetExtension(stout::v1::flag);

    const auto [name, help_info] =
        GetHelpInfoFromField(&flag, standart_field);

    help +=
        std::string(indent, ' ') + name
        + std::string(step_, ' ') + help_info + "\n";
  }

  auto SetHelpInfoForDescriptor =
      [this](
          const google::protobuf::Descriptor* descriptor,
          std::string& help,
          std::size_t indent) {
        for (int j = 0; j < descriptor->field_count(); ++j) {
          const auto* field = descriptor->field(j);
          if (field->options().HasExtension(stout::v1::subcommand)) {
            continue;
          }
          if (field->options().HasExtension(stout::v1::flag)) {
            const auto& flag = field->options().GetExtension(stout::v1::flag);
            const auto [name, help_info] = GetHelpInfoFromField(&flag, field);
            help +=
                std::string(indent, ' ') + name
                + std::string(step_, ' ') + help_info + "\n";
          }
          if (field->options().HasExtension(stout::v1::argument)) {
            const auto& argument =
                field->options().GetExtension(stout::v1::argument);
            const auto [name, help_info] =
                GetHelpInfoFromField(&argument, field);
            help +=
                std::string(indent, ' ') + name
                + std::string(step_, ' ') + help_info + "\n";
          }
        }
      };

  auto SetHelpInfoForSubcommandField =
      [this](
          const google::protobuf::FieldDescriptor* field,
          std::string& help,
          std::size_t indent) {
        const auto& subcommand =
            field->options().GetExtension(stout::v1::subcommand);
        const auto [name, help_info] =
            GetHelpInfoFromField(&subcommand, field);
        help +=
            std::string(indent, ' ') + name
            + std::string(step_, ' ') + help_info + "\n";
      };

  std::vector<SubcommandHelp> top_level_subcommands =
      (help_nodes_[0].subcommands.size())
      ? help_nodes_[0].subcommands
      : std::vector<SubcommandHelp>{};

  // Print flags|pos args|subcommands help.
  for (std::size_t i = 0; i < help_nodes_.size(); ++i) {
    // Print help info for subcommand.
    if (i && top_level_subcommands.size()) {
      SetHelpInfoForSubcommandField(
          top_level_subcommands[i - 1].field,
          help,
          top_level_subcommands[i - 1].indent);
      SetHelpInfoForDescriptor(
          help_nodes_[i].descriptor,
          help,
          step_);
    }
    if (!i) {
      SetHelpInfoForDescriptor(
          help_nodes_[i].descriptor,
          help,
          0u);
    }

    for (std::size_t j = 0; j < help_nodes_[i].subcommands.size(); ++j) {
      if (!i) {
        break;
      }
      SetHelpInfoForSubcommandField(
          help_nodes_[i].subcommands[j].field,
          help,
          help_nodes_[i].subcommands[j].indent);
      // Print flags|pos args help info for subcommand fields.
      SetHelpInfoForDescriptor(
          help_nodes_[i].subcommands[j].field->message_type(),
          help,
          help_nodes_[i].subcommands[j].indent + step_);
    }
  }

  std::cerr << help << std::endl;
}

////////////////////////////////////////////////////////////////////////

} // namespace stout::flags

////////////////////////////////////////////////////////////////////////
