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
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <Operation.h>
#include <Datetime.h>
#include <Duration.h>
#include <Lexer.h>
#include <RX.h>
#include <shared.h>

time_t Operation::_now = 0;

////////////////////////////////////////////////////////////////////////////////
Operation::Operation (const Operation& other)
{
  _type      = other._type;
  _uuid      = other._uuid;
  _property  = other._property;
  _value     = other._value;
  _timestamp = other._timestamp;
}

////////////////////////////////////////////////////////////////////////////////
Operation::Operation (enum type type, std::string uuid)
: _type (type)
, _uuid (uuid)
{
}

////////////////////////////////////////////////////////////////////////////////
Operation Operation::new_create (std::string uuid)
{
    return Operation (op_create, uuid);
}

////////////////////////////////////////////////////////////////////////////////
Operation Operation::new_delete (std::string uuid)
{
    return Operation (op_delete, uuid);
}

////////////////////////////////////////////////////////////////////////////////
Operation Operation::new_update (std::string uuid, std::string property, std::string value)
{
    Operation op (op_update, uuid);
    op._property = property;
    op._value = std::optional<std::string>{value};
    op._timestamp = _now ? _now : time(NULL);
    return op;
}

////////////////////////////////////////////////////////////////////////////////
Operation Operation::new_update_remove (std::string uuid, std::string property)
{
    Operation op (op_update, uuid);
    op._property = property;
    op._value = std::nullopt;
    op._timestamp = _now ? _now : time(NULL);
    return op;
}

////////////////////////////////////////////////////////////////////////////////
std::tuple<std::optional<Operation>, std::optional<Operation>> Operation::transform(Operation &op1, Operation &op2)
{
    // if the operations are for different tasks, then they have no interdependency and need not be transformed
    if (op1._uuid == op2._uuid) {
        // Two creations or deletions of the same uuid reach the same state, so there's no need
        // for any further operations to bring the state together.
        if (op1._type == op2._type && (op1._type == op_create || op1._type == op_delete)) {
            return std::make_tuple(std::nullopt, std::nullopt);
        }

        // Given a create and a delete of the same task, one of the operations is invalid: the
        // create implies the task does not exist, but the delete implies it exists.  Somewhat
        // arbitrarily, we prefer the Create
        if (op1._type == op_create && op2._type == op_delete) {
            return std::make_tuple(std::optional<Operation>{op1}, std::nullopt);
        }
        if (op1._type == op_delete && op2._type == op_create) {
            return std::make_tuple(std::nullopt, std::optional<Operation>{op2});
        }

        // Similarly, prefer an update over a create (as the update implies the create)
        if (op1._type == op_create && op2._type == op_update) {
            return std::make_tuple(std::nullopt, std::optional<Operation>{op2});
        }
        if (op1._type == op_update && op2._type == op_create) {
            return std::make_tuple(std::optional<Operation>{op1}, std::nullopt);
        }

        // Similarly, prefer a delete over an update
        if (op1._type == op_delete && op2._type == op_update) {
            return std::make_tuple(std::optional<Operation>{op1}, std::nullopt);
        }
        if (op1._type == op_update && op2._type == op_delete) {
            return std::make_tuple(std::nullopt, std::optional<Operation>{op2});
        }

        // Finally, given two updates, prefer the later one, or failing that just
        // the first one
        if (op1._type == op_update && op2._type == op_update && op1._property == op2._property) {
            // if both are updating to the same value, there's nothing to do
            if (op1._value == op2._value) {
                return std::make_tuple(std::nullopt, std::nullopt);
            }

            // otherwise prefer the later modification, or if equal arbitrarily
            // prefer the first operation
            if (op1._timestamp < op2._timestamp) {
                return std::make_tuple(std::nullopt, std::optional<Operation>{op2});
            } else {
                return std::make_tuple(std::optional<Operation>{op1}, std::nullopt);
            }
        }
    }

    // in any other case, the tasks are independent and need not be transformed.
    return std::make_tuple(std::optional<Operation>{op1}, std::optional<Operation>{op2});
}

////////////////////////////////////////////////////////////////////////////////
bool Operation::operator== (const Operation& other) const
{
  Operation left (*this);
  Operation right (other);

  if (left._type != right._type)
    return false;

  if (left._uuid != right._uuid)
    return false;

  if (left._type == op_update) {
    if (left._property != right._property)
      return false;
    if (left._value != right._value)
      return false;
    if (left._timestamp != right._timestamp)
      return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Operation::operator!= (const Operation& other) const
{
  return !(*this == other);
}

////////////////////////////////////////////////////////////////////////////////
void Operation::throw_if_not_update () const
{
    if (_type != op_update) {
        throw std::string ("operation is not an update");
    }
}

////////////////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, Operation const &op) { 
    switch (op.get_type()) {
    case Operation::op_create:
        return os << "Create(" << op.get_uuid() << ")";
    case Operation::op_delete:
        return os << "Delete(" << op.get_uuid() << ")";
    case Operation::op_update:
        std::optional<std::string> v = op.get_value();
        return os << "Update(" << op.get_uuid() << ", \""
                  << op.get_property() << "\", \""
                  << v.value_or("null") << "\", "
                  << op.get_timestamp()
                  << ")";
    }

    return os;
}

////////////////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, std::optional<Operation> const &op) { 
    if (op.has_value()) {
        return os << *op;
    } else {
        return os << "nullopt";
    }
}

////////////////////////////////////////////////////////////////////////////////
