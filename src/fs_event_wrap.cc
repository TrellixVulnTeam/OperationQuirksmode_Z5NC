

#include "node.h"
#include "handle_wrap.h"

#include <stdlib.h>

using namespace v8;

namespace node {

static Persistent<String> change_sym;
static Persistent<String> onchange_sym;
static Persistent<String> rename_sym;

class FSEventWrap: public HandleWrap {
public:
  static void Initialize(Handle<Object> target);
  static Handle<Value> New(const Arguments& args);
  static Handle<Value> Start(const Arguments& args);
  static Handle<Value> Close(const Arguments& args);

private:
  FSEventWrap(Handle<Object> object);
  virtual ~FSEventWrap();

  static void OnEvent(uv_fs_event_t* handle, const char* filename, int events,
    int status);

  uv_fs_event_t handle_;
  bool initialized_;
};


FSEventWrap::FSEventWrap(Handle<Object> object)
    : HandleWrap(object, reinterpret_cast<uv_handle_t*>(&handle_)) {
  initialized_ = false;
}


FSEventWrap::~FSEventWrap() {
  assert(initialized_ == false);
}


void FSEventWrap::Initialize(Handle<Object> target) {
  HandleWrap::Initialize(target);

  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("FSEvent"));

  NODE_SET_PROTOTYPE_METHOD(t, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(t, "close", Close);

  target->Set(String::NewSymbol("FSEvent"),
              Persistent<FunctionTemplate>::New(node_isolate,
                                                t)->GetFunction());

  change_sym = NODE_PSYMBOL("change");
  onchange_sym = NODE_PSYMBOL("onchange");
  rename_sym = NODE_PSYMBOL("rename");
}


Handle<Value> FSEventWrap::New(const Arguments& args) {
  HandleScope scope(node_isolate);

  assert(args.IsConstructCall());
  new FSEventWrap(args.This());

  return scope.Close(args.This());
}


Handle<Value> FSEventWrap::Start(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(FSEventWrap)

  if (args.Length() < 1 || !args[0]->IsString()) {
    return ThrowException(Exception::TypeError(String::New("Bad arguments")));
  }

  String::Utf8Value path(args[0]);

  int r = uv_fs_event_init(uv_default_loop(), &wrap->handle_, *path, OnEvent, 0);
  if (r == 0) {
    // Check for persistent argument
    if (!args[1]->IsTrue()) {
      uv_unref(reinterpret_cast<uv_handle_t*>(&wrap->handle_));
    }
    wrap->initialized_ = true;
  } else {
    SetErrno(uv_last_error(uv_default_loop()));
  }

  return scope.Close(Integer::New(r, node_isolate));
}


void FSEventWrap::OnEvent(uv_fs_event_t* handle, const char* filename,
    int events, int status) {
  HandleScope scope(node_isolate);
  Handle<String> eventStr;

  FSEventWrap* wrap = static_cast<FSEventWrap*>(handle->data);

  assert(wrap->object_.IsEmpty() == false);

  // We're in a bind here. libuv can set both UV_RENAME and UV_CHANGE but
  // the Node API only lets us pass a single event to JS land.
  //
  // The obvious solution is to run the callback twice, once for each event.
  // However, since the second event is not allowed to fire if the handle is
  // closed after the first event, and since there is no good way to detect
  // closed handles, that option is out.
  //
  // For now, ignore the UV_CHANGE event if UV_RENAME is also set. Make the
  // assumption that a rename implicitly means an attribute change. Not too
  // unreasonable, right? Still, we should revisit this before v1.0.
  if (status) {
    SetErrno(uv_last_error(uv_default_loop()));
    eventStr = String::Empty(node_isolate);
  }
  else if (events & UV_RENAME) {
    eventStr = rename_sym;
  }
  else if (events & UV_CHANGE) {
    eventStr = change_sym;
  }
  else {
    assert(0 && "bad fs events flag");
    abort();
  }

  Handle<Value> argv[3] = {
    Integer::New(status, node_isolate),
    eventStr,
    filename ? String::New(filename) : v8::Null(node_isolate)
  };

  MakeCallback(wrap->object_, onchange_sym, ARRAY_SIZE(argv), argv);
}


Handle<Value> FSEventWrap::Close(const Arguments& args) {
  HandleScope scope(node_isolate);

  // Unwrap manually here. The UNWRAP() macro asserts that wrap != NULL.
  // That usually indicates an error but not here: double closes are possible
  // and legal, HandleWrap::Close() deals with them the same way.
  assert(!args.This().IsEmpty());
  assert(args.This()->InternalFieldCount() > 0);
  void* ptr = args.This()->GetAlignedPointerFromInternalField(0);
  FSEventWrap* wrap = static_cast<FSEventWrap*>(ptr);

  if (wrap == NULL || wrap->initialized_ == false) {
    return Undefined(node_isolate);
  }
  wrap->initialized_ = false;

  return HandleWrap::Close(args);
}


} // namespace node

NODE_MODULE(node_fs_event_wrap, node::FSEventWrap::Initialize)
