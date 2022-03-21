// *****************************************************************************
/*!
  \file      src/PDE/CompFlow/Problem/ShockDensityWave.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Problem configuration for shock-density wave problem
  \details   This file defines a policy class for the compressible flow
    equations, defined in PDE/CompFlow/CompFlow.h. See PDE/CompFlow/Problem.h
    for general requirements on Problem policy classes for CompFlow.
*/
// *****************************************************************************
#ifndef CompFlowProblemShockDensityWave_h
#define CompFlowProblemShockDensityWave_h

#include <string>
#include <unordered_set>

#include "Types.hpp"
#include "Fields.hpp"
#include "FunctionPrototypes.hpp"
#include "SystemComponents.hpp"
#include "Inciter/Options/Problem.hpp"

namespace inciter {

//! MultiMat system of PDEs problem: Shock-density Wave
//! \see C. Shu, S. Osher. Efficient implementation of essentially
//!   non-oscillatory shock-capturing schemes, II. J. Comput. Phys., 1989.
class CompFlowProblemShockDensityWave {

  protected:
    using ncomp_t = tk::ctr::ncomp_t;
    using eq = tag::compflow;

  public:
    //! Initialize numerical solution
    static tk::InitializeFn::result_type
    initialize( ncomp_t system, ncomp_t, tk::real x, tk::real, tk::real,
                tk::real );

    //! Evaluate analytical solution at (x,y,z,t) for all components
    static tk::InitializeFn::result_type
    analyticSolution( ncomp_t system, ncomp_t, tk::real x, tk::real,  tk::real,
                      tk::real );

    //! Compute and return source term for this problem
    //! \param[in,out] r Density source
    //! \param[in,out] ru X momentum source
    //! \param[in,out] rv Y momentum source
    //! \param[in,out] rw Z momentum source
    //! \param[in,out] re Specific total energy source
    //! \note The function signature must follow tk::SrcFn
    static tk::CompFlowSrcFn::result_type
    src( ncomp_t, tk::real, tk::real, tk::real, tk::real,
         tk::real& r, tk::real& ru, tk::real& rv, tk::real& rw, tk::real& re )
    { r = ru = rv = rw = re = 0.0; }

    //! Return analytic field names to be output to file
    std::vector< std::string > analyticFieldNames( ncomp_t ) const;

    //! Return names of integral variables to be output to diagnostics file
    std::vector< std::string > names( ncomp_t ) const;

    //! Return problem type
    static ctr::ProblemType type() noexcept
    { return ctr::ProblemType::SHOCKDENSITY_WAVE; }
};

} // inciter::

#endif // CompFlowProblemShockDensityWave_h