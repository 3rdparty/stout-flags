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

// On POSIX systems (e.g. Linux, Cygwin, and Mac), googletest
// uses the POSIX extended regular expression syntax. To learn
// about this syntax, you may want to check the link below:
// https://tinyurl.com/yta5f24r
// On Windows, googletest uses its own simple regular
// expression implementation. Check also the link below.
// https://tinyurl.com/2r59uuuv
#ifdef _WIN32
  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\\n\\n"
      "\\* Flag 'foo' not parsed but required";
#else
  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      ".+"
      "\\* Flag 'foo' not parsed but required";
#endif

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
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
