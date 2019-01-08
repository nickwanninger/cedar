/* Auto-generated by tools/scripts/generate_opcode_h.py */
#pragma once
#ifndef __OPCODE_H
#define __OPCODE_H

/* Instruction opcodes for compiled code */
#define OP_NOP                      0x00
#define OP_NIL                      0x01
#define OP_CONST                    0x02
#define OP_FLOAT                    0x03
#define OP_INT                      0x04
#define OP_LOAD_LOCAL               0x05
#define OP_SET_LOCAL                0x06
#define OP_LOAD_GLOBAL              0x07
#define OP_SET_GLOBAL               0x08
#define OP_CONS                     0x09
#define OP_CALL                     0x0a
#define OP_MAKE_FUNC                0x0b
#define OP_ARG_POP                  0x0c
#define OP_RETURN                   0x0d

/* Instruction opcode foreach macro for code generation */
/* Arg order: (name, bytecode, type, stack effect */
#define CEDAR_FOREACH_OPCODE(V) \
  V(NOP, OP_NOP, no_arg, 0) \
  V(NIL, OP_NIL, no_arg, 1) \
  V(CONST, OP_CONST, imm_int, 1) \
  V(FLOAT, OP_FLOAT, imm_float, 1) \
  V(INT, OP_INT, imm_int, 1) \
  V(LOAD_LOCAL, OP_LOAD_LOCAL, imm_int, 1) \
  V(SET_LOCAL, OP_SET_LOCAL, imm_int, -1) \
  V(LOAD_GLOBAL, OP_LOAD_GLOBAL, imm_int, 0) \
  V(SET_GLOBAL, OP_SET_GLOBAL, imm_int, -1) \
  V(CONS, OP_CONS, no_arg, -1) \
  V(CALL, OP_CALL, no_arg, -1) \
  V(MAKE_FUNC, OP_MAKE_FUNC, imm_int, 1) \
  V(ARG_POP, OP_ARG_POP, imm_int, 1) \
  V(RETURN, OP_RETURN, no_arg, 0)

#endif
