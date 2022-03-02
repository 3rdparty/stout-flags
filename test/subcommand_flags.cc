#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, MissingSubcommandExtension) {
  EXPECT_DEATH(
      []() {
        test::SubcommandFlagsWithoutExtension flag;
        auto builder = stout::flags::Parser::Builder(&flag);
        builder.Build();
      }(),
      "Every field of the 'oneof subcommand' must have"
      " .stout.v1.subcommand. extension.");
}

TEST(FlagsTest, IncorrectSubcommandExtension) {
  EXPECT_DEATH(
      []() {
        test::FlagsWithIncorrectExtension flag;
        auto builder = stout::flags::Parser::Builder(&flag);
        builder.Build();
      }(),
      ".stout.v1.subcommand. extension should be inside only"
      " a 'oneof subcommand' field.");
}

TEST(FlagsTest, IncorrectOneofName) {
  EXPECT_DEATH(
      []() {
        test::IncorrectOneofName flag;
        auto builder = stout::flags::Parser::Builder(&flag);
        builder.Build();
      }(),
      "'oneof' field must have 'subcommand' name. "
      "Other names are illegal.");
}

TEST(FlagsTest, SubcommandFlagExtension) {
  EXPECT_DEATH(
      []() {
        test::SubcommandFlagExtension flag;
        auto builder = stout::flags::Parser::Builder(&flag);
        builder.Build();
      }(),
      "Every field of the 'oneof subcommand' must have"
      " .stout.v1.subcommand. extension.");
}
