// *****************************************************************************
/*!
  \file      src/Control/Inciter/InputDeck/Grammar.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Inciter's input deck grammar definition
  \details   Inciter's input deck grammar definition. We use the Parsing
  Expression Grammar Template Library (PEGTL) to create the grammar and the
  associated parser. Word of advice: read from the bottom up.
*/
// *****************************************************************************
#ifndef InciterInputDeckGrammar_h
#define InciterInputDeckGrammar_h

#include <limits>
#include <cmath>

#include "CommonGrammar.hpp"
#include "CartesianProduct.hpp"
#include "Keywords.hpp"
#include "ContainerUtil.hpp"
#include "Centering.hpp"
#include "PDE/MultiMat/MultiMatIndexing.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck_defaults;

//! Inciter input deck facilitating user input for computing shock hydrodynamics
namespace deck {

  //! \brief Specialization of tk::grm::use for Inciter's input deck parser
  template< typename keyword >
  using use = tk::grm::use< keyword, ctr::InputDeck::keywords >;

  // Inciter's InputDeck state

  //! \brief Number of registered equations
  //! \details Counts the number of parsed equation blocks during parsing.
  static tk::TaggedTuple< brigand::list<
             tag::transport,   std::size_t
           , tag::compflow,    std::size_t
           , tag::multimat,    std::size_t
         > > neq;

  //! \brief Parser-lifetime storage for point names
  //! \details Used to track the point names registered so that parsing new ones
  //!    can be required to be unique.
  static std::set< std::string > pointnames;

  //! Parser-lifetime storage of elem or node centering
  static tk::Centering centering = tk::Centering::NODE;

  //! Accepted multimat output variable labels and associated index functions
  //! \details The keys are a list of characters accepted as labels for
  //! denoting (matvar-style) output variables used for multi-material variable
  //! output. We use a case- insesitive comparitor, since when this set is used
  //! we only care about whether the variable is selected or not and not whether
  //! it denotes a full variable (upper case) or a fluctuation (lower case).
  //! This is true when matching these labels for output variables with
  //! instantaenous variables as well terms of products in parsing requested
  //! statistics (for turbulence). The values are associated indexing functions
  //! used to index into the state of the multimaterial system, all must follow
  //! the same signature.
  static std::map< char, tk::MultiMatIdxFn,
                   tk::ctr::CaseInsensitiveCharLess >
    multimatvars{
      { 'd', densityIdx }       // density
    , { 'f', volfracIdx }       // volume fraction
    , { 'm', momentumIdx }      // momentum
    , { 'e', energyIdx }        // specific total energy
    , { 'u', velocityIdx }      // velocity (primitive)
    , { 'p', pressureIdx }      // material pressure (primitive)
  };

} // ::deck
} // ::inciter

namespace tk {
namespace grm {

  using namespace tao;

  // Note that PEGTL action specializations must be in the same namespace as the
  // template being specialized. See http://stackoverflow.com/a/3052604.

  // Inciter's InputDeck actions

  //! Rule used to trigger action
  template< class eq > struct register_inciter_eq : pegtl::success {};
  //! \brief Register differential equation after parsing its block
  //! \details This is used by the error checking functors (check_*) during
  //!    parsing to identify the recently-parsed block.
  template< class eq >
  struct action< register_inciter_eq< eq > > {
    template< typename Input, typename Stack >
    static void apply( const Input&, Stack& ) {
      using inciter::deck::neq;
      ++neq.get< eq >();
    }
  };

  //! Rule used to trigger action
  template< class eq > struct check_transport : pegtl::success {};
  //! \brief Set defaults and do error checking on the transport equation block
  //! \details This is error checking that only the transport equation block
  //!   must satisfy. Besides error checking we also set defaults here as
  //!   this block is called when parsing of a transport...end block has
  //!   just finished.
  template< class eq >
  struct action< check_transport< eq > > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::neq;
      using tag::param;

      // Error out if no dependent variable has been selected
      auto& depvar = stack.template get< param, eq, tag::depvar >();
      if (depvar.empty() || depvar.size() != neq.get< eq >())
        Message< Stack, ERROR, MsgKey::NODEPVAR >( stack, in );

      // If no number of components has been selected, default to 1
      auto& ncomp = stack.template get< tag::component, eq >();
      if (ncomp.empty() || ncomp.size() != neq.get< eq >())
        ncomp.push_back( 1 );

      // If physics type is not given, default to 'advection'
      auto& physics = stack.template get< param, eq, tag::physics >();
      if (physics.empty() || physics.size() != neq.get< eq >())
        physics.push_back( inciter::ctr::PhysicsType::ADVECTION );

      // If physics type is advection-diffusion, check for correct number of
      // advection velocity, shear, and diffusion coefficients
      if (physics.back() == inciter::ctr::PhysicsType::ADVDIFF) {
        auto& u0 = stack.template get< param, eq, tag::u0 >();
        if (u0.back().size() != ncomp.back())  // must define 1 component
          Message< Stack, ERROR, MsgKey::WRONGSIZE >( stack, in );
        auto& diff = stack.template get< param, eq, tag::diffusivity >();
        if (diff.back().size() != 3*ncomp.back())  // must define 3 components
          Message< Stack, ERROR, MsgKey::WRONGSIZE >( stack, in );
        auto& lambda = stack.template get< param, eq, tag::lambda >();
        if (lambda.back().size() != 2*ncomp.back()) // must define 2 shear comps
          Message< Stack, ERROR, MsgKey::WRONGSIZE >( stack, in );
      }
      // If problem type is not given, error out
      auto& problem = stack.template get< param, eq, tag::problem >();
      if (problem.empty() || problem.size() != neq.get< eq >())
        Message< Stack, ERROR, MsgKey::NOPROBLEM >( stack, in );
      // Error check Dirichlet boundary condition block for all transport eq
      // configurations
      const auto& bc = stack.template get< param, eq, tag::bc, tag::bcdir >();
      for (const auto& s : bc)
        if (s.empty()) Message< Stack, ERROR, MsgKey::BC_EMPTY >( stack, in );

      // If interface compression is not specified, default to 'false'
      auto& intsharp = stack.template get< param, eq, tag::intsharp >();
      if (intsharp.empty() || intsharp.size() != neq.get< eq >())
        intsharp.push_back( 0 );

