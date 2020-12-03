// *****************************************************************************
/*!
  \file      src/DiffEq/SkewNormal/SkewNormalCoeffPolicy.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Skew-normal coefficients policies
  \details   This file defines coefficients policy classes for the diagonal
    skew-normal SDE, defined in DiffEq/SkewNormal.h.

    General requirements on the skew-normal SDE coefficients policy classes:

    - Must define a _constructor_, which is used to initialize the SDE
      coefficients, timescale, sigmasq, and lambda. Required signature:
      \code{.cpp}
        CoeffPolicyName(
          tk::ctr::ncomp_t ncomp,
          const std::vector< kw::sde_T::info::expect::type >& timescale_,
          const std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq_,
          const std::vector< kw::sde_lambda::info::expect::type >& lambda_,
          std::vector< kw::sde_T::info::expect::type >& timescale,
          std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq,
          std::vector< kw::sde_lambda::info::expect::type >& lambda )
      \endcode
      where
      - ncomp denotes the number of scalar components of the system of the
        skew-normal SDEs.
      - Constant references to timescale_, sigmasq_, and lambda_, which denote
        three vectors of real values used to initialize the parameter vectors of
        the system of skew-normal SDEs. The length of the vectors must be equal
        to the number of components given by ncomp.
      - References to timescale, sigmasq, and lambda, which denote the parameter
        vectors to be initialized based on timescale_, sigmasq_, and lambda_.

    - Must define the static function _type()_, returning the enum value of the
      policy option. Example:
      \code{.cpp}
        static ctr::CoeffPolicyType type() noexcept {
          return ctr::CoeffPolicyType::CONST_COEFF;
        }
      \endcode
      which returns the enum value of the option from the underlying option
      class, collecting all possible options for coefficients policies.
*/
// *****************************************************************************
#ifndef SkewNormalCoeffPolicy_h
#define SkewNormalCoeffPolicy_h

#include <brigand/sequences/list.hpp>

#include "Types.hpp"
#include "Walker/Options/CoeffPolicy.hpp"
#include "SystemComponents.hpp"

namespace walker {

//! Skew-normal SDE constant coefficients policity: constants in time
class SkewNormalCoeffConst {

  public:
    //! Constructor: initialize coefficients
    SkewNormalCoeffConst(
      tk::ctr::ncomp_t ncomp,
      const std::vector< kw::sde_T::info::expect::type >& timescale_,
      const std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq_,
      const std::vector< kw::sde_lambda::info::expect::type >& lambda_,
      std::vector< kw::sde_T::info::expect::type >& timescale,
      std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq,
      std::vector< kw::sde_lambda::info::expect::type >& lambda );

    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::CONST_COEFF; }
};

//! List of all Skew-normal SDE's coefficients policies
using SkewNormalCoeffPolicies = brigand::list< SkewNormalCoeffConst >;

} // walker::

#endif // SkewNormalCoeffPolicy_h