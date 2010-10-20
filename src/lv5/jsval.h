#ifndef _IV_LV5_JSVAL_H_
#define _IV_LV5_JSVAL_H_
#include <cmath>
#include <limits>
#include <algorithm>
#include <tr1/array>
#include <tr1/cstdint>
#include <tr1/type_traits>
#include "enable_if.h"
#include "static_assert.h"
#include "utils.h"
#include "jsstring.h"
#include "jsobject.h"
#include "jserrorcode.h"
#include "conversions-inl.h"

namespace iv {
namespace lv5 {

class JSEnv;
class Context;
class JSReference;
class JSEnv;
class JSVal;

namespace detail {
template<std::size_t PointerSize>
struct Layout {
};
#if defined(IS_LITTLE_ENDIAN)
template<>
struct Layout<4> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
      uint32_t tag_;
    } struct_;
  };

  static const std::size_t kExpectedSize = sizeof(double);
};

template<>
struct Layout<8> {
  union {
    struct {
      uint32_t overhead_;
      double as_;
    } number_;
    struct {
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
      uint32_t tag_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 12;
};
#else
template<>
struct Layout<4> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = sizeof(double);
};

template<>
struct Layout<8> {
  union {
    struct {
      double as_;
    } number_;
    struct {
      uint32_t tag_;
      union {
        bool boolean_;
        JSObject* object_;
        JSString* string_;
        JSReference* reference_;
        JSEnv* environment_;
      } payload_;
    } struct_;
  };

  static const std::size_t kExpectedSize = 12;
};
#endif  // define(IS_LITTLE_ENDIAN)

struct JSTrueType { };
struct JSFalseType { };
struct JSNullType { };
struct JSUndefinedType { };

}  // namespace iv::lv5::detail

typedef bool (*JSTrueKeywordType)(JSVal, detail::JSTrueType);
typedef bool (*JSFalseKeywordType)(JSVal, detail::JSFalseType);
typedef bool (*JSNullKeywordType)(JSVal, detail::JSNullType);
typedef bool (*JSUndefinedKeywordType)(JSVal, detail::JSUndefinedType);
inline bool JSTrue(JSVal x, detail::JSTrueType dummy);
inline bool JSFalse(JSVal x, detail::JSFalseType dummy);
inline bool JSNull(JSVal x, detail::JSNullType dummy);
inline bool JSUndefined(JSVal x, detail::JSUndefinedType dummy);

class JSVal {
 public:
  typedef JSVal this_type;
  typedef detail::Layout<core::Size::kPointerSize> value_type;

  IV_STATIC_ASSERT(sizeof(value_type) == value_type::kExpectedSize);
  IV_STATIC_ASSERT(std::tr1::is_pod<value_type>::value);

  enum Tag {
    NUMBER = 0,
    OBJECT,
    STRING,
    BOOLEAN,
    NULLVALUE,
    UNDEFINED,
    REFERENCE,
    ENVIRONMENT
  };

  static const uint32_t kTrueTag        = 0xffffffff;
  static const uint32_t kFalseTag       = 0xfffffffe;
  static const uint32_t kEmptyTag       = 0xfffffffd;
  static const uint32_t kEnvironmentTag = 0xfffffffc;
  static const uint32_t kReferenceTag   = 0xfffffffb;
  static const uint32_t kUndefinedTag   = 0xfffffffa;
  static const uint32_t kNullTag        = 0xfffffff9;
  static const uint32_t kBoolTag        = 0xfffffff8;
  static const uint32_t kStringTag      = 0xfffffff7;
  static const uint32_t kObjectTag      = 0xfffffff6;
  static const uint32_t kNumberTag      = 0xfffffff5;
  static const uint32_t kHighestTag     = kTrueTag;
  static const uint32_t kLowestTag      = kNumberTag;
  static const uint32_t kVtables        = kHighestTag - kLowestTag + 1;

