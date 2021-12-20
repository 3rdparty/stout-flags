#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(FlagsTest, DuplicateFlagName) {
  EXPECT_DEATH(
      []() {
        test::DuplicateFlagName duplicate_flag_name;
        auto builder = stout::flags::Parser::Builder(&duplicate_flag_name);
        builder.Build();
      }(),
      "Encountered duplicate flag name 'same' for "
      "field 'test.DuplicateFlagName.s2'");
}
