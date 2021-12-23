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

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "program: Failed while parsing and validating flags:"
      ".+"
      "\\* 'bar' must be true"
      ".+"
      "\\* 'baz' must be greater than 42");
}