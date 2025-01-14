// *****************************************************************************
/*!
  \file      src/PDE/MultiMat/Physics/DGVelEq.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Physics policy for the Euler equation governing multi-material flow
    using a finite volume method
  \details   This file defines a Physics policy class for the compressible
    flow equations class dg::MultiMat, defined in PDE/MultiMat/DGMultiMat.h.
    This specific algorithm assumes multi-material flow with a single velocity
    (velocity equilibirum) and uses a finite volume discretization scheme. See
    PDE/MultiMat/Physics/DG.h for general requirements on Physics policy classes
    for dg::MultiMat.
*/
// *****************************************************************************
#ifndef MultiMatPhysicsDGVelEq_h
#define MultiMatPhysicsDGVelEq_h

#include "Types.hpp"
#include "Exception.hpp"
#include "Inciter/Options/Physics.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

namespace dg {

//! MultiMat system of PDEs problem: VelEq (velocity equilibrium)
//! \details This class is a no-op, consistent with no additional physics needed
//!   to make the basic implementation in MultiMat the Euler equations
//!   governing multi-material compressible flow.
class MultiMatPhysicsVelEq {

  public:
    //! Return enum denoting physics policy
    //! \return Enum denoting physics policy.
    static ctr::PhysicsType type() noexcept { return ctr::PhysicsType::VELEQ; }
};

} // dg::

} // inciter::

#endif // CompFlowPhysicsDGMultiMatVelEq_h
