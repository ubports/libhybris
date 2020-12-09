#ifndef thatasyncsafeheaderthingy_replacement__
#define thatasyncsafeheaderthingy_replacement__
#include <stdio.h>
enum {
  ANDROID_LOG_UNKNOWN = 0,
  ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */

  ANDROID_LOG_VERBOSE,
  ANDROID_LOG_DEBUG,
  ANDROID_LOG_INFO,
  ANDROID_LOG_WARN,
  ANDROID_LOG_ERROR,
  ANDROID_LOG_FATAL,

  ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
};

#define async_safe_fatal(...) { fprintf(stderr, __VA_ARGS__); abort(); }

#define async_safe_format_fd(fd, ...) dprintf(fd, __VA_ARGS__)

// Don't really care about verbose/debug logs for now
#define async_safe_format_log_va_list(loglevel, category, format, ...) { if (loglevel > ANDROID_LOG_DEFAULT) { vfprintf(stdout, format, __VA_ARGS__); /*vfprintf(stderr, "%s", "\n");*/} }

#endif

