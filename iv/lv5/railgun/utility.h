#ifndef IV_LV5_RAILGUN_UTILITY_H_
#define IV_LV5_RAILGUN_UTILITY_H_
#include <iv/detail/memory.h>
#include <iv/parser.h>
#include <iv/lv5/factory.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/error.h>
#include <iv/lv5/eval_source.h>
#include <iv/lv5/railgun/context.h>
#include <iv/lv5/railgun/jsscript.h>
#include <iv/lv5/railgun/jsfunction.h>
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline Code* CompileFunction(Context* ctx, const JSString* str, Error* e) {
  std::shared_ptr<EvalSource> const src(new EvalSource(*str));
  AstFactory factory(ctx);
  core::Parser<AstFactory, EvalSource> parser(&factory, *src, ctx->symbol_table());
  const FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    e->Report(Error::Syntax, parser.error());
    return NULL;
  }
  const FunctionLiteral* const func =
      internal::IsOneFunctionExpression(*eval, e);
  if (*e) {
    return NULL;
  }
  JSScript* script = JSEvalScript<EvalSource>::New(ctx, src);
  return CompileFunction(ctx, *func, script);
}

inline void Instantiate(Context* ctx,
                        Code* code,
                        Frame* frame,
                        bool is_eval,
                        bool is_global_env,
                        JSVMFunction* func,  // maybe NULL
                        Error* e) {
  // step 1
  JSVal this_value = frame->GetThis();
  JSEnv* env = frame->variable_env();
  if (!code->strict()) {
    if (this_value.IsNullOrUndefined()) {
      this_value = ctx->global_obj();
    } else if (!this_value.IsObject()) {
      JSObject* const obj = this_value.ToObject(ctx, IV_LV5_ERROR_VOID(e));
      this_value = obj;
    }
  }
  frame->set_this_binding(this_value);

  // step 2
  const bool configurable_bindings = is_eval;

  // step 4
  {
    const std::size_t arg_count = frame->argc_;
    JSVal* args = frame->arguments_begin();
    std::size_t n = 0;
    for (Code::Names::const_iterator it = code->params().begin(),
         last = code->params().end(); it != last; ++it) {
      ++n;
      const Symbol& arg_name = *it;
      if (!env->HasBinding(ctx, arg_name)) {
        env->CreateMutableBinding(ctx, arg_name,
                                  configurable_bindings, IV_LV5_ERROR_VOID(e));
      }
      if (n > arg_count) {
        env->SetMutableBinding(
            ctx, arg_name,
            JSUndefined, code->strict(), IV_LV5_ERROR_VOID(e));
      } else {
        env->SetMutableBinding(
            ctx, arg_name,
            args[n-1], code->strict(), IV_LV5_ERROR_VOID(e));
      }
    }
  }

  // step 5
  for (Code::Codes::const_iterator it = code->codes().begin(),
       last = code->codes().end(); it != last; ++it) {
    if ((*it)->IsFunctionDeclaration()) {
      const Symbol& fn = (*it)->name();
      const JSVal fo = JSVMFunction::New(ctx, *it, env);
      // see 10.5 errata
      if (!env->HasBinding(ctx, fn)) {
        env->CreateMutableBinding(ctx, fn,
                                  configurable_bindings, IV_LV5_ERROR_VOID(e));
      } else if (is_global_env) {
        JSObject* const go = ctx->global_obj();
        const PropertyDescriptor existing_prop = go->GetProperty(ctx, fn);
        if (existing_prop.IsConfigurable()) {
          go->DefineOwnProperty(
              ctx,
              fn,
              DataDescriptor(
                  JSUndefined,
                  ATTR::W | ATTR::E |
                  ((configurable_bindings) ? ATTR::C : ATTR::NONE)),
              true, IV_LV5_ERROR_VOID(e));
        } else {
          if (existing_prop.IsAccessorDescriptor()) {
            e->Report(Error::Type,
                      "create mutable function binding failed");
            return;
          }
          const DataDescriptor* const data = existing_prop.AsDataDescriptor();
          if (!data->IsWritable() || !data->IsEnumerable()) {
            e->Report(Error::Type,
                      "create mutable function binding failed");
            return;
          }
        }
      }
      env->SetMutableBinding(ctx, fn, fo, code->strict(), IV_LV5_ERROR_VOID(e));
    }
  }

  // step 6, 7
  if (func) {
    if (code->IsShouldCreateArguments()) {
      JSDeclEnv* decl_env = static_cast<JSDeclEnv*>(env);
      JSObject* args_obj = NULL;
      if (!code->strict()) {
        args_obj = JSNormalArguments::New(
            ctx, func,
            code->params(),
            frame->arguments_rbegin(),
            frame->arguments_rend(), decl_env,
            IV_LV5_ERROR_VOID(e));
      } else {
        args_obj = JSStrictArguments::New(
            ctx, func,
            frame->arguments_rbegin(),
            frame->arguments_rend(),
            IV_LV5_ERROR_VOID(e));
      }
      if (code->strict()) {
        decl_env->CreateImmutableBinding(symbol::arguments());
        decl_env->InitializeImmutableBinding(symbol::arguments(), args_obj);
      } else {
        decl_env->CreateMutableBinding(
            ctx, symbol::arguments(),
            configurable_bindings, IV_LV5_ERROR_VOID(e));
        decl_env->SetMutableBinding(
            ctx, symbol::arguments(),
            args_obj, false, IV_LV5_ERROR_VOID(e));
      }
    }
  }

  // step 8
  for (Code::Names::const_iterator it = code->varnames().begin(),
       last = code->varnames().end(); it != last; ++it) {
    const Symbol& dn = *it;
    if (!env->HasBinding(ctx, dn)) {
      env->CreateMutableBinding(ctx, dn,
                                configurable_bindings, IV_LV5_ERROR_VOID(e));
      env->SetMutableBinding(ctx, dn,
                             JSUndefined, code->strict(), IV_LV5_ERROR_VOID(e));
    }
  }
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_UTILITY_H_