

#include "node.h"
#include "node_buffer.h"
#include "req_wrap.h"
#include "handle_wrap.h"
#include "stream_wrap.h"
#include "pipe_wrap.h"

namespace node {

using v8::Arguments;
using v8::Boolean;
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
using v8::Value;

Persistent<Function> pipeConstructor;

static Persistent<String> onconnection_sym;
static Persistent<String> oncomplete_sym;


// TODO share with TCPWrap?
typedef class ReqWrap<uv_connect_t> ConnectWrap;


uv_pipe_t* PipeWrap::UVHandle() {
  return &handle_;
}


Local<Object> PipeWrap::Instantiate() {
  HandleScope scope(node_isolate);
  assert(!pipeConstructor.IsEmpty());
  return scope.Close(pipeConstructor->NewInstance());
}


PipeWrap* PipeWrap::Unwrap(Local<Object> obj) {
  assert(!obj.IsEmpty());
  assert(obj->InternalFieldCount() > 0);
  return static_cast<PipeWrap*>(obj->GetAlignedPointerFromInternalField(0));
}


void PipeWrap::Initialize(Handle<Object> target) {
  StreamWrap::Initialize(target);

  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->SetClassName(String::NewSymbol("Pipe"));

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
  NODE_SET_PROTOTYPE_METHOD(t, "ref", HandleWrap::Ref);

  NODE_SET_PROTOTYPE_METHOD(t, "readStart", StreamWrap::ReadStart);
  NODE_SET_PROTOTYPE_METHOD(t, "readStop", StreamWrap::ReadStop);
  NODE_SET_PROTOTYPE_METHOD(t, "shutdown", StreamWrap::Shutdown);

  NODE_SET_PROTOTYPE_METHOD(t, "writeBuffer", StreamWrap::WriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(t, "writeAsciiString", StreamWrap::WriteAsciiString);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUtf8String", StreamWrap::WriteUtf8String);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUcs2String", StreamWrap::WriteUcs2String);

  NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
  NODE_SET_PROTOTYPE_METHOD(t, "listen", Listen);
  NODE_SET_PROTOTYPE_METHOD(t, "connect", Connect);
  NODE_SET_PROTOTYPE_METHOD(t, "open", Open);

#ifdef _WIN32
  NODE_SET_PROTOTYPE_METHOD(t, "setPendingInstances", SetPendingInstances);
#endif

  pipeConstructor = Persistent<Function>::New(node_isolate, t->GetFunction());

  target->Set(String::NewSymbol("Pipe"), pipeConstructor);
}


Handle<Value> PipeWrap::New(const Arguments& args) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());

  HandleScope scope(node_isolate);
  PipeWrap* wrap = new PipeWrap(args.This(), args[0]->IsTrue());
  assert(wrap);

  return scope.Close(args.This());
}


PipeWrap::PipeWrap(Handle<Object> object, bool ipc)
    : StreamWrap(object, reinterpret_cast<uv_stream_t*>(&handle_)) {
  int r = uv_pipe_init(uv_default_loop(), &handle_, ipc);
  assert(r == 0); // How do we proxy this error up to javascript?
                  // Suggestion: uv_pipe_init() returns void.
  UpdateWriteQueueSize();
}


Handle<Value> PipeWrap::Bind(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(PipeWrap)

  String::AsciiValue name(args[0]);

  int r = uv_pipe_bind(&wrap->handle_, *name);

  // Error starting the pipe.
  if (r) SetErrno(uv_last_error(uv_default_loop()));

  return scope.Close(Integer::New(r, node_isolate));
}


#ifdef _WIN32
Handle<Value> PipeWrap::SetPendingInstances(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(PipeWrap)

  int instances = args[0]->Int32Value();

  uv_pipe_pending_instances(&wrap->handle_, instances);

  return v8::Null(node_isolate);
}
#endif


