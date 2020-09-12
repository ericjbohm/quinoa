// *****************************************************************************
/*!
  \file      src/DiffEq/Dirichlet/DirichletCoeffPolicy.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Dirichlet coefficients policies
  \details   This file defines coefficients policy classes for the Dirichlet
             SDE, defined in DiffEq/Dirichlet/Dirichlet.h. For general
             requirements on Dirichlet SDE coefficients policy classes see the
             header file.
*/
// *****************************************************************************

#include "DirichletCoeffPolicy.hpp"

using walker::DirichletCoeffConst;

DirichletCoeffConst::DirichletCoeffConst(
  tk::ctr::ncomp_t ncomp,
  const std::vector< kw::sde_b::info::expect::type >& b_,
  const std::vector< kw::sde_S::info::expect::type >& S_,
  const std::vector< kw::sde_kappa::info::expect::type >& k_,
  std::vector< kw::sde_b::info::expect::type  >& b,
  std::vector< kw::sde_S::info::expect::type >& S,
  std::vector< kw::sde_kappa::info::expect::type >& k )
// *****************************************************************************
// Constructor: initialize coefficients
//! \param[in] ncomp Number of scalar components in this SDE system
//! \param[in] b_ Vector used to initialize coefficient vector b
//! \param[in] S_ Vector used to initialize coefficient vector S
//! \param[in] k_ Vector used to initialize coefficient vector k
//! \param[in,out] b Coefficient vector to be initialized
//! \param[in,out] S Coefficient vector to be initialized
//! \param[in,out] k Coefficient vector to be initialized
// *****************************************************************************
{
  ErrChk( b_.size() == ncomp,
          "Wrong number of Dirichlet SDE parameters 'b'");
  ErrChk( S_.size() == ncomp,
          "Wrong number of Dirichlet SDE parameters 'S'");
  ErrChk( k_.size() == ncomp,
          "Wrong number of Dirichlet SDE parameters 'kappa'");

  b = b_;
  S = S_;
  k = k_;
}
