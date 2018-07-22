#include "gtest/gtest.h"
#include "clang_parser.h"
#include "driver.h"

namespace bpftrace {
namespace test {
namespace clang_parser {

void parse(const std::string &input, StructMap &structs)
{
  auto extended_input = input + "kprobe:sys_read { 1 }";
  Driver driver;
  ASSERT_EQ(driver.parse_str(extended_input), 0);

  ClangParser clang;
  clang.parse(driver.root_, structs);
}

TEST(clang_parser, integers)
{
  StructMap structs;
  parse("struct Foo { int x; int y, z; }", structs);

  ASSERT_EQ(structs.size(), 1);
  ASSERT_EQ(structs.count("Foo"), 1);

  EXPECT_EQ(structs["Foo"].size, 12);
  ASSERT_EQ(structs["Foo"].fields.size(), 3);
  ASSERT_EQ(structs["Foo"].fields.count("x"), 1);
  ASSERT_EQ(structs["Foo"].fields.count("y"), 1);
  ASSERT_EQ(structs["Foo"].fields.count("z"), 1);

  EXPECT_EQ(structs["Foo"].fields["x"].type.type, Type::integer);
  EXPECT_EQ(structs["Foo"].fields["x"].type.size, 4);
  EXPECT_EQ(structs["Foo"].fields["x"].offset, 0);

  EXPECT_EQ(structs["Foo"].fields["y"].type.type, Type::integer);
  EXPECT_EQ(structs["Foo"].fields["y"].type.size, 4);
  EXPECT_EQ(structs["Foo"].fields["y"].offset, 4);

  EXPECT_EQ(structs["Foo"].fields["z"].type.type, Type::integer);
  EXPECT_EQ(structs["Foo"].fields["z"].type.size, 4);
  EXPECT_EQ(structs["Foo"].fields["z"].offset, 8);
}

TEST(clang_parser, integer_ptr)
{
  StructMap structs;
  parse("struct Foo { int *x; }", structs);

  ASSERT_EQ(structs.size(), 1);
  ASSERT_EQ(structs.count("Foo"), 1);

  EXPECT_EQ(structs["Foo"].size, 8);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("x"), 1);

  EXPECT_EQ(structs["Foo"].fields["x"].type.type, Type::integer);
  EXPECT_EQ(structs["Foo"].fields["x"].type.size, sizeof(uintptr_t));
  EXPECT_EQ(structs["Foo"].fields["x"].type.is_pointer, true);
  EXPECT_EQ(structs["Foo"].fields["x"].type.pointee_size, sizeof(int));
  EXPECT_EQ(structs["Foo"].fields["x"].offset, 0);
}

TEST(clang_parser, string_ptr)
{
  StructMap structs;
  parse("struct Foo { char *str; }", structs);

  ASSERT_EQ(structs.size(), 1);
  ASSERT_EQ(structs.count("Foo"), 1);

  EXPECT_EQ(structs["Foo"].size, 8);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("str"), 1);

  EXPECT_EQ(structs["Foo"].fields["str"].type.type, Type::integer);
  EXPECT_EQ(structs["Foo"].fields["str"].type.size, sizeof(uintptr_t));
  EXPECT_EQ(structs["Foo"].fields["str"].type.is_pointer, true);
  EXPECT_EQ(structs["Foo"].fields["str"].type.pointee_size, 1);
  EXPECT_EQ(structs["Foo"].fields["str"].offset, 0);
}

TEST(clang_parser, string_array)
{
  StructMap structs;
  parse("struct Foo { char str[32]; }", structs);

  ASSERT_EQ(structs.size(), 1);
  ASSERT_EQ(structs.count("Foo"), 1);

  EXPECT_EQ(structs["Foo"].size, 32);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("str"), 1);

  EXPECT_EQ(structs["Foo"].fields["str"].type.type, Type::string);
  EXPECT_EQ(structs["Foo"].fields["str"].type.size, 32);
  EXPECT_EQ(structs["Foo"].fields["str"].offset, 0);
}

TEST(clang_parser, nested_struct_named)
{
  StructMap structs;
  parse("struct Bar { int x; } struct Foo { struct Bar bar; }", structs);

  ASSERT_EQ(structs.size(), 2);
  ASSERT_EQ(structs.count("Foo"), 1);
  ASSERT_EQ(structs.count("Bar"), 1);

  EXPECT_EQ(structs["Foo"].size, 4);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("bar"), 1);

  EXPECT_EQ(structs["Foo"].fields["bar"].type.type, Type::cast);
  EXPECT_EQ(structs["Foo"].fields["bar"].type.cast_type, "Bar");
  EXPECT_EQ(structs["Foo"].fields["bar"].type.size, 4);
  EXPECT_EQ(structs["Foo"].fields["bar"].offset, 0);
}

TEST(clang_parser, nested_struct_ptr_named)
{
  StructMap structs;
  parse("struct Bar { int x; } struct Foo { struct Bar *bar; }", structs);

  ASSERT_EQ(structs.size(), 2);
  ASSERT_EQ(structs.count("Foo"), 1);
  ASSERT_EQ(structs.count("Bar"), 1);

  EXPECT_EQ(structs["Foo"].size, 8);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("bar"), 1);

  EXPECT_EQ(structs["Foo"].fields["bar"].type.type, Type::cast);
  EXPECT_EQ(structs["Foo"].fields["bar"].type.cast_type, "Bar");
  EXPECT_EQ(structs["Foo"].fields["bar"].type.size, sizeof(uintptr_t));
  EXPECT_EQ(structs["Foo"].fields["bar"].type.is_pointer, true);
  EXPECT_EQ(structs["Foo"].fields["bar"].type.pointee_size, 4);
  EXPECT_EQ(structs["Foo"].fields["bar"].offset, 0);
}

TEST(clang_parser, nested_struct_anon)
{
  StructMap structs;
  parse("struct Foo { struct { int x; } bar; }", structs);

  ASSERT_EQ(structs.size(), 2);
  ASSERT_EQ(structs.count("Foo"), 1);

  EXPECT_EQ(structs["Foo"].size, 4);
  ASSERT_EQ(structs["Foo"].fields.size(), 1);
  ASSERT_EQ(structs["Foo"].fields.count("bar"), 1);

  EXPECT_EQ(structs["Foo"].fields["bar"].type.type, Type::cast);
  EXPECT_EQ(structs["Foo"].fields["bar"].type.cast_type, "Foo::(anonymous at definitions.h:1:14)");
  EXPECT_EQ(structs["Foo"].fields["bar"].type.size, 4);
  EXPECT_EQ(structs["Foo"].fields["bar"].offset, 0);
}

} // namespace clang_parser
} // namespace test
} // namespace bpftrace
