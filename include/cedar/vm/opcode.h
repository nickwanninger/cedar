/* Auto-generated by tools/scripts/generate_opcode_h.py */
#pragma once
#ifndef __OPCODE_H
#define __OPCODE_H

/* Instruction opcodes for compiled code */
#define OP_NOP                      0x00
#define OP_NIL                      0x03
#define OP_NILS                     0x06
#define OP_TRUE                     0x09
#define OP_CONST                    0x0c
#define OP_FLOAT                    0x0f
#define OP_INT                      0x12
#define OP_LOAD_LOCAL               0x15
#define OP_SET_LOCAL                0x18

/* Instruction opcode foreach macro for code generation */
#define CEDAR_FOREACH_OPCODE(V) \
  V(NOP, OP_NOP, no_arg) \
  V(NIL, OP_NIL, no_arg) \
  V(NILS, OP_NILS, imm_int) \
  V(TRUE, OP_TRUE, no_arg) \
  V(CONST, OP_CONST, imm_int) \
  V(FLOAT, OP_FLOAT, imm_float) \
  V(INT, OP_INT, imm_int) \
  V(LOAD_LOCAL, OP_LOAD_LOCAL, imm_int) \
  V(SET_LOCAL, OP_SET_LOCAL, imm_int)

#endif