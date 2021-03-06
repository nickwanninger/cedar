/*
 * MIT License
 *
 * Copyright (c) 2018 Nick Wanninger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdio>

#include <string>
#include <functional>
#include <cedar/ref.h>
#include <cedar/object.h>
#include <cedar/object/vector.h>
#include <cedar/object/list.h>
#include <cedar/object/symbol.h>
#include <cedar/memory.h>
#include <cedar/util.hpp>


using namespace cedar;


cedar::vector::vector(immer::flex_vector<ref> v) {
  m_type = vector_type;
  items = v;
}

cedar::vector::vector(void) {
  m_type = vector_type;
}

cedar::vector::~vector(void) {
}

ref vector::at(int i) {
  return const_cast<ref&>(items[i]);
}

cedar::runes vector::to_string(bool human) {
	cedar::runes str = "[";

  for (u32 i = 0; i < items.size(); i++) {
    str += at(i).to_string(false);
    if (i != items.size()-1) str += " ";
  }
	str += "]";
	return str;
}

ref vector::get(ref k) {
  i64 i = k.to_int();
  if (i < 0 || i >= (i64)items.size()) return nullptr;
	return at(i);
}


ref vector::set(ref k, ref v) {
  i64 i = k.to_int();
  if (i < 0 || i >= (i64)items.size())
    throw cedar::make_exception("vector set out of bounds");

  return new vector(items.set(i, v));
}

ref vector::append(ref v) {
  const auto v2 = items.push_back(v);
  return new vector(v2);
}

u64 vector::hash(void) {

	u64 x = 0;
	u64 y = 0;

	i64 mult = 1000003UL; // prime multiplier

	x = 0x345678UL;

	for (auto it : items) {
		y = const_cast<ref&>(it).hash();
		x = (x ^ y) * mult;
		mult += (u64)(852520UL + 2);
	}
	return x;
}


ref vector::first(void) {
  if (items.size() == 0) return nullptr;
  return at(0);
}

ref vector::rest(void) {
  if (items.size() <= 1) return nullptr;
  return new vector(items.drop(1));
}

ref vector::cons(ref f) {
  return new vector(items.push_front(f));
}
