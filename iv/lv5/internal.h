#ifndef IV_LV5_INTERNAL_H_
#define IV_LV5_INTERNAL_H_
#include <iv/platform_math.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/context.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace internal {

inline JSVal FromPropertyDescriptor(Context* ctx,
                                    const PropertyDescriptor& desc) {
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  JSObject* const obj = JSObject::New(ctx);
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    obj->DefineOwnProperty(
        ctx, context::Intern(ctx, "value"),
        DataDescriptor(data->value(), ATTR::W | ATTR::E | ATTR::C),
        false, NULL);
    obj->DefineOwnProperty(
        ctx, context::Intern(ctx, "writable"),
        DataDescriptor(JSVal::Bool(data->IsWritable()),
                       ATTR::W | ATTR::E | ATTR::C),
        false, NULL);
  } else {
    assert(desc.IsAccessorDescriptor());
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    const JSVal getter = (accs->get()) ? accs->get() : JSVal(JSUndefined);
    obj->DefineOwnProperty(
        ctx, context::Intern(ctx, "get"),
        DataDescriptor(getter, ATTR::W | ATTR::E | ATTR::C),
        false, NULL);
    const JSVal setter = (accs->set()) ? accs->set() : JSVal(JSUndefined);
    obj->DefineOwnProperty(
        ctx, context::Intern(ctx, "set"),
        DataDescriptor(setter, ATTR::W | ATTR::E | ATTR::C),
        false, NULL);
  }
  obj->DefineOwnProperty(
      ctx, context::Intern(ctx, "enumerable"),
      DataDescriptor(JSVal::Bool(desc.IsEnumerable()),
                     ATTR::W | ATTR::E | ATTR::C),
      false, NULL);
  obj->DefineOwnProperty(
      ctx, context::Intern(ctx, "configurable"),
      DataDescriptor(JSVal::Bool(desc.IsConfigurable()),
                     ATTR::W | ATTR::E | ATTR::C),
      false, NULL);
  return obj;
}

inline PropertyDescriptor ToPropertyDescriptor(Context* ctx,
                                               const JSVal& target,
                                               Error* e) {
  if (!target.IsObject()) {
    e->Report(Error::Type, "ToPropertyDescriptor requires Object argument");
    return JSEmpty;
  }
  int attr = ATTR::DEFAULT;
  JSObject* const obj = target.object();
  JSVal value = JSUndefined;
  JSObject* getter = NULL;
  JSObject* setter = NULL;
  {
    // step 3
    const Symbol sym = context::Intern(ctx, "enumerable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      const bool enumerable = r.ToBoolean(IV_LV5_ERROR(e));
      if (enumerable) {
        attr = (attr & ~ATTR::UNDEF_ENUMERABLE) | ATTR::ENUMERABLE;
      } else {
        attr = (attr & ~ATTR::UNDEF_ENUMERABLE);
      }
    }
  }
  {
    // step 4
    const Symbol sym = context::Intern(ctx, "configurable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      const bool configurable = r.ToBoolean(IV_LV5_ERROR(e));
      if (configurable) {
        attr = (attr & ~ATTR::UNDEF_CONFIGURABLE) | ATTR::CONFIGURABLE;
      } else {
        attr = (attr & ~ATTR::UNDEF_CONFIGURABLE);
      }
    }
  }
  {
    // step 5
    const Symbol sym = context::Intern(ctx, "value");
    if (obj->HasProperty(ctx, sym)) {
      value = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      attr |= ATTR::DATA;
      attr &= ~ATTR::UNDEF_VALUE;
    }
  }
  {
    // step 6
    const Symbol sym = context::Intern(ctx, "writable");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      const bool writable = r.ToBoolean(IV_LV5_ERROR(e));
      attr |= ATTR::DATA;
      attr &= ~ATTR::UNDEF_WRITABLE;
      if (writable) {
        attr |= ATTR::WRITABLE;
      }
    }
  }
  {
    // step 7
    const Symbol sym = context::Intern(ctx, "get");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      if (!r.IsCallable() && !r.IsUndefined()) {
        e->Report(Error::Type, "property \"get\" is not callable");
        return JSEmpty;
      }
      attr |= ATTR::ACCESSOR;
      if (!r.IsUndefined()) {
        getter = r.object();
      }
      attr &= ~ATTR::UNDEF_GETTER;
    }
  }
  {
    // step 8
    const Symbol sym = context::Intern(ctx, "set");
    if (obj->HasProperty(ctx, sym)) {
      const JSVal r = obj->Get(ctx, sym, IV_LV5_ERROR(e));
      if (!r.IsCallable() && !r.IsUndefined()) {
        e->Report(Error::Type, "property \"set\" is not callable");
        return JSEmpty;
      }
      attr |= ATTR::ACCESSOR;
      if (!r.IsUndefined()) {
        setter = r.object();
      }
      attr &= ~ATTR::UNDEF_SETTER;
    }
  }
  // step 9
  if (attr & ATTR::ACCESSOR) {
    if (attr & ATTR::DATA) {
      e->Report(Error::Type, "invalid object for property descriptor");
      return JSEmpty;
    }
  }
  if (attr & ATTR::ACCESSOR) {
    return AccessorDescriptor(getter, setter, attr);
  } else if (attr & ATTR::DATA) {
    return DataDescriptor(value, attr);
  } else {
    return GenericDescriptor(attr);
  }
}

template<typename Builder>
inline void BuildFunctionSource(Builder* builder,
                                const Arguments& args, Error* e) {
  const std::size_t arg_count = args.size();
  Context* const ctx = args.ctx();
  if (arg_count == 0) {
    builder->Append("(function() { \n})");
  } else if (arg_count == 1) {
    builder->Append("(function() { ");
    JSString* const str = args[0].ToString(ctx, IV_LV5_ERROR_VOID(e));
    builder->AppendJSString(*str);
    builder->Append("\n})");
  } else {
    builder->Append("(function(");
    Arguments::const_pointer it = args.data();
    const Arguments::const_pointer last = args.data() + args.size() - 1;
    do {
      JSString* const str = it->ToString(ctx, IV_LV5_ERROR_VOID(e));
      builder->AppendJSString(*str);
      ++it;
      if (it != last) {
        builder->push_back(',');
      } else {
        break;
      }
    } while (true);
    builder->Append(") { ");
    JSString* const prog = last->ToString(ctx, IV_LV5_ERROR_VOID(e));
    builder->AppendJSString(*prog);
    builder->Append("\n})");
  }
}

inline const FunctionLiteral* IsOneFunctionExpression(
    const FunctionLiteral& func,
    Error* e) {
  const FunctionLiteral::Statements& stmts = func.body();
  if (stmts.size() == 1) {
    const Statement& stmt = *stmts[0];
    if (stmt.AsExpressionStatement()) {
      if (const FunctionLiteral* func =
          stmt.AsExpressionStatement()->expr()->AsFunctionLiteral()) {
        return func;
      }
    }
  }
  e->Report(Error::Syntax, "Function Constructor with invalid arguments");
  return NULL;
}

inline uint32_t GetLength(Context* ctx, JSObject* obj, Error* e) {
  if (obj->IsClass<Class::Array>()) {
    return static_cast<JSArray*>(obj)->GetLength();
  }
  const JSVal length = obj->Get(ctx,
                                symbol::length(),
                                IV_LV5_ERROR_WITH(e, 0));
  return length.ToUInt32(ctx, e);
}

} } }  // namespace iv::lv5::internal
#endif  // IV_LV5_INTERNAL_H_