      // If interface compression parameter is not specified, default to 1.0
      auto& intsharp_p = stack.template get< param, eq,
                                            tag::intsharp_param >();
      if (intsharp_p.empty() || intsharp_p.size() != neq.get< eq >())
        intsharp_p.push_back( 1.0 );
    }
  };

  //! Rule used to trigger action
  template< class eq > struct check_compflow : pegtl::success {};
  //! \brief Set defaults and do error checking on the compressible flow
  //!   equation block
  //! \details This is error checking that only the compressible flow equation
  //!   block must satisfy. Besides error checking we also set defaults here as
  //!   this block is called when parsing of a compflow...end block has
  //!   just finished.
  template< class eq >
  struct action< check_compflow< eq > > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::neq;
      using tag::param;

      // Error out if no dependent variable has been selected
      auto& depvar = stack.template get< param, eq, tag::depvar >();
      if (depvar.empty() || depvar.size() != neq.get< eq >())
        Message< Stack, ERROR, MsgKey::NODEPVAR >( stack, in );

      // If physics type is not given, default to 'euler'
      auto& physics = stack.template get< param, eq, tag::physics >();
      if (physics.empty() || physics.size() != neq.get< eq >()) {
        physics.push_back( inciter::ctr::PhysicsType::EULER );
      }

      // Set number of components to 5 (mass, 3 x mom, energy)
      stack.template get< tag::component, eq >().push_back( 5 );

      // Set default to sysfct (on/off) if not specified
      auto& sysfct = stack.template get< param, eq, tag::sysfct >();
      if (sysfct.empty() || sysfct.size() != neq.get< eq >())
        sysfct.push_back( 1 );

      // Set default flux to HLLC if not specified
      auto& flux = stack.template get< tag::param, eq, tag::flux >();
      if (flux.empty() || flux.size() != neq.get< eq >())
        flux.push_back( inciter::ctr::FluxType::HLLC );

      // Verify that sysfctvar variables are within bounds (if specified) and
      // defaults if not
      auto& sysfctvar = stack.template get< param, eq, tag::sysfctvar >();
      // If sysfctvar is not specified, use all variables for system FCT
      if (sysfctvar.empty() || sysfctvar.back().empty()) {
        sysfctvar.push_back( {0,1,2,3,4} );
      } else {  // if specified, do error checking on variables
        auto& vars = sysfctvar.back();
        if (vars.size() > 5) {
          Message< Stack, ERROR, MsgKey::SYSFCTVAR >( stack, in );
        }
        for (const auto& i : vars) {
          if (i > 4) Message< Stack, ERROR, MsgKey::SYSFCTVAR >( stack, in );
        }
      }

      // Verify correct number of material properties configured
      const auto& gamma = stack.template get< param, eq, tag::gamma >();
      if (gamma.empty() || gamma.back().size() != 1)
        Message< Stack, ERROR, MsgKey::EOSGAMMA >( stack, in );

      // If specific heat is not given, set defaults
      using cv_t = kw::mat_cv::info::expect::type;
      auto& cv = stack.template get< param, eq, tag::cv >();
      // As a default, the specific heat of air (717.5 J/Kg-K) is used
      if (cv.empty() || cv.size() != neq.get< eq >())
        cv.push_back( std::vector< cv_t >( 1, 717.5 ) );
      // If specific heat vector is wrong size, error out
      if (cv.back().size() != 1)
        Message< Stack, ERROR, MsgKey::EOSCV >( stack, in );

      // If stiffness coefficient is not given, set defaults
      using pstiff_t = kw::mat_pstiff::info::expect::type;
      auto& pstiff = stack.template get< param, eq, tag::pstiff >();
      if (pstiff.empty() || pstiff.size() != neq.get< eq >())
        pstiff.push_back( std::vector< pstiff_t >( 1, 0.0 ) );
      // If stiffness coefficient vector is wrong size, error out
      if (pstiff.back().size() != 1)
        Message< Stack, ERROR, MsgKey::EOSPSTIFF >( stack, in );

      // If problem type is not given, default to 'user_defined'
      auto& problem = stack.template get< param, eq, tag::problem >();
      if (problem.empty() || problem.size() != neq.get< eq >())
        problem.push_back( inciter::ctr::ProblemType::USER_DEFINED );
      else if (problem.back() == inciter::ctr::ProblemType::VORTICAL_FLOW) {
        const auto& alpha = stack.template get< param, eq, tag::alpha >();
        const auto& beta = stack.template get< param, eq, tag::beta >();
        const auto& p0 = stack.template get< param, eq, tag::p0 >();
        if ( alpha.size() != problem.size() ||
             beta.size() != problem.size() ||
             p0.size() != problem.size() )
          Message< Stack, ERROR, MsgKey::VORTICAL_UNFINISHED >( stack, in );
      }
      else if (problem.back() == inciter::ctr::ProblemType::NL_ENERGY_GROWTH) {
        const auto& alpha = stack.template get< param, eq, tag::alpha >();
        const auto& betax = stack.template get< param, eq, tag::betax >();
        const auto& betay = stack.template get< param, eq, tag::betay >();
        const auto& betaz = stack.template get< param, eq, tag::betaz >();
        const auto& kappa = stack.template get< param, eq, tag::kappa >();
        const auto& r0 = stack.template get< param, eq, tag::r0 >();
        const auto& ce = stack.template get< param, eq, tag::ce >();
        if ( alpha.size() != problem.size() ||
             betax.size() != problem.size() ||
             betay.size() != problem.size() ||
             betaz.size() != problem.size() ||
             kappa.size() != problem.size() ||
             r0.size() != problem.size() ||
             ce.size() != problem.size() )
          Message< Stack, ERROR, MsgKey::ENERGY_UNFINISHED >( stack, in);
      }
      else if (problem.back() == inciter::ctr::ProblemType::RAYLEIGH_TAYLOR) {
        const auto& alpha = stack.template get< param, eq, tag::alpha >();
        const auto& betax = stack.template get< param, eq, tag::betax >();
        const auto& betay = stack.template get< param, eq, tag::betay >();
        const auto& betaz = stack.template get< param, eq, tag::betaz >();
        const auto& kappa = stack.template get< param, eq, tag::kappa >();
        const auto& p0 = stack.template get< param, eq, tag::p0 >();
        const auto& r0 = stack.template get< param, eq, tag::r0 >();
        if ( alpha.size() != problem.size() ||
             betax.size() != problem.size() ||
             betay.size() != problem.size() ||
             betaz.size() != problem.size() ||
             kappa.size() != problem.size() ||
             p0.size() != problem.size() ||
             r0.size() != problem.size() )
          Message< Stack, ERROR, MsgKey::RT_UNFINISHED >( stack, in);
      }

      // Error check on user-defined problem type
      auto& ic = stack.template get< param, eq, tag::ic >();
      auto& bgdensityic = ic.template get< tag::density >();
      auto& bgvelocityic = ic.template get< tag::velocity >();
      auto& bgpressureic = ic.template get< tag::pressure >();
      auto& bgenergyic = ic.template get< tag::energy >();
      auto& bgtemperatureic = ic.template get< tag::temperature >();
      if (problem.back() == inciter::ctr::ProblemType::USER_DEFINED) {
        // must have defined background ICs for user-defined ICs
        auto n = neq.get< eq >();
        if ( bgdensityic.size() != n || bgvelocityic.size() != n ||
             ( bgpressureic.size() != n && bgenergyic.size() != n &&
               bgtemperatureic.size() != n ) )
        {
          Message< Stack, ERROR, MsgKey::BGICMISSING >( stack, in );
        }

        // put in empty vectors for background ICs so client code can directly
        // index into these vectors using the eq system id
        bgdensityic.push_back( {} );
        bgvelocityic.push_back( {} );
        bgpressureic.push_back( {} );
        bgenergyic.push_back( {} );
        bgtemperatureic.push_back( {} );

        // Error check Dirichlet boundary condition block for all compflow
        // configurations
        const auto& bc = stack.template get< param, eq, tag::bc, tag::bcdir >();
        for (const auto& s : bc)
          if (s.empty()) Message< Stack, ERROR, MsgKey::BC_EMPTY >( stack, in );

        // Error check stagnation BC block
        const auto& bcstag = stack.template get<tag::param, eq, tag::bcstag>();
        const auto& spoint = bcstag.template get< tag::point >();
        const auto& sradius = bcstag.template get< tag::radius >();
        if ( (!spoint.empty() && !spoint.back().empty() &&
              !sradius.empty() && !sradius.back().empty() &&
              spoint.back().size() != 3*sradius.back().size())
         || (!sradius.empty() && !sradius.back().empty() &&
              !spoint.empty() && !spoint.back().empty() &&
              spoint.back().size() != 3*sradius.back().size())
         || (!spoint.empty() && !spoint.back().empty() &&
              (sradius.empty() || (!sradius.empty() && sradius.back().empty())))
         || (!sradius.empty() && !sradius.back().empty() &&
              (spoint.empty() || (!spoint.empty() && spoint.back().empty()))) )
        {
          Message< Stack, ERROR, MsgKey::STAGBCWRONG >( stack, in );
        }

        // Error check skip BC block
        const auto& bcskip = stack.template get<tag::param, eq, tag::bcskip>();
        const auto& kpoint = bcskip.template get< tag::point >();
        const auto& kradius = bcskip.template get< tag::radius >();
        if ( (!kpoint.empty() && !kpoint.back().empty() &&
              !kradius.empty() && !kradius.back().empty() &&
              kpoint.back().size() != 3*kradius.back().size())
          || (!kradius.empty() && !kradius.back().empty() &&
              !kpoint.empty() && !kpoint.back().empty() &&
              kpoint.back().size() != 3*kradius.back().size())
          || (!kpoint.empty() && !kpoint.back().empty() &&
              (kradius.empty() || (!kradius.empty() && kradius.back().empty())))
          || (!kradius.empty() && !kradius.back().empty() &&
              (kpoint.empty() || (!kpoint.empty() && kpoint.back().empty()))) )
        {
          Message< Stack, ERROR, MsgKey::SKIPBCWRONG >( stack, in );
        }

        // Set default inititate type for box ICs
        auto& icbox = ic.template get< tag::box >();
        auto& initiate = icbox.template get< tag::initiate >();
        auto& inittype = initiate.template get< tag::init >();
        if (inittype.size() != neq.get< eq >())
          inittype.push_back( inciter::ctr::InitiateType::IMPULSE );

        // put in empty vectors for non-user-defined box ICs so client code can
        // directly index into these vectors using the eq system id
        icbox.template get< tag::density >().push_back( {} );
        icbox.template get< tag::velocity >().push_back( {} );
        icbox.template get< tag::pressure >().push_back( {} );
        icbox.template get< tag::energy >().push_back( {} );
        icbox.template get< tag::temperature >().push_back( {} );
      }
    }
  };

  //! Rule used to trigger action
  template< class eq > struct check_multimat : pegtl::success {};
  //! \brief Set defaults and do error checking on the multimaterial
  //!    compressible flow equation block
  //! \details This is error checking that only the multimaterial compressible
  //!   flow equation block must satisfy. Besides error checking we also set
  //!   defaults here as this block is called when parsing of a
  //!   multimat...end block has just finished.
  template< class eq >
  struct action< check_multimat< eq > > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::neq;
      using tag::param;

      // Error out if no dependent variable has been selected
      auto& depvar = stack.template get< param, eq, tag::depvar >();
      if (depvar.empty() || depvar.size() != neq.get< eq >())
        Message< Stack, ERROR, MsgKey::NODEPVAR >( stack, in );

      // If physics type is not given, default to 'veleq'
      auto& physics = stack.template get< param, eq, tag::physics >();
      if (physics.empty() || physics.size() != neq.get< eq >())
        physics.push_back( inciter::ctr::PhysicsType::VELEQ );

      // Set default flux to AUSM if not specified
      auto& flux = stack.template get< tag::param, eq, tag::flux >();
      if (flux.empty() || flux.size() != neq.get< eq >())
        flux.push_back( inciter::ctr::FluxType::AUSM );

      // Set number of scalar components based on number of materials
      auto& nmat = stack.template get< param, eq, tag::nmat >();
      auto& ncomp = stack.template get< tag::component, eq >();
      if (physics.back() == inciter::ctr::PhysicsType::VELEQ) {
        // physics = veleq: m-material compressible flow
        // scalar components: volfrac:m + mass:m + momentum:3 + energy:m
        // if nmat is unspecified, configure it be 2
        if (nmat.empty() || nmat.size() != neq.get< eq >()) {
          Message< Stack, WARNING, MsgKey::NONMAT >( stack, in );
          nmat.push_back( 2 );
        }
        // set ncomp based on nmat
        auto m = nmat.back();
        ncomp.push_back( m + m + 3 + m );
      }

      // Verify correct number of multi-material properties configured
      auto& gamma = stack.template get< param, eq, tag::gamma >();
      if (gamma.empty() || gamma.back().size() != nmat.back())
        Message< Stack, ERROR, MsgKey::EOSGAMMA >( stack, in );

      // If pressure relaxation is not specified, default to 'false'
      auto& prelax = stack.template get< param, eq, tag::prelax >();
      if (prelax.empty() || prelax.size() != neq.get< eq >())
        prelax.push_back( 0 );

      // If pressure relaxation time-scale is not specified, default to 1.0
      auto& prelax_ts = stack.template get< param, eq,
                                            tag::prelax_timescale >();
      if (prelax_ts.empty() || prelax_ts.size() != neq.get< eq >())
        prelax_ts.push_back( 1.0 );

      // If interface compression is not specified, default to 'false'
      auto& intsharp = stack.template get< param, eq, tag::intsharp >();
      if (intsharp.empty() || intsharp.size() != neq.get< eq >())
        intsharp.push_back( 0 );

      // If interface compression parameter is not specified, default to 1.0
      auto& intsharp_p = stack.template get< param, eq,
                                            tag::intsharp_param >();
      if (intsharp_p.empty() || intsharp_p.size() != neq.get< eq >())
        intsharp_p.push_back( 1.0 );

      // If specific heats are not given, set defaults
      using cv_t = kw::mat_cv::info::expect::type;
      auto& cv = stack.template get< param, eq, tag::cv >();
      // As a default, the specific heat of air (717.5 J/Kg-K) is used
      if (cv.empty())
        cv.push_back( std::vector< cv_t >( nmat.back(), 717.5 ) );
      // If specific heat vector is wrong size, error out
      if (cv.back().size() != nmat.back())
        Message< Stack, ERROR, MsgKey::EOSCV >( stack, in );

      // If stiffness coefficients are not given, set defaults
      using pstiff_t = kw::mat_pstiff::info::expect::type;
      auto& pstiff = stack.template get< param, eq, tag::pstiff >();
      if (pstiff.empty())
        pstiff.push_back( std::vector< pstiff_t >( nmat.back(), 0.0 ) );
      // If stiffness coefficient vector is wrong size, error out
      if (pstiff.back().size() != nmat.back())
        Message< Stack, ERROR, MsgKey::EOSPSTIFF >( stack, in );

      // If problem type is not given, default to 'user_defined'
      auto& problem = stack.template get< param, eq, tag::problem >();
      if (problem.empty() || problem.size() != neq.get< eq >())
        problem.push_back( inciter::ctr::ProblemType::USER_DEFINED );
      else if (problem.back() == inciter::ctr::ProblemType::VORTICAL_FLOW) {
        const auto& alpha = stack.template get< param, eq, tag::alpha >();
        const auto& beta = stack.template get< param, eq, tag::beta >();
        const auto& p0 = stack.template get< param, eq, tag::p0 >();
        if ( alpha.size() != problem.size() ||
             beta.size() != problem.size() ||
             p0.size() != problem.size() )
          Message< Stack, ERROR, MsgKey::VORTICAL_UNFINISHED >( stack, in );
      }

      // Error check Dirichlet boundary condition block for all multimat
      // configurations
      const auto& bc = stack.template get< param, eq, tag::bc, tag::bcdir >();
      for (const auto& s : bc)
        if (s.empty()) Message< Stack, ERROR, MsgKey::BC_EMPTY >( stack, in );
    }
  };

  //! Rule used to trigger action
  template< class Option, typename...tags >
  struct store_inciter_option : pegtl::success {};
  //! \brief Put option in state at position given by tags
  //! \details This is simply a wrapper around tk::grm::store_option passing the
  //!    stack defaults for inciter.
  template< class Option, typename... tags >
  struct action< store_inciter_option< Option, tags... > > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      store_option< Stack, inciter::deck::use, Option, inciter::ctr::InputDeck,
                    Input, tags... >
                  ( stack, in, inciter::g_inputdeck_defaults );
    }
  };

  //! Function object to ensure disjoint side sets for all boundary conditions
  //! \details This is instantiated using a Cartesian product of all PDE types
  //!    and all BC types at compile time. It goes through all side sets
  //!    configured by the user and triggers an error if a side set is assigned
  //!    a BC more than once.
  template< typename Input, typename Stack >
  struct ensure_disjoint {
    const Input& m_input;
    Stack& m_stack;
    std::unordered_set< int >& m_bcset;
    explicit ensure_disjoint( const Input& in,
                              Stack& stack,
                              std::unordered_set< int >& bcset ) :
      m_input( in ), m_stack( stack ), m_bcset( bcset ) {}
    template< typename U > void operator()( brigand::type_<U> ) {
      using Eq = typename brigand::front< U >;
      using BC = typename brigand::back< U >;
      const auto& bc = m_stack.template get< tag::param, Eq, tag::bc, BC >();
      for (const auto& eq : bc)
        for (const auto& s : eq) {
          auto id = std::stoi(s);
          if (m_bcset.find(id) != end(m_bcset))
            Message< Stack, ERROR, MsgKey::NONDISJOINTBC >( m_stack, m_input );
          else
            m_bcset.insert( id );
        }
    }
  };

  //! Rule used to trigger action
  struct configure_scheme : pegtl::success {};
  //! Configure scheme selected by user
  //! \details This grammar action configures the number of degrees of freedom
  //! (NDOF) used for DG methods. For finite volume (or DGP0), the DOF are the
  //! cell-averages. This implies ndof=1 for DGP0. Similarly, ndof=4 and 10 for
  //! DGP1 and DGP2 respectively, since they evolve higher (>1) order solution
  //! information (e.g. gradients) as well. "rdof" includes degrees of freedom
  //! that are both, evolved and reconstructed. For rDGPnPm methods (e.g. P0P1
  //! and P1P2), "n" denotes the evolved solution-order and "m" denotes the
  //! reconstructed solution-order; i.e. P0P1 has ndof=1 and rdof=4, whereas
  //! P1P2 has ndof=4 and rdof=10. For a pure DG method without reconstruction
  //! (DGP0, DGP1, DGP2), rdof=ndof. For more information about rDGPnPm methods,
  //! ref. Luo, H. et al. (2013).  A reconstructed discontinuous Galerkin method
  //! based on a hierarchical WENO reconstruction for compressible flows on
  //! tetrahedral grids. Journal of Computational Physics, 236, 477-492.
  template<> struct action< configure_scheme > {
    template< typename Input, typename Stack >
    static void apply( const Input&, Stack& stack ) {
      using inciter::ctr::SchemeType;
      auto& discr = stack.template get< tag::discr >();
      auto& ndof = discr.template get< tag::ndof >();
      auto& rdof = discr.template get< tag::rdof >();
      auto scheme = discr.template get< tag::scheme >();
      if (scheme == SchemeType::P0P1) {
        ndof = 1; rdof = 4;
      } else if (scheme == SchemeType::DGP1) {
        ndof = rdof = 4;
      } else if (scheme == SchemeType::DGP2) {
        ndof = rdof = 10;
      } else if (scheme == SchemeType::PDG) {
        ndof = rdof = 10;
        stack.template get< tag::pref, tag::pref >() = true;
      }
    }
  };

  //! Rule used to trigger action
  struct check_inciter : pegtl::success {};
  //! \brief Do error checking on the inciter block
  template<> struct action< check_inciter > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::neq;
      using inciter::g_inputdeck_defaults;

      // Error out if no dt policy has been selected
      const auto& dt = stack.template get< tag::discr, tag::dt >();
      const auto& cfl = stack.template get< tag::discr, tag::cfl >();
      if ( std::abs(dt - g_inputdeck_defaults.get< tag::discr, tag::dt >()) <
            std::numeric_limits< tk::real >::epsilon() &&
          std::abs(cfl - g_inputdeck_defaults.get< tag::discr, tag::cfl >()) <
            std::numeric_limits< tk::real >::epsilon() )
        Message< Stack, ERROR, MsgKey::NODT >( stack, in );

      // If both dt and cfl are given, warn that dt wins over cfl
      if ( std::abs(dt - g_inputdeck_defaults.get< tag::discr, tag::dt >()) >
            std::numeric_limits< tk::real >::epsilon() &&
          std::abs(cfl - g_inputdeck_defaults.get< tag::discr, tag::cfl >()) >
            std::numeric_limits< tk::real >::epsilon() )
        Message< Stack, WARNING, MsgKey::MULDT >( stack, in );

      // Do error checking on time history points
      const auto& hist = stack.template get< tag::history, tag::point >();
      if (std::any_of( begin(hist), end(hist),
           [](const auto& p){ return p.size() != 3; } ) )
      {
        Message< Stack, ERROR, MsgKey::WRONGSIZE >( stack, in );
      }

      // Do error checking on residual eq system component index
      const auto rc = stack.template get< tag::discr, tag::rescomp >();
      const auto& ncomps = stack.template get< tag::component >();
      if (rc < 1 || rc > ncomps.nprop())
        Message< Stack, ERROR, MsgKey::LARGECOMP >( stack, in );

      // Ensure no different BC types are assigned to the same side set
      using PDETypes = inciter::ctr::parameters::Keys;
      using BCTypes = inciter::ctr::bc::Keys;
      std::unordered_set< int > bcset;
      brigand::for_each< tk::cartesian_product< PDETypes, BCTypes > >(
        ensure_disjoint< Input, Stack >( in, stack, bcset ) );

      // Do error checking on time history point names (this is a programmer
      // error if triggers, hence assert)
      Assert(
        (stack.template get< tag::history, tag::id >().size() == hist.size()),
        "Number of history points and ids must equal" );
    }
  };

  //! Rule used to trigger action
  struct enable_amr : pegtl::success {};
  //! Enable adaptive mesh refinement (AMR)
  template<>
  struct action< enable_amr > {
    template< typename Input, typename Stack >
    static void apply( const Input&, Stack& stack ) {
      stack.template get< tag::amr, tag::amr >() = true;
    }
  };

  //! Rule used to trigger action
  struct compute_refvar_idx : pegtl::success {};
  //! Compute indices of refinement variables
  //! \details This functor computes the indices in the unknown vector for all
  //!   refinement variables in the system of systems of dependent variables
  //!   after the refvar...end block has been parsed in the amr...end block.
  //!   After basic error checking, the vector at stack.get<tag::amr,tag::id>()
  //!   is filled.
  template<>
  struct action< compute_refvar_idx > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      // reference variables just parsed by refvar...end block
      const auto& refvar = stack.template get< tag::amr, tag::refvar >();
      // get ncomponents object from this input deck
      const auto& ncomps = stack.template get< tag::component >();
      // compute offset map associating offsets to dependent variables
      auto offsetmap = ncomps.offsetmap( stack );
      // compute number of components associated to dependent variabels
      auto ncompmap = ncomps.ncompmap( stack );
      // reference variable index vector to fill
      auto& refidx = stack.template get< tag::amr, tag::id >();
      // Compute indices for all refvars
      for (const auto& v : refvar) {    // for all reference variables parsed
        // depvar is the first char of a refvar
        auto depvar = v[0];
        // the field ID is optional and is the rest of the depvar string
        std::size_t f = (v.size()>1 ? std::stoul(v.substr(1)) : 1) - 1;
        // field ID must be less than or equal to the number of scalar
        // components configured for the eq system for this dependent variable
        if (f >= tk::cref_find( ncompmap, depvar ))
          Message< Stack, ERROR, MsgKey::NOSUCHCOMPONENT >( stack, in );
        // get offset for depvar
        auto eqsys_offset = tk::cref_find( offsetmap, depvar );
        // the index is the eq offset + field ID
        auto idx = eqsys_offset + f;
        // save refvar index in system of all systems
        refidx.push_back( idx );
      }
    }
  };

  //! Rule used to trigger action
  struct check_amr_errors : pegtl::success {};
  //! Do error checking for the amr...end block
  //! \details This is error checking that only the amr...end block
  //!   must satisfy. Besides error checking this can also set defaults
  //!   as this block is called when parsing of a amr...end block has
  //!   just finished.
  template<>
  struct action< check_amr_errors > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      // Error out if refvar size does not equal refidx size (programmer error)
      Assert( (stack.template get< tag::amr, tag::refvar >().size() ==
               stack.template get< tag::amr, tag::id >().size()),
              "The size of refvar and refidx vectors must equal" );
      const auto& initref = stack.template get< tag::amr, tag::init >();
      const auto& refvar = stack.template get< tag::amr, tag::refvar >();
      const auto& edgelist = stack.template get< tag::amr, tag::edge >();
      // Error out if initref edge list is not divisible by 2 (user error)
      if (edgelist.size() % 2 == 1)
        Message< Stack, ERROR, MsgKey::T0REFODD >( stack, in );
      // Warn if initial AMR will be a no-op
      if ( stack.template get< tag::amr, tag::t0ref >() && initref.empty() )
        Message< Stack, WARNING, MsgKey::T0REFNOOP >( stack, in );
      // Error out if timestepping AMR will be a no-op (user error)
      if ( stack.template get< tag::amr, tag::dtref >() && refvar.empty() )
        Message< Stack, ERROR, MsgKey::DTREFNOOP >( stack, in );
      // Error out if mesh refinement frequency is zero (programmer error)
      Assert( (stack.template get< tag::amr, tag::dtfreq >() > 0),
              "Mesh refinement frequency must be positive" );
    }
  };

  //! Rule used to trigger action
  struct check_pref_errors : pegtl::success {};
  //! Do error checking for the pref...end block
  template<>
  struct action< check_pref_errors > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      auto& tolref = stack.template get< tag::pref, tag::tolref >();
      if (tolref < 0.0 || tolref > 1.0)
        Message< Stack, ERROR, MsgKey::PREFTOL >( stack, in );
    }
  };

  //! Rule used to trigger action
  struct match_pointname : pegtl::success {};
  //! \brief Match PDF name to the registered ones
  //! \details This is used to check the set of PDF names dependent previously
  //!    registered to make sure all are unique.
  template<>
  struct action< match_pointname > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::pointnames;
      // find matched name in set of registered ones
      if (pointnames.find( in.string() ) == pointnames.end()) {
        pointnames.insert( in.string() );
        stack.template get< tag::history, tag::id >().push_back( in.string() );
      }
      else  // error out if name matched var is already registered
        Message< Stack, ERROR, MsgKey::POINTEXISTS >( stack, in );
    }
  };

  //! Rule used to trigger action
  struct push_depvar : pegtl::success {};
  //! Add matched outvar based on depvar into vector of outvars
  //! \details Push outvar based on depvar: use first char of matched token as
  //! OutVar::var, OutVar::name = "" by default. OutVar::name being empty will
  //! be used to differentiate a depvar-based outvar from a human-readable
  //! outvar. Depvar-based outvars can directly access solution arrays using
  //! their field. Human-readable outvars need a mechanism (a function) to read
  //! and compute their variables from solution arrays. The 'getvar' function,
  //! used to compute a physics variable from the numerical solution is assigned
  //! after initial migration and thus not assigned here (during parsing).
  template<>
  struct action< push_depvar > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::centering;
      using inciter::ctr::OutVar;
      auto& vars = stack.template get< tag::cmd, tag::io, tag::outvar >();
      vars.emplace_back(OutVar(in.string()[0], field, centering));
      field = 0;        // reset field
    }
  };

  //! Rule used to trigger action
  struct push_matvar : pegtl::success {};
  //! Add matched outvar based on matvar into vector of outvars
  //! \details Push outvar based on matvar: use depvar char as OutVar::var,
  //! OutVar::name = "" by default. Matvar-based outvars are similar to
  //! depvar-base outvars, in that the OutVar has empty name and the OutVar::var
  //! is a depvar, but instead of having the user try to guess the field id, the
  //! grammar accepts a physics label (accepted multimatvars) and a material
  //! index, which are then converted to a depvar + field index.
  template<>
  struct action< push_matvar > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using tag::param;
      using tag::multimat;
      using inciter::deck::centering;
      using inciter::deck::multimatvars;
      using inciter::ctr::OutVar;
      auto& vars = stack.template get< tag::cmd, tag::io, tag::outvar >();
      auto nmat = stack.template get< param, multimat, tag::nmat >().back();
      auto depvar = stack.template get< param, multimat, tag::depvar >().back();
      // first char of matched token: accepted multimatvar label char
      char v = static_cast<char>( in.string()[0] );
      // Since multimat outvars are configured based on an acceptable character
      // label (see inciter::deck::multimatvars, denoting a physics variable),
      // and a material index (instead of a depvar + a component index),
      // multimat material bounds are checked here. Note that for momentum and
      // velocity, the field id is the spatial direction not the material id.
      // Also note that field (in grammar's state) starts from 0.
      if ( ((v=='u'||v=='U'||v=='m'||v=='M') && field>2) ||
           ((v!='u'&&v!='U'&&v!='m'&&v!='M') && field>=nmat) )
        Message< Stack, ERROR, MsgKey::NOSUCHCOMPONENT >( stack, in );
      // field contains material id, compute multiat component index
      auto comp = tk::cref_find( multimatvars, v )( nmat, field );
      // save depvar + component index based on physics label + material id,
      // also save physics label + material id as matvar
      vars.emplace_back(
        OutVar(depvar, comp, centering, {}, {}, v+std::to_string(field+1)) );
      field = 0;        // reset field
    }
  };

  //! Function object for adding a human-readable output variable
  //! \details Since human-readable outvars do not necessarily have any
  //! reference to the depvar of their system they refer to, nor which system
  //! they refer to, we configure them for all of the systems they are preceded
  //! by. If there is only a single system of the type the outvar is configured,
  //! we simply look up the depvar and use that as OutVar::var. If there are
  //! multiple systems configured upstream to which the outvar could refer to,
  //! we configure an outvar for all systems configured, and postfix the
  //! human-readable OutVar::name with '_' + depvar. Hence this function object
  //! so the code below can be invoked for all equation types.
  template< typename Stack >
  struct AddOutVarHuman {
    Stack& stack;
    const std::string& in_string;
    explicit AddOutVarHuman( Stack& s, const std::string& ins )
      : stack(s), in_string(ins) {}
    template< typename Eq > void operator()( brigand::type_<Eq> ) {
      using inciter::deck::centering;
      using inciter::ctr::OutVar;
      const auto& depvar = stack.template get< tag::param, Eq, tag::depvar >();
      auto& vars = stack.template get< tag::cmd, tag::io, tag::outvar >();
      if (depvar.size() == 1)
        vars.emplace_back( OutVar( depvar[0], 0, centering, in_string ) );
      else
        for (auto d : depvar)
          vars.emplace_back( OutVar( d, 0, centering, in_string + '_' + d ) );
    }
  };

  //! Rule used to trigger action
  struct push_humanvar : pegtl::success {};
  //! Add matched outvar based on depvar into vector of vector of outvars
  //! \details Push outvar based on human readable string for which
  //! OutVar::name = matched token. OutVar::name being not empty will be used to
  //! differentiate a depvar-based outvar from a human-readable outvar.
  //! Depvar-based outvars can directly access solution arrays using their
  //! field. Human-readable outvars need a mechanism (a function) to read and
  //! compute their variables from solution arrays. The 'getvar' function, used
  //! to compute a physics variable from the numerical solution is assigned
  //! after initial migration and thus not assigned here (during parsing).
  template<>
  struct action< push_humanvar > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      brigand::for_each< inciter::ctr::parameters::Keys >
                       ( AddOutVarHuman< Stack >( stack, in.string() ) );
    }
  };

  //! Rule used to trigger action
  struct set_outvar_alias : pegtl::success {};
  //! Set alias of last pushed output variable
  template<>
  struct action< set_outvar_alias > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      // Set alias of last pushed outvar:
      auto& vars = stack.template get< tag::cmd, tag::io, tag::outvar >();
      if (!vars.empty()) vars.back().alias = in.string();
    }
  };

  //! Function object for error checking outvar bounds for each equation type
  template< typename Stack >
  struct OutVarBounds {
    const Stack& stack;
    bool& inbounds;
    explicit OutVarBounds( const Stack& s, bool& i )
      : stack(s), inbounds(i) { inbounds = false; }
    template< typename U > void operator()( brigand::type_<U> ) {
      if (std::is_same_v< U, tag::multimat >) inbounds = true;  // skip multimat
      const auto& depvar = stack.template get< tag::param, U, tag::depvar >();
      const auto& ncomp = stack.template get< tag::component, U >();
      Assert( depvar.size() == ncomp.size(), "Size mismatch" );
      // called after matching each outvar, so only check the last one
      auto& vars = stack.template get< tag::cmd, tag::io, tag::outvar >();
      const auto& last_outvar = vars.back();
      const auto& v = static_cast<char>( std::tolower(last_outvar.var) );
      for (std::size_t e=0; e<depvar.size(); ++e)
        if (v == depvar[e] && last_outvar.field < ncomp[e]) inbounds = true;
    }
  };

  //! Rule used to trigger action
  struct check_outvar : pegtl::success {};
  //! Bounds checking for output variables at the end of a var ... end block
  template<>
  struct action< check_outvar > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      bool inbounds;
      brigand::for_each< inciter::ctr::parameters::Keys >
                       ( OutVarBounds< Stack >( stack, inbounds ) );
      if (!inbounds)
        Message< Stack, ERROR, MsgKey::NOSUCHCOMPONENT >( stack, in );
    }
  };

  //! Rule used to trigger action
  struct set_centering : pegtl::success {};
  //! Set variable centering in parser's state
  template<>
  struct action< set_centering > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& ) {
      inciter::deck::centering =
        (in.string() == "node") ? tk::Centering::NODE : tk::Centering::ELEM;
    }
  };

  //! Rule used to trigger action
  struct match_outvar : pegtl::success {};
  //! Match output variable based on depvar
  template<>
  struct action< match_outvar > {
    template< typename Input, typename Stack >
    static void apply( const Input& in, Stack& stack ) {
      using inciter::deck::neq;
      using inciter::deck::multimatvars;
      // convert matched string to char
      auto var = stack.template convert< char >( in.string() );
      if (neq.get< tag::multimat >() == 0) {    // if not multimat
        // find matched variable in set of selected ones
        if (depvars.find(var) != end(depvars))
          action< push_depvar >::apply( in, stack );
        else  // error out if matched var is not selected
          Message< Stack, ERROR, MsgKey::NOSUCHOUTVAR >( stack, in );
      } else {    // if multimat
        // find matched variable in set accepted for multimat
        if (multimatvars.find(var) != end(multimatvars))
          action< push_matvar >::apply( in, stack );
        else
          Message< Stack, ERROR, MsgKey::NOSUCHMULTIMATVAR >( stack, in );
      }
    }
  };

} // ::grm
} // ::tk

