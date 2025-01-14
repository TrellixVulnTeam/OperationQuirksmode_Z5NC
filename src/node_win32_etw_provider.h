

#ifndef SRC_ETW_H_
#define SRC_ETW_H_

#include <evntprov.h>
#include "node_dtrace.h"

namespace node {

using namespace v8;

#if defined(_MSC_VER)
# define INLINE __forceinline
#else
# define INLINE inline
#endif

typedef ULONG (NTAPI *EventRegisterFunc)(
  LPCGUID ProviderId,
  PENABLECALLBACK EnableCallback,
  PVOID CallbackContext,
  PREGHANDLE RegHandle
);

typedef ULONG (NTAPI *EventUnregisterFunc)(
  REGHANDLE RegHandle
);

typedef ULONG (NTAPI *EventWriteFunc)(
  REGHANDLE RegHandle,
  PCEVENT_DESCRIPTOR EventDescriptor,
  ULONG UserDataCount,
  PEVENT_DATA_DESCRIPTOR UserData
);

void init_etw();
void shutdown_etw();

INLINE void NODE_HTTP_SERVER_REQUEST(node_dtrace_http_server_request_t* req,
  node_dtrace_connection_t* conn, const char *remote, int port,
  const char *method, const char *url, int fd);
INLINE void NODE_HTTP_SERVER_RESPONSE(node_dtrace_connection_t* conn,
  const char *remote, int port, int fd);
INLINE void NODE_HTTP_CLIENT_REQUEST(node_dtrace_http_client_request_t* req,
  node_dtrace_connection_t* conn, const char *remote, int port,
  const char *method, const char *url, int fd);
INLINE void NODE_HTTP_CLIENT_RESPONSE(node_dtrace_connection_t* conn,
  const char *remote, int port, int fd);
INLINE void NODE_NET_SERVER_CONNECTION(node_dtrace_connection_t* conn,
  const char *remote, int port, int fd);
INLINE void NODE_NET_STREAM_END(node_dtrace_connection_t* conn,
  const char *remote, int port, int fd);
INLINE void NODE_GC_START(GCType type, GCCallbackFlags flags);
INLINE void NODE_GC_DONE(GCType type, GCCallbackFlags flags);
INLINE void NODE_V8SYMBOL_REMOVE(const void* addr1, const void* addr2);
INLINE void NODE_V8SYMBOL_MOVE(const void* addr1, const void* addr2);
INLINE void NODE_V8SYMBOL_RESET();
INLINE void NODE_V8SYMBOL_ADD(LPCSTR symbol,
                              int symbol_len,
                              const void* addr1,
                              int len);

INLINE bool NODE_HTTP_SERVER_REQUEST_ENABLED();
INLINE bool NODE_HTTP_SERVER_RESPONSE_ENABLED();
INLINE bool NODE_HTTP_CLIENT_REQUEST_ENABLED();
INLINE bool NODE_HTTP_CLIENT_RESPONSE_ENABLED();
INLINE bool NODE_NET_SERVER_CONNECTION_ENABLED();
INLINE bool NODE_NET_STREAM_END_ENABLED();
INLINE bool NODE_NET_SOCKET_READ_ENABLED();
INLINE bool NODE_NET_SOCKET_WRITE_ENABLED();
INLINE bool NODE_V8SYMBOL_ENABLED();

#define NODE_NET_SOCKET_READ(arg0, arg1)
#define NODE_NET_SOCKET_WRITE(arg0, arg1)
}
#endif  // SRC_ETW_H_
