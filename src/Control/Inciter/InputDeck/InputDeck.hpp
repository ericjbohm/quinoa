// *****************************************************************************
/*!
  \file      src/Control/Inciter/InputDeck/InputDeck.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Inciter's input deck definition
  \details   This file defines the heterogeneous stack that is used for storing
     the data from user input during the control file parsing of the
     computational shock hydrodynamics tool, Inciter.
*/
// *****************************************************************************
#ifndef InciterInputDeck_h
#define InciterInputDeck_h

#include <limits>
#include <iomanip>
#include <iostream>

#include <brigand/algorithms/for_each.hpp>

#include "NoWarning/set.hpp"

#include "Inciter/CmdLine/CmdLine.hpp"
#include "Inciter/Components.hpp"

namespace inciter {
namespace ctr {

//! Member data for tagged tuple
using InputDeckMembers = brigand::list<
    tag::cmd,        CmdLine
  , tag::title,      kw::title::info::expect::type
  , tag::selected,   selects
  , tag::amr,        amr
  , tag::pref,       pref
  , tag::discr,      discretization
  , tag::prec,       precision
  , tag::flformat,   floatformat
  , tag::component,  ncomps
  , tag::interval,   intervals
  , tag::param,      parameters
  , tag::diag,       diagnostics
  , tag::error,      std::vector< std::string >
  , tag::history,    history
>;

//! \brief InputDeck : Control< specialized to Inciter >, see Types.h,
//! \details The stack is a tagged tuple, a hierarchical heterogeneous data
//!    structure where all parsed information is stored.
//! \see Base/TaggedTuple.h
//! \see Control/Inciter/Types.h
class InputDeck : public tk::TaggedTuple< InputDeckMembers > {