  JSVal();
  JSVal(const JSVal& rhs);
  JSVal(const double& val);  // NOLINT
  JSVal(JSObject* val);  // NOLINT
  JSVal(JSString* val);  // NOLINT
  JSVal(JSReference* val);  // NOLINT
  JSVal(JSEnv* val);  // NOLINT
  JSVal(JSTrueKeywordType val);  // NOLINT
  JSVal(JSFalseKeywordType val);  // NOLINT
  JSVal(JSNullKeywordType val);  // NOLINT
  JSVal(JSUndefinedKeywordType val);  // NOLINT
  template<typename T>
  JSVal(T val, typename enable_if<std::tr1::is_same<bool, T> >::type* = 0) {
    typedef std::tr1::is_same<bool, T> cond;
    IV_STATIC_ASSERT(!(cond::value));
  }

  this_type& operator=(const this_type&);

  inline void set_value(double val) {
    value_.number_.as_ = val;
  }
  inline void set_value(JSObject* val) {
    value_.struct_.payload_.object_ = val;
    value_.struct_.tag_ = kObjectTag;
  }
  inline void set_value(JSString* val) {
    value_.struct_.payload_.string_ = val;
    value_.struct_.tag_ = kStringTag;
  }
  inline void set_value(JSReference* ref) {
    value_.struct_.payload_.reference_ = ref;
    value_.struct_.tag_ = kReferenceTag;
  }
  inline void set_value(JSEnv* ref) {
    value_.struct_.payload_.environment_ = ref;
    value_.struct_.tag_ = kEnvironmentTag;
  }
  inline void set_value(JSTrueKeywordType val) {
    value_.struct_.payload_.boolean_ = true;
    value_.struct_.tag_ = kBoolTag;
  }
  inline void set_value(JSFalseKeywordType val) {
    value_.struct_.payload_.boolean_ = false;
    value_.struct_.tag_ = kBoolTag;
  }
  inline void set_value(JSNullKeywordType val) {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kNullTag;
  }
  inline void set_value(JSUndefinedKeywordType val) {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kUndefinedTag;
  }
  inline void set_null() {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kNullTag;
  }
  inline void set_undefined() {
    value_.struct_.payload_.boolean_ = NULL;
    value_.struct_.tag_ = kUndefinedTag;
  }

  inline JSReference* reference() const {
    assert(IsReference());
    return value_.struct_.payload_.reference_;
  }

  inline JSEnv* environment() const {
    assert(IsEnvironment());
    return value_.struct_.payload_.environment_;
  }

  inline JSString* string() const {
    assert(IsString());
    return value_.struct_.payload_.string_;
  }

  inline JSObject* object() const {
    assert(IsObject());
    return value_.struct_.payload_.object_;
  }

  inline const double& number() const {
    assert(IsNumber());
    return value_.number_.as_;
  }

  inline bool boolean() const {
    assert(IsBoolean());
    return value_.struct_.payload_.boolean_;
  }

  inline bool IsUndefined() const {
    return value_.struct_.tag_ == kUndefinedTag;
  }
  inline bool IsNull() const {
    return value_.struct_.tag_ == kNullTag;
  }
  inline bool IsBoolean() const {
    return value_.struct_.tag_ == kBoolTag;
  }
  inline bool IsString() const {
    return value_.struct_.tag_ == kStringTag;
  }
  inline bool IsObject() const {
    return value_.struct_.tag_ == kObjectTag;
  }
  inline bool IsNumber() const {
    return value_.struct_.tag_ < kLowestTag;
  }
  inline bool IsReference() const {
    return value_.struct_.tag_ == kReferenceTag;
  }
  inline bool IsEnvironment() const {
    return value_.struct_.tag_ == kEnvironmentTag;
  }
  inline bool IsPrimitive() const {
    return IsNumber() || IsString() || IsBoolean();
  }
  inline JSString* TypeOf(Context* ctx) const {
    if (IsObject()) {
      if (object()->IsCallable()) {
        return JSString::NewAsciiString(ctx, "function");
      } else {
        return JSString::NewAsciiString(ctx, "object");
      }
    } else if (IsNumber()) {
      return JSString::NewAsciiString(ctx, "number");
    } else if (IsString()) {
      return JSString::NewAsciiString(ctx, "string");
    } else if (IsBoolean()) {
      return JSString::NewAsciiString(ctx, "boolean");
    } else if (IsNull()) {
      return JSString::NewAsciiString(ctx, "null");
    } else {
      assert(IsUndefined());
      return JSString::NewAsciiString(ctx, "undefined");
    }
  }
  inline uint32_t type() const {
    return IsNumber() ? kNumberTag : value_.struct_.tag_;
  }