namespace inciter {

//! Inciter input deck facilitating user input for computing shock hydrodynamics
namespace deck {

  using namespace tao;

  // Inciter's InputDeck grammar

  //! scan and store_back equation keyword and option
  template< typename keyword, class eq >
  struct scan_eq :
         tk::grm::scan< typename keyword::pegtl_string,
                        tk::grm::store_back_option< use,
                                                    ctr::PDE,
                                                    tag::selected,
                                                    tag::pde > > {};

  //! Error checks after an equation...end block has been parsed
  template< class eq, template< class > class eqchecker >
  struct check_errors :
         pegtl::seq<
           // register differential equation block
           tk::grm::register_inciter_eq< eq >,
           // do error checking on this block
           eqchecker< eq > > {};

  //! Match discretization option
  template< template< class > class use, class keyword, class Option,
            class Tag >
  struct discroption :
         tk::grm::process< use< keyword >,
                           tk::grm::store_inciter_option<
                             Option, tag::discr, Tag >,
                           pegtl::alpha > {};

  //! Discretization parameters
  struct discretization :
         pegtl::sor<
           tk::grm::discrparam< use, kw::nstep, tag::nstep >,
           tk::grm::discrparam< use, kw::term, tag::term >,
           tk::grm::discrparam< use, kw::t0, tag::t0 >,
           tk::grm::discrparam< use, kw::dt, tag::dt >,
           tk::grm::discrparam< use, kw::cfl, tag::cfl >,
           tk::grm::discrparam< use, kw::residual, tag::residual >,
           tk::grm::discrparam< use, kw::rescomp, tag::rescomp >,
           tk::grm::process< use< kw::fcteps >,
                             tk::grm::Store< tag::discr, tag::fcteps > >,
           tk::grm::process< use< kw::fctclip >,
                             tk::grm::Store< tag::discr, tag::fctclip >,
                             pegtl::alpha >,
           tk::grm::process< use< kw::fct >,
                             tk::grm::Store< tag::discr, tag::fct >,
                             pegtl::alpha >,
           tk::grm::process< use< kw::ctau >,
                             tk::grm::Store< tag::discr, tag::ctau > >,
           tk::grm::process< use< kw::pelocal_reorder >,
                             tk::grm::Store< tag::discr, tag::pelocal_reorder >,
                             pegtl::alpha >,
           tk::grm::process< use< kw::operator_reorder >,
                             tk::grm::Store< tag::discr, tag::operator_reorder >,
                             pegtl::alpha >,
           tk::grm::process< use< kw::steady_state >,
                             tk::grm::Store< tag::discr, tag::steady_state >,
                             pegtl::alpha >,
           tk::grm::interval< use< kw::ttyi >, tag::tty >,
           tk::grm::process_alpha< use< kw::scheme >,
                                   tk::grm::store_inciter_option<
                                     inciter::ctr::Scheme,
                                     tag::discr,
                                     tag::scheme >,
                                   tk::grm::configure_scheme >,
           discroption< use, kw::limiter, inciter::ctr::Limiter, tag::limiter >,
           tk::grm::discrparam< use, kw::cweight, tag::cweight >
         > {};

