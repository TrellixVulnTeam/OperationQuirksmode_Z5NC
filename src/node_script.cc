

#include "node.h"
#include "node_script.h"
#include "node_watchdog.h"
#include <assert.h>

namespace node {

using v8::Context;
using v8::Script;
using v8::Value;
using v8::Handle;
using v8::HandleScope;
using v8::Object;
using v8::Arguments;
using v8::ThrowException;
using v8::TryCatch;
using v8::String;
using v8::Exception;
using v8::Local;
using v8::Array;
using v8::Persistent;
using v8::Integer;
using v8::Function;
using v8::FunctionTemplate;
using v8::V8;


class WrappedContext : ObjectWrap {
 public:
  static void Initialize(Handle<Object> target);
  static Handle<Value> New(const Arguments& args);

  Persistent<Context> GetV8Context();
  static Local<Object> NewInstance();
  static bool InstanceOf(Handle<Value> value);

 protected:

  static Persistent<FunctionTemplate> constructor_template;

  WrappedContext();
  ~WrappedContext();

  Persistent<Context> context_;
};


Persistent<FunctionTemplate> WrappedContext::constructor_template;


class WrappedScript : ObjectWrap {
 public:
  static void Initialize(Handle<Object> target);

  enum EvalInputFlags { compileCode, unwrapExternal };
  enum EvalContextFlags { thisContext, newContext, userContext };
  enum EvalOutputFlags { returnResult, wrapExternal };
  enum EvalTimeoutFlags { noTimeout, useTimeout };

  template <EvalInputFlags input_flag,
            EvalContextFlags context_flag,
            EvalOutputFlags output_flag,
            EvalTimeoutFlags timeout_flag>
  static Handle<Value> EvalMachine(const Arguments& args);

 protected:
  static Persistent<FunctionTemplate> constructor_template;

  WrappedScript() : ObjectWrap() {}
  ~WrappedScript();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> CreateContext(const Arguments& arg);
  static Handle<Value> RunInContext(const Arguments& args);
  static Handle<Value> RunInThisContext(const Arguments& args);
  static Handle<Value> RunInNewContext(const Arguments& args);
  static Handle<Value> CompileRunInContext(const Arguments& args);
  static Handle<Value> CompileRunInThisContext(const Arguments& args);
  static Handle<Value> CompileRunInNewContext(const Arguments& args);

