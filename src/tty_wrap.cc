

#include "node.h"
#include "node_buffer.h"
#include "req_wrap.h"
#include "handle_wrap.h"
#include "stream_wrap.h"
#include "tty_wrap.h"

namespace node {

using v8::Arguments;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::PropertyAttribute;
using v8::String;
using v8::TryCatch;
using v8::Undefined;
using v8::Value;


void TTYWrap::Initialize(Handle<Object> target) {
  StreamWrap::Initialize(target);

  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->SetClassName(String::NewSymbol("TTY"));

  t->InstanceTemplate()->SetInternalFieldCount(1);

  enum PropertyAttribute attributes =
      static_cast<PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
  t->InstanceTemplate()->SetAccessor(String::New("fd"),
                                     StreamWrap::GetFD,
                                     NULL,
                                     Handle<Value>(),
                                     v8::DEFAULT,
                                     attributes);

  NODE_SET_PROTOTYPE_METHOD(t, "close", HandleWrap::Close);
  NODE_SET_PROTOTYPE_METHOD(t, "unref", HandleWrap::Unref);

  NODE_SET_PROTOTYPE_METHOD(t, "readStart", StreamWrap::ReadStart);
  NODE_SET_PROTOTYPE_METHOD(t, "readStop", StreamWrap::ReadStop);

  NODE_SET_PROTOTYPE_METHOD(t, "writeBuffer", StreamWrap::WriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(t, "writeAsciiString", StreamWrap::WriteAsciiString);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUtf8String", StreamWrap::WriteUtf8String);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUcs2String", StreamWrap::WriteUcs2String);

  NODE_SET_PROTOTYPE_METHOD(t, "getWindowSize", TTYWrap::GetWindowSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setRawMode", SetRawMode);

  NODE_SET_METHOD(target, "isTTY", IsTTY);
  NODE_SET_METHOD(target, "guessHandleType", GuessHandleType);

  target->Set(String::NewSymbol("TTY"), t->GetFunction());
}


TTYWrap* TTYWrap::Unwrap(Local<Object> obj) {
  assert(!obj.IsEmpty());
  assert(obj->InternalFieldCount() > 0);
  return static_cast<TTYWrap*>(obj->GetAlignedPointerFromInternalField(0));
}


uv_tty_t* TTYWrap::UVHandle() {
  return &handle_;
}


Handle<Value> TTYWrap::GuessHandleType(const Arguments& args) {
  HandleScope scope(node_isolate);
  int fd = args[0]->Int32Value();
  assert(fd >= 0);

  uv_handle_type t = uv_guess_handle(fd);

  switch (t) {
    case UV_TCP:
      return scope.Close(String::New("TCP"));

    case UV_TTY:
      return scope.Close(String::New("TTY"));

    case UV_UDP:
      return scope.Close(String::New("UDP"));

    case UV_NAMED_PIPE:
      return scope.Close(String::New("PIPE"));

    case UV_FILE:
      return scope.Close(String::New("FILE"));

    case UV_UNKNOWN_HANDLE:
      return scope.Close(String::New("UNKNOWN"));

    default:
      assert(0);
      return v8::Undefined(node_isolate);
  }
}


Handle<Value> TTYWrap::IsTTY(const Arguments& args) {
  HandleScope scope(node_isolate);
  int fd = args[0]->Int32Value();
  assert(fd >= 0);

  if (uv_guess_handle(fd) == UV_TTY) {
    return v8::True(node_isolate);
  }

  return v8::False(node_isolate);
}


Handle<Value> TTYWrap::GetWindowSize(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(TTYWrap)

  int width, height;
  int r = uv_tty_get_winsize(&wrap->handle_, &width, &height);

  if (r) {
    SetErrno(uv_last_error(uv_default_loop()));
    return v8::Undefined(node_isolate);
  }

  Local<v8::Array> a = v8::Array::New(2);
  a->Set(0, Integer::New(width, node_isolate));
  a->Set(1, Integer::New(height, node_isolate));

  return scope.Close(a);
}


Handle<Value> TTYWrap::SetRawMode(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(TTYWrap)

  int r = uv_tty_set_mode(&wrap->handle_, args[0]->IsTrue());

  if (r) {
    SetErrno(uv_last_error(uv_default_loop()));
  }

  return scope.Close(Integer::New(r, node_isolate));
}


Handle<Value> TTYWrap::New(const Arguments& args) {
  HandleScope scope(node_isolate);

  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());

  int fd = args[0]->Int32Value();
  assert(fd >= 0);

  TTYWrap* wrap = new TTYWrap(args.This(), fd, args[1]->IsTrue());
  assert(wrap);
  wrap->UpdateWriteQueueSize();

  return scope.Close(args.This());
}


TTYWrap::TTYWrap(Handle<Object> object, int fd, bool readable)
    : StreamWrap(object, reinterpret_cast<uv_stream_t*>(&handle_)) {
  uv_tty_init(uv_default_loop(), &handle_, fd, readable);
}

}  // namespace node

NODE_MODULE(node_tty_wrap, node::TTYWrap::Initialize)
