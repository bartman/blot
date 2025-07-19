/* blot: error handling utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <glib-unix.h>
#include <stdarg.h>
#include <errno.h>

#include "blot_utils.h"

static inline void blot_set_error_valist(GError **error,
					 GQuark domain, gint code,
					 const gchar *format, va_list args)
{
	if (unlikely (!error))
		return;

	if (unlikely (*error))
		g_clear_error(error);

	*error = g_error_new_valist(domain, code, format, args);
}

static inline void blot_set_error(GError **error, GQuark domain, gint code,
				  const gchar *format, ...)
{
	va_list ap;
	va_start(ap, format);
	blot_set_error_valist(error, domain, code, format, ap);
	va_end(ap);
}

static inline void __blot_set_error_unix_valist(GError **error, int errno_code,
						const char *fn, int line,
						const gchar *format, va_list args)
{
	gchar *text;

	if (unlikely (!error))
		return;

	if (unlikely (*error))
		g_clear_error(error);

	text = g_strdup_vprintf(format, args);
	g_assert_nonnull(text);

	*error = g_error_new(G_UNIX_ERROR, errno_code,
			     "%s:%u: %s: %s", fn, line,
			     text, g_strerror(errno_code));

	g_free(text);
}
#define blot_set_error_unix_valist(_error,_errno_code,_format,_args) \
	__blot_set_error_unix_valist(_error,_errno_code,__func__,__LINE__,_format,_args)

static inline void __blot_set_error_unix(GError **error, int errno_code,
					 const char *fn, int line,
					 const gchar *format, ...)
{
	va_list ap;

	if (unlikely (!error))
		return;

	va_start(ap, format);
	__blot_set_error_unix_valist(error, errno_code, fn, line, format, ap);
	va_end(ap);
}
#define blot_set_error_unix(_error,_errno_code,_format,_a...) \
	__blot_set_error_unix(_error,_errno_code,__func__,__LINE__,_format,##_a)

#define blot_set_error_errno(_error,_format,_a...) \
	__blot_set_error_unix(_error,errno,__func__,__LINE__,_format,##_a)


#undef RETURN_ERRORx
#define RETURN_ERRORx(_condition,_ret,_error,_errno_code,_format,_a...) ({ \
	if (unlikely (_condition)) { \
		blot_set_error_unix(_error,_errno_code,_format,##_a); \
		return _ret; \
	} \
})

#undef RETURN_ERROR
#define RETURN_ERROR(_condition,_ret,_error,_format,_a...) \
	RETURN_ERRORx(_condition,_ret,_error,errno,_format,##_a)

#undef RETURN_ERROR_IF
#define RETURN_ERROR_IF(_condition,_ret,_error,_errno_code) \
	RETURN_ERRORx(_condition, _ret, _error, _errno_code, "%s", #_condition)

#undef RETURN_EFAULT_IF
#define RETURN_EFAULT_IF(_condition,_ret,_error) \
	RETURN_ERROR_IF(_condition,_ret,_error,EFAULT)

#undef RETURN_EINVAL_IF
#define RETURN_EINVAL_IF(_condition,_ret,_error) \
	RETURN_ERROR_IF(_condition,_ret,_error,EINVAL)

#undef RETURN_IF
#define RETURN_IF(_condition, _ret) ({ \
	if (unlikely (_condition)) \
		return _ret; \
})