  Persistent<Script> script_;
};


Persistent<Function> cloneObjectMethod;

void CloneObject(Handle<Object> recv,
                 Handle<Value> source, Handle<Value> target) {
  HandleScope scope(node_isolate);

  Handle<Value> args[] = {source, target};

  // Init
  if (cloneObjectMethod.IsEmpty()) {
    Local<Function> cloneObjectMethod_ = Local<Function>::Cast(
      Script::Compile(String::New(
        "(function(source, target) {\n\
           Object.getOwnPropertyNames(source).forEach(function(key) {\n\
           try {\n\
             var desc = Object.getOwnPropertyDescriptor(source, key);\n\
             if (desc.value === source) desc.value = target;\n\
             Object.defineProperty(target, key, desc);\n\
           } catch (e) {\n\
            // Catch sealed properties errors\n\
           }\n\
         });\n\
        })"
      ), String::New("binding:script"))->Run()
    );
    cloneObjectMethod = Persistent<Function>::New(node_isolate,
                                                  cloneObjectMethod_);
  }

  cloneObjectMethod->Call(recv, 2, args);
}


void WrappedContext::Initialize(Handle<Object> target) {
  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(WrappedContext::New);
  constructor_template = Persistent<FunctionTemplate>::New(node_isolate, t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("Context"));

  target->Set(String::NewSymbol("Context"),
              constructor_template->GetFunction());
}


bool WrappedContext::InstanceOf(Handle<Value> value) {
  return !value.IsEmpty() && constructor_template->HasInstance(value);
}


Handle<Value> WrappedContext::New(const Arguments& args) {
  HandleScope scope(node_isolate);

  WrappedContext *t = new WrappedContext();
  t->Wrap(args.This());

  return args.This();
}


WrappedContext::WrappedContext() : ObjectWrap() {
  context_ = Persistent<Context>::New(node_isolate, Context::New(node_isolate));
}


WrappedContext::~WrappedContext() {
  context_.Dispose(node_isolate);
}


Local<Object> WrappedContext::NewInstance() {
  Local<Object> context = constructor_template->GetFunction()->NewInstance();
  return context;
}


Persistent<Context> WrappedContext::GetV8Context() {
  return context_;
}


Persistent<FunctionTemplate> WrappedScript::constructor_template;


void WrappedScript::Initialize(Handle<Object> target) {
  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(WrappedScript::New);
  constructor_template = Persistent<FunctionTemplate>::New(node_isolate, t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  // Note: We use 'NodeScript' instead of 'Script' so that we do not
  // conflict with V8's Script class defined in v8/src/messages.js
  // See GH-203 https://github.com/joyent/node/issues/203
  constructor_template->SetClassName(String::NewSymbol("NodeScript"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template,
                            "createContext",
                            WrappedScript::CreateContext);

  NODE_SET_PROTOTYPE_METHOD(constructor_template,
                            "runInContext",
                            WrappedScript::RunInContext);

  NODE_SET_PROTOTYPE_METHOD(constructor_template,
                            "runInThisContext",
                            WrappedScript::RunInThisContext);

  NODE_SET_PROTOTYPE_METHOD(constructor_template,
                            "runInNewContext",
                            WrappedScript::RunInNewContext);

  NODE_SET_METHOD(constructor_template,
                  "createContext",
                  WrappedScript::CreateContext);

  NODE_SET_METHOD(constructor_template,
                  "runInContext",
                  WrappedScript::CompileRunInContext);

  NODE_SET_METHOD(constructor_template,
                  "runInThisContext",
                  WrappedScript::CompileRunInThisContext);

  NODE_SET_METHOD(constructor_template,
                  "runInNewContext",
                  WrappedScript::CompileRunInNewContext);

  target->Set(String::NewSymbol("NodeScript"),
              constructor_template->GetFunction());
}


Handle<Value> WrappedScript::New(const Arguments& args) {
  if (!args.IsConstructCall()) {
    return FromConstructorTemplate(constructor_template, args);
  }

  HandleScope scope(node_isolate);

  WrappedScript *t = new WrappedScript();
  t->Wrap(args.This());

  return
    WrappedScript::EvalMachine<
      compileCode, thisContext, wrapExternal, noTimeout>(args);
}


WrappedScript::~WrappedScript() {
  script_.Dispose(node_isolate);
}


Handle<Value> WrappedScript::CreateContext(const Arguments& args) {
  HandleScope scope(node_isolate);

  Local<Object> context = WrappedContext::NewInstance();

  if (args.Length() > 0) {
    if (args[0]->IsObject()) {
      Local<Object> sandbox = args[0].As<Object>();

      CloneObject(args.This(), sandbox, context);
    } else {
      return ThrowException(Exception::TypeError(String::New(
          "createContext() accept only object as first argument.")));
    }
  }


  return scope.Close(context);
}


Handle<Value> WrappedScript::RunInContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      unwrapExternal, userContext, returnResult, useTimeout>(args);
}


Handle<Value> WrappedScript::RunInThisContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      unwrapExternal, thisContext, returnResult, useTimeout>(args);
}


Handle<Value> WrappedScript::RunInNewContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      unwrapExternal, newContext, returnResult, useTimeout>(args);
}


Handle<Value> WrappedScript::CompileRunInContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      compileCode, userContext, returnResult, useTimeout>(args);
}


Handle<Value> WrappedScript::CompileRunInThisContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      compileCode, thisContext, returnResult, useTimeout>(args);
}


Handle<Value> WrappedScript::CompileRunInNewContext(const Arguments& args) {
  return
    WrappedScript::EvalMachine<
      compileCode, newContext, returnResult, useTimeout>(args);
}


template <WrappedScript::EvalInputFlags input_flag,
          WrappedScript::EvalContextFlags context_flag,
          WrappedScript::EvalOutputFlags output_flag,
          WrappedScript::EvalTimeoutFlags timeout_flag>
