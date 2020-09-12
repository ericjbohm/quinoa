// *****************************************************************************
/*!
  \file      src/DiffEq/Beta/MassFractionBetaCoeffPolicy.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Mass-fraction beta SDE coefficients policies
  \details   This file defines coefficients policy classes for the mass-fraction
             beta SDE, defined in DiffEq/Beta/MassFractionBeta.h. For general
             requirements on mass-fraction beta SDE coefficients policy classes
             see the header file.
*/
// *****************************************************************************

#include "MassFractionBetaCoeffPolicy.hpp"

using walker::MassFractionBetaCoeffConst;

MassFractionBetaCoeffConst::MassFractionBetaCoeffConst(
   tk::ctr::ncomp_t ncomp,
   const std::vector< kw::sde_b::info::expect::type >& b_,
   const std::vector< kw::sde_S::info::expect::type >& S_,
   const std::vector< kw::sde_kappa::info::expect::type >& k_,
   const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
   const std::vector< kw::sde_r::info::expect::type >& r_,
   std::vector< kw::sde_b::info::expect::type  >& b,
   std::vector< kw::sde_S::info::expect::type >& S,
   std::vector< kw::sde_kappa::info::expect::type >& k,
   std::vector< kw::sde_rho2::info::expect::type >& rho2,
   std::vector< kw::sde_r::info::expect::type >& r )
// *****************************************************************************
// Constructor: initialize coefficients
//! \param[in] ncomp Number of scalar components in this SDE system
//! \param[in] b_ Vector used to initialize coefficient vector b
//! \param[in] S_ Vector used to initialize coefficient vector S
//! \param[in] k_ Vector used to initialize coefficient vector k
//! \param[in] rho2_ Vector used to initialize coefficient vector rho2
//! \param[in] r_ Vector used to initialize coefficient vector r
//! \param[in,out] b Coefficient vector to be initialized
//! \param[in,out] S Coefficient vector to be initialized
//! \param[in,out] k Coefficient vector to be initialized
//! \param[in,out] rho2 Coefficient vector to be initialized
//! \param[in,out] r Coefficient vector to be initialized
// *****************************************************************************
{
  ErrChk( b_.size() == ncomp,
          "Wrong number of number-fraction beta SDE parameters 'b'");
  ErrChk( S_.size() == ncomp,
          "Wrong number of number-fraction beta SDE parameters 'S'");
  ErrChk( k_.size() == ncomp,
          "Wrong number of number-fraction beta SDE parameters 'kappa'");
  ErrChk( rho2_.size() == ncomp,
          "Wrong number of number-fraction beta SDE parameters 'rho2'");
  ErrChk( r_.size() == ncomp,
          "Wrong number of number-fraction beta SDE parameters 'r'");

  b = b_;
  S = S_;
  k = k_;
  rho2 = rho2_;
  r = r_;
}
