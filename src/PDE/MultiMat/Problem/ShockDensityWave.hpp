// *****************************************************************************
/*!
  \file      src/PDE/MultiMat/Problem/ShockDensityWave.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Problem configuration for Shock-density Wave
  \details   This file defines a policy class for the multi-material
    compressible flow equations, defined in PDE/MultiMat/MultiMat.hpp.
    See PDE/MultiMat/Problem.hpp for general requirements on Problem policy
    classes for MultiMat.
*/
// *****************************************************************************
#ifndef MultiMatProblemShockDensityWave_h
#define MultiMatProblemShockDensityWave_h

#include <string>

#include "Types.hpp"
#include "Fields.hpp"
#include "FunctionPrototypes.hpp"
#include "SystemComponents.hpp"
#include "Inciter/Options/Problem.hpp"
#include "EoS/EoS_Base.hpp"

namespace inciter {

//! MultiMat system of PDEs problem: Shock-density Wave
//! \see Y. Lv, M. Ihme. Discontinuous Galerkin method for multicomponent
//!   chemically reacting flows and combustion. J. Comput. Phys., 2014.
//!   And C. Shu, S. Osher. Efficient implementation of essentially
//!   non-oscillatory shock-capturing schemes, II. J. Comput. Phys., 1989.
class MultiMatProblemShockDensityWave {

  protected:
    using ncomp_t = tk::ctr::ncomp_t;
    using eq = tag::multimat;

  public:
    //! Initialize numerical solution
    static tk::InitializeFn::result_type
    initialize( ncomp_t system, ncomp_t ncomp, const std::vector< EoS_Base* >&,
                tk::real x, tk::real, tk::real, tk::real );

    //! Evaluate analytical solution at (x,y,z,t) for all components
    static std::vector< tk::real >
    analyticSolution( ncomp_t system, ncomp_t ncomp, 
                      const std::vector< EoS_Base* >& mat_blk, tk::real x,
                      tk::real y, tk::real z, tk::real t )
    { return initialize( system, ncomp, mat_blk, x, y, z, t ); }

    //! Compute and return source term for this problem
    static tk::SrcFn::result_type
    src( ncomp_t, ncomp_t, const std::vector< EoS_Base* >&, tk::real, tk::real,
         tk::real, tk::real, std::vector< tk::real >& sv )
    {
      for (std::size_t i=0; i<sv.size(); ++i) {
        sv[i] = 0.0;
      }
    }

    //! Return problem type
    static ctr::ProblemType type() noexcept
    { return ctr::ProblemType::SHOCKDENSITY_WAVE; }
};

} // inciter::

#endif // MultiMatProblemShockDensityWave_h
