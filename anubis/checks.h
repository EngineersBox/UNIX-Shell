#ifndef ANUBIS_CHECKS_H
#define ANUBIS_CHECKS_H

#include <errno.h>
#include "error.h"

#define __INSTANCE_NULL_CHECK(_instanceName, _this, _ret) \
	if (_this == NULL) {\
		ERROR(EINVAL, "Cannot invoke instance method on null " _instanceName " instance");\
		_ret;\
	}
#define INSTANCE_NULL_CHECK_RETURN(_instanceName, _this, _returnValue) __INSTANCE_NULL_CHECK(_instanceName, _this, return _returnValue)
#define INSTANCE_NULL_CHECK(_instanceName, _this) __INSTANCE_NULL_CHECK(_instanceName, _this, return);

#define transparent_return(expr)\
	ret = (expr);\
	if (ret) {\
		return ret;\
	}

#define ret_return(expr, code, ...)\
	ret = (expr);\
	if (ret code) {\
		ERROR(ret, __VA_ARGS__);\
		return ret;\
	}

#define verrno_return(expr, rv, ...)\
	if ((expr) == (rv)) {\
		ERROR(errno, __VA_ARGS__);\
		return rv;\
	}

#define errno_return(expr, code, ...)\
	if ((expr) == (code)) {\
		ERROR(errno, __VA_ARGS__);\
		return errno;\
	}

#define checked_invocation(func, value) ((value) == NULL ? NULL : (func)((value)))

#endif // ANUBIS_CHECKS_H
