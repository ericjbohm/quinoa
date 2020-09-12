// *****************************************************************************
/*!
  \file      src/NoWarning/HYPRE_krylov.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Include HYPRE_krylov.h with turning off specific compiler
             warnings
*/
// *****************************************************************************
#ifndef nowarning_HYPRE_krylov_h
#define nowarning_HYPRE_krylov_h

#include "Macro.hpp"

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif

#include <HYPRE_krylov.h>

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif // nowarning_HYPRE_krylov_h
