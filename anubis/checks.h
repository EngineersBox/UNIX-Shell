#ifndef ANUBIS_CHECKS_H
#define ANUBIS_CHECKS_H

#include <errno.h>
#include "error.h"

#define __INSTANCE_NULL_CHECK(_instanceName, _this, _ret) \
	if (_this == NULL) {\
		ERROR(EINVAL, "Cannot invoke instance method on null " _instanceName " instance\n");\
		_ret;\
	}
#define INSTANCE_NULL_CHECK_RETURN(_instanceName, _this, _returnValue) __INSTANCE_NULL_CHECK(_instanceName, _this, return _returnValue)
#define INSTANCE_NULL_CHECK(_instanceName, _this) __INSTANCE_NULL_CHECK(_instanceName, _this, return);

#endif // ANUBIS_CHECKS_H
