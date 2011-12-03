#ifndef IV_LV5_TELEPORTER_INTERPRETER_H_
#define IV_LV5_TELEPORTER_INTERPRETER_H_
#include <cstdio>
#include <cassert>
#include <cmath>
#include <iv/detail/tuple.h>
#include <iv/detail/array.h>
#include <iv/token.h>
#include <iv/maybe.h>
#include <iv/platform_math.h>
#include <iv/lv5/hint.h>
#include <iv/lv5/jsreference.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsregexp.h>
#include <iv/lv5/jserror.h>
#include <iv/lv5/jsarguments.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/context.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/teleporter/interpreter_fwd.h>
#include <iv/lv5/teleporter/context.h>
#include <iv/lv5/teleporter/utility.h>
namespace iv {
namespace lv5 {
namespace teleporter {

#define CHECK IV_LV5_ERROR_VOID(ctx_->error())

#define CHECK_IN_STMT  ctx_->error());\
  if (ctx_->IsError()) {\
    RETURN_STMT(Context::THROW, JSEmpty, NULL);\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define RETURN_STMT(type, val, target)\
  do {\
    ctx_->SetStatement(type, val, target);\
    return;\
  } while (0)


#define ABRUPT()\
  do {\
    return;\
  } while (0)

#define EVAL(node)\
  node->Accept(this);\
  if (ctx_->IsError()) {\
    return;\
  }

#define EVAL_IN_STMT(node)\
  node->Accept(this);\
  if (ctx_->IsError()) {\
    RETURN_STMT(Context::THROW, JSEmpty, NULL);\
  }

// section 13.2.1 [[Call]]
void Interpreter::Invoke(JSCodeFunction* code,
                         const Arguments& args, Error* e) {
  // step 1
  JSVal this_value = args.this_binding();
  if (!code->IsStrict()) {
    if (this_value.IsNullOrUndefined()) {
      this_value = ctx_->global_obj();
    } else if (!this_value.IsObject()) {
      JSObject* const obj = this_value.ToObject(ctx_, CHECK_IN_STMT);
      this_value = obj;
    }
  }
  // section 10.5 Declaration Binding Instantiation
  const Scope& scope = code->code()->scope();

  // step 1
  JSDeclEnv* const env = JSDeclEnv::New(ctx_, code->scope(), 0);
  const ContextSwitcher switcher(ctx_, env, env, this_value,
                                 code->IsStrict());

  // step 2
  const bool configurable_bindings = false;

  // step 4
  {
    const std::size_t arg_count = args.size();
    std::size_t n = 0;
    for (Symbols::const_iterator it = code->code()->params().begin(),
         last = code->code()->params().end(); it != last; ++it) {
      ++n;
      const Symbol arg_name = *it;
      if (!env->HasBinding(ctx_, arg_name)) {
        env->CreateMutableBinding(ctx_, arg_name,
                                  configurable_bindings, CHECK_IN_STMT);
      }
      if (n > arg_count) {
        env->SetMutableBinding(ctx_, arg_name,
                               JSUndefined, ctx_->IsStrict(), CHECK_IN_STMT);
      } else {
        env->SetMutableBinding(ctx_, arg_name,
                               args[n-1], ctx_->IsStrict(), CHECK_IN_STMT);
      }
    }
  }

  // step 5
  for (Scope::FunctionLiterals::const_iterator it =
       scope.function_declarations().begin(),
       last = scope.function_declarations().end();
       it != last; ++it) {
    const FunctionLiteral* f = *it;
    const Symbol fn = f->name().Address()->symbol();
    EVAL_IN_STMT(f);
    const JSVal fo = ctx_->ret();
    if (!env->HasBinding(ctx_, fn)) {
      env->CreateMutableBinding(ctx_, fn,
                                configurable_bindings, CHECK_IN_STMT);
    } else {
      // 10.5 errata
    }
    env->SetMutableBinding(ctx_, fn, fo, ctx_->IsStrict(), CHECK_IN_STMT);
  }

  // step 6, 7
  if (!env->HasBinding(ctx_, symbol::arguments())) {
    JSObject* args_obj = NULL;
    if (!ctx_->IsStrict()) {
      args_obj = JSNormalArguments::New(
          ctx_, code,
          code->code()->params(),
          args.rbegin(),
          args.rend(), env,
          CHECK_IN_STMT);
    } else {
      args_obj = JSStrictArguments::New(
          ctx_, code,
          args.rbegin(),
          args.rend(),
          CHECK_IN_STMT);
    }
    if (ctx_->IsStrict()) {
      env->CreateImmutableBinding(symbol::arguments());
      env->InitializeImmutableBinding(symbol::arguments(), args_obj);
    } else {
      env->CreateMutableBinding(ctx_, symbol::arguments(),
                                configurable_bindings, CHECK_IN_STMT);
      env->SetMutableBinding(ctx_, symbol::arguments(),
                             args_obj, false, CHECK_IN_STMT);
    }
  }

  // step 8
  for (Scope::Variables::const_iterator it = scope.variables().begin(),
       last = scope.variables().end(); it != last; ++it) {
    const Scope::Variable& var = *it;
    const Symbol dn = var.first;
    if (!env->HasBinding(ctx_, dn)) {
      env->CreateMutableBinding(ctx_, dn,
                                configurable_bindings, CHECK_IN_STMT);
      env->SetMutableBinding(ctx_, dn,
                             JSUndefined, ctx_->IsStrict(), CHECK_IN_STMT);
    }
  }

  {
    const FunctionLiteral::DeclType type = code->code()->type();
    if (type == FunctionLiteral::STATEMENT ||
        (type == FunctionLiteral::EXPRESSION && code->HasName())) {
      const Symbol name = code->name();
      if (!env->HasBinding(ctx_, name)) {
        env->CreateImmutableBinding(name);
        env->InitializeImmutableBinding(name, code);
      }
    }
  }

  for (Statements::const_iterator it = code->code()->body().begin(),
       last = code->code()->body().end(); it != last; ++it) {
    const Statement* stmt = *it;
    EVAL_IN_STMT(stmt);
    if (ctx_->IsMode<Context::THROW>()) {
      RETURN_STMT(Context::THROW, ctx_->ret(), NULL);
    }
    if (ctx_->IsMode<Context::RETURN>()) {
      RETURN_STMT(Context::RETURN, ctx_->ret(), NULL);
    }
    assert(ctx_->IsMode<Context::NORMAL>());
  }
  RETURN_STMT(Context::NORMAL, JSUndefined, NULL);
}


void Interpreter::Run(const FunctionLiteral* global, bool is_eval) {
  // section 10.5 Declaration Binding Instantiation
  const bool configurable_bindings = is_eval;
  const Scope& scope = global->scope();
  JSEnv* const env = ctx_->variable_env();
  const StrictSwitcher switcher(ctx_, global->strict());
  const bool is_global_env = (env->AsJSObjectEnv() == ctx_->global_env());
  for (Scope::FunctionLiterals::const_iterator it =
       scope.function_declarations().begin(),
       last = scope.function_declarations().end();
       it != last; ++it) {
    const FunctionLiteral* f = *it;
    const Symbol fn = f->name().Address()->symbol();
    EVAL_IN_STMT(f);
    JSVal fo = ctx_->ret();
    if (!env->HasBinding(ctx_, fn)) {
      env->CreateMutableBinding(ctx_, fn,
                                configurable_bindings, CHECK_IN_STMT);
    } else if (is_global_env) {
      JSObject* const go = ctx_->global_obj();
      const PropertyDescriptor existing_prop = go->GetProperty(ctx_, fn);
      if (existing_prop.IsConfigurable()) {
        go->DefineOwnProperty(
            ctx_,
            fn,
            DataDescriptor(
                JSUndefined,
                ATTR::WRITABLE |
                ATTR::ENUMERABLE |
                ((configurable_bindings) ? ATTR::CONFIGURABLE : ATTR::NONE)),
            true, CHECK_IN_STMT);
      } else {
        if (existing_prop.IsAccessorDescriptor()) {
          ctx_->error()->Report(Error::Type,
                                "create mutable function binding failed");
          RETURN_STMT(Context::THROW, JSEmpty, NULL);
        }
        const DataDescriptor* const data = existing_prop.AsDataDescriptor();
        if (!data->IsWritable() ||
            !data->IsEnumerable()) {
          ctx_->error()->Report(Error::Type,
                                "create mutable function binding failed");
          RETURN_STMT(Context::THROW, JSEmpty, NULL);
        }
      }
    }
    env->SetMutableBinding(ctx_, fn, fo, ctx_->IsStrict(), CHECK_IN_STMT);
  }

  for (Scope::Variables::const_iterator it = scope.variables().begin(),
       last = scope.variables().end(); it != last; ++it) {
    const Scope::Variable& var = *it;
    const Symbol dn = var.first;
    if (!env->HasBinding(ctx_, dn)) {
      env->CreateMutableBinding(ctx_, dn,
                                configurable_bindings, CHECK_IN_STMT);
      env->SetMutableBinding(ctx_, dn,
                             JSUndefined, ctx_->IsStrict(), CHECK_IN_STMT);
    }
  }

  JSVal value = JSUndefined;
  // section 14 Program
  for (Statements::const_iterator it = global->body().begin(),
       last = global->body().end(); it != last; ++it) {
    const Statement* stmt = *it;
    EVAL_IN_STMT(stmt);
    if (ctx_->IsMode<Context::THROW>()) {
      // section 12.1 step 4
      RETURN_STMT(Context::THROW, value, NULL);
    }
    if (!ctx_->ret().IsEmpty()) {
      value = ctx_->ret();
    }
    if (!ctx_->IsMode<Context::NORMAL>()) {
      ABRUPT();
    }
  }
  assert(ctx_->IsMode<Context::NORMAL>());
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(const Block* block) {
  // section 12.1 Block
  ctx_->set_mode(Context::NORMAL);
  JSVal value = JSEmpty;
  for (Statements::const_iterator it = block->body().begin(),
       last = block->body().end(); it != last; ++it) {
    const Statement* stmt = *it;
    EVAL_IN_STMT(stmt);
    if (ctx_->IsMode<Context::THROW>()) {
      // section 12.1 step 4
      RETURN_STMT(Context::THROW, value, NULL);
    }
    if (!ctx_->ret().IsEmpty()) {
      value = ctx_->ret();
    }

    if (ctx_->IsMode<Context::BREAK>() &&
        ctx_->InCurrentLabelSet(block)) {
      RETURN_STMT(Context::NORMAL, value, NULL);
    }
    if (!ctx_->IsMode<Context::NORMAL>()) {
      RETURN_STMT(ctx_->mode(), value, ctx_->target());
    }
  }
  ctx_->Return(value);
}


void Interpreter::Visit(const FunctionStatement* stmt) {
  const FunctionLiteral* const func = stmt->function();
  // FunctionStatement must have name
  assert(func->name());
  Resolve(func->name().Address()->symbol());
  const JSVal lhs = ctx_->ret();
  Visit(func);
  const JSVal val = GetValue(ctx_->ret(), CHECK_IN_STMT);
  PutValue(lhs, val, CHECK_IN_STMT);
}


void Interpreter::Visit(const VariableStatement* var) {
  // bool is_const = var->IsConst();
  for (Declarations::const_iterator it = var->decls().begin(),
       last = var->decls().end(); it != last; ++it) {
    const Declaration* decl = *it;
    Resolve(decl->name()->symbol());
    const JSVal lhs = ctx_->ret();
    if (const core::Maybe<const Expression> expr = decl->expr()) {
      EVAL_IN_STMT(expr.Address());
      const JSVal val = GetValue(ctx_->ret(), CHECK_IN_STMT);
      PutValue(lhs, val, CHECK_IN_STMT);
    }
  }
  RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
}


void Interpreter::Visit(const FunctionDeclaration* func) {
  RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
}


void Interpreter::Visit(const EmptyStatement* empty) {
  RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
}


void Interpreter::Visit(const IfStatement* stmt) {
  EVAL_IN_STMT(stmt->cond());
  const JSVal expr = GetValue(ctx_->ret(), CHECK_IN_STMT);
  const bool val = expr.ToBoolean(CHECK_IN_STMT);
  if (val) {
    EVAL_IN_STMT(stmt->then_statement());
    // through then statement's result
  } else {
    if (const core::Maybe<const Statement> else_stmt = stmt->else_statement()) {
      EVAL_IN_STMT(else_stmt.Address());
      // through else statement's result
    } else {
      RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
    }
  }
}


void Interpreter::Visit(const DoWhileStatement* stmt) {
  JSVal value = JSEmpty;
  bool iterating = true;
  while (iterating) {
    EVAL_IN_STMT(stmt->body());
    if (!ctx_->ret().IsEmpty()) {
      value = ctx_->ret();
    }
    if (!ctx_->IsMode<Context::CONTINUE>() ||
        !ctx_->InCurrentLabelSet(stmt)) {
      if (ctx_->IsMode<Context::BREAK>() &&
          ctx_->InCurrentLabelSet(stmt)) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
      if (!ctx_->IsMode<Context::NORMAL>()) {
        ABRUPT();
      }
    }
    EVAL_IN_STMT(stmt->cond());
    const JSVal expr = GetValue(ctx_->ret(), CHECK_IN_STMT);
    const bool val = expr.ToBoolean(CHECK_IN_STMT);
    iterating = val;
  }
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(const WhileStatement* stmt) {
  JSVal value = JSEmpty;
  while (true) {
    EVAL_IN_STMT(stmt->cond());
    const JSVal expr = GetValue(ctx_->ret(), CHECK_IN_STMT);
    const bool val = expr.ToBoolean(CHECK_IN_STMT);
    if (val) {
      EVAL_IN_STMT(stmt->body());
      if (!ctx_->ret().IsEmpty()) {
        value = ctx_->ret();
      }
      if (!ctx_->IsMode<Context::CONTINUE>() ||
          !ctx_->InCurrentLabelSet(stmt)) {
        if (ctx_->IsMode<Context::BREAK>() &&
            ctx_->InCurrentLabelSet(stmt)) {
          RETURN_STMT(Context::NORMAL, value, NULL);
        }
        if (!ctx_->IsMode<Context::NORMAL>()) {
          ABRUPT();
        }
      }
    } else {
      RETURN_STMT(Context::NORMAL, value, NULL);
    }
  }
}


void Interpreter::Visit(const ForStatement* stmt) {
  if (const core::Maybe<const Statement> init = stmt->init()) {
    EVAL_IN_STMT(init.Address());
    GetValue(ctx_->ret(), CHECK_IN_STMT);
  }
  JSVal value = JSEmpty;
  while (true) {
    if (const core::Maybe<const Expression> cond = stmt->cond()) {
      EVAL_IN_STMT(cond.Address());
      const JSVal expr = GetValue(ctx_->ret(), CHECK_IN_STMT);
      const bool val = expr.ToBoolean(CHECK_IN_STMT);
      if (!val) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
    }
    EVAL_IN_STMT(stmt->body());
    if (!ctx_->ret().IsEmpty()) {
      value = ctx_->ret();
    }
    if (!ctx_->IsMode<Context::CONTINUE>() ||
        !ctx_->InCurrentLabelSet(stmt)) {
      if (ctx_->IsMode<Context::BREAK>() &&
          ctx_->InCurrentLabelSet(stmt)) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
      if (!ctx_->IsMode<Context::NORMAL>()) {
        ABRUPT();
      }
    }
    if (const core::Maybe<const Expression> next = stmt->next()) {
      EVAL_IN_STMT(next.Address());
      GetValue(ctx_->ret(), CHECK_IN_STMT);
    }
  }
}


void Interpreter::Visit(const ForInStatement* stmt) {
  const Expression* lexpr = NULL;
  Symbol for_decl = symbol::kDummySymbol;
  if (stmt->each()->AsVariableStatement()) {
    const Declaration* decl =
        stmt->each()->AsVariableStatement()->decls().front();
    for_decl = decl->name()->symbol();
    Resolve(for_decl);
    if (ctx_->IsError()) {
      RETURN_STMT(Context::THROW, JSEmpty, NULL);
    }
    const JSVal lhs = ctx_->ret();
    if (const core::Maybe<const Expression> expr = decl->expr()) {
      EVAL_IN_STMT(expr.Address());
      const JSVal val = GetValue(ctx_->ret(), CHECK_IN_STMT);
      PutValue(lhs, val, CHECK_IN_STMT);
    }
  } else {
    assert(stmt->each()->AsExpressionStatement());
    lexpr = stmt->each()->AsExpressionStatement()->expr();
  }
  EVAL_IN_STMT(stmt->enumerable());
  const JSVal expr = GetValue(ctx_->ret(), CHECK_IN_STMT);
  if (expr.IsNullOrUndefined()) {
    RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
  }
  JSObject* const obj = expr.ToObject(ctx_, CHECK_IN_STMT);
  JSVal value = JSEmpty;
  std::vector<Symbol> keys;
  obj->GetPropertyNames(ctx_, &keys, JSObject::EXCLUDE_NOT_ENUMERABLE);

  for (std::vector<Symbol>::const_iterator it = keys.begin(),
       last = keys.end(); it != last; ++it) {
    const JSVal rhs(JSString::New(ctx_, *it));
    if (lexpr) {
      EVAL_IN_STMT(lexpr);
    } else {
      Resolve(for_decl);
      if (ctx_->IsError()) {
        RETURN_STMT(Context::THROW, JSEmpty, NULL);
      }
    }
    const JSVal lhs = ctx_->ret();
    PutValue(lhs, rhs, CHECK_IN_STMT);
    EVAL_IN_STMT(stmt->body());
    if (!ctx_->ret().IsEmpty()) {
      value = ctx_->ret();
    }
    if (!ctx_->IsMode<Context::CONTINUE>() ||
        !ctx_->InCurrentLabelSet(stmt)) {
      if (ctx_->IsMode<Context::BREAK>() &&
          ctx_->InCurrentLabelSet(stmt)) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
      if (!ctx_->IsMode<Context::NORMAL>()) {
        ABRUPT();
      }
    }
  }
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(const ContinueStatement* stmt) {
  RETURN_STMT(Context::CONTINUE, JSEmpty, stmt->target());
}


void Interpreter::Visit(const BreakStatement* stmt) {
  if (!stmt->target() && stmt->label() != symbol::kDummySymbol) {
    // interpret as EmptyStatement
    RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
  }
  RETURN_STMT(Context::BREAK, JSEmpty, stmt->target());
}


void Interpreter::Visit(const ReturnStatement* stmt) {
  if (const core::Maybe<const Expression> expr = stmt->expr()) {
    EVAL_IN_STMT(expr.Address());
    const JSVal value = GetValue(ctx_->ret(), CHECK_IN_STMT);
    RETURN_STMT(Context::RETURN, value, NULL);
  } else {
    RETURN_STMT(Context::RETURN, JSUndefined, NULL);
  }
}


// section 12.10 The with Statement
void Interpreter::Visit(const WithStatement* stmt) {
  EVAL_IN_STMT(stmt->context());
  const JSVal val = GetValue(ctx_->ret(), CHECK_IN_STMT);
  JSObject* const obj = val.ToObject(ctx_, CHECK_IN_STMT);
  JSEnv* const old_env = ctx_->lexical_env();
  JSObjectEnv* const new_env =
      JSObjectEnv::New(ctx_, old_env, obj);
  new_env->set_provide_this(true);
  {
    const LexicalEnvSwitcher switcher(ctx_, new_env);
    EVAL_IN_STMT(stmt->body());  // RETURN_STMT is body's value
  }
}


void Interpreter::Visit(const LabelledStatement* stmt) {
  EVAL_IN_STMT(stmt->body());
}


void Interpreter::Visit(const SwitchStatement* stmt) {
  EVAL_IN_STMT(stmt->expr());
  const JSVal cond = GetValue(ctx_->ret(), CHECK_IN_STMT);
  // Case Block
  JSVal value = JSEmpty;
  {
    typedef SwitchStatement::CaseClauses CaseClauses;
    bool found = false;
    bool default_found = false;
    bool finalize = false;
    const CaseClauses& clauses = stmt->clauses();
    CaseClauses::const_iterator default_it = clauses.end();
    for (CaseClauses::const_iterator it = clauses.begin(),
         last = clauses.end(); it != last; ++it) {
      const CaseClause* const clause = *it;
      if (const core::Maybe<const Expression> expr = clause->expr()) {
        // case expr: pattern
        if (!found) {
          EVAL_IN_STMT(expr.Address());
          const JSVal res = GetValue(ctx_->ret(), CHECK_IN_STMT);
          if (JSVal::StrictEqual(cond, res)) {
            found = true;
          }
        }
      } else {
        // default: pattern
        default_it = it;
        default_found = true;
      }
      // case's fall through
      if (found) {
        for (Statements::const_iterator it = clause->body().begin(),
             last = clause->body().end(); it != last; ++it) {
          const Statement* st = *it;
          EVAL_IN_STMT(st);
          if (!ctx_->ret().IsEmpty()) {
            value = ctx_->ret();
          }
          if (!ctx_->IsMode<Context::NORMAL>()) {
            ctx_->ret() = value;
            finalize = true;
            break;
          }
        }
        if (finalize) {
          break;
        }
      }
    }
    if (!finalize && !found && default_found) {
      for (CaseClauses::const_iterator it = default_it,
           last = clauses.end(); it != last; ++it) {
        for (Statements::const_iterator jt = (*it)->body().begin(),
             jlast = (*it)->body().end(); jt != jlast; ++jt) {
          const Statement* st = *jt;
          EVAL_IN_STMT(st);
          if (!ctx_->ret().IsEmpty()) {
            value = ctx_->ret();
          }
          if (!ctx_->IsMode<Context::NORMAL>()) {
            ctx_->ret() = value;
            finalize = true;
            break;
          }
        }
        if (finalize) {
          break;
        }
      }
    }
  }

  if (ctx_->IsMode<Context::BREAK>() && ctx_->InCurrentLabelSet(stmt)) {
    RETURN_STMT(Context::NORMAL, value, NULL);
  }
  RETURN_STMT(ctx_->mode(), value, ctx_->target());
}


// section 12.13 The throw Statement
void Interpreter::Visit(const ThrowStatement* stmt) {
  EVAL_IN_STMT(stmt->expr());
  const JSVal ref = GetValue(ctx_->ret(), CHECK_IN_STMT);
  ctx_->error()->Report(ref);
  RETURN_STMT(Context::THROW, ref, NULL);
}


// section 12.14 The try Statement
void Interpreter::Visit(const TryStatement* stmt) {
  stmt->body()->Accept(this);  // evaluate with no error check
  if (ctx_->IsMode<Context::THROW>() || ctx_->IsError()) {
    if (const core::Maybe<const Block> block = stmt->catch_block()) {
      const JSVal ex = ctx_->ErrorVal();
      ctx_->set_mode(Context::NORMAL);
      ctx_->error()->Clear();
      JSEnv* const old_env = ctx_->lexical_env();
      const Symbol name = stmt->catch_name();
      JSStaticEnv* const catch_env = JSStaticEnv::New(ctx_, old_env, name, ex);
      {
        const LexicalEnvSwitcher switcher(ctx_, catch_env);
        // evaluate with no error check (finally)
        (*block).Accept(this);
      }
    }
  }

  if (const core::Maybe<const Block> block = stmt->finally_block()) {
    const Context::Mode mode = ctx_->mode();
    JSVal value = ctx_->ret();
    if (ctx_->IsError()) {
      value = ctx_->ErrorVal();
    }
    const BreakableStatement* const target = ctx_->target();

    ctx_->error()->Clear();
    ctx_->SetStatement(Context::NORMAL, JSEmpty, NULL);
    (*block).Accept(this);
    if (ctx_->IsMode<Context::NORMAL>()) {
      if (mode == Context::THROW) {
        ctx_->error()->Report(value);
      }
      RETURN_STMT(mode, value, target);
    }
  }
}


void Interpreter::Visit(const DebuggerStatement* stmt) {
  // section 12.15 debugger statement
  // implementation define debugging facility is not available
  RETURN_STMT(Context::NORMAL, JSEmpty, NULL);
}


void Interpreter::Visit(const ExpressionStatement* stmt) {
  EVAL_IN_STMT(stmt->expr());
  const JSVal value = GetValue(ctx_->ret(), CHECK);
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(const Assignment* assign) {
  using core::Token;
  EVAL(assign->left());
  const JSVal lref(ctx_->ret());
  JSVal result;
  if (assign->op() == Token::TK_ASSIGN) {  // =
    EVAL(assign->right());
    const JSVal rhs = GetValue(ctx_->ret(), CHECK);
    result = rhs;
  } else {
    const JSVal lhs = GetValue(lref, CHECK);
    EVAL(assign->right());
    const JSVal rhs = GetValue(ctx_->ret(), CHECK);
    switch (assign->op()) {
      case Token::TK_ASSIGN_ADD: {  // +=
        const JSVal lprim = lhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
        const JSVal rprim = rhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
        if (lprim.IsString() || rprim.IsString()) {
          JSString* const lstr = lprim.ToString(ctx_, CHECK);
          JSString* const rstr = rprim.ToString(ctx_, CHECK);
          result = JSString::New(ctx_, lstr, rstr);
          break;
        }
        const double left_num = lprim.ToNumber(ctx_, CHECK);
        const double right_num = rprim.ToNumber(ctx_, CHECK);
        result = left_num + right_num;
        break;
      }
      case Token::TK_ASSIGN_SUB: {  // -=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = left_num - right_num;
        break;
      }
      case Token::TK_ASSIGN_MUL: {  // *=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = left_num * right_num;
        break;
      }
      case Token::TK_ASSIGN_MOD: {  // %=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::Modulo(left_num, right_num);
        break;
      }
      case Token::TK_ASSIGN_DIV: {  // /=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = left_num / right_num;
        break;
      }
      case Token::TK_ASSIGN_SAR: {  // >>=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::DoubleToInt32(left_num)
            >> (core::DoubleToInt32(right_num) & 0x1f);
        break;
      }
      case Token::TK_ASSIGN_SHR: {  // >>>=
        const uint32_t left = lhs.ToUInt32(ctx_, CHECK);
        const double right = rhs.ToNumber(ctx_, CHECK);
        const uint32_t res = left >> (core::DoubleToInt32(right) & 0x1f);
        result = res;
        break;
      }
      case Token::TK_ASSIGN_SHL: {  // <<=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::DoubleToInt32(left_num)
            << (core::DoubleToInt32(right_num) & 0x1f);
        break;
      }
      case Token::TK_ASSIGN_BIT_AND: {  // &=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::DoubleToInt32(left_num)
            & (core::DoubleToInt32(right_num));
        break;
      }
      case Token::TK_ASSIGN_BIT_OR: {  // |=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::DoubleToInt32(left_num)
            | (core::DoubleToInt32(right_num));
        break;
      }
      case Token::TK_ASSIGN_BIT_XOR: {  // ^=
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        result = core::DoubleToInt32(left_num)
            ^ (core::DoubleToInt32(right_num));
        break;
      }
      default: {
        UNREACHABLE();
        break;
      }
    }
  }
  // when parser find eval / arguments identifier in strict code,
  // parser raise "SyntaxError".
  // so, this path is not used in interpreter. (section 11.13.1 step4)
  PutValue(lref, result, CHECK);
  ctx_->Return(result);
}


void Interpreter::Visit(const BinaryOperation* binary) {
  using core::Token;
  const Token::Type token = binary->op();
  EVAL(binary->left());
  const JSVal lhs = GetValue(ctx_->ret(), CHECK);
  {
    switch (token) {
      case Token::TK_LOGICAL_AND: {  // &&
        const bool cond = lhs.ToBoolean(CHECK);
        if (!cond) {
          ctx_->Return(lhs);
          return;
        } else {
          EVAL(binary->right());
          ctx_->ret() = GetValue(ctx_->ret(), CHECK);
          return;
        }
      }

      case Token::TK_LOGICAL_OR: {  // ||
        const bool cond = lhs.ToBoolean(CHECK);
        if (cond) {
          ctx_->Return(lhs);
          return;
        } else {
          EVAL(binary->right());
          ctx_->ret() = GetValue(ctx_->ret(), CHECK);
          return;
        }
      }

      default:
        break;
        // pass
    }
  }

  {
    EVAL(binary->right());
    const JSVal rhs = GetValue(ctx_->ret(), CHECK);
    switch (token) {
      case Token::TK_MUL: {  // *
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(left_num * right_num);
        return;
      }

      case Token::TK_DIV: {  // /
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(left_num / right_num);
        return;
      }

      case Token::TK_MOD: {  // %
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(std::fmod(left_num, right_num));
        return;
      }

      case Token::TK_ADD: {  // +
        // section 11.6.1 NOTE
        // no hint is provided in the calls to ToPrimitive
        const JSVal lprim = lhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
        const JSVal rprim = rhs.ToPrimitive(ctx_, Hint::NONE, CHECK);
        if (lprim.IsString() || rprim.IsString()) {
          JSString* const lstr = lprim.ToString(ctx_, CHECK);
          JSString* const rstr = rprim.ToString(ctx_, CHECK);
          ctx_->Return(JSString::New(ctx_, lstr, rstr));
          return;
        }
        const double left_num = lprim.ToNumber(ctx_, CHECK);
        const double right_num = rprim.ToNumber(ctx_, CHECK);
        ctx_->Return(left_num + right_num);
        return;
      }

      case Token::TK_SUB: {  // -
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(left_num - right_num);
        return;
      }

      case Token::TK_SHL: {  // <<
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(core::DoubleToInt32(left_num)
                     << (core::DoubleToInt32(right_num) & 0x1f));
        return;
      }

      case Token::TK_SAR: {  // >>
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(core::DoubleToInt32(left_num)
                     >> (core::DoubleToInt32(right_num) & 0x1f));
        return;
      }

      case Token::TK_SHR: {  // >>>
        const uint32_t left = lhs.ToUInt32(ctx_, CHECK);
        const double right = rhs.ToNumber(ctx_, CHECK);
        const uint32_t res = left >> (core::DoubleToInt32(right) & 0x1f);
        ctx_->Return(res);
        return;
      }

      case Token::TK_LT: {  // <
        const CompareResult res = JSVal::Compare<true>(ctx_, lhs, rhs, CHECK);
        ctx_->Return(JSVal::Bool(res == CMP_TRUE));
        return;
      }

      case Token::TK_GT: {  // >
        const CompareResult res = JSVal::Compare<false>(ctx_, rhs, lhs, CHECK);
        ctx_->Return(JSVal::Bool(res == CMP_TRUE));
        return;
      }

      case Token::TK_LTE: {  // <=
        const CompareResult res = JSVal::Compare<false>(ctx_, rhs, lhs, CHECK);
        ctx_->Return(JSVal::Bool(res == CMP_FALSE));
        return;
      }

      case Token::TK_GTE: {  // >=
        const CompareResult res = JSVal::Compare<true>(ctx_, lhs, rhs, CHECK);
        ctx_->Return(JSVal::Bool(res == CMP_FALSE));
        return;
      }

      case Token::TK_INSTANCEOF: {  // instanceof
        if (!rhs.IsObject()) {
          ctx_->error()->Report(Error::Type, "instanceof requires object");
          return;
        }
        JSObject* const robj = rhs.object();
        if (!robj->IsCallable()) {
          ctx_->error()->Report(Error::Type, "instanceof requires constructor");
          return;
        }
        const bool res = robj->AsCallable()->HasInstance(ctx_, lhs, CHECK);
        if (res) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      case Token::TK_IN: {  // in
        if (!rhs.IsObject()) {
          ctx_->error()->Report(Error::Type, "in requires object");
          return;
        }
        const JSString* const name = lhs.ToString(ctx_, CHECK);
        const bool res =
            rhs.object()->HasProperty(ctx_,
                                      context::Intern(ctx_, name));
        if (res) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      case Token::TK_EQ: {  // ==
        const bool res = JSVal::AbstractEqual(ctx_, lhs, rhs, CHECK);
        if (res) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      case Token::TK_NE: {  // !=
        const bool res = JSVal::AbstractEqual(ctx_, lhs, rhs, CHECK);
        if (!res) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      case Token::TK_EQ_STRICT: {  // ===
        if (JSVal::StrictEqual(lhs, rhs)) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      case Token::TK_NE_STRICT: {  // !==
        if (!JSVal::StrictEqual(lhs, rhs)) {
          ctx_->Return(JSTrue);
        } else {
          ctx_->Return(JSFalse);
        }
        return;
      }

      // bitwise op
      case Token::TK_BIT_AND: {  // &
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(core::DoubleToInt32(left_num)
                     & (core::DoubleToInt32(right_num)));
        return;
      }

      case Token::TK_BIT_XOR: {  // ^
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(core::DoubleToInt32(left_num)
                     ^ (core::DoubleToInt32(right_num)));
        return;
      }

      case Token::TK_BIT_OR: {  // |
        const double left_num = lhs.ToNumber(ctx_, CHECK);
        const double right_num = rhs.ToNumber(ctx_, CHECK);
        ctx_->Return(core::DoubleToInt32(left_num)
                     | (core::DoubleToInt32(right_num)));
        return;
      }

      case Token::TK_COMMA:  // ,
        ctx_->Return(rhs);
        return;

      default:
        return;
    }
  }
}


void Interpreter::Visit(const ConditionalExpression* cond) {
  EVAL(cond->cond());
  const JSVal expr = GetValue(ctx_->ret(), CHECK);
  const bool condition = expr.ToBoolean(CHECK);
  if (condition) {
    EVAL(cond->left());
    ctx_->ret() = GetValue(ctx_->ret(), CHECK);
    return;
  } else {
    EVAL(cond->right());
    ctx_->ret() = GetValue(ctx_->ret(), CHECK);
    return;
  }
}


void Interpreter::Visit(const UnaryOperation* unary) {
  using core::Token;
  switch (unary->op()) {
    case Token::TK_DELETE: {
      EVAL(unary->expr());
      if (!ctx_->ret().IsReference()) {
        ctx_->Return(JSTrue);
        return;
      }
      // UnresolvableReference / EnvironmentReference is always created
      // by Identifier. and Identifier doesn't create PropertyReference.
      // so, in parser, when "delete Identifier" is found in strict code,
      // parser raise SyntaxError.
      // so, this path (section 11.4.1 step 3.a, step 5.a) is not used.
      const JSReference* const ref = ctx_->ret().reference();
      if (ref->IsUnresolvableReference()) {
        ctx_->Return(JSTrue);
        return;
      } else if (ref->IsPropertyReference()) {
        JSObject* const obj = ref->base().ToObject(ctx_, CHECK);
        const bool result = obj->Delete(ctx_,
                                        ref->GetReferencedName(),
                                        ref->IsStrictReference(), CHECK);
        ctx_->Return(JSVal::Bool(result));
      } else {
        assert(ref->base().IsEnvironment());
        const bool res = ref->base().environment()->DeleteBinding(
            ctx_,
            ref->GetReferencedName());
        ctx_->Return(JSVal::Bool(res));
      }
      return;
    }

    case Token::TK_VOID: {
      EVAL(unary->expr());
      GetValue(ctx_->ret(), CHECK);
      ctx_->Return(JSUndefined);
      return;
    }

    case Token::TK_TYPEOF: {
      EVAL(unary->expr());
      if (ctx_->ret().IsReference()) {
        if (ctx_->ret().reference()->base().IsUndefined()) {
          ctx_->Return(
              JSString::NewAsciiString(ctx_, "undefined"));
          return;
        }
      }
      const JSVal expr = GetValue(ctx_->ret(), CHECK);
      ctx_->Return(expr.TypeOf(ctx_));
      return;
    }

    case Token::TK_INC: {
      EVAL(unary->expr());
      const JSVal expr = ctx_->ret();
      // when parser find eval / arguments identifier in strict code,
      // parser raise "SyntaxError".
      // so, this path is not used in interpreter. (section 11.4.4 step2)
      const JSVal value = GetValue(expr, CHECK);
      const double old_value = value.ToNumber(ctx_, CHECK);
      const JSVal new_value(old_value + 1);
      PutValue(expr, new_value, CHECK);
      ctx_->Return(new_value);
      return;
    }

    case Token::TK_DEC: {
      EVAL(unary->expr());
      const JSVal expr = ctx_->ret();
      // when parser find eval / arguments identifier in strict code,
      // parser raise "SyntaxError".
      // so, this path is not used in interpreter. (section 11.4.5 step2)
      const JSVal value = GetValue(expr, CHECK);
      const double old_value = value.ToNumber(ctx_, CHECK);
      const JSVal new_value(old_value - 1);
      PutValue(expr, new_value, CHECK);
      ctx_->Return(new_value);
      return;
    }

    case Token::TK_ADD: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(ctx_->ret(), CHECK);
      const double val = expr.ToNumber(ctx_, CHECK);
      ctx_->Return(val);
      return;
    }

    case Token::TK_SUB: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(ctx_->ret(), CHECK);
      const double old_value = expr.ToNumber(ctx_, CHECK);
      ctx_->Return(-old_value);
      return;
    }

    case Token::TK_BIT_NOT: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(ctx_->ret(), CHECK);
      const double value = expr.ToNumber(ctx_, CHECK);
      ctx_->Return(~core::DoubleToInt32(value));
      return;
    }

    case Token::TK_NOT: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(ctx_->ret(), CHECK);
      const bool value = expr.ToBoolean(CHECK);
      if (!value) {
        ctx_->Return(JSTrue);
      } else {
        ctx_->Return(JSFalse);
      }
      return;
    }

    default:
      UNREACHABLE();
  }
}


void Interpreter::Visit(const PostfixExpression* postfix) {
  EVAL(postfix->expr());
  const JSVal lref = ctx_->ret();
  // when parser find eval / arguments identifier in strict code,
  // parser raise "SyntaxError".
  // so, this path is not used in interpreter.
  // (section 11.3.1 step2, 11.3.2 step2)
  const JSVal old = GetValue(lref, CHECK);
  const double value = old.ToNumber(ctx_, CHECK);
  const double new_value = value +
      ((postfix->op() == core::Token::TK_INC) ? 1 : -1);
  PutValue(lref, new_value, CHECK);
  ctx_->Return(value);
}


void Interpreter::Visit(const StringLiteral* str) {
  ctx_->Return(JSString::New(ctx_, str->value()));
}


void Interpreter::Visit(const NumberLiteral* num) {
  ctx_->Return(num->value());
}


void Interpreter::Visit(const Identifier* ident) {
  return Resolve(ident->symbol());
}

void Interpreter::Resolve(Symbol sym) {
  // section 10.3.1 Identifier Resolution
  JSEnv* const env = ctx_->lexical_env();
  ctx_->Return(GetIdentifierReference(env, sym, ctx_->IsStrict()));
}

void Interpreter::Visit(const ThisLiteral* literal) {
  ctx_->Return(ctx_->this_binding());
}


void Interpreter::Visit(const NullLiteral* lit) {
  ctx_->Return(JSNull);
}


void Interpreter::Visit(const TrueLiteral* lit) {
  ctx_->Return(JSTrue);
}


void Interpreter::Visit(const FalseLiteral* lit) {
  ctx_->Return(JSFalse);
}

void Interpreter::Visit(const RegExpLiteral* regexp) {
  ctx_->Return(
      JSRegExp::New(ctx_,
                    regexp->value(),
                    regexp->regexp()));
}


void Interpreter::Visit(const ArrayLiteral* literal) {
  // when in parse phase, have already removed last elision.
  JSArray* const ary = JSArray::New(ctx_);
  uint32_t current = 0;
  for (MaybeExpressions::const_iterator it = literal->items().begin(),
       last = literal->items().end(); it != last; ++it) {
    if (const core::Maybe<const Expression> expr = *it) {
      EVAL(expr.Address());
      const JSVal value = GetValue(ctx_->ret(), CHECK);
      ary->DefineOwnProperty(
          ctx_, symbol::MakeSymbolFromIndex(current),
          DataDescriptor(value, ATTR::WRITABLE |
                                ATTR::ENUMERABLE |
                                ATTR::CONFIGURABLE),
          false, CHECK);
    }
    ++current;
  }
  ary->Put(ctx_, symbol::length(),
           JSVal::UInt32(current), false, CHECK);
  ctx_->Return(ary);
}


void Interpreter::Visit(const ObjectLiteral* literal) {
  using std::get;
  JSObject* const obj = JSObject::New(ctx_);

  // section 11.1.5
  for (ObjectLiteral::Properties::const_iterator it =
       literal->properties().begin(),
       last = literal->properties().end();
       it != last; ++it) {
    const ObjectLiteral::Property& prop = *it;
    const ObjectLiteral::PropertyDescriptorType type(get<0>(prop));
    const Symbol name = get<1>(prop);
    PropertyDescriptor desc;
    if (type == ObjectLiteral::DATA) {
      EVAL(get<2>(prop));
      const JSVal value = GetValue(ctx_->ret(), CHECK);
      desc = DataDescriptor(value,
                            ATTR::WRITABLE |
                            ATTR::ENUMERABLE |
                            ATTR::CONFIGURABLE);
    } else {
      EVAL(get<2>(prop));
      if (type == ObjectLiteral::GET) {
        desc = AccessorDescriptor(ctx_->ret().object(), NULL,
                                  ATTR::ENUMERABLE |
                                  ATTR::CONFIGURABLE |
                                  ATTR::UNDEF_SETTER);
      } else {
        desc = AccessorDescriptor(NULL, ctx_->ret().object(),
                                  ATTR::ENUMERABLE |
                                  ATTR::CONFIGURABLE|
                                  ATTR::UNDEF_GETTER);
      }
    }
    // section 11.1.5 step 4
    // Syntax error detection is already passed in parser phase.
    // Because syntax error is early error (section 16 Errors)
    // syntax error is reported at parser phase.
    // So, in interpreter phase, there's nothing to do.
    obj->DefineOwnProperty(ctx_, name, desc, false, CHECK);
  }
  ctx_->Return(obj);
}


void Interpreter::Visit(const FunctionLiteral* func) {
  ctx_->Return(
      JSCodeFunction::New(ctx_,
                          func,
                          ctx_->current_script(),
                          ctx_->lexical_env()));
}


void Interpreter::Visit(const IdentifierAccess* prop) {
  EVAL(prop->target());
  const JSVal base_value = GetValue(ctx_->ret(), CHECK);
  base_value.CheckObjectCoercible(CHECK);
  ctx_->Return(
      JSReference::New(ctx_, base_value, prop->key(), ctx_->IsStrict()));
}


void Interpreter::Visit(const IndexAccess* prop) {
  EVAL(prop->target());
  const JSVal base_value = GetValue(ctx_->ret(), CHECK);
  EVAL(prop->key());
  const JSVal name_value = GetValue(ctx_->ret(), CHECK);
  base_value.CheckObjectCoercible(CHECK);
  const JSString* const name = name_value.ToString(ctx_, CHECK);
  ctx_->Return(
      JSReference::New(ctx_,
                       base_value,
                       context::Intern(ctx_, name),
                       ctx_->IsStrict()));
}


void Interpreter::Visit(const FunctionCall* call) {
  EVAL(call->target());
  const JSVal target = ctx_->ret();
  const JSVal func = GetValue(target, CHECK);

  ScopedArguments args(ctx_, call->args().size(), CHECK);
  std::size_t n = 0;
  for (Expressions::const_iterator it = call->args().begin(),
       last = call->args().end(); it != last; ++it) {
    const Expression* expr = *it;
    EVAL(expr);
    args[n++] = GetValue(ctx_->ret(), CHECK);
  }
  if (!func.IsCallable()) {
    ctx_->error()->Report(Error::Type, "not callable object");
    return;
  }
  JSFunction* const callable = func.object()->AsCallable();
  JSVal this_binding = JSUndefined;
  if (target.IsReference()) {
    const JSReference* const ref = target.reference();
    if (ref->IsPropertyReference()) {
      this_binding = ref->base();
    } else {
      assert(ref->base().IsEnvironment());
      this_binding = ref->base().environment()->ImplicitThisValue();
      // direct call to eval check
      {
        if (JSAPI native = callable->NativeFunction()) {
          if (native == &GlobalEval) {
            // this function is eval function
            const Identifier* const maybe_eval = call->target()->AsIdentifier();
            if (maybe_eval &&
                maybe_eval->symbol() == symbol::eval()) {
              // direct call to eval point
              args.set_this_binding(this_binding);
              ctx_->ret() = DirectCallToEval(args, CHECK);
              return;
            }
          }
        }
      }
    }
  }
  ctx_->ret() = callable->Call(&args, this_binding, CHECK);
}


void Interpreter::Visit(const ConstructorCall* call) {
  EVAL(call->target());
  const JSVal func = GetValue(ctx_->ret(), CHECK);

  ScopedArguments args(ctx_, call->args().size(), CHECK);
  args.set_constructor_call(true);
  std::size_t n = 0;
  for (Expressions::const_iterator it = call->args().begin(),
       last = call->args().end(); it != last; ++it) {
    const Expression* expr = *it;
    EVAL(expr);
    args[n++] = GetValue(ctx_->ret(), CHECK);
  }
  if (!func.IsCallable()) {
    ctx_->error()->Report(Error::Type, "not callable object");
    return;
  }
  ctx_->ret() = func.object()->AsCallable()->Construct(&args, CHECK);
}

void Interpreter::Visit(const Declaration* dummy) {
  UNREACHABLE();
}


void Interpreter::Visit(const CaseClause* dummy) {
  UNREACHABLE();
}


// section 8.7.1 GetValue
JSVal Interpreter::GetValue(const JSVal& val, Error* e) {
  if (!val.IsReference()) {
    return val;
  }
  const JSReference* const ref = val.reference();
  const JSVal& base = ref->base();
  if (ref->IsUnresolvableReference()) {
    core::UStringBuilder builder;
    builder.Append('"');
    builder.Append(symbol::GetSymbolString(ref->GetReferencedName()));
    builder.Append("\" not defined");
    e->Report(Error::Reference, builder.BuildPiece());
    return JSEmpty;
  }
  if (ref->IsPropertyReference()) {
    if (ref->HasPrimitiveBase()) {
      // section 8.7.1 special [[Get]]
      const JSObject* const o = base.ToObject(ctx_, IV_LV5_ERROR(e));
      const PropertyDescriptor desc = o->GetProperty(ctx_,
                                                     ref->GetReferencedName());
      if (desc.IsEmpty()) {
        return JSUndefined;
      }
      if (desc.IsDataDescriptor()) {
        return desc.AsDataDescriptor()->value();
      } else {
        assert(desc.IsAccessorDescriptor());
        const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
        if (ac->get()) {
          ScopedArguments a(ctx_, 0, IV_LV5_ERROR(e));
          const JSVal res =
              ac->get()->AsCallable()->Call(&a, base, IV_LV5_ERROR(e));
          return res;
        } else {
          return JSUndefined;
        }
      }
    } else {
      const JSVal res =
          base.object()->Get(ctx_, ref->GetReferencedName(), IV_LV5_ERROR(e));
      return res;
    }
    return JSUndefined;
  } else {
    const JSVal res = base.environment()->GetBindingValue(
        ctx_, ref->GetReferencedName(),
        ref->IsStrictReference(), IV_LV5_ERROR(e));
    return res;
  }
}

// section 8.7.2 PutValue
void Interpreter::PutValue(const JSVal& val, const JSVal& w, Error* e) {
  if (!val.IsReference()) {
    e->Report(Error::Reference,
              "target is not reference");
    return;
  }
  const JSReference* const ref = val.reference();
  const JSVal& base = ref->base();
  if (ref->IsUnresolvableReference()) {
    if (ref->IsStrictReference()) {
      e->Report(Error::Reference,
                "putting to unresolvable reference "
                "not allowed in strict reference");
      return;
    }
    ctx_->global_obj()->Put(ctx_, ref->GetReferencedName(),
                            w, false, IV_LV5_ERROR_VOID(e));
  } else if (ref->IsPropertyReference()) {
    if (ref->HasPrimitiveBase()) {
      const Symbol sym = ref->GetReferencedName();
      const bool th = ref->IsStrictReference();
      JSObject* const o = base.ToObject(ctx_, IV_LV5_ERROR_VOID(e));
      if (!o->CanPut(ctx_, sym)) {
        if (th) {
          e->Report(Error::Type, "cannot put value to object");
        }
        return;
      }
      const PropertyDescriptor own_desc = o->GetOwnProperty(ctx_, sym);
      if (!own_desc.IsEmpty() && own_desc.IsDataDescriptor()) {
        if (th) {
          e->Report(Error::Type,
                    "value to symbol defined and not data descriptor");
        }
        return;
      }
      const PropertyDescriptor desc = o->GetProperty(ctx_, sym);
      if (!desc.IsEmpty() && desc.IsAccessorDescriptor()) {
        ScopedArguments a(ctx_, 1, IV_LV5_ERROR_VOID(e));
        a[0] = w;
        const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
        assert(ac->set());
        ac->set()->AsCallable()->Call(&a, base, IV_LV5_ERROR_VOID(e));
      } else {
        if (th) {
          e->Report(Error::Type, "value to symbol in transient object");
        }
      }
      return;
    } else {
      base.object()->Put(ctx_, ref->GetReferencedName(), w,
                         ref->IsStrictReference(), IV_LV5_ERROR_VOID(e));
    }
  } else {
    assert(base.environment());
    base.environment()->SetMutableBinding(
        ctx_,
        ref->GetReferencedName(), w,
        ref->IsStrictReference(), IV_LV5_ERROR_VOID(e));
  }
}

JSReference* Interpreter::GetIdentifierReference(JSEnv* lex,
                                                 Symbol name, bool strict) {
  JSEnv* env = lex;
  while (env) {
    if (env->HasBinding(ctx_, name)) {
      return JSReference::New(ctx_, env, name, strict);
    } else {
      env = env->outer();
    }
  }
  return JSReference::New(ctx_, JSUndefined, name, strict);
}

#undef CHECK
#undef CHECK_IN_STMT
#undef RETURN_STMT
#undef ABRUPT
#undef EVAL
#undef EVAL_IN_STMT

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_INTERPRETER_H_