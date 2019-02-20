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

#include <algorithm>
#include <array>
#include <chrono>
#include <ratio>

#include <cedar/scheduler.h>
#include <cedar/object.h>
#include <cedar/object/dict.h>
#include <cedar/object/lambda.h>
#include <cedar/object/list.h>
#include <cedar/object/string.h>
#include <cedar/object/symbol.h>
#include <cedar/object/fiber.h>
#include <cedar/objtype.h>
#include <cedar/parser.h>
#include <cedar/ref.h>
#include <cedar/types.h>
#include <cedar/vm/machine.h>
#include <cedar/vm/opcode.h>
#include <unistd.h>
#include <cedar/util.hpp>
#include <thread>
#include <cedar/globals.h>

#include <gc/gc.h>
#include <unordered_map>
using namespace cedar;

static std::unordered_map<int, ref> macros;

bool vm::is_macro(int id) {
  if (macros.count(id)) {
    return true;
  }
  return false;
}

lambda *vm::get_macro(int id) {
  lambda *mac = ref_cast<lambda>(macros.at(id));
  return mac;
}

void vm::set_macro(int id, ref mac) {
  if (ref_cast<lambda>(mac) == nullptr) {
    throw cedar::make_exception("unable to add macro ", get_symbol_id_runes(id), " to non-lambda ", mac);
  }

  macros[id] = mac;
}

ref vm::macroexpand_1(ref obj) {
  if (obj.is<list>()) {
    // loop till the macroexpand doesnt change the object
    ref first = obj.first();
    symbol *s = ref_cast<symbol>(first);
    if (s != nullptr) {
      int sid = s->id;
      if (vm::is_macro(sid)) {
        lambda *mac = vm::get_macro(sid);
        int argc = 0;
        std::vector<ref> argv;

        ref args = obj.rest();

        while (!args.is_nil()) {
          argv.push_back(args.first());
          argc++;
          args = args.rest();
        }


        call_context ctx;
        ref expanded = call_function(mac, argc, argv.data(), &ctx);

        return expanded;
      }
    }
  }

  return obj;
}

vm::machine *cedar::primary_machine = nullptr;


void init_binding(cedar::vm::machine *m);


vm::machine::machine(void) : m_compiler(this) {
  primary_machine = this;
  // before creating anything, init the types
  type_init();
  init_binding(this);
}

vm::machine::~machine() {}

ref vm::machine::eval_file(cedar::runes path) {
  ref file_star = new symbol("*file*");

  cedar::runes src = cedar::util::read_file(path);

  ref path_obj = new string(path);

  def_global(file_star, path_obj);

  ref val = this->eval_string(std::move(src));

  return val;
}

ref vm::machine::eval_string(cedar::runes expr) {
  cedar::reader reader;
  ref res = nullptr;

  reader.lex_source(expr);
  bool valid = true;


  while (true) {
    ref e = reader.read_one(&valid);
    if (!valid) break;
    res = eval(e);
  }
  return res;
}

// #define VM_TRACE
// #define VM_TRACE_INTERACTIVE
// #define VM_TRACE_INTERACTIVE_AUTO

#ifdef VM_TRACE

static const char *instruction_name(uint8_t op) {
  switch (op) {
#define OP_NAME(name, code, op_type, effect) \
  case code:                                 \
    return #name;
    CEDAR_FOREACH_OPCODE(OP_NAME);
#undef OP_NAME
  }
  return "unknown";
}

#endif

ref vm::machine::eval(ref obj) {
  try {
    ref compiled_lambda = m_compiler.compile(obj, this);
    lambda *raw_program = ref_cast<cedar::lambda>(compiled_lambda);
    ref val = eval_lambda(raw_program);
    return val;
  } catch (std::exception &e) {
    std::cerr << "Uncaught Exception: " << e.what() << std::endl;
  } catch (cedar::ref e) {
    std::cerr << "Uncaught Exception: " << e << std::endl;
  }
  std::cerr << "Exception thrown when evaluating '" << obj << "'" << std::endl;
  return nullptr;
}

