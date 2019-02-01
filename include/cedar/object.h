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

#include <cedar/memory.h>
#include <cedar/ref.h>
#include <cedar/runes.h>
#include <cedar/exception.hpp>
#include <cedar/runes.h>
#include <cedar/types.h>
#include <cxxabi.h>
#include <atomic>
#include <cstdint>
#include <new>
#include <map>
#include <cedar/with_meta.h>

#define GC_OPERATOR_NEW_ARRAY
#include <gc/gc.h>
#include <gc/gc_cpp.h>


namespace cedar {


  class object;
  class object_type;

  extern long object_count;

  // forward declare the type object and the dict type
  class type;
  class dict;

  class object : public gc_cleanup, public with_meta {
   public:

    // object_type *type = nullptr;
    // refcount is used by the `ref` class to determine how many things hold
    // references to this particular object on the heap
    u32 refcount = 0;

    type *m_type;
    dict *m_attrs;


    virtual u64 hash(void) = 0;

    object();
    ~object();

    virtual const char *object_type_name(void) = 0;

    bool is_pair(void);

   protected:

    // refs should be able to access values of objects, as it is always safer to
    // go through a reference to call these functions
    friend ref;


    virtual cedar::runes to_string(bool human = false) = 0;

    /*
     * type_name
     *
     * returns the c++ abi demangled type name of the polymorphic object class
     * that is extending the cedar::object class
     */
    inline cedar::runes type_name() {
      int status;

      const char *given_name = typeid(*this).name();
      // use the c++ abi to demangle the name that typeid gives back;
      char *realname = abi::__cxa_demangle(given_name, 0, 0, &status);
      if (status) {
        throw cedar::make_exception(
            "unable to demangle name of type with the c++ abi: ", given_name);
      }

      cedar::runes r = realname;
      delete realname;
      return r;
    }

    /*
     * is<T>
     *
     * returns if the object is a T or not.
     */
    template <typename T>
    inline bool is() const {
      return typeid(T).hash_code() == type_id();
    }

    /*
     * as<T>
     *
     * attempts to cast the object to a shared pointer of another object type
     */
    template <typename T>
    inline T *as() {
      return dynamic_cast<T *>(this);
    }

    /*
     * type_id
     *
     * returns the type hash_code from typeid()
     */
    inline size_t type_id() const { return typeid(*this).hash_code(); }

  };

}  // namespace cedar
