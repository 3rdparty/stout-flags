#include <array>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, Help) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--help",
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
      "Usage: program \\[\\.\\.\\.\\]"
      "\\n\\n"
      "  --\\[no-\\]bar         help\\n"
      "  --baz=\\.\\.\\.          help\\n"
      "  --duration=\\.\\.\\.     help\\n"
      "  --foo=\\.\\.\\.          help\\n"
      "  --\\[no-\\]help        whether or not to display this help message";
#else
  const std::string regex =
      "Usage: program \\[\\.\\.\\.\\]"
      ".+"
      "  --\\[no-\\]bar         help.+"
      "  --baz=\\.\\.\\.          help.+"
      "  --duration=\\.\\.\\.     help.+"
      "  --foo=\\.\\.\\.          help.+"
      "  --\\[no-\\]help        whether or not to display this help message";
#endif

  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      testing::ContainsRegex(regex));
}