  inline JSObject* ToObject(Context* ctx, JSErrorCode::Type* res) const {
    if (IsObject()) {
      return object();
    } else if (IsNumber()) {
      return JSNumberObject::New(ctx, number());
    } else if (IsString()) {
      return JSStringObject::New(ctx, string());
    } else if (IsBoolean()) {
      return JSBooleanObject::New(ctx, boolean());
    } else if (IsNull() || IsUndefined()) {
      *res = JSErrorCode::TypeError;
      return NULL;
    } else {
      UNREACHABLE();
    }
  }

  inline JSString* ToString(Context* ctx,
                            JSErrorCode::Type* res) const {
    if (IsString()) {
      return string();
    } else if (IsNumber()) {
      std::tr1::array<char, 80> buffer;
      const char* const str = core::DoubleToCString(number(),
                                                    buffer.data(),
                                                    buffer.size());
      return JSString::NewAsciiString(ctx, str);
    } else if (IsBoolean()) {
      return JSString::NewAsciiString(ctx, (boolean() ? "true" : "false"));
    } else if (IsNull()) {
      return JSString::NewAsciiString(ctx, "null");
    } else if (IsUndefined()) {
      return JSString::NewAsciiString(ctx, "undefined");
    } else {
      assert(IsObject());
      JSVal prim = object()->DefaultValue(ctx, JSObject::STRING, res);
      if (*res) {
        return NULL;
      }
      return prim.ToString(ctx, res);
    }
  }

  inline double ToNumber(Context* ctx, JSErrorCode::Type* res) const {
    if (IsNumber()) {
      return number();
    } else if (IsString()) {
      return core::StringToDouble(*string());
    } else if (IsBoolean()) {
      return boolean() ? 1 : +0;
    } else if (IsNull()) {
      return +0;
    } else if (IsUndefined()) {
      return std::numeric_limits<double>::quiet_NaN();
    } else {
      assert(IsObject());
      JSVal prim = object()->DefaultValue(ctx, JSObject::NUMBER, res);
      if (*res) {
        return NULL;
      }
      return prim.ToNumber(ctx, res);
    }
  }

  inline JSVal ToPrimitive(Context* ctx,
                           JSObject::Hint hint, JSErrorCode::Type* res) const {
    if (IsObject()) {
      return object()->DefaultValue(ctx, hint, res);
    } else {
      assert(!IsEnvironment() && !IsReference());
      return *this;
    }
  }

  inline void CheckObjectCoercible(JSErrorCode::Type* res) const {
    assert(!IsEnvironment() && !IsReference());
    if (IsNull() || IsUndefined()) {
      *res = JSErrorCode::TypeError;
    }
  }

  inline bool IsCallable() const {
    return IsObject() && object()->IsCallable();
  }

  inline bool ToBoolean(JSErrorCode::Type* res) const {
    if (IsNumber()) {
      const double& num = number();
      return num != 0 && !std::isnan(num);
    } else if (IsString()) {
      return !string()->empty();
    } else if (IsNull()) {
      return false;
    } else if (IsUndefined()) {
      return false;
    } else if (IsBoolean()) {
      return boolean();
    } else {
      return true;
    }
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(value_, rhs.value_);
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

 private:
   value_type value_;
};

inline bool JSTrue(JSVal x, detail::JSTrueType dummy = detail::JSTrueType()) {
  return true;
}

inline bool JSFalse(JSVal x, detail::JSFalseType dummy = detail::JSFalseType()) {
  return false;
}

inline bool JSNull(JSVal x, detail::JSNullType dummy = detail::JSNullType()) {
  return false;
}

inline bool JSUndefined(JSVal x, detail::JSUndefinedType dummy = detail::JSUndefinedType()) {
  return false;
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSVAL_H_