Handle<Value> PipeWrap::Listen(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(PipeWrap)

  int backlog = args[0]->Int32Value();

  int r = uv_listen(reinterpret_cast<uv_stream_t*>(&wrap->handle_),
                    backlog,
                    OnConnection);

  // Error starting the pipe.
  if (r) SetErrno(uv_last_error(uv_default_loop()));

  return scope.Close(Integer::New(r, node_isolate));
}


// TODO maybe share with TCPWrap?
void PipeWrap::OnConnection(uv_stream_t* handle, int status) {
  HandleScope scope(node_isolate);

  PipeWrap* wrap = static_cast<PipeWrap*>(handle->data);
  assert(&wrap->handle_ == reinterpret_cast<uv_pipe_t*>(handle));

  // We should not be getting this callback if someone as already called
  // uv_close() on the handle.
  assert(wrap->object_.IsEmpty() == false);

  if (status != 0) {
    SetErrno(uv_last_error(uv_default_loop()));
    MakeCallback(wrap->object_, "onconnection", 0, NULL);
    return;
  }

  // Instanciate the client javascript object and handle.
  Local<Object> client_obj = pipeConstructor->NewInstance();

  // Unwrap the client javascript object.
  assert(client_obj->InternalFieldCount() > 0);
  PipeWrap* client_wrap =
      static_cast<PipeWrap*>(client_obj->GetAlignedPointerFromInternalField(0));
  uv_stream_t* client_handle =
      reinterpret_cast<uv_stream_t*>(&client_wrap->handle_);
  if (uv_accept(handle, client_handle)) return;

  // Successful accept. Call the onconnection callback in JavaScript land.
  Local<Value> argv[1] = { client_obj };
  if (onconnection_sym.IsEmpty()) {
    onconnection_sym = NODE_PSYMBOL("onconnection");
  }
  MakeCallback(wrap->object_, onconnection_sym, ARRAY_SIZE(argv), argv);
}

// TODO Maybe share this with TCPWrap?
void PipeWrap::AfterConnect(uv_connect_t* req, int status) {
  ConnectWrap* req_wrap = (ConnectWrap*) req->data;
  PipeWrap* wrap = (PipeWrap*) req->handle->data;

  HandleScope scope(node_isolate);

  // The wrap and request objects should still be there.
  assert(req_wrap->object_.IsEmpty() == false);
  assert(wrap->object_.IsEmpty() == false);

  bool readable, writable;

  if (status) {
    SetErrno(uv_last_error(uv_default_loop()));
    readable = writable = 0;
  } else {
    readable = uv_is_readable(req->handle) != 0;
    writable = uv_is_writable(req->handle) != 0;
  }

  Local<Value> argv[5] = {
    Integer::New(status, node_isolate),
    Local<Value>::New(node_isolate, wrap->object_),
    Local<Value>::New(node_isolate, req_wrap->object_),
    Local<Value>::New(node_isolate, Boolean::New(readable)),
    Local<Value>::New(node_isolate, Boolean::New(writable))
  };

  if (oncomplete_sym.IsEmpty()) {
    oncomplete_sym = NODE_PSYMBOL("oncomplete");
  }
  MakeCallback(req_wrap->object_, oncomplete_sym, ARRAY_SIZE(argv), argv);

  delete req_wrap;
}


Handle<Value> PipeWrap::Open(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(PipeWrap)

  int fd = args[0]->IntegerValue();

  uv_pipe_open(&wrap->handle_, fd);

  return scope.Close(v8::Null(node_isolate));
}


Handle<Value> PipeWrap::Connect(const Arguments& args) {
  HandleScope scope(node_isolate);

  UNWRAP(PipeWrap)

  String::AsciiValue name(args[0]);

  ConnectWrap* req_wrap = new ConnectWrap();

  uv_pipe_connect(&req_wrap->req_,
                  &wrap->handle_,
                  *name,
                  AfterConnect);

  req_wrap->Dispatched();

  return scope.Close(req_wrap->object_);
}


}  // namespace node

NODE_MODULE(node_pipe_wrap, node::PipeWrap::Initialize)
