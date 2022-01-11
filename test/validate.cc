#include <array>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, Validate) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags)
                    .Validate(
                        "'bar' must be true",
                        [](const auto& flags) {
                          return flags.bar();
                        })
                    .Validate(
                        "'baz' must be greater than 42",
                        [](const auto& flags) {
                          return flags.baz() > 42;
                        })
                    .Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-bar",
      "--baz=42",
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
      "\\* 'bar' must be true"
      "\\n\\n"
      "\\* 'baz' must be greater than 42";
#else
  const std::string regex =
      "program: Failed while parsing and validating flags:"
      ".+"
      "\\* 'bar' must be true"
      ".+"
      "\\* 'baz' must be greater than 42";
#endif

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}
