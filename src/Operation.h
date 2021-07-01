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

#ifndef INCLUDED_OPERATION
#define INCLUDED_OPERATION

#include <optional>
#include <string>

/* An operation represents a change to the task database.
 *
 * Operations are critical to the OT-based synchronization mechanism.  They must
 * remain simple and always-applicable.  Most task-specific functionality (such
 * as adding tags) is handled on top of the operation abstraction.
 *
 * There are three forms of operation:
 *
 *  - Create(uuid) - create a new, empty task with the given UUID
 *  - Delete(uuid) - delete a task, including all properties
 *  - Update(uuid, property, optional<value>, timestamp) - update a property
 *    on the given task, either setting it or (if the value is nullopt) removin
 *    it.  The timestamp is used to order updates during synchronization.
 *
 * Operations are immutable once they are created.  Getters will fail if called for
 * an operation type that does not have the requested attribute.
 *
 * INVARIANT: if _type is op_update, then _property is non-null and timestamp is set
 */
class Operation
{
public:
  enum type {op_create, op_update, op_delete};

  // Constructors for the available types of operations
  static Operation new_create(std::string uuid);
  static Operation new_delete(std::string uuid);
  static Operation new_update(std::string uuid, std::string property, std::string value);
  static Operation new_update_remove(std::string uuid, std::string property);

  Operation (const Operation&);

  bool operator== (const Operation&) const;
  bool operator!= (const Operation&) const;

  // getters
  enum type get_type() { return _type; }
  std::string get_uuid() { return _uuid; }
  std::string get_property() { throw_if_not_update(); return _property.value(); }
  std::optional<std::string> get_value() { throw_if_not_update(); return _value; }
  time_t get_timestamp() { throw_if_not_update(); return _timestamp; }

private:
  Operation (enum type type, std::string uuid);

  enum type   _type;
  std::string _uuid;

  // only used for op_update
  std::optional<std::string> _property     {std::nullopt};
  std::optional<std::string> _value        {std::nullopt};
  time_t                     _timestamp    {0};

  void throw_if_not_update();
};

#endif

////////////////////////////////////////////////////////////////////////////////
