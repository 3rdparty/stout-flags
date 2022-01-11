#include <array>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

// Tests that by default we'll overload parsing a
// 'google.protobuf.Duration'.
TEST(FlagsTest, DefaultOverloadParsingDuration) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags)
                    .Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--duration=-1000000001ns",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(-1, flags.duration().seconds());
  EXPECT_EQ(-1, flags.duration().nanos());
}

// Tests that we can still overload the default overloaded parsing of
// 'google.protobuf.Duration'.
TEST(FlagsTest, OverloadParsingDuration) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags)
                    .OverloadParsing<google::protobuf::Duration>(
                        [](const std::string& value, auto* duration) {
                          return std::optional<std::string>("unimplemented");
                        })
                    .Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--duration=-1000000001ns",
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
      "\\* Failed to parse flag 'duration' from normalized value "
      "'-1000000001ns' due to overloaded parsing error: unimplemented";
#else
  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      ".+"
      "\\* Failed to parse flag 'duration' from normalized value "
      "'-1000000001ns' due to overloaded parsing error: unimplemented";
#endif

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}
