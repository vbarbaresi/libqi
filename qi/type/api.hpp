#pragma once
/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QI_API_HPP_
#define _QI_API_HPP_

#include <qi/macro.hpp>
#include <qi/type/config.hpp>

#define QI_API QI_LIB_API(qitype)

/* dynamic_casting template partial specializations
 * between shared objects poses problems with
 * clang under macos.
 * This macro is a workaround that seems to work
 * for now.
 */
#ifdef __clang__
# define QITYPE_TEMPLATE_API QI_API
#else
# define QITYPE_TEMPLATE_API
#endif

#endif  // _QI_API_HPP_