  //! PDE parameter vector
  template< class keyword, class eq, class param, class... xparams >
  struct pde_parameter_vector :
         tk::grm::parameter_vector< use,
                                    use< keyword >,
                                    tk::grm::Store_back_back,
                                    tk::grm::start_vector,
                                    tk::grm::check_vector,
                                    eq, param, xparams... > {};

  //! put in PDE parameter for equation matching keyword
  template< typename eq, typename keyword, typename param,
            class kw_type = tk::grm::number >
  struct parameter :
         tk::grm::process< use< keyword >,
                           tk::grm::Store_back< tag::param, eq, param >,
                           kw_type > {};

  //! put in PDE bool parameter for equation matching keyword into vector< int >
  template< typename eq, typename keyword, typename p >
  struct parameter_bool :
         tk::grm::process< use< keyword >,
                           tk::grm::Store_back_bool< tag::param, eq, p >,
                           pegtl::alpha > {};

  //! Boundary conditions block
  template< class keyword, class eq, class param >
  struct bc :
         pegtl::if_must<
           tk::grm::readkw< typename use< keyword >::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             tk::grm::parameter_vector< use,
                                        use< kw::sideset >,
                                        tk::grm::Store_back_back,
                                        tk::grm::start_vector,
                                        tk::grm::check_vector,
                                        eq, tag::bc, param > > > {};

