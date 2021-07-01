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
    time(&op._timestamp);
    return op;
}

////////////////////////////////////////////////////////////////////////////////
Operation Operation::new_update_remove (std::string uuid, std::string property)
{
    Operation op (op_update, uuid);
    op._property = property;
    op._value = std::nullopt;
    time(&op._timestamp);
    return op;
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
void Operation::throw_if_not_update ()
{
    if (_type != op_update) {
        throw std::string ("operation is not an update");
    }
}

////////////////////////////////////////////////////////////////////////////////

