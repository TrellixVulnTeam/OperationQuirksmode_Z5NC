

#include "node.h"
#include "queue.h"
#include "handle_wrap.h"

namespace node {

using v8::Arguments;
using v8::Array;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::TryCatch;
using v8::Undefined;
using v8::Value;


// defined in node.cc
extern QUEUE handle_wrap_queue;
static Persistent<String> close_sym;


void HandleWrap::Initialize(Handle<Object> target) {
  /* Doesn't do anything at the moment. */
}


Handle<Value> HandleWrap::Ref(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP_NO_ABORT(HandleWrap)

  if (wrap != NULL && wrap->handle__ != NULL) {
    uv_ref(wrap->handle__);
    wrap->flags_ &= ~kUnref;
  }

  return v8::Undefined(node_isolate);
}


Handle<Value> HandleWrap::Unref(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP_NO_ABORT(HandleWrap)

  if (wrap != NULL && wrap->handle__ != NULL) {
    uv_unref(wrap->handle__);
    wrap->flags_ |= kUnref;
  }

  return v8::Undefined(node_isolate);
}


Handle<Value> HandleWrap::Close(const Arguments& args) {
  HandleScope scope(node_isolate);

  HandleWrap *wrap = static_cast<HandleWrap*>(
      args.This()->GetAlignedPointerFromInternalField(0));

  // guard against uninitialized handle or double close
  if (wrap == NULL || wrap->handle__ == NULL) {
    return Undefined(node_isolate);
  }

  assert(!wrap->object_.IsEmpty());
  uv_close(wrap->handle__, OnClose);
  wrap->handle__ = NULL;

  if (args[0]->IsFunction()) {
    if (close_sym.IsEmpty() == true) close_sym = NODE_PSYMBOL("close");
    wrap->object_->Set(close_sym, args[0]);
    wrap->flags_ |= kCloseCallback;
  }

  return Undefined(node_isolate);
}


HandleWrap::HandleWrap(Handle<Object> object, uv_handle_t* h) {
  flags_ = 0;
  handle__ = h;
  handle__->data = this;

  HandleScope scope(node_isolate);
  assert(object_.IsEmpty());
  assert(object->InternalFieldCount() > 0);
  object_ = v8::Persistent<v8::Object>::New(node_isolate, object);
  object_->SetAlignedPointerInInternalField(0, this);
  QUEUE_INSERT_TAIL(&handle_wrap_queue, &handle_wrap_queue_);
}


HandleWrap::~HandleWrap() {
  assert(object_.IsEmpty());
  QUEUE_REMOVE(&handle_wrap_queue_);
}


void HandleWrap::OnClose(uv_handle_t* handle) {
  HandleWrap* wrap = static_cast<HandleWrap*>(handle->data);

  // The wrap object should still be there.
  assert(wrap->object_.IsEmpty() == false);

  // But the handle pointer should be gone.
  assert(wrap->handle__ == NULL);

  if (wrap->flags_ & kCloseCallback) {
    assert(close_sym.IsEmpty() == false);
    MakeCallback(wrap->object_, close_sym, 0, NULL);
  }

  wrap->object_->SetAlignedPointerInInternalField(0, NULL);
  wrap->object_.Dispose(node_isolate);
  wrap->object_.Clear();

  delete wrap;
}


}  // namespace node
