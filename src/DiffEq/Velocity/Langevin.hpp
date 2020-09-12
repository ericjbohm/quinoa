// *****************************************************************************
/*!
  \file      src/DiffEq/Velocity/Langevin.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Functionality implementing Langevin models for the velocity
  \details   Functionality implementing Langevin models for the velocity.
*/
// *****************************************************************************
#ifndef Langevin_h
#define Langevin_h

#include <array>

#include "Types.hpp"
#include "StatCtr.hpp"
#include "Walker/Options/Depvar.hpp"

namespace walker {

//! Calculate the 2nd order tensor Gij based on the simplified Langevin model
std::array< tk::real, 9 >
slm( tk::real hts, tk::real C0 );

//! Calculate the 2nd order tensor Gij based on the generalized Langevin model
std::array< tk::real, 9 >
glm( tk::real hts,
     tk::real C0,
     const std::array< tk::real, 6 >& rs,
     const std::array< tk::real, 9 >& dU );

//! Compute the Reynolds stress tensor
std::array< tk::real, 6 >
reynoldsStress( char depvar,
                ctr::DepvarType solve,
                const std::map< tk::ctr::Product, tk::real >& moments );

//! Compute the turbulent kinetic energy
tk::real
tke( char depvar,
     ctr::DepvarType solve,
     const std::map< tk::ctr::Product, tk::real >& moments );

} // walker::

#endif // Langevin_h
