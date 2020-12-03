// *****************************************************************************
/*!
  \file      src/NoWarning/append.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Include brigand/sequences/append.hpp with turning off specific
             compiler warnings
*/
// *****************************************************************************
#ifndef nowarning_append_h
#define nowarning_append_h

#include "Macro.hpp"

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif

#include <brigand/sequences/append.hpp>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif // nowarning_append_h