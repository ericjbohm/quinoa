// *****************************************************************************
/*!
  \file      src/NoWarning/threefry.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Include Random123/threefry.h with turning off specific compiler
             warnings
*/
// *****************************************************************************
#ifndef nowarning_threefry_h
#define nowarning_threefry_h

#include "Macro.hpp"

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wold-style-cast"
  #pragma clang diagnostic ignored "-Wsign-conversion"
  #pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
  #pragma clang diagnostic ignored "-Wextra-semi"
  #pragma clang diagnostic ignored "-Wconversion"
  #pragma clang diagnostic ignored "-Wexpansion-to-defined"
#endif

#ifdef __powerpc__
  #define POWERPC
  #undef __powerpc__
  #define __x86_64__
  #define R123_USE_MULHILO64_MULHI_INTRIN 0
  #define R123_USE_GNU_UINT128 1
#endif

#include <Random123/threefry.h>

#ifdef POWERPC
  #define __powerpc__
  #undef __x86_64__
#endif

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif // nowarning_threefry_h
