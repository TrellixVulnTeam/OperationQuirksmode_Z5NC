

#include "node_stat_watcher.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

namespace node {

using namespace v8;

Persistent<FunctionTemplate> StatWatcher::constructor_template;
static Persistent<String> onchange_sym;
static Persistent<String> onstop_sym;


void StatWatcher::Initialize(Handle<Object> target) {
  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(StatWatcher::New);
  constructor_template = Persistent<FunctionTemplate>::New(node_isolate, t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("StatWatcher"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "start", StatWatcher::Start);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "stop", StatWatcher::Stop);

  target->Set(String::NewSymbol("StatWatcher"), constructor_template->GetFunction());
}


static void Delete(uv_handle_t* handle) {
  delete reinterpret_cast<uv_fs_poll_t*>(handle);
}


StatWatcher::StatWatcher()
  : ObjectWrap()
  , watcher_(new uv_fs_poll_t)
{
  uv_fs_poll_init(uv_default_loop(), watcher_);
  watcher_->data = static_cast<void*>(this);
}


StatWatcher::~StatWatcher() {
  Stop();
  uv_close(reinterpret_cast<uv_handle_t*>(watcher_), Delete);
}


void StatWatcher::Callback(uv_fs_poll_t* handle,
                           int status,
                           const uv_stat_t* prev,
                           const uv_stat_t* curr) {
  StatWatcher* wrap = static_cast<StatWatcher*>(handle->data);
  assert(wrap->watcher_ == handle);
  HandleScope scope(node_isolate);
  Local<Value> argv[3];
  argv[0] = BuildStatsObject(curr);
  argv[1] = BuildStatsObject(prev);
  argv[2] = Integer::New(status, node_isolate);
  if (status == -1) {
    SetErrno(uv_last_error(wrap->watcher_->loop));
  }
  if (onchange_sym.IsEmpty()) {
    onchange_sym = NODE_PSYMBOL("onchange");
  }
  MakeCallback(wrap->handle_, onchange_sym, ARRAY_SIZE(argv), argv);
}


Handle<Value> StatWatcher::New(const Arguments& args) {
  assert(args.IsConstructCall());
  HandleScope scope(node_isolate);
  StatWatcher* s = new StatWatcher();
  s->Wrap(args.This());
  return args.This();
}


Handle<Value> StatWatcher::Start(const Arguments& args) {
  assert(args.Length() == 3);
  HandleScope scope(node_isolate);

  StatWatcher* wrap = ObjectWrap::Unwrap<StatWatcher>(args.This());
  String::Utf8Value path(args[0]);
  const bool persistent = args[1]->BooleanValue();
  const uint32_t interval = args[2]->Uint32Value();

  if (!persistent) uv_unref(reinterpret_cast<uv_handle_t*>(wrap->watcher_));
  uv_fs_poll_start(wrap->watcher_, Callback, *path, interval);
  wrap->Ref();

  return Undefined(node_isolate);
}


Handle<Value> StatWatcher::Stop(const Arguments& args) {
  HandleScope scope(node_isolate);
  StatWatcher* wrap = ObjectWrap::Unwrap<StatWatcher>(args.This());
  if (onstop_sym.IsEmpty()) {
    onstop_sym = NODE_PSYMBOL("onstop");
  }
  MakeCallback(wrap->handle_, onstop_sym, 0, NULL);
  wrap->Stop();
  return Undefined(node_isolate);
}


void StatWatcher::Stop () {
  if (!uv_is_active(reinterpret_cast<uv_handle_t*>(watcher_))) return;
  uv_fs_poll_stop(watcher_);
  Unref();
}


}  // namespace node