  public:
    //! \brief Inciter input deck keywords
    //! \see tk::grm::use and its documentation
    using keywords = brigand::set< kw::title,
                                   kw::nstep,
                                   kw::term,
                                   kw::t0,
                                   kw::dt,
                                   kw::ttyi,
                                   kw::transport,
                                   kw::end,
                                   kw::shear_diff,
                                   kw::slot_cyl,
                                   kw::problem,
                                   kw::plotvar,
                                   kw::interval,
                                   kw::partitioning,
                                   kw::algorithm,
                                   kw::rcb,
                                   kw::rib,
                                   kw::hsfc,
                                   kw::phg,
                                   kw::inciter,
                                   kw::ncomp,
                                   kw::nmat,
                                   kw::pde_diffusivity,
                                   kw::pde_lambda,
                                   kw::pde_u0,
                                   kw::bc_dirichlet,
                                   kw::sideset,
                                   kw::compflow,
                                   kw::multimat,
                                   kw::ic,
                                   kw::box,
                                   kw::massic,
                                   kw::densityic,
                                   kw::velocityic,
                                   kw::pressureic,
                                   kw::energyic,
                                   kw::energy_content_ic,
                                   kw::temperatureic,
                                   kw::xmin,
                                   kw::xmax,
                                   kw::ymin,
                                   kw::ymax,
                                   kw::zmin,
                                   kw::zmax,
                                   kw::txt_float_format,
                                   kw::txt_float_default,
                                   kw::txt_float_fixed,
                                   kw::txt_float_scientific,
                                   kw::precision,
                                   kw::diagnostics,
                                   kw::history,
                                   kw::material,
                                   kw::id,
                                   kw::mat_gamma,
                                   kw::mat_pstiff,
                                   kw::mat_mu,
                                   kw::mat_cv,
                                   kw::mat_k,
                                   kw::npar,
                                   kw::physics,
                                   kw::advection,
                                   kw::advdiff,
                                   kw::navierstokes,
                                   kw::euler,
                                   kw::veleq,
                                   kw::user_defined,
                                   kw::vortical_flow,
                                   kw::pde_alpha,
                                   kw::pde_beta,
                                   kw::pde_p0,
                                   kw::ctau,
                                   kw::cfl,
                                   kw::mj,
                                   kw::depvar,
                                   kw::nl_energy_growth,
                                   kw::pde_betax,
                                   kw::pde_betay,
                                   kw::pde_betaz,
                                   kw::pde_ce,
                                   kw::pde_kappa,
                                   kw::pde_r0,
                                   kw::rayleigh_taylor,
                                   kw::taylor_green,
                                   kw::filetype,
                                   kw::exodusii,
                                   kw::root,
                                   kw::error,
                                   kw::l2,
                                   kw::linf,
                                   kw::fct,
                                   kw::fctclip,
                                   kw::fcteps,
                                   kw::sysfct,
                                   kw::sysfctvar,
                                   kw::pelocal_reorder,
                                   kw::operator_reorder,
                                   kw::steady_state,
                                   kw::residual,
                                   kw::amr,
                                   kw::amr_t0ref,
                                   kw::amr_dtref,
                                   kw::amr_dtref_uniform,
                                   kw::amr_dtfreq,
                                   kw::amr_initial,
                                   kw::amr_uniform,
                                   kw::amr_uniform_derefine,
                                   kw::amr_initial_conditions,
                                   kw::amr_coords,
                                   kw::amr_error,
                                   kw::amr_jump,
                                   kw::amr_hessian,
                                   kw::amr_refvar,
                                   kw::amr_tolref,
                                   kw::amr_tolderef,
                                   kw::amr_edgelist,
                                   kw::amr_coordref,
                                   kw::amr_xminus,
                                   kw::amr_xplus,
                                   kw::amr_yminus,
                                   kw::amr_yplus,
                                   kw::amr_zminus,
                                   kw::amr_zplus,
                                   kw::pref,
                                   kw::pref_indicator,
                                   kw::pref_spectral_decay,
                                   kw::pref_non_conformity,
                                   kw::pref_ndofmax,
                                   kw::pref_tolref,
                                   kw::scheme,
                                   kw::diagcg,
                                   kw::alecg,
                                   kw::dg,
                                   kw::p0p1,
                                   kw::dgp1,
                                   kw::dgp2,
                                   kw::pdg,
                                   kw::flux,
                                   kw::laxfriedrichs,
                                   kw::hllc,
                                   kw::upwind,
                                   kw::ausm,
                                   kw::hll,
                                   kw::limiter,
                                   kw::cweight,
                                   kw::nolimiter,
                                   kw::wenop1,
                                   kw::superbeep1,
                                   kw::vertexbasedp1,
                                   kw::prelax,
                                   kw::prelax_timescale,
                                   kw::bc_sym,
                                   kw::bc_inlet,
                                   kw::bc_outlet,
                                   kw::bc_characteristic,
                                   kw::bc_extrapolate,
                                   kw::bc_stag,
                                   kw::point,
                                   kw::radius,
                                   kw::farfield_pressure,
                                   kw::farfield_density,
                                   kw::farfield_velocity,
                                   kw::gauss_hump,
                                   kw::rotated_sod_shocktube,
                                   kw::cyl_advect,
                                   kw::shedding_flow,
                                   kw::sod_shocktube,
                                   kw::sedov_blastwave,
                                   kw::interface_advection,
                                   kw::gauss_hump_compflow,
                                   kw::waterair_shocktube,
                                   kw::triple_point,
                                   kw::gas_impact,
                                   kw::shock_hebubble >;

    //! Set of tags to ignore when printing this InputDeck
    using ignore = CmdLine::ignore;

