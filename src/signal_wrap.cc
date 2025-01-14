

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

static Persistent<String> onsignal_sym;


class SignalWrap : public HandleWrap {
 public:
  static void Initialize(Handle<Object> target) {
    HandleScope scope(node_isolate);

    HandleWrap::Initialize(target);

    Local<FunctionTemplate> constructor = FunctionTemplate::New(New);
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Signal"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "close", HandleWrap::Close);
    NODE_SET_PROTOTYPE_METHOD(constructor, "ref", HandleWrap::Ref);
    NODE_SET_PROTOTYPE_METHOD(constructor, "unref", HandleWrap::Unref);
    NODE_SET_PROTOTYPE_METHOD(constructor, "start", Start);
    NODE_SET_PROTOTYPE_METHOD(constructor, "stop", Stop);

    onsignal_sym = NODE_PSYMBOL("onsignal");

    target->Set(String::NewSymbol("Signal"), constructor->GetFunction());
  }

 private:
  static Handle<Value> New(const Arguments& args) {
    // This constructor should not be exposed to public javascript.
    // Therefore we assert that we are not trying to call this as a
    // normal function.
    assert(args.IsConstructCall());

    HandleScope scope(node_isolate);
    new SignalWrap(args.This());

    return scope.Close(args.This());
  }

  SignalWrap(Handle<Object> object)
      : HandleWrap(object, reinterpret_cast<uv_handle_t*>(&handle_)) {
    int r = uv_signal_init(uv_default_loop(), &handle_);
    assert(r == 0);
  }

  ~SignalWrap() {
  }

  static Handle<Value> Start(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(SignalWrap)

    int signum = args[0]->Int32Value();

    int r = uv_signal_start(&wrap->handle_, OnSignal, signum);

    if (r) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(r, node_isolate));
  }

  static Handle<Value> Stop(const Arguments& args) {
    HandleScope scope(node_isolate);

    UNWRAP(SignalWrap)

    int r = uv_signal_stop(&wrap->handle_);

    if (r) SetErrno(uv_last_error(uv_default_loop()));

    return scope.Close(Integer::New(r, node_isolate));
  }

  static void OnSignal(uv_signal_t* handle, int signum) {
    HandleScope scope(node_isolate);

    SignalWrap* wrap = container_of(handle, SignalWrap, handle_);
    assert(wrap);

    Local<Value> argv[1] = { Integer::New(signum, node_isolate) };
    MakeCallback(wrap->object_, onsignal_sym, ARRAY_SIZE(argv), argv);
  }

  uv_signal_t handle_;
};


}  // namespace node


NODE_MODULE(node_signal_wrap, node::SignalWrap::Initialize)
