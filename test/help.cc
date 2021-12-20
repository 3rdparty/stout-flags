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

  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      "Usage: program \\[\\.\\.\\.\\]"
      ".+"
      "  --\\[no-\\]bar         help.+"
      "  --baz=\\.\\.\\.          help.+"
      "  --duration=\\.\\.\\.     help.+"
      "  --foo=\\.\\.\\.          help.+"
      "  --\\[no-\\]help        whether or not to display this help message");
}
