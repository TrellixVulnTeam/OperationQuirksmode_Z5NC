

#ifndef SRC_NODE_INTERNALS_H_
#define SRC_NODE_INTERNALS_H_

#include <stdlib.h>

#include "v8.h"

namespace node {

// Defined in node.cc
extern v8::Isolate* node_isolate;

// Defined in node.cc at startup.
extern v8::Persistent<v8::Object> process;

#ifdef _WIN32
// emulate snprintf() on windows, _snprintf() doesn't zero-terminate the buffer
// on overflow...
#include <stdarg.h>
inline static int snprintf(char* buf, unsigned int len, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = _vsprintf_p(buf, len, fmt, ap);
  if (len) buf[len - 1] = '\0';
  va_end(ap);
  return n;
}
#endif

#if defined(__x86_64__)
# define BITS_PER_LONG 64
#else
# define BITS_PER_LONG 32
#endif

#ifndef offset_of
// g++ in strict mode complains loudly about the system offsetof() macro
// because it uses NULL as the base address.
# define offset_of(type, member) \
  ((intptr_t) ((char *) &(((type *) 8)->member) - 8))
#endif

#ifndef container_of
# define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offset_of(type, member)))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))
#endif

#ifndef ROUND_UP
# define ROUND_UP(a, b) ((a) % (b) ? ((a) + (b)) - ((a) % (b)) : (a))
#endif

// this would have been a template function were it not for the fact that g++
// sometimes fails to resolve it...
#define THROW_ERROR(fun)                                                      \
  do {                                                                        \
    v8::HandleScope scope(node_isolate);                                      \
    return v8::ThrowException(fun(v8::String::New(errmsg)));                  \
  }                                                                           \
  while (0)

inline static v8::Handle<v8::Value> ThrowError(const char* errmsg) {
  THROW_ERROR(v8::Exception::Error);
}

inline static v8::Handle<v8::Value> ThrowTypeError(const char* errmsg) {
  THROW_ERROR(v8::Exception::TypeError);
}

inline static v8::Handle<v8::Value> ThrowRangeError(const char* errmsg) {
  THROW_ERROR(v8::Exception::RangeError);
}

#define UNWRAP(type)                                                        \
  assert(!args.This().IsEmpty());                                           \
  assert(args.This()->InternalFieldCount() > 0);                            \
  type* wrap = static_cast<type*>(                                          \
      args.This()->GetAlignedPointerFromInternalField(0));                  \
  if (!wrap) {                                                              \
    fprintf(stderr, #type ": Aborting due to unwrap failure at %s:%d\n",    \
            __FILE__, __LINE__);                                            \
    abort();                                                                \
  }

v8::Handle<v8::Value> FromConstructorTemplate(
    v8::Persistent<v8::FunctionTemplate> t,
    const v8::Arguments& args);

// allow for quick domain check
extern bool using_domains;

enum Endianness {
  kLittleEndian,  // _Not_ LITTLE_ENDIAN, clashes with endian.h.
  kBigEndian
};

inline enum Endianness GetEndianness() {
  // Constant-folded by the compiler.
  const union {
    uint8_t u8[2];
    uint16_t u16;
  } u = {
    { 1, 0 }
  };
  return u.u16 == 1 ? kLittleEndian : kBigEndian;
}

inline bool IsLittleEndian() {
  return GetEndianness() == kLittleEndian;
}

inline bool IsBigEndian() {
  return GetEndianness() == kBigEndian;
}

} // namespace node

#endif // SRC_NODE_INTERNALS_H_
