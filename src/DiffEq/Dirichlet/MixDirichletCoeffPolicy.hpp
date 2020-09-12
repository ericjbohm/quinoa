// *****************************************************************************
/*!
  \file      src/DiffEq/Dirichlet/MixDirichletCoeffPolicy.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     MixDirichlet coefficients policies
  \details   This file defines coefficients policy classes for the MixDirichlet
    SDE, defined in DiffEq/MixDirichlet.h.

    General requirements on Dirichlet SDE coefficients policy classes:

    - Must define a _constructor_, which is used to initialize the SDE
      coefficients, b, S, and kappa. Required signature:
      \code{.cpp}
        CoeffPolicyName(
          tk::ctr::ncomp_t ncomp,
          ctr::NormalizationType norm,
          const std::vector< kw::sde_b::info::expect::type >& b_,
          const std::vector< kw::sde_S::info::expect::type >& S_,
          const std::vector< kw::sde_kappa::info::expect::type >& kprime_,
          const std::vector< kw::sde_r::info::expect::type >& rho_,
          std::vector< kw::sde_b::info::expect::type  >& b,
          std::vector< kw::sde_kappa::info::expect::type >& kprime,
          std::vector< kw::sde_S::info::expect::type >& S,
          std::vector< kw::sde_rho::info::expect::type >& rho,
          std::vector< kw::sde_r::info::expect::type >& r,
          std::vector< kw::sde_kappa::info::expect::type >& k );
      \endcode
      where
      - _ncomp_ denotes the number of scalar components of the system of
        MixDirichlet SDEs.
      - _norm_ selects the type of normalization used (heavy or light).
      - Constant references to b_, S_, kprime_, rho_, which denote vectors
        of real values used to initialize the parameter vectors of the
        MixDirichlet SDEs. The length of the vectors must be equal to the number
        of components given by ncomp.
      - References to b, kprime, S, rho, r, k, which denote the parameter
        vectors to be initialized based on b_, S_, kprime_, rho_.

    - Must define the static function _type()_, returning the enum value of the
      policy option. Example:
      \code{.cpp}
        static ctr::CoeffPolicyType type() noexcept {
          return ctr::CoeffPolicyType::CONST_COEFF;
        }
      \endcode
      which returns the enum value of the option from the underlying option
      class, collecting all possible options for coefficients policies.

    - Must define the function _update()_, called from
      MixDirichlet::advance(), updating the model coefficients.
      Required signature:
      \code{.cpp}
        void update(
               char depvar,
               ncomp_t ncomp,
               ctr::NormalizationType norm,
               std::size_t density_offset,
               std::size_t volume_offset,
               const std::map< tk::ctr::Product, tk::real >& moments,
               const std::vector< kw::sde_rho::info::expect::type >& rho,
               const std::vector< kw::sde_r::info::expect::type >& r,
               const std::vector< kw::sde_kappa::info::expect::type >& kprime,
               const std::vector< kw::sde_b::info::expect::type >& b,
               std::vector< kw::sde_kappa::info::expect::type >& k,
               std::vector< kw::sde_kappa::info::expect::type >& S ) const {}
      \endcode
      where _depvar_ is the dependent variable associated with the mix
      Dirichlet SDE, specified in the control file by the user, _ncomp_
      is the number of components in the system, _norm_ selects the type of
      normalization used (heavy or light). _density_offset_ is the offset
      of the particle density in the solution array relative to the Nth scalar,
      _volume_offset_ is the offset of the particle specific volume in the
      solution array relative to the Nth scalar, _moments_ is the map
      associating moment IDs (tk::ctr::vector< tk::ctr::Term >) to values of
      statistical moments, _rho_, _r_, _kprime_, _b_ are user-defined
      parameters, and _k_ and _S_ are the SDE parameters computed/updated, see
      also DiffEq/DiffEq/MixDirichlet.h.
*/
// *****************************************************************************
#ifndef MixDirichletCoeffPolicy_h
#define MixDirichletCoeffPolicy_h

#include <brigand/sequences/list.hpp>

