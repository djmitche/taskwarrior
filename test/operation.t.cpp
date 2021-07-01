////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2021, Göteborg Bit Factory.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// https://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
#include <iostream>
#include <optional>
#include <test.h>
#include <Operation.h>

////////////////////////////////////////////////////////////////////////////////
void test_transform_one_way(
    UnitTest &t,
    Operation op1,
    Operation op2,
    std::optional<Operation> exp1,
    std::optional<Operation> exp2,
    std::string comment)
{
    std::tuple<std::optional<Operation>, std::optional<Operation>> got = Operation::transform(op1, op2);
    std::tuple<std::optional<Operation>, std::optional<Operation>> exp = std::make_tuple(exp1, exp2);

    if (exp == got) {
        t.pass(comment);
    } else {
        t.fail(comment);
        std::cout << "# expected: "
                  << "(" << exp1 << ", " << exp2 << ")\n"
                  << "#      got: "
                  << "(" << std::get<0>(got) << ", " << std::get<1>(got) << ")\n";
        t.pass("hi");
    }
}

////////////////////////////////////////////////////////////////////////////////
void test_transform(
    UnitTest &t,
    Operation op1,
    Operation op2,
    std::optional<Operation> exp1,
    std::optional<Operation> exp2,
    std::string comment)
{
    test_transform_one_way(t, op1, op2, exp1, exp2, comment);
    test_transform_one_way(t, op2, op1, exp2, exp1, comment + " (reverse)");
}

////////////////////////////////////////////////////////////////////////////////
int main (int, char**)
{
  UnitTest t (14);

  Operation op1 = Operation::new_create("abc123");
  Operation op2 = Operation::new_create("999xyz");
  Operation op3 = Operation::new_delete("999xyz");
  Operation op4 = Operation::new_update("abcdef", "description", "hello");
  Operation op5 = Operation::new_update_remove("abcdef", "tag.foo");

  // override the "now" time for Operation::new_update_*
  time_t now = time(NULL);
  Operation::_now = now;

  t.is(op1 == op2, false, "different ops not equal");
  t.is(op1 == op1, true, "same ops equal");
  t.is(op1.get_type(), Operation::op_create, "create has correct type");
  t.is(op1.get_uuid(), "abc123", "op has correct uuid");
  t.is(op3.get_type(), Operation::op_delete, "delete has correct type");

  t.is(op4.get_type(), Operation::op_update, "update has correct type");
  t.is(op4.get_property(), "description", "update get_property");
  t.is(op4.get_value().value(), "hello", "update get_value");
  t.is((int)op4.get_timestamp(), (int)now, "update get_timestamp is within a few seconds");

  t.is(op5.get_type(), Operation::op_update, "update with remove has correct type");
  t.ok(op5.get_value() == std::nullopt, "update get_value for remove");

  try { op1.get_property();  t.fail ("non-update get_property should fail"); }
  catch (...) {              t.pass ("non-update get_property should fail"); }

  try { op1.get_value();     t.fail ("non-update get_value should fail"); }
  catch (...) {              t.pass ("non-update get_value should fail"); }

  try { op1.get_timestamp(); t.fail ("non-update get_timestamp should fail"); }
  catch (...) {              t.pass ("non-update get_timestamp should fail"); }

  // create + ...

  test_transform(t,
      Operation::new_create("abc"),
      Operation::new_create("abc"),
      std::nullopt,
      std::nullopt,
      "create/create");

  test_transform(t,
      Operation::new_create("abc"),
      Operation::new_delete("abc"),
      Operation::new_create("abc"),
      std::nullopt,
      "create/delete");

  test_transform(t,
      Operation::new_create("abc"),
      Operation::new_update("abc", "description", "hello"),
      std::nullopt,
      Operation::new_update("abc", "description", "hello"),
      "create/update");

  // delete + ...

  test_transform(t,
      Operation::new_delete("abc"),
      Operation::new_delete("abc"),
      std::nullopt,
      std::nullopt,
      "delete/delete");

  test_transform(t,
      Operation::new_delete("abc"),
      Operation::new_update("abc", "description", "hello"),
      Operation::new_delete("abc"),
      std::nullopt,
      "delete/update");

  // update + ...

  test_transform(t,
      Operation::new_update("abc", "description", "hello"),
      Operation::new_update("abc", "description", "hello"),
      std::nullopt,
      std::nullopt,
      "update/update, exactly the same");

  test_transform(t,
      Operation::new_update("abc", "description", "hello"),
      Operation::new_update("abc", "note", "world"),
      Operation::new_update("abc", "description", "hello"),
      Operation::new_update("abc", "note", "world"),
      "update/update, different props");

  {
      Operation::_now = 10000;
      Operation op1 = Operation::new_update("abc", "description", "hello");
      Operation::_now = 20000;
      Operation op2 = Operation::new_update("abc", "description", "hello");
      test_transform(t, op1, op2, std::nullopt, std::nullopt, "update/update, same values, differnt times");
  }

  {
      Operation::_now = 10000;
      Operation op1 = Operation::new_update("abc", "description", "hello");
      Operation::_now = 20000;
      Operation op2 = Operation::new_update("abc", "description", "world");
      test_transform(t, op1, op2, std::nullopt, op2, "update/update, different values + times");
  }

  {
      Operation::_now = 10000;
      Operation op1 = Operation::new_update("abc", "description", "hello");
      Operation op2 = Operation::new_update("abc", "description", "world");
      // this prefers teh first, so only test it one way
      test_transform_one_way(t, op1, op2, op1, std::nullopt, "update/update, different values, same times");
  }

  // reset the patch
  Operation::_now = 0;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
