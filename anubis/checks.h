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

#define ret_return(expr, code, msg, ...)\
	ret = (expr);\
	if (ret code) {\
		ERROR(ret, "" msg ": %s", __VA_ARGS__ strerror(ret));\
		return ret;\
	}

#define errno_return(expr, code, msg, ...)\
	if ((expr) == (code)) {\
		ERROR(errno, "" msg ": %s", __VA_ARGS__ strerror(errno));\
		return errno;\
	}

#endif // ANUBIS_CHECKS_H
