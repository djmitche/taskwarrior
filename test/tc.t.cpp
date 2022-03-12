////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2022, Dustin J. Mitchell
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
#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#include "tc/Replica.h"
#include "tc/Task.h"
#include "tc/util.h"

////////////////////////////////////////////////////////////////////////////////
int main (int, char**)
{
  UnitTest t (12);

  //// util

  {
    auto s1 = std::string ("a\0string!");
    auto stc = tc::string2tc (s1);
    auto s2 = tc::tc2string (stc);
    t.is (s1, s2, "round-trip to tc string and back (containing an embedded NUL)");
  }

  {
    auto s1 = std::string ("62123ec9-c443-4f7e-919a-35362a8bef8d");
    auto tcuuid = tc::uuid2tc (s1);
    auto s2 = tc::tc2uuid (tcuuid);
    t.is(s1, s2, "round-trip to TCUuid and back");
  }

  //// Replica

  auto rep = tc::Replica ();
  t.pass ("replica constructed");

  auto maybe_task = rep.get_task("24478a28-4609-4257-bc19-44ec51391431");
  t.notok(maybe_task.has_value(), "task with fixed uuid does not exist");

  auto task = rep.new_task (tc::Status::Pending, "a test");
  t.pass ("new task constructed");
  t.is (task.get_description (), std::string ("a test"), "task description round-trip");
  t.is (task.get_status (), tc::Status::Pending, "task status round-trip");

  auto uuid = task.get_uuid();

  auto maybe_task2 = rep.get_task (uuid);
  t.ok(maybe_task2.has_value(), "task lookup by uuid finds task");
  t.is ((*maybe_task2).get_description (), std::string ("a test"), "task description round-trip");

  rep.rebuild_working_set ();
  t.pass ("rebuild_working_set");

  auto tasks = rep.all_tasks ();
  t.is ((int)tasks.size(), 1, "all_tasks returns one task");
  t.is (tasks[0].get_uuid(), uuid, "returned task has correct uuid");

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

