#include <array>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, ParseRequired) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "program: Failed while parsing and validating flags:"
      ".+"
      "\\* Flag 'foo' not parsed but required");
}

TEST(FlagsTest, ParseString) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ("hello world", flags.foo());
}

TEST(FlagsTest, ParseImplicitBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
}

TEST(FlagsTest, ParseExplicitBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--bar=true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
}

TEST(FlagsTest, ParseNegatedBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_FALSE(flags.bar());
}

TEST(FlagsTest, ModifiedArgcArgv) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "one",
      "--bar",
      "two",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
  EXPECT_EQ("hello world", flags.foo());

  EXPECT_EQ(3, argc);

  EXPECT_EQ("/path/to/program", argv[0]);
  EXPECT_EQ("one", argv[1]);
  EXPECT_EQ("two", argv[2]);
}