Handle<Value> WrappedScript::EvalMachine(const Arguments& args) {
  HandleScope scope(node_isolate);

  if (input_flag == compileCode && args.Length() < 1) {
    return ThrowException(Exception::TypeError(
          String::New("needs at least 'code' argument.")));
  }

  const int sandbox_index = input_flag == compileCode ? 1 : 0;
  if (context_flag == userContext
    && !WrappedContext::InstanceOf(args[sandbox_index]))
  {
    return ThrowException(Exception::TypeError(
          String::New("needs a 'context' argument.")));
  }


  Local<String> code;
  if (input_flag == compileCode) code = args[0]->ToString();

  Local<Object> sandbox;
  if (context_flag == newContext) {
    sandbox = args[sandbox_index]->IsObject() ? args[sandbox_index]->ToObject()
                                              : Object::New();
  } else if (context_flag == userContext) {
    sandbox = args[sandbox_index]->ToObject();
  }

  const int filename_index = sandbox_index +
                             (context_flag == thisContext? 0 : 1);
  Local<String> filename = args.Length() > filename_index
                           ? args[filename_index]->ToString()
                           : String::New("evalmachine.<anonymous>");

  uint64_t timeout = 0;
  const int timeout_index = filename_index + 1;
  if (timeout_flag == useTimeout && args.Length() > timeout_index) {
    if (!args[timeout_index]->IsUint32()) {
      return ThrowException(Exception::TypeError(
            String::New("needs an unsigned integer 'ms' argument.")));
    }
    timeout = args[timeout_index]->Uint32Value();
  }

  const int display_error_index = timeout_index +
                                  (timeout_flag == noTimeout ? 0 : 1);
  bool display_error = false;
  if (args.Length() > display_error_index &&
      args[display_error_index]->IsBoolean() &&
      args[display_error_index]->BooleanValue() == true) {
    display_error = true;
  }

  Handle<Context> context = Context::GetCurrent();

  Local<Array> keys;
  if (context_flag == newContext) {
    // Create the new context
    context = Context::New(node_isolate);

  } else if (context_flag == userContext) {
    // Use the passed in context
    WrappedContext *nContext = ObjectWrap::Unwrap<WrappedContext>(sandbox);
    context = nContext->GetV8Context();
  }

  Context::Scope context_scope(context);

  // New and user context share code. DRY it up.
  if (context_flag == userContext || context_flag == newContext) {
    // Copy everything from the passed in sandbox (either the persistent
    // context for runInContext(), or the sandbox arg to runInNewContext()).
    CloneObject(args.This(), sandbox, context->Global()->GetPrototype());
  }

  // Catch errors
  TryCatch try_catch;

  Handle<Value> result;
  Handle<Script> script;

  if (input_flag == compileCode) {
    // well, here WrappedScript::New would suffice in all cases, but maybe
    // Compile has a little better performance where possible
    script = output_flag == returnResult ? Script::Compile(code, filename)
                                         : Script::New(code, filename);
    if (script.IsEmpty()) {
      // FIXME UGLY HACK TO DISPLAY SYNTAX ERRORS.
      if (display_error) DisplayExceptionLine(try_catch);

      // Hack because I can't get a proper stacktrace on SyntaxError
      return try_catch.ReThrow();
    }
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(args.This());
    if (!n_script) {
      return ThrowException(Exception::Error(
            String::New("Must be called as a method of Script.")));
    } else if (n_script->script_.IsEmpty()) {
      return ThrowException(Exception::Error(
            String::New("'this' must be a result of previous "
                        "new Script(code) call.")));
    }

    script = n_script->script_;
  }


  if (output_flag == returnResult) {
    if (timeout) {
      Watchdog wd(timeout);
      result = script->Run();
    } else {
      result = script->Run();
    }
    if (try_catch.HasCaught() && try_catch.HasTerminated()) {
      V8::CancelTerminateExecution(args.GetIsolate());
      return ThrowException(Exception::Error(
            String::New("Script execution timed out.")));
    }
    if (result.IsEmpty()) {
      if (display_error) DisplayExceptionLine(try_catch);
      return try_catch.ReThrow();
    }
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(args.This());
    if (!n_script) {
      return ThrowException(Exception::Error(
            String::New("Must be called as a method of Script.")));
    }
    n_script->script_ = Persistent<Script>::New(node_isolate, script);
    result = args.This();
  }

  if (context_flag == userContext || context_flag == newContext) {
    // success! copy changes back onto the sandbox object.
    CloneObject(args.This(), context->Global()->GetPrototype(), sandbox);
  }

  return result == args.This() ? result : scope.Close(result);
}


void InitEvals(Handle<Object> target) {
  HandleScope scope(node_isolate);

  WrappedContext::Initialize(target);
  WrappedScript::Initialize(target);
}


}  // namespace node


NODE_MODULE(node_evals, node::InitEvals)

