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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cedar/object.h>
#include <cedar/ref.h>

namespace cedar {

  class sequence_ : virtual public object {
   public:
    virtual ~sequence_(){};
    // get the first of the sequence
    virtual ref first(void) = 0;
    // get the rest of the sequence
    virtual ref rest(void) = 0;
    // cons an object onto the object, returning the mutated item
    virtual ref cons(ref f);


    virtual void set_first(ref) = 0;
    virtual void set_rest(ref) = 0;

   protected:
    cedar::runes to_string(bool) {
      cedar::runes s;

      if (rest().is_nil()) {
        if (first().is_nil()) return "(nil)";
        s += "(";
        s += first().to_string();
        s += ")";
        return s;
      }

      if (is_pair()) {
        s += "(";
        s += first().to_string();
        s += " . ";
        s += rest().to_string();
        s += ")";
        return s;
      }

      s += "(";
      ref curr = this;

      while (true) {
        s += curr.first().to_string();

        if (curr.rest().is_nil()) {
          break;
        } else {
          s += " ";
        }

        if (!curr.rest().is_nil()) {
          if (curr.rest()->is_pair()) {
            s += curr.rest().first().to_string();
            s += " . ";
            s += curr.rest().rest().to_string();
            break;
          }
        }

        curr = curr.rest();
      }

      s += ")";

      return s;
    }
  };
}  // namespace cedar

