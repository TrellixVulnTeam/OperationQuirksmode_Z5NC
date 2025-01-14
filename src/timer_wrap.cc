

#include "node.h"
#include "handle_wrap.h"

namespace node {

using v8::Object;
using v8::Handle;
using v8::Local;
using v8::Persistent;
using v8::Value;
using v8::HandleScope;
using v8::FunctionTemplate;
using v8::String;
using v8::Function;
using v8::TryCatch;
using v8::Context;
using v8::Arguments;
using v8::Integer;

static Persistent<String> ontimeout_sym;

class TimerWrap : public HandleWrap {
 public:
  static void Initialize(Handle<Object> target) {
    HandleScope scope(node_isolate);

    HandleWrap::Initialize(target);

    Local<FunctionTemplate> constructor = FunctionTemplate::New(New);
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Timer"));

    NODE_SET_METHOD(constructor, "now", Now);

    NODE_SET_PROTOTYPE_METHOD(constructor, "close", HandleWrap::Close);
    NODE_SET_PROTOTYPE_METHOD(constructor, "ref", HandleWrap::Ref);
    NODE_SET_PROTOTYPE_METHOD(constructor, "unref", HandleWrap::Unref);

    NODE_SET_PROTOTYPE_METHOD(constructor, "start", Start);
    NODE_SET_PROTOTYPE_METHOD(constructor, "stop", Stop);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setRepeat", SetRepeat);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getRepeat", GetRepeat);
    NODE_SET_PROTOTYPE_METHOD(constructor, "again", Again);

    ontimeout_sym = NODE_PSYMBOL("ontimeout");

    target->Set(String::NewSymbol("Timer"), constructor->GetFunction());
  }

 private:
  static Handle<Value> New(const Arguments& args) {
    // This constructor should not be exposed to public javascript.
    // Therefore we assert that we are not trying to call this as a
    // normal function.
    assert(args.IsConstructCall());

    HandleScope scope(node_isolate);
    TimerWrap *wrap = new TimerWrap(args.This());
    assert(wrap);

    return scope.Close(args.This());
  }

  TimerWrap(Handle<Object> object)
      : HandleWrap(object, reinterpret_cast<uv_handle_t*>(&handle_)) {
    int r = uv_timer_init(uv_default_loop(), &handle_);
    assert(r == 0);
  }

  ~TimerWrap() {
  }

  static Handle<Value> Start(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(TimerWrap)

    int64_t timeout = args[0]->IntegerValue();
    int64_t repeat = args[1]->IntegerValue();

    int r = uv_timer_start(&wrap->handle_, OnTimeout, timeout, repeat);

    if (r) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(r, node_isolate));
  }

  static Handle<Value> Stop(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(TimerWrap)

    int r = uv_timer_stop(&wrap->handle_);

    if (r) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(r, node_isolate));
  }

  static Handle<Value> Again(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(TimerWrap)

    int r = uv_timer_again(&wrap->handle_);

    if (r) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(r, node_isolate));
  }

  static Handle<Value> SetRepeat(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(TimerWrap)

    int64_t repeat = args[0]->IntegerValue();

    uv_timer_set_repeat(&wrap->handle_, repeat);

    return scope.Close(Integer::New(0, node_isolate));
  }

  static Handle<Value> GetRepeat(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(TimerWrap)

    int64_t repeat = uv_timer_get_repeat(&wrap->handle_);

    if (repeat < 0) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(repeat, node_isolate));
  }

  static void OnTimeout(uv_timer_t* handle, int status) {
    HandleScope scope(node_isolate);

    TimerWrap* wrap = static_cast<TimerWrap*>(handle->data);
    assert(wrap);

    Local<Value> argv[1] = { Integer::New(status, node_isolate) };
    MakeCallback(wrap->object_, ontimeout_sym, ARRAY_SIZE(argv), argv);
  }

  static Handle<Value> Now(const Arguments& args) {
    HandleScope scope(node_isolate);

    double now = static_cast<double>(uv_now(uv_default_loop()));
    return scope.Close(v8::Number::New(now));
  }

  uv_timer_t handle_;
};


}  // namespace node

NODE_MODULE(node_timer_wrap, node::TimerWrap::Initialize)
