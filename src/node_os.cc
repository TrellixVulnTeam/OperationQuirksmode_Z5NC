


#include "node.h"
#include "node_os.h"

#include "v8.h"

#include <errno.h>
#include <string.h>

#ifdef __MINGW32__
# include <io.h>
#endif

#ifdef __POSIX__
# include <netdb.h>         // MAXHOSTNAMELEN on Solaris.
# include <unistd.h>        // gethostname, sysconf
# include <sys/param.h>     // MAXHOSTNAMELEN on Linux and the BSDs.
# include <sys/utsname.h>
#endif

// Add Windows fallback.
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

namespace node {

using namespace v8;

static Handle<Value> GetEndianness(const Arguments& args) {
  HandleScope scope(node_isolate);
  return scope.Close(String::New(IsBigEndian() ? "BE" : "LE"));
}

static Handle<Value> GetHostname(const Arguments& args) {
  HandleScope scope(node_isolate);
  char buf[MAXHOSTNAMELEN + 1];

  if (gethostname(buf, sizeof(buf))) {
#ifdef __POSIX__
    return ThrowException(ErrnoException(errno, "gethostname"));
#else // __MINGW32__
    return ThrowException(ErrnoException(WSAGetLastError(), "gethostname"));
#endif // __MINGW32__
  }
  buf[sizeof(buf) - 1] = '\0';

  return scope.Close(String::New(buf));
}

static Handle<Value> GetOSType(const Arguments& args) {
  HandleScope scope(node_isolate);

#ifdef __POSIX__
  struct utsname info;
  if (uname(&info) < 0) {
    return ThrowException(ErrnoException(errno, "uname"));
  }
  return scope.Close(String::New(info.sysname));
#else // __MINGW32__
  return scope.Close(String::New("Windows_NT"));
#endif
}

static Handle<Value> GetOSRelease(const Arguments& args) {
  HandleScope scope(node_isolate);

#ifdef __POSIX__
  struct utsname info;
  if (uname(&info) < 0) {
    return ThrowException(ErrnoException(errno, "uname"));
  }
  return scope.Close(String::New(info.release));
#else // __MINGW32__
  char release[256];
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);

  if (GetVersionEx(&info) == 0) {
    return Undefined(node_isolate);
  }

  sprintf(release, "%d.%d.%d", static_cast<int>(info.dwMajorVersion),
      static_cast<int>(info.dwMinorVersion), static_cast<int>(info.dwBuildNumber));
  return scope.Close(String::New(release));
#endif

}

static Handle<Value> GetCPUInfo(const Arguments& args) {
  HandleScope scope(node_isolate);
  uv_cpu_info_t* cpu_infos;
  int count, i;

  uv_err_t err = uv_cpu_info(&cpu_infos, &count);

  if (err.code != UV_OK) {
    return Undefined(node_isolate);
  }

  Local<Array> cpus = Array::New();

  for (i = 0; i < count; i++) {
    Local<Object> times_info = Object::New();
    times_info->Set(String::New("user"),
      Integer::New(cpu_infos[i].cpu_times.user, node_isolate));
    times_info->Set(String::New("nice"),
      Integer::New(cpu_infos[i].cpu_times.nice, node_isolate));
    times_info->Set(String::New("sys"),
      Integer::New(cpu_infos[i].cpu_times.sys, node_isolate));
    times_info->Set(String::New("idle"),
      Integer::New(cpu_infos[i].cpu_times.idle, node_isolate));
    times_info->Set(String::New("irq"),
      Integer::New(cpu_infos[i].cpu_times.irq, node_isolate));

    Local<Object> cpu_info = Object::New();
    cpu_info->Set(String::New("model"), String::New(cpu_infos[i].model));
    cpu_info->Set(String::New("speed"),
                  Integer::New(cpu_infos[i].speed, node_isolate));
    cpu_info->Set(String::New("times"), times_info);
    (*cpus)->Set(i,cpu_info);
  }

  uv_free_cpu_info(cpu_infos, count);

  return scope.Close(cpus);
}

