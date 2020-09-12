// *****************************************************************************
/*!
  \file      src/DiffEq/OrnsteinUhlenbeck/OrnsteinUhlenbeckCoeffPolicy.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Ornstein-Uhlenbeck coefficients policies
  \details   This file defines coefficients policy classes for the
    Ornstein-Uhlenbeck SDE, defined in DiffEq/OrnsteinUhlenbeck.h.

    General requirements on the Ornstein-Uhlenbeck SDE coefficients policy
    classes:

    - Must define a _constructor_, which is used to initialize the SDE
      coefficients, sigmasq, theta, and mu. Required signature:
      \code{.cpp}
        CoeffPolicyName(
          tk::ctr::ncomp_t ncomp,
          const std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq_,
          const std::vector< kw::sde_theta::info::expect::type >& theta_,
          const std::vector< kw::sde_mu::info::expect::type >& mu_,
          std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq,
          std::vector< kw::sde_theta::info::expect::type >& theta,
          std::vector< kw::sde_mu::info::expect::type >& mu )
      \endcode
      where
      - ncomp denotes the number of scalar components of the system of the
        Ornstein-Uhlenbeck SDEs.
      - Constant references to sigmasq_, theta_, and mu_, which denote three
        vectors of real values used to initialize the parameter vectors of the
        system of Ornstein-Uhlenbeck SDEs. The length of the vector sigmasq_
        must be equal to ncomp*(ncomp+1)/2, while the number of components of
        the vectors theta_, and mu_, must be equal to ncomp.
      - References to sigmasq, theta, and mu, which denote the parameter vectors
        to be initialized based on sigmasq_, theta_, and mu_.

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
#ifndef OrnsteinUhlenbeckCoeffPolicy_h
#define OrnsteinUhlenbeckCoeffPolicy_h

#include <brigand/sequences/list.hpp>

#include "Types.hpp"
#include "Walker/Options/CoeffPolicy.hpp"
#include "SystemComponents.hpp"

namespace walker {

//! Ornstein-Uhlenbeck constant coefficients policity: constants in time
class OrnsteinUhlenbeckCoeffConst {

  public:
    //! Constructor: initialize coefficients
    OrnsteinUhlenbeckCoeffConst(
      tk::ctr::ncomp_t ncomp,
      const std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq_,
      const std::vector< kw::sde_theta::info::expect::type >& theta_,
      const std::vector< kw::sde_mu::info::expect::type >& mu_,
      std::vector< kw::sde_sigmasq::info::expect::type >& sigmasq,
      std::vector< kw::sde_theta::info::expect::type >& theta,
      std::vector< kw::sde_mu::info::expect::type >& mu );
  
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::CONST_COEFF; }
};

//! List of all Ornstein-Uhlenbeck's coefficients policies
using OrnsteinUhlenbeckCoeffPolicies =
  brigand::list< OrnsteinUhlenbeckCoeffConst >;

} // walker::

#endif // OrnsteinUhlenbeckCoeffPolicy_h
