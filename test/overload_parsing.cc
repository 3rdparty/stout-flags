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

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "program: Failed while parsing and validating flags:"
      ".+"
      "\\* Failed to parse flag 'duration' from normalized value "
      "'-1000000001ns' due to overloaded parsing error: unimplemented");
}