  //! Stagnation boundary conditions block
  template< class eq, class bc, class kwbc >
  struct bc_spec :
         pegtl::if_must<
           tk::grm::readkw< typename kwbc::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             tk::grm::parameter_vector< use,
                                        use< kw::radius >,
                                        tk::grm::Store_back_back,
                                        tk::grm::start_vector,
                                        tk::grm::check_vector,
                                        eq, bc, tag::radius >,
             tk::grm::parameter_vector< use,
                                        use< kw::point >,
                                        tk::grm::Store_back_back,
                                        tk::grm::start_vector,
                                        tk::grm::check_vector,
                                        eq, bc, tag::point > > > {};

  //! Farfield boundary conditions block
  template< class keyword, class eq, class param >
  struct farfield_bc :
         pegtl::if_must<
           tk::grm::readkw< typename use< keyword >::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             parameter< eq, kw::pressure, tag::farfield_pressure >,
             parameter< eq, kw::density, tag::farfield_density >,
             pde_parameter_vector< kw::velocity, eq, tag::farfield_velocity >,
             tk::grm::parameter_vector< use,
                                        use< kw::sideset >,
                                        tk::grm::Store_back_back,
                                        tk::grm::start_vector,
                                        tk::grm::check_vector,
                                        eq, tag::bc, param > > > {};