#include "Types.hpp"
#include "Walker/Options/CoeffPolicy.hpp"
#include "Walker/Options/Normalization.hpp"
#include "SystemComponents.hpp"

namespace walker {

//! \brief MixDirichlet constant coefficients policity: constants in time
class MixDirichletCoeffConst {

  private:
    using ncomp_t = tk::ctr::ncomp_t;

  public:
    //! Constructor: initialize coefficients
    MixDirichletCoeffConst(
      ncomp_t ncomp,
      ctr::NormalizationType norm,
      const std::vector< kw::sde_b::info::expect::type >& b_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho::info::expect::type >& rho_,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& kprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_rho::info::expect::type >& rho,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_kappa::info::expect::type >& k );

    //! Update coefficients
    void update(
      char depvar,
      ncomp_t ncomp,
      ctr::NormalizationType norm,
      std::size_t density_offset,
      std::size_t volume_offset,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_rho::info::expect::type >& rho,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime,
      const std::vector< kw::sde_b::info::expect::type >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_kappa::info::expect::type >& S ) const;

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::CONST_COEFF; }
};

//! Compute parameter vector r based on r_i = rho_N/rho_i - 1
std::vector< kw::sde_r::info::expect::type >
MixDir_r( const std::vector< kw::sde_rho::info::expect::type >& rho,
          ctr::NormalizationType norm );

//! MixDirichlet coefficients policity: mean(rho) forced const in time
//! \details User-defined parameter vector S is constrained to make
//!   \f$\mathrm{d}<rho>/\mathrm{d}t = 0\f$.
class MixDirichletHomogeneous {

  private:
    using ncomp_t = tk::ctr::ncomp_t;

  public:
    //! Constructor: initialize coefficients
    MixDirichletHomogeneous(
      ncomp_t ncomp,
      ctr::NormalizationType norm,
      const std::vector< kw::sde_b::info::expect::type >& b_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho::info::expect::type >& rho_,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& kprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_rho::info::expect::type >& rho,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_kappa::info::expect::type >& k );

    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::HOMOGENEOUS; }

    //! Update coefficients
    void update(
      char depvar,
      ncomp_t ncomp,
      ctr::NormalizationType norm,
      std::size_t density_offset,
      std::size_t volume_offset,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_rho::info::expect::type >& rho,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime,
      const std::vector< kw::sde_b::info::expect::type >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_kappa::info::expect::type >& S ) const;
};

//! MixDirichlet coefficients policity: mean(rho) forced const in time
//! \details User-defined parameters b' and kappa' are functions of an
//!   externally, e.g., DNS-, provided hydrodynamics time scale ensuring decay
//!   in the evolution of \<y_alpha^2\>. Additionally, S_alpha is constrained to
//!   make d\<rho\>/dt = 0. Additionally, we pull in a hydrodynamic timescale
//!   from an external function.
//! \see kw::hydrotimescale_info
class MixDirichletHydroTimeScale {

  private:
    using ncomp_t = tk::ctr::ncomp_t;

  public:
    //! Constructor: initialize coefficients
    MixDirichletHydroTimeScale(
      tk::ctr::ncomp_t ncomp,
      ctr::NormalizationType norm,
      const std::vector< kw::sde_b::info::expect::type >& b_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho::info::expect::type >& rho_,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& kprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_rho::info::expect::type >& rho,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_kappa::info::expect::type >& k );

    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::HYDROTIMESCALE; }

    //! Update coefficients
    void update(
      char depvar,
      ncomp_t ncomp,
      ctr::NormalizationType norm,
      std::size_t density_offset,
      std::size_t volume_offset,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_rho::info::expect::type >& rho,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< kw::sde_kappa::info::expect::type >& kprime,
      const std::vector< kw::sde_b::info::expect::type >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_kappa::info::expect::type >& S ) const;
};

//! List of all MixDirichlet's coefficients policies
using MixDirichletCoeffPolicies = brigand::list< MixDirichletCoeffConst
                                               , MixDirichletHomogeneous
                                               , MixDirichletHydroTimeScale >;

} // walker::

#endif // MixDirichletCoeffPolicy_h
