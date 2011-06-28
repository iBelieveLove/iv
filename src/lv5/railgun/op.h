#ifndef _IV_LV5_RAILGUN_OP_H_
#define _IV_LV5_RAILGUN_OP_H_
#include <cassert>
#include "detail/cstdint.h"
#include "detail/array.h"
#include "static_assert.h"
namespace iv {
namespace lv5 {
namespace railgun {

#define IV_LV5_RAILGUN_OP_LIST(V)\
V(STOP_CODE)\
V(NOP)\
\
V(POP_TOP)\
V(POP_TOP_AND_RET)\
V(ROT_TWO)\
V(ROT_THREE)\
V(ROT_FOUR)\
V(DUP_TOP)\
V(DUP_TWO)\
\
V(UNARY_POSITIVE)\
V(UNARY_NEGATIVE)\
V(UNARY_NOT)\
V(UNARY_BIT_NOT)\
\
V(BINARY_ADD)\
V(BINARY_SUBTRACT)\
V(BINARY_MULTIPLY)\
V(BINARY_DIVIDE)\
V(BINARY_MODULO)\
V(BINARY_LSHIFT)\
V(BINARY_RSHIFT)\
V(BINARY_RSHIFT_LOGICAL)\
V(BINARY_LT)\
V(BINARY_LTE)\
V(BINARY_GT)\
V(BINARY_GTE)\
V(BINARY_INSTANCEOF)\
V(BINARY_IN)\
V(BINARY_EQ)\
V(BINARY_STRICT_EQ)\
V(BINARY_NE)\
V(BINARY_STRICT_NE)\
V(BINARY_BIT_AND)\
V(BINARY_BIT_XOR)\
V(BINARY_BIT_OR)\
\
V(RETURN)\
V(RETURN_RET_VALUE)\
V(THROW)\
V(POP_ENV)\
V(WITH_SETUP)\
V(RETURN_SUBROUTINE)\
V(DEBUGGER)\
\
V(PUSH_EMPTY)\
V(PUSH_UNDEFINED)\
V(PUSH_TRUE)\
V(PUSH_FALSE)\
V(PUSH_NULL)\
V(PUSH_THIS)\
V(PUSH_ARGUMENTS)\
\
V(BUILD_OBJECT)\
V(BUILD_REGEXP)\
\
V(LOAD_ELEMENT)\
V(STORE_ELEMENT)\
V(DELETE_ELEMENT)\
V(CALL_ELEMENT)\
V(INCREMENT_ELEMENT)\
V(DECREMENT_ELEMENT)\
V(POSTFIX_INCREMENT_ELEMENT)\
V(POSTFIX_DECREMENT_ELEMENT)\
\
V(STORE_CALL_RESULT)\
V(DELETE_CALL_RESULT)\
V(CALL_CALL_RESULT)\
V(INCREMENT_CALL_RESULT)\
V(DECREMENT_CALL_RESULT)\
V(POSTFIX_INCREMENT_CALL_RESULT)\
V(POSTFIX_DECREMENT_CALL_RESULT)\
\
V(TYPEOF)\
\
/* opcodes over this requres argument */\
V(HAVE_ARGUMENT)\
\
V(STORE_OBJECT_DATA)\
V(STORE_OBJECT_GET)\
V(STORE_OBJECT_SET)\
\
V(LOAD_NAME)\
V(STORE_NAME)\
V(DELETE_NAME)\
V(CALL_NAME)\
V(INCREMENT_NAME)\
V(DECREMENT_NAME)\
V(POSTFIX_INCREMENT_NAME)\
V(POSTFIX_DECREMENT_NAME)\
\
V(LOAD_PROP)\
V(STORE_PROP)\
V(DELETE_PROP)\
V(CALL_PROP)\
V(INCREMENT_PROP)\
V(DECREMENT_PROP)\
V(POSTFIX_INCREMENT_PROP)\
V(POSTFIX_DECREMENT_PROP)\
\
/* not implemented */\
V(LOAD_GLOBAL)\
/* not implemented */\
V(STORE_GLOBAL)\
/* not implemented */\
V(DELETE_GLOBAL)\
\
V(LOAD_CONST)\
\
V(TYPEOF_NAME)\
\
V(JUMP_FORWARD)\
V(JUMP_SUBROUTINE)\
V(JUMP_RETURN_HOOKED_SUBROUTINE)\
V(JUMP_IF_FALSE_OR_POP)\
V(JUMP_IF_TRUE_OR_POP)\
V(JUMP_ABSOLUTE)\
V(POP_JUMP_IF_FALSE)\
V(POP_JUMP_IF_TRUE)\
\
V(FORIN_SETUP)\
V(FORIN_ENUMERATE)\
V(SWITCH_CASE)\
V(SWITCH_DEFAULT)\
V(TRY_CATCH_SETUP)\
\
/* not implemented */\
V(LOAD_LOCAL)\
/* not implemented */\
V(STORE_LOCAL)\
/* not implemented */\
V(DELETE_LOCAL)\
\
V(CALL)\
V(CONSTRUCT)\
V(EVAL)\
\
V(MAKE_CLOSURE)\
\
V(INIT_ARRAY_ELEMENT)\
V(BUILD_ARRAY)\
\
/* opcodes over this requres 2 argument */\
V(HAVE_EX_ARGUMENT)\

struct ERR {
  enum Type {
    NOT_REFERENCE = 0,
    NUM_OF_ERR
  };
};

struct OP {
#define IV_LV5_RAILGUN_DEFINE_ENUM(V) V,
  enum Type {
    IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_ENUM)
    NUM_OF_OP
  };
#undef IV_LV5_RAILGUN_DEFINE_ENUM

  IV_STATIC_ASSERT(NUM_OF_OP <= 256);

  static inline bool HasArg(uint8_t opcode) {
    return opcode >= HAVE_ARGUMENT;
  }

  template<OP::Type op>
  struct IsNameLookupOP {
    static const bool value =
        op == OP::LOAD_NAME ||
        op == OP::DELETE_NAME ||
        op == OP::CALL_NAME ||
        op == OP::INCREMENT_NAME ||
        op == OP::DECREMENT_NAME ||
        op == OP::POSTFIX_INCREMENT_NAME ||
        op == OP::POSTFIX_DECREMENT_NAME ||
        op == OP::TYPEOF_NAME;
  };

  static inline const char* String(int op);
};

#define IV_LV5_RAILGUN_DEFINE_STRINGS(V) #V,
static const std::array<const char*, OP::NUM_OF_OP + 1> kOPString = { {
  IV_LV5_RAILGUN_OP_LIST(IV_LV5_RAILGUN_DEFINE_STRINGS)
  "NUM_OF_OP"
} };
#undef IV_LV5_RAILGUN_DEFINE_STRINGS

const char* OP::String(int op) {
  assert(0 <= op && op < NUM_OF_OP);
  return kOPString[op];
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_OP_H_