  //! edgelist ... end block
  struct edgelist :
         tk::grm::vector< use< kw::amr_edgelist >,
                          tk::grm::Store_back< tag::amr, tag::edge >,
                          use< kw::end >,
                          tk::grm::check_vector< tag::amr, tag::edge > > {};

  //! xminus configuring coordinate-based edge tagging for mesh refinement
  template< typename keyword, typename Tag >
  struct half_world :
         tk::grm::control< use< keyword >, pegtl::digit, tag::amr, Tag > {};

  //! coords ... end block
  struct coords :
           pegtl::if_must<
             tk::grm::readkw< use< kw::amr_coords >::pegtl_string >,
             tk::grm::block< use< kw::end >,
                             half_world< kw::amr_xminus, tag::xminus >,
                             half_world< kw::amr_xplus, tag::xplus >,
                             half_world< kw::amr_yminus, tag::yminus >,
                             half_world< kw::amr_yplus, tag::yplus >,
                             half_world< kw::amr_zminus, tag::zminus >,
                             half_world< kw::amr_zplus, tag::zplus > > > {};

  //! initial conditins box block
  template< class eq >
  struct box :
         pegtl::if_must<
           tk::grm::readkw< use< kw::box >::pegtl_string >,
           tk::grm::block< use< kw::end >,
             tk::grm::control< use< kw::xmin >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::xmin >,
             tk::grm::control< use< kw::xmax >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::xmax >,
             tk::grm::control< use< kw::ymin >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::ymin >,
             tk::grm::control< use< kw::ymax >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::ymax >,
             tk::grm::control< use< kw::zmin >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::zmin >,
             tk::grm::control< use< kw::zmax >, tk::grm::number,
                               tag::param, eq, tag::ic, tag::box, tag::zmax >,
             pegtl::sor<
               pde_parameter_vector< kw::density,
                                     eq, tag::ic, tag::box, tag::density >,
               pde_parameter_vector< kw::velocity,
                                     eq, tag::ic, tag::box, tag::velocity >,
               pde_parameter_vector< kw::pressure,
                                     eq, tag::ic, tag::box, tag::pressure >,
               pde_parameter_vector< kw::temperature,
                                     eq, tag::ic, tag::box, tag::temperature >,
               pde_parameter_vector< kw::mass,
                                     eq, tag::ic, tag::box, tag::mass >,
               pde_parameter_vector< kw::energy_content,
                                   eq, tag::ic, tag::box, tag::energy_content >,
               pde_parameter_vector< kw::energy,
                                     eq, tag::ic, tag::box, tag::energy >,
               tk::grm::process< use< kw::initiate >,
                                 tk::grm::store_back_option< use,
                                                             ctr::Initiate,
                                                             tag::param,
                                                             eq,
                                                             tag::ic,
                                                             tag::box,
                                                             tag::initiate,
                                                             tag::init >,
                                 pegtl::alpha >,
               pegtl::if_must<
                 tk::grm::readkw< use< kw::linear >::pegtl_string >,
                 tk::grm::block< use< kw::end >,
                   tk::grm::parameter_vector< use,
                                              use< kw::point >,
                                              tk::grm::Store_back_back,
                                              tk::grm::start_vector,
                                              tk::grm::check_vector,
                                              eq,
                                              tag::ic,
                                              tag::box,
                                              tag::initiate,
                                              tag::point >,
                   tk::grm::parameter_vector< use,
                                              use< kw::radius >,
                                              tk::grm::Store_back_back,
                                              tk::grm::start_vector,
                                              tk::grm::check_vector,
                                              eq,
                                              tag::ic,
                                              tag::box,
                                              tag::initiate,
                                              tag::radius >,
                   pde_parameter_vector< kw::velocity,
                     eq, tag::ic, tag::box, tag::initiate, tag::velocity > > >
             > > > {};

