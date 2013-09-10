// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/preprocessor.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

template <typename FuncT>
struct FuncResult;

#if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

template <typename R, typename... Args>
struct FuncResult<R (Args...)>
{
  typedef R type;
};

#else // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)
  
HADESMEM_DETAIL_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < 
  BOOST_PP_LIMIT_REPEAT);

HADESMEM_DETAIL_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < 
  BOOST_PP_LIMIT_ITERATION);

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#endif // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

}

}
