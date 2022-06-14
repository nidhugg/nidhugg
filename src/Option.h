/* Copyright (C) 2017 Magnus Lång
 *
 * This file is part of Nidhugg.
 *
 * Nidhugg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nidhugg is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __OPTION_H__
#define __OPTION_H__

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>

template<typename Value>
class Option {
public:
  Option() : has_value(false) {}
  Option(std::nullptr_t) : has_value(false) {}
  Option(Value value) : has_value(true), value(std::move(value)) {}
  ~Option() { if (has_value) value.~Value(); }
  Option(const Option &o) : has_value(o.has_value) {
    if (has_value) new((void*)&value) Value(o.value);
  }
  Option(Option &&o) : has_value(o.has_value) {
    if (has_value) new((void*)&value) Value(std::move(o.value));
  }
  Option &operator=(const Option &o) {
    if (has_value) value.~Value();
    if ((has_value = o.has_value)) new((void*)&value) Value(o.value);
    return *this;
  }
  Option &operator=(Option &&o) {
    if (has_value) value.~Value();
    if ((has_value = o.has_value)) new((void*)&value) Value(std::move(o.value));
    return *this;
  }

  operator bool() const noexcept { return has_value; }
  Value const& operator *() const { assert(has_value); return value; }
  Value& operator *() { assert(has_value); return value; }
  Value const *operator ->() const { assert(has_value); return &value; }
  Value *operator ->() { assert(has_value); return &value; }

  Value const &value_or(const Value &def) const {
    return has_value ? value : def;
  }

  /* Monadic bind; transform the value of the option (if any) */
  template<typename F> auto map(F f) ->
    Option<typename std::result_of<F(Value&)>::type> {
    if (!has_value) return {};
    else return f(value);
  }

private:
  bool has_value;
  union{
    Value value;
  };
};

#endif
