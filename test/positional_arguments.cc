#include <array>

#include "gtest/gtest.h"
#include "stout/flags.h"
#include "test/test.pb.h"

TEST(PositionalArguments, RenameSucceed) {
  test::Rename rename;

  auto parser = stout::flags::Parser::Builder(&rename).Build();

  std::array arguments = {
      "rename",
      "foo.cc",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_EQ("rename", argv[0]);
  EXPECT_EQ("foo.cc", rename.cur_file_name());
  EXPECT_EQ("bar.cc", rename.new_file_name());
}

TEST(PositionalArguments, RenameFailOnMissingRequiredArgument) {
  test::Rename rename;

  auto parser = stout::flags::Parser::Builder(&rename).Build();

  std::array arguments = {
      "rename",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "rename: Failed while parsing positional arguments:"
      "\?\n\?\n"
      ". Positional argument 'new_file_name' .aka 'new_file'."
      " not parsed but required");
}

TEST(PositionalArguments, BuildFileSucceed) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "program",
      "build",
      "--debug",
      "foo.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_FALSE(msg.has_rename());
  EXPECT_TRUE(msg.has_build());
  EXPECT_TRUE(msg.build().debug_mode());
  EXPECT_EQ("foo.cc", msg.build().file());
  EXPECT_EQ("program", argv[0]);
}

TEST(PositionalArguments, ProcessFileRenameSucceed) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "program",
      "rename",
      "foo.cc",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_TRUE(msg.has_rename());
  EXPECT_FALSE(msg.has_build());
  EXPECT_EQ("foo.cc", msg.rename().cur_file_name());
  EXPECT_EQ("bar.cc", msg.rename().new_file_name());
  EXPECT_EQ("program", argv[0]);
}

TEST(PositionalArguments, BuildFileFailOnMissingRequiredArgument) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "program",
      "build",
      "--debug=true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "program: Failed while parsing positional arguments:"
      "\?\n\?\n"
      ". Positional argument 'file' .aka 'file_name'. "
      "not parsed but required");
}

TEST(PositionalArguments, RedundantPositionalArguments) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(&msg).Build();

  std::array arguments = {
      "program",
      "build",
      "main.cc",
      "--debug=true",
      "45",
      "redundant",
      "true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(4, argc);
  EXPECT_FALSE(msg.has_rename());
  EXPECT_TRUE(msg.has_build());
  EXPECT_TRUE(msg.build().debug_mode());
  EXPECT_EQ("main.cc", msg.build().file());
  EXPECT_EQ("program", argv[0]);
  EXPECT_EQ("45", argv[1]);
  EXPECT_EQ("redundant", argv[2]);
  EXPECT_EQ("true", argv[3]);
}

TEST(PositionalArguments, IllegalPositionalArgument) {
  test::IllegalPositionalArg msg;

  EXPECT_DEATH(
      stout::flags::Parser::Builder(&msg).Build(),
      "Field 'test.IllegalPositionalArg.num' "
      "with 'stout::v1::argument' extension "
      "must have string type");
}