  //! initial conditions block for compressible flow
  template< class eq >
  struct ic :
         pegtl::if_must<
           tk::grm::readkw< use< kw::ic >::pegtl_string >,
           tk::grm::block< use< kw::end >,
             pegtl::sor<
               pde_parameter_vector< kw::density,
                                     eq, tag::ic, tag::density >,
               pde_parameter_vector< kw::velocity,
                                     eq, tag::ic, tag::velocity >,
               pde_parameter_vector< kw::pressure,
                                     eq, tag::ic, tag::pressure >,
               pde_parameter_vector< kw::temperature,
                                     eq, tag::ic, tag::temperature >,
               pde_parameter_vector< kw::energy,
                                     eq, tag::ic, tag::energy > >,
               pegtl::seq< box< eq > > > > {};

  //! put in material property for equation matching keyword
  template< typename eq, typename keyword, typename property >
  struct material_property :
         pde_parameter_vector< keyword, eq, property > {};

  //! Material properties block for compressible flow
  template< class eq >
  struct material_properties :
         pegtl::if_must<
           tk::grm::readkw< use< kw::material >::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             material_property< eq, kw::mat_gamma, tag::gamma >,
             material_property< eq, kw::mat_pstiff, tag::pstiff >,
             material_property< eq, kw::mat_mu, tag::mu >,
             material_property< eq, kw::mat_cv, tag::cv >,
             material_property< eq, kw::mat_k, tag::k > > > {};

  //! transport equation for scalars
  struct transport :
         pegtl::if_must<
           scan_eq< use< kw::transport >, tag::transport >,
           tk::grm::block< use< kw::end >,
                           tk::grm::policy< use,
                                            use< kw::physics >,
                                            ctr::Physics,
                                            tag::transport,
                                            tag::physics >,
                           tk::grm::policy< use,
                                            use< kw::problem >,
                                            ctr::Problem,
                                            tag::transport,
                                            tag::problem >,
                           tk::grm::depvar< use,
                                            tag::transport,
                                            tag::depvar >,
                           tk::grm::component< use< kw::ncomp >,
                                               tag::transport >,
                           pde_parameter_vector< kw::pde_diffusivity,
                                                 tag::transport,
                                                 tag::diffusivity >,
                           pde_parameter_vector< kw::pde_lambda,
                                                 tag::transport,
                                                 tag::lambda >,
                           pde_parameter_vector< kw::pde_u0,
                                                 tag::transport,
                                                 tag::u0 >,
                           bc< kw::bc_dirichlet, tag::transport, tag::bcdir >,
                           bc< kw::bc_sym, tag::transport, tag::bcsym >,
                           bc< kw::bc_inlet, tag::transport, tag::bcinlet >,
                           bc< kw::bc_outlet, tag::transport, tag::bcoutlet >,
                           bc< kw::bc_extrapolate, tag::transport,
                               tag::bcextrapolate >,
                           parameter< tag::transport,
                                      kw::intsharp_param,
                                      tag::intsharp_param >,
                           parameter< tag::transport,
                                      kw::intsharp,
                                      tag::intsharp > >,
           check_errors< tag::transport, tk::grm::check_transport > > {};

  //! compressible flow
  struct compflow :
         pegtl::if_must<
           scan_eq< use< kw::compflow >, tag::compflow >,
           tk::grm::block< use< kw::end >,
                           tk::grm::policy< use,
                                            use< kw::physics >,
                                            ctr::Physics,
                                            tag::compflow,
                                            tag::physics >,
                           tk::grm::policy< use,
                                            use< kw::problem >,
                                            ctr::Problem,
                                            tag::compflow,
                                            tag::problem >,
                           tk::grm::depvar< use,
                                            tag::compflow,
                                            tag::depvar >,
                           tk::grm::process<
                             use< kw::flux >,
                               tk::grm::store_back_option< use,
                                                           ctr::Flux,
                                                           tag::param,
                                                           tag::compflow,
                                                           tag::flux >,
                             pegtl::alpha >,
                           ic< tag::compflow >,
                           tk::grm::lua< use, tag::param, tag::compflow >,
                           material_properties< tag::compflow >,
                           pde_parameter_vector< kw::sysfctvar,
                                                 tag::compflow,
                                                 tag::sysfctvar >,
                           parameter_bool< tag::compflow,
                                           kw::sysfct,
                                           tag::sysfct >,
                           parameter< tag::compflow, kw::npar,
                                      tag::npar, pegtl::digit >,
                           parameter< tag::compflow, kw::pde_alpha,
                                      tag::alpha >,
                           parameter< tag::compflow, kw::pde_p0,
                                      tag::p0 >,
                           parameter< tag::compflow, kw::pde_betax,
                                      tag::betax >,
                           parameter< tag::compflow, kw::pde_betay,
                                      tag::betay >,
                           parameter< tag::compflow, kw::pde_betaz,
                                      tag::betaz >,
                           parameter< tag::compflow, kw::pde_beta,
                                      tag::beta >,
                           parameter< tag::compflow, kw::pde_r0,
                                      tag::r0 >,
                           parameter< tag::compflow, kw::pde_ce,
                                      tag::ce >,
                           parameter< tag::compflow, kw::pde_kappa,
                                      tag::kappa >,
                           bc< kw::bc_dirichlet, tag::compflow, tag::bcdir >,
                           bc< kw::bc_sym, tag::compflow, tag::bcsym >,
                           bc_spec< tag::compflow, tag::bcstag, kw::bc_stag >,
                           bc_spec< tag::compflow, tag::bcskip, kw::bc_skip >,
                           bc< kw::bc_inlet, tag::compflow, tag::bcinlet >,
                           farfield_bc< kw::bc_farfield,
                                        tag::compflow,
                                        tag::bcfarfield >,
                           bc< kw::bc_extrapolate, tag::compflow,
                               tag::bcextrapolate > >,
           check_errors< tag::compflow, tk::grm::check_compflow > > {};

  //! compressible multi-material flow
  struct multimat :
         pegtl::if_must<
           scan_eq< use< kw::multimat >, tag::multimat >,
           tk::grm::block< use< kw::end >,
                           tk::grm::policy< use,
                                            use< kw::physics >,
                                            ctr::Physics,
                                            tag::multimat,
                                            tag::physics >,
                           tk::grm::policy< use,
                                            use< kw::problem >,
                                            ctr::Problem,
                                            tag::multimat,
                                            tag::problem >,
                           tk::grm::depvar< use,
                                            tag::multimat,
                                            tag::depvar >,
                           parameter< tag::multimat,
                                      kw::nmat,
                                      tag::nmat >,
                           tk::grm::process<
                             use< kw::flux >,
                               tk::grm::store_back_option< use,
                                                           ctr::Flux,
                                                           tag::param,
                                                           tag::multimat,
                                                           tag::flux >,
                             pegtl::alpha >,
                           material_properties< tag::multimat >,
                           parameter< tag::multimat,
                                      kw::pde_alpha,
                                      tag::alpha >,
                           parameter< tag::multimat,
                                      kw::pde_p0,
                                      tag::p0 >,
                           parameter< tag::multimat,
                                      kw::pde_beta,
                                      tag::beta >,
                           bc< kw::bc_dirichlet,
                               tag::multimat,
                               tag::bcdir >,
                           bc< kw::bc_sym,
                               tag::multimat,
                               tag::bcsym >,
                           bc< kw::bc_inlet,
                               tag::multimat,
                               tag::bcinlet >,
                           bc< kw::bc_outlet,
                               tag::multimat,
                               tag::bcoutlet >,
                           bc< kw::bc_extrapolate,
                               tag::multimat,
                               tag::bcextrapolate >,
                           parameter< tag::multimat,
                                      kw::prelax_timescale,
                                      tag::prelax_timescale >,
                           parameter< tag::multimat,
                                      kw::prelax,
                                      tag::prelax >,
                           parameter< tag::multimat,
                                      kw::intsharp_param,
                                      tag::intsharp_param >,
                           parameter< tag::multimat,
                                      kw::intsharp,
                                      tag::intsharp > >,
           check_errors< tag::multimat, tk::grm::check_multimat > > {};