    //! \brief Constructor: set defaults
    //! \param[in] cl Previously parsed and store command line
    //! \details Anything not set here is initialized by the compiler using the
    //!   default constructor for the corresponding type.
    explicit InputDeck( const CmdLine& cl = {} ) {
      // Set previously parsed command line
      get< tag::cmd >() = cl;
      // Default discretization parameters
      get< tag::discr, tag::nstep >() =
         std::numeric_limits< kw::nstep::info::expect::type >::max();
      get< tag::discr, tag::term >() =
         std::numeric_limits< kw::term::info::expect::type >::max();
      get< tag::discr, tag::t0 >() = 0.0;
      get< tag::discr, tag::dt >() = 0.0;
      get< tag::discr, tag::cfl >() = 0.0;
      get< tag::discr, tag::fct >() = true;
      get< tag::discr, tag::fctclip >() = false;
      get< tag::discr, tag::ctau >() = 1.0;
      get< tag::discr, tag::fcteps >() =
        std::numeric_limits< tk::real >::epsilon();
      get< tag::discr, tag::pelocal_reorder >() = false;
      get< tag::discr, tag::operator_reorder >() = false;
      get< tag::discr, tag::steady_state >() = false;
      get< tag::discr, tag::residual >() = 1.0e-8;
      get< tag::discr, tag::scheme >() = SchemeType::DiagCG;
      get< tag::discr, tag::ndof >() = 1;
      get< tag::discr, tag::limiter >() = LimiterType::NOLIMITER;
      get< tag::discr, tag::cweight >() = 1.0;
      get< tag::discr, tag::ndof >() = 1;
      get< tag::discr, tag::rdof >() = 1;
      // Default field output file type
      get< tag::selected, tag::filetype >() = tk::ctr::FieldFileType::EXODUSII;
      // Default AMR settings
      get< tag::amr, tag::amr >() = false;
      get< tag::amr, tag::t0ref >() = false;
      get< tag::amr, tag::dtref >() = false;
      get< tag::amr, tag::dtref_uniform >() = false;
      get< tag::amr, tag::dtfreq >() = 3;
      get< tag::amr, tag::error >() = AMRErrorType::JUMP;
      get< tag::amr, tag::tolref >() = 0.2;
      get< tag::amr, tag::tolderef >() = 0.05;
      auto rmax =
        std::numeric_limits< kw::amr_xminus::info::expect::type >::max() / 100;
      get< tag::amr, tag::xminus >() = rmax;
      get< tag::amr, tag::xplus >() = -rmax;
      get< tag::amr, tag::yminus >() = rmax;
      get< tag::amr, tag::yplus >() = -rmax;
      get< tag::amr, tag::zminus >() = rmax;
      get< tag::amr, tag::zplus >() = -rmax;
      // Default p-refinement settings
      get< tag::pref, tag::pref >() = false;
      get< tag::pref, tag::indicator >() = PrefIndicatorType::SPECTRAL_DECAY;
      get< tag::pref, tag::ndofmax >() = 10;
      get< tag::pref, tag::tolref >() = 0.5;
      // Default txt floating-point output precision in digits
      get< tag::prec, tag::diag >() = std::cout.precision();
      get< tag::prec, tag::history >() = std::cout.precision();
      // Default intervals
      get< tag::interval, tag::tty >() = 1;
      get< tag::interval, tag::field >() = 1;
      get< tag::interval, tag::diag >() = 1;
      get< tag::interval, tag::history >() = 1;
      // Initialize help: fill own keywords
      const auto& ctrinfoFill = tk::ctr::Info( get< tag::cmd, tag::ctrinfo >() );
      brigand::for_each< keywords >( ctrinfoFill );
    }

    /** @name Pack/Unpack: Serialize InputDeck object for Charm++ */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er& p ) { tk::TaggedTuple< InputDeckMembers >::pup(p); }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i InputDeck object reference
    friend void operator|( PUP::er& p, InputDeck& i ) { i.pup(p); }
    //@}

    //! Extract surface side set ids along which user wants to save solution
    //! \return Unique set of surface side set ids along which user wants to
    //!   save solution field variables
    //! \note This returns an ordered set so the order of the set ids are
    //!   always the same.
    std::set< int > outsets() const {
      std::set< int > ids;
      for (const auto& s : get< tag::cmd, tag::io, tag::surface >()) {
        std::stringstream conv( s );
        int num;
        conv >> num;
        ids.insert( num );
      }
      return ids;
    }

    //! Query stagnation point BC configuration
    //! \tparam eq PDE type to query
    //! \param[in] system Equation system id
    //! \return Vectors configuring the stagnation points and their radii
    template< class eq >
    std::tuple< std::vector< tk::real >, std::vector< tk::real > >
    stagnationBC( std::size_t system ) {
      const auto& bcstag = get< tag::param, eq, tag::bcstag >();
      const auto& point = bcstag.template get< tag::point >();
      const auto& radius = bcstag.template get< tag::radius >();
      std::vector< tk::real > stag_pnt;
      std::vector< tk::real > stag_rad;
      if (point.size() > system && radius.size() > system) {
        stag_pnt = point[ system ];
        stag_rad = radius[ system ];
      }
      Assert( stag_pnt.size() == 3*stag_rad.size(), "Size mismatch" );
      return { std::move(stag_pnt), std::move(stag_rad) };
    }
};

} // ctr::
} // inciter::

#endif // InciterInputDeck_h