static Handle<Value> GetFreeMemory(const Arguments& args) {
  HandleScope scope(node_isolate);
  double amount = uv_get_free_memory();

  if (amount < 0) {
    return Undefined(node_isolate);
  }

  return scope.Close(Number::New(amount));
}

static Handle<Value> GetTotalMemory(const Arguments& args) {
  HandleScope scope(node_isolate);
  double amount = uv_get_total_memory();

  if (amount < 0) {
    return Undefined(node_isolate);
  }

  return scope.Close(Number::New(amount));
}

static Handle<Value> GetUptime(const Arguments& args) {
  HandleScope scope(node_isolate);
  double uptime;

  uv_err_t err = uv_uptime(&uptime);

  if (err.code != UV_OK) {
    return Undefined(node_isolate);
  }

  return scope.Close(Number::New(uptime));
}

static Handle<Value> GetLoadAvg(const Arguments& args) {
  HandleScope scope(node_isolate);
  double loadavg[3];
  uv_loadavg(loadavg);

  Local<Array> loads = Array::New(3);
  loads->Set(0, Number::New(loadavg[0]));
  loads->Set(1, Number::New(loadavg[1]));
  loads->Set(2, Number::New(loadavg[2]));

  return scope.Close(loads);
}


static Handle<Value> GetInterfaceAddresses(const Arguments& args) {
  HandleScope scope(node_isolate);
  uv_interface_address_t* interfaces;
  int count, i;
  char ip[INET6_ADDRSTRLEN];
  char netmask[INET6_ADDRSTRLEN];
  Local<Object> ret, o;
  Local<String> name, family;
  Local<Array> ifarr;

  uv_err_t err = uv_interface_addresses(&interfaces, &count);

  if (err.code != UV_OK)
    return ThrowException(UVException(err.code, "uv_interface_addresses"));

  ret = Object::New();

  for (i = 0; i < count; i++) {
    name = String::New(interfaces[i].name);
    if (ret->Has(name)) {
      ifarr = Local<Array>::Cast(ret->Get(name));
    } else {
      ifarr = Array::New();
      ret->Set(name, ifarr);
    }

    if (interfaces[i].address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interfaces[i].address.address4,ip, sizeof(ip));
      uv_ip4_name(&interfaces[i].netmask.netmask4, netmask, sizeof(netmask));
      family = String::New("IPv4");
    } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
      uv_ip6_name(&interfaces[i].netmask.netmask6, netmask, sizeof(netmask));
      family = String::New("IPv6");
    } else {
      strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
      family = String::New("<unknown>");
    }

    o = Object::New();
    o->Set(String::New("address"), String::New(ip));
    o->Set(String::New("netmask"), String::New(netmask));
    o->Set(String::New("family"), family);

    const bool internal = interfaces[i].is_internal;
    o->Set(String::New("internal"),
           internal ? True(node_isolate) : False(node_isolate));

    ifarr->Set(ifarr->Length(), o);
  }

  uv_free_interface_addresses(interfaces, count);

  return scope.Close(ret);
}


void OS::Initialize(v8::Handle<v8::Object> target) {
  HandleScope scope(node_isolate);

  NODE_SET_METHOD(target, "getEndianness", GetEndianness);
  NODE_SET_METHOD(target, "getHostname", GetHostname);
  NODE_SET_METHOD(target, "getLoadAvg", GetLoadAvg);
  NODE_SET_METHOD(target, "getUptime", GetUptime);
  NODE_SET_METHOD(target, "getTotalMem", GetTotalMemory);
  NODE_SET_METHOD(target, "getFreeMem", GetFreeMemory);
  NODE_SET_METHOD(target, "getCPUs", GetCPUInfo);
  NODE_SET_METHOD(target, "getOSType", GetOSType);
  NODE_SET_METHOD(target, "getOSRelease", GetOSRelease);
  NODE_SET_METHOD(target, "getInterfaceAddresses", GetInterfaceAddresses);
}


}  // namespace node

NODE_MODULE(node_os, node::OS::Initialize)