  //! partitioning ... end block
  struct partitioning :
         pegtl::if_must<
           tk::grm::readkw< use< kw::partitioning >::pegtl_string >,
           tk::grm::block< use< kw::end >,
                           tk::grm::process<
                             use< kw::algorithm >,
                             tk::grm::store_inciter_option<
                               tk::ctr::PartitioningAlgorithm,
                               tag::selected,
                               tag::partitioner >,
                             pegtl::alpha > > > {};

  //! equation types
  struct equations :
         pegtl::sor< transport, compflow, multimat > {};

  //! refinement variable(s) (refvar) ... end block
  struct refvars :
         pegtl::if_must<
           tk::grm::vector< use< kw::amr_refvar >,
                            tk::grm::match_depvar<
                              tk::grm::Store_back< tag::amr, tag::refvar > >,
                            use< kw::end >,
                            tk::grm::check_vector< tag::amr, tag::refvar >,
                            tk::grm::fieldvar< pegtl::alpha > >,
           tk::grm::compute_refvar_idx > {};

  //! adaptive mesh refinement (AMR) amr...end block
  struct amr :
         pegtl::if_must<
           tk::grm::readkw< use< kw::amr >::pegtl_string >,
           tk::grm::enable_amr, // enable AMR if amr...end block encountered
           tk::grm::block< use< kw::end >,
                           refvars,
                           edgelist,
                           coords,
                           tk::grm::process<
                             use< kw::amr_initial >,
                             tk::grm::store_back_option< use,
                                                         ctr::AMRInitial,
                                                         tag::amr,
                                                         tag::init >,
                             pegtl::alpha >,
                           tk::grm::process<
                             use< kw::amr_error >,
                             tk::grm::store_inciter_option<
                               ctr::AMRError,
                               tag::amr, tag::error >,
                             pegtl::alpha >,
                           tk::grm::control< use< kw::amr_tolref >,
                                             pegtl::digit,
                                             tag::amr,
                                             tag::tolref >,
                           tk::grm::control< use< kw::amr_tolderef >,
                                             pegtl::digit,
                                             tag::amr,
                                             tag::tolderef >,
                           tk::grm::process< use< kw::amr_t0ref >,
                             tk::grm::Store< tag::amr, tag::t0ref >,
                             pegtl::alpha >,
                           tk::grm::process< use< kw::amr_dtref_uniform >,
                             tk::grm::Store< tag::amr, tag::dtref_uniform >,
                             pegtl::alpha >,
                           tk::grm::process< use< kw::amr_dtref >,
                             tk::grm::Store< tag::amr, tag::dtref >,
                             pegtl::alpha >,
                           tk::grm::process< use< kw::amr_dtfreq >,
                             tk::grm::Store< tag::amr, tag::dtfreq >,
                             pegtl::digit >
                         >,
           tk::grm::check_amr_errors > {};

  //! p-adaptive refinement (pref) ...end block
  struct pref :
         pegtl::if_must<
           tk::grm::readkw< use< kw::pref >::pegtl_string >,
           tk::grm::block< use< kw::end >,
                           tk::grm::control< use< kw::pref_tolref >,
                                             pegtl::digit,
                                             tag::pref,
                                             tag::tolref  >,
                           tk::grm::control< use< kw::pref_ndofmax >,
                                             pegtl::digit,
                                             tag::pref,
                                             tag::ndofmax >,
                           tk::grm::process<
                             use< kw::pref_indicator >,
                             tk::grm::store_inciter_option<
                               ctr::PrefIndicator,
                               tag::pref, tag::indicator >,
                             pegtl::alpha >
                         >,
           tk::grm::check_pref_errors > {};

  //! Match output variable alias
  struct outvar_alias :
         tk::grm::quoted< tk::grm::set_outvar_alias > {};

  //! Match an output variable in a human readable form: var must be a keyword
  template< class var >
  struct outvar_human :
         tk::grm::exact_scan< use< var >, tk::grm::push_humanvar > {};

  //! Match an output variable based on depvar defined upstream of input file
  struct outvar_depvar :
           tk::grm::scan< tk::grm::fieldvar< pegtl::upper >,
             tk::grm::match_outvar, tk::grm::check_outvar > {};

  //! Parse a centering token and if matches, set centering in parser's state
  struct outvar_centering :
         pegtl::sor<
           tk::grm::exact_scan< use< kw::node >, tk::grm::set_centering >,
           tk::grm::exact_scan< use< kw::elem >, tk::grm::set_centering > > {};

  //! outvar ... end block
  struct outvar_block :
         pegtl::if_must<
           tk::grm::readkw< use< kw::outvar >::pegtl_string >,
           tk::grm::block<
             use< kw::end >
           , outvar_centering
           , outvar_depvar
           , outvar_alias
           , outvar_human< kw::outvar_density >
           , outvar_human< kw::outvar_xmomentum >
           , outvar_human< kw::outvar_ymomentum >
           , outvar_human< kw::outvar_zmomentum >
           , outvar_human< kw::outvar_specific_total_energy >
           , outvar_human< kw::outvar_volumetric_total_energy >
           , outvar_human< kw::outvar_xvelocity >
           , outvar_human< kw::outvar_yvelocity >
           , outvar_human< kw::outvar_zvelocity >
           , outvar_human< kw::outvar_pressure >
           , outvar_human< kw::outvar_analytic >
           > > {};

  //! field_output ... end block
  struct field_output :
         pegtl::if_must<
           tk::grm::readkw< use< kw::field_output >::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             outvar_block,
             tk::grm::process< use< kw::filetype >,
                               tk::grm::store_inciter_option<
                                 tk::ctr::FieldFile,
                                 tag::selected,
                                 tag::filetype >,
                               pegtl::alpha >,
             tk::grm::interval< use< kw::interval >, tag::field >,
             tk::grm::process<
               use< kw::refined >,
               tk::grm::Store< tag::cmd, tag::io, tag::refined >,
               pegtl::alpha >,
             pegtl::if_must<
               tk::grm::vector<
                 use< kw::sideset >,
                 tk::grm::Store_back< tag::cmd, tag::io, tag::surface >,
                 use< kw::end > > > > > {};

  //! history_output ... end block
  struct history_output :
         pegtl::if_must<
           tk::grm::readkw< use< kw::history_output >::pegtl_string >,
           tk::grm::block<
             use< kw::end >,
             outvar_block,
             tk::grm::interval< use< kw::interval >, tag::history >,
             tk::grm::precision< use, tag::history >,
             tk::grm::process<
               use< kw::txt_float_format >,
               tk::grm::store_inciter_option< tk::ctr::TxtFloatFormat,
                                              tag::flformat,
                                              tag::history >,
               pegtl::alpha >,
             pegtl::if_must<
               tk::grm::readkw< use< kw::point >::pegtl_string >,
               tk::grm::act< pegtl::identifier, tk::grm::match_pointname >,
               pegtl::seq<
                 tk::grm::start_vector< tag::history, tag::point >,
                 tk::grm::block<
                   use< kw::end >,
                   tk::grm::scan< tk::grm::number,
                     tk::grm::Store_back_back< tag::history, tag::point > > >
               > > > > {};

  //! 'inciter' block
  struct inciter :
         pegtl::if_must<
           tk::grm::readkw< use< kw::inciter >::pegtl_string >,
           pegtl::sor<
             pegtl::seq< tk::grm::block<
                           use< kw::end >,
                           discretization,
                           equations,
                           amr,
                           pref,
                           partitioning,
                           field_output,
                           history_output,
                           tk::grm::diagnostics<
                             use,
                             tk::grm::store_inciter_option > >,
                         tk::grm::check_inciter >,
            tk::grm::msg< tk::grm::MsgType::ERROR,
                          tk::grm::MsgKey::UNFINISHED > > > {};

  //! \brief All keywords
  struct keywords :
         pegtl::sor< tk::grm::title< use >, inciter > {};

  //! \brief Grammar entry point: parse keywords and ignores until eof
  struct read_file :
         tk::grm::read_file< keywords, tk::grm::ignore > {};

} // deck::
} // inciter::

#endif // InciterInputDeckGrammar_h
