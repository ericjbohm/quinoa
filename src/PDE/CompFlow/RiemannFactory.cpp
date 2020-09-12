// *****************************************************************************
/*!
  \file      src/PDE/CompFlow/RiemannFactory.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Register available Riemann solvers for single material compressible
             hydrodynamics into a factory
  \details   Register available Riemann solvers for single material compressible
             hydrodynamics into a factory.
*/
// *****************************************************************************

#include <brigand/sequences/list.hpp>
#include <brigand/algorithms/for_each.hpp>

#include "RiemannFactory.hpp"
#include "Riemann/HLLC.hpp"
#include "Riemann/LaxFriedrichs.hpp"

inciter::CompFlowRiemannFactory
inciter::compflowRiemannSolvers()
// *****************************************************************************
// \brief Register available Riemann solvers for compressible hydrodynamics into
//   a factory
//! \return Riemann solver factory
// *****************************************************************************
{
  using RiemannSolverList = brigand::list< HLLC, LaxFriedrichs >;
  CompFlowRiemannFactory r;
  brigand::for_each< RiemannSolverList >( registerRiemannSolver( r ) );
  return r;
}
