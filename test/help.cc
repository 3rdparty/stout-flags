#include <array>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, FlagsHelpNoSubcommands) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--help",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "Usage:"
      "\?\n\?\n"
      "program .\\.\\.\\.."
      "\?\n\?\n"
      ".\\.\\.\\.. - flags or positional arguments"
      "\?\n\?\n"
      "--\\(no-\\)help     whether or not to display this help message\?\n"
      "--foo=\\.\\.\\.     help\?\n"
      "--s=\\.\\.\\.     help\?\n"
      "--\\(no-\\)bar     help\?\n"
      "--baz=\\.\\.\\.     help\?\n"
      "--duration=\\.\\.\\.     help\?\n";

  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, FlagsHelpSubcommands) {
  test::ComplicatedSubcommandMessage msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "/path/to/program",
      "--help",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "Usage:"
      "\?\n\?\n"
      "program .\\.\\.\\.. .sub1.sub2. .\\.\\.\\.. "
      ".build.info_flag. .\\.\\.\\.."
      "\?\n\?\n"
      ".\\.\\.\\.. - flags or positional arguments"
      "\?\n\?\n"
      ".\\.\\.\\..\\.\\.\\.. - subcommands"
      "\?\n\?\n"
      "NOTE: subcommands must follow in correct order.\?\n"
      "REMEMBER, only one subcommand from the list .\\.\\.\\..\?\n"
      "can be set at a time!\?\n"
      "Check more specific information about the\?\n"
      "subcommands below.\?\n\?\n"
      "--\\(no-\\)help     whether or not to display this help message\?\n"
      "--flag=\\.\\.\\.     help\?\n"
      "--other=\\.\\.\\.     help\?\n"
      "sub1     help\?\n"
      "     --another=\\.\\.\\.     help\?\n"
      "     --num=\\.\\.\\.     help\?\n"
      "     build     help\?\n"
      "          --other_flag=\\.\\.\\.     help\?\n"
      "     info_flag     help\?\n"
      "          --info=\\.\\.\\.     help\?\n"
      "sub2     help\?\n"
      "     --s=\\.\\.\\.     help\?\n"
      "     build     help\?\n"
      "          --other_flag=\\.\\.\\.     help\?\n"
      "     info_flag     help\?\n"
      "          --info=\\.\\.\\.     help\?\n";


  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, FlagsHelpMoreComplicatedCase) {
  test::TopLevel msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "/path/to/program",
      "--help",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "Usage:"
      "\?\n\?\n"
      "program .\\.\\.\\.. .a1.a2. .\\.\\.\\.. "
      ".b1.b2.c1.c2. .\\.\\.\\.. "
      ".d1.d2.e1.e2.f1.f2.g1.g2. .\\.\\.\\.."
      "\?\n\?\n"
      ".\\.\\.\\.. - flags or positional arguments"
      "\?\n\?\n"
      ".\\.\\.\\..\\.\\.\\.. - subcommands"
      "\?\n\?\n"
      "NOTE: subcommands must follow in correct order.\?\n"
      "REMEMBER, only one subcommand from the list .\\.\\.\\..\?\n"
      "can be set at a time!\?\n"
      "Check more specific information about the\?\n"
      "subcommands below.\?\n\?\n"
      "--\\(no-\\)help     whether or not to display this help message\?\n"
      "a1     help\?\n"
      "     b1     help\?\n"
      "          d1     help\?\n"
      "               --d1=\\.\\.\\.     help\?\n"
      "          d2     help\?\n"
      "               --d2=\\.\\.\\.     help\?\n"
      "     b2     help\?\n"
      "          e1     help\?\n"
      "               --e1=\\.\\.\\.     help\?\n"
      "          e2     help\?\n"
      "               --e2=\\.\\.\\.     help\?\n"
      "a2     help\?\n"
      "     c1     help\?\n"
      "          f1     help\?\n"
      "               --f1=\\.\\.\\.     help\?\n"
      "          f2     help\?\n"
      "               --f2=\\.\\.\\.     help\?\n"
      "     c2     help\?\n"
      "          g1     help\?\n"
      "               --g1=\\.\\.\\.     help\?\n"
      "          g2     help\?\n"
      "               --g2=\\.\\.\\.     help\?\n";

  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      testing::ContainsRegex(regex));
}
