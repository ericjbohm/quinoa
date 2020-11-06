// *****************************************************************************
/*!
  \file      src/PDE/Transport/DGTransport.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Scalar transport using disccontinous Galerkin discretization
  \details   This file implements the physics operators governing transported
     scalars using disccontinuous Galerkin discretization.
*/
// *****************************************************************************
#ifndef DGTransport_h
#define DGTransport_h

#include <vector>
#include <array>
#include <limits>
#include <cmath>
#include <unordered_set>
#include <map>

#include "Macro.hpp"
#include "Exception.hpp"
#include "Vector.hpp"
#include "Inciter/Options/BC.hpp"
#include "UnsMesh.hpp"
#include "Integrate/Basis.hpp"
#include "Integrate/Quadrature.hpp"
#include "Integrate/Initialize.hpp"
#include "Integrate/Mass.hpp"
#include "Integrate/Surface.hpp"
#include "Integrate/Boundary.hpp"
#include "Integrate/Volume.hpp"
#include "Riemann/Upwind.hpp"
#include "Reconstruction.hpp"
#include "Limiter.hpp"
#include "FieldOutputUtil.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

namespace dg {

//! \brief Transport equation used polymorphically with tk::DGPDE
//! \details The template argument(s) specify policies and are used to configure
//!   the behavior of the class. The policies are:
//!   - Physics - physics configuration, see PDE/Transport/Physics.h
//!   - Problem - problem configuration, see PDE/Transport/Problem.h
//! \note The default physics is DGAdvection, set in
//!    inciter::deck::check_transport()
template< class Physics, class Problem >
class Transport {

  private:
    using eq = tag::transport;

  public:
    //! Constructor
    //! \param[in] c Equation system index (among multiple systems configured)
    explicit Transport( ncomp_t c ) :
      m_physics( Physics() ),
      m_problem( Problem() ),
      m_system( c ),
      m_ncomp(
        g_inputdeck.get< tag::component >().get< eq >().at(c) ),
      m_offset(
        g_inputdeck.get< tag::component >().offset< eq >(c) )
    {
      // associate boundary condition configurations with state functions, the
      // order in which the state functions listed matters, see ctr::bc::Keys
      brigand::for_each< ctr::bc::Keys >( ConfigBC< eq >( m_system, m_bc,
        { dirichlet
        , invalidBC  // Symmetry BC not implemented
        , inlet
        , outlet
        , invalidBC  // Characteristic BC not implemented
        , extrapolate } ) );
      m_problem.errchk( m_system, m_ncomp );
    }

    //! Find the number of primitive quantities required for this PDE system
    //! \return The number of primitive quantities required to be stored for
    //!   this PDE system
    std::size_t nprim() const
    {
      // transport does not need/store any primitive quantities currently
      return 0;
    }

    //! Determine elements that lie inside the user-defined IC box
    void IcBoxElems( const tk::Fields&,
      std::size_t,
      std::unordered_set< std::size_t >& ) const
    {}

    //! Initalize the transport equations for DG
    //! \param[in] L Element mass matrix
    //! \param[in] inpoel Element-node connectivity
    //! \param[in] coord Array of nodal coordinates
//    //! \param[in,out] inbox List of elements at which box user ICs are set
    //! \param[in,out] unk Array of unknowns
    //! \param[in] t Physical time
    //! \param[in] nielem Number of internal elements
    void initialize( const tk::Fields& L,
                     const std::vector< std::size_t >& inpoel,
                     const tk::UnsMesh::Coords& coord,
                     const std::unordered_set< std::size_t >& /*inbox*/,
                     tk::Fields& unk,
                     tk::real t,
                     const std::size_t nielem ) const
    {
      tk::initialize( m_system, m_ncomp, m_offset, L, inpoel, coord,
                      Problem::solution, unk, t, nielem );
    }

    //! Compute the left hand side mass matrix
    //! \param[in] geoElem Element geometry array
    //! \param[in,out] l Block diagonal mass matrix
    void lhs( const tk::Fields& geoElem, tk::Fields& l ) const {
      const auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
      tk::mass( m_ncomp, m_offset, ndof, geoElem, l );
    }

    //! Update the primitives for this PDE system
    //! \details This function computes and stores the dofs for primitive
    //!   quantities, which are currently unused for transport.
    void updatePrimitives( const tk::Fields&,
                           tk::Fields&,
                           std::size_t ) const {}

    //! Clean up the state of trace materials for this PDE system
    //! \details This function cleans up the state of materials present in trace
    //!   quantities in each cell. This is currently unused for transport.
    void cleanTraceMaterial( const tk::Fields&,
                             tk::Fields&,
                             tk::Fields&,
                             std::size_t ) const {}

    //! Reconstruct second-order solution from first-order
    //! \param[in] t Physical time
    //! \param[in] geoFace Face geometry array
    //! \param[in] geoElem Element geometry array
    //! \param[in] fd Face connectivity and boundary conditions object
    //! \param[in] inpoel Element-node connectivity
    //! \param[in] coord Array of nodal coordinates
    //! \param[in,out] U Solution vector at recent time step
    //! \param[in,out] P Primitive vector at recent time step
    void reconstruct( tk::real t,
                      const tk::Fields& geoFace,
                      const tk::Fields& geoElem,
                      const inciter::FaceData& fd,
                      const std::map< std::size_t, std::vector< std::size_t > >&,
                      const std::vector< std::size_t >& inpoel,
                      const tk::UnsMesh::Coords& coord,
                      tk::Fields& U,
                      tk::Fields& P ) const
    {
      const auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();
      const auto nelem = fd.Esuel().size()/4;

      Assert( U.nprop() == rdof*m_ncomp, "Number of components in solution "
              "vector must equal "+ std::to_string(rdof*m_ncomp) );
      Assert( fd.Inpofa().size()/3 == fd.Esuf().size()/2,
              "Mismatch in inpofa size" );

      // allocate and initialize matrix and vector for reconstruction
      std::vector< std::array< std::array< tk::real, 3 >, 3 > >
        lhs_ls( nelem, {{ {{0.0, 0.0, 0.0}},
                          {{0.0, 0.0, 0.0}},
                          {{0.0, 0.0, 0.0}} }} );
      std::vector< std::vector< std::array< tk::real, 3 > > >
        rhs_ls( nelem, std::vector< std::array< tk::real, 3 > >
          ( m_ncomp,
            {{ 0.0, 0.0, 0.0 }} ) );

      // reconstruct x,y,z-derivatives of unknowns
      // 0. get lhs matrix, which is only geometry dependent
      tk::lhsLeastSq_P0P1(fd, geoElem, geoFace, lhs_ls);

      // 1. internal face contributions
      tk::intLeastSq_P0P1( m_ncomp, m_offset, rdof, fd, geoElem, U, rhs_ls );

      // 2. boundary face contributions
      for (const auto& b : m_bc)
        tk::bndLeastSqConservedVar_P0P1( m_system, m_ncomp, m_offset, rdof,
          b.first, fd, geoFace, geoElem, t, b.second, P, U, rhs_ls );

      // 3. solve 3x3 least-squares system
      tk::solveLeastSq_P0P1( m_ncomp, m_offset, rdof, lhs_ls, rhs_ls, U );

      // 4. transform reconstructed derivatives to Dubiner dofs
      tk::transform_P0P1( m_ncomp, m_offset, rdof, nelem, inpoel, coord, U );
    }

    //! Limit second-order solution
    //! \param[in] t Physical time
    //! \param[in] geoFace Face geometry array
    //! \param[in] geoElem Element geometry array
    //! \param[in] fd Face connectivity and boundary conditions object
    //! \param[in] inpoel Element-node connectivity
    //! \param[in] coord Array of nodal coordinates
    //! \param[in] ndofel Vector of local number of degrees of freedome
    //! \param[in,out] U Solution vector at recent time step
    void limit( [[maybe_unused]] tk::real t,
                [[maybe_unused]] const tk::Fields& geoFace,
                [[maybe_unused]] const tk::Fields& geoElem,
                const inciter::FaceData& fd,
                const std::map< std::size_t, std::vector< std::size_t > >&,
                const std::vector< std::size_t >& inpoel,
                const tk::UnsMesh::Coords& coord,
                const std::vector< std::size_t >& ndofel,
                tk::Fields& U,
                tk::Fields& ) const
    {
      const auto limiter = g_inputdeck.get< tag::discr, tag::limiter >();

      if (limiter == ctr::LimiterType::WENOP1)
        WENO_P1( fd.Esuel(), m_offset, U );
      else if (limiter == ctr::LimiterType::SUPERBEEP1)
        Superbee_P1( fd.Esuel(), inpoel, ndofel, m_offset, coord, U );
    }

    //! Compute right hand side
    //! \param[in] t Physical time
    //! \param[in] geoFace Face geometry array
    //! \param[in] geoElem Element geometry array
    //! \param[in] fd Face connectivity and boundary conditions object
    //! \param[in] inpoel Element-node connectivity
    //! \param[in] coord Array of nodal coordinates
    //! \param[in] U Solution vector at recent time step
    //! \param[in] P Primitive vector at recent time step
    //! \param[in] ndofel Vector of local number of degrees of freedom
    //! \param[in,out] R Right-hand side vector computed
    void rhs( tk::real t,
              const tk::Fields& geoFace,
              const tk::Fields& geoElem,
              const inciter::FaceData& fd,
              const std::vector< std::size_t >& inpoel,
              const std::unordered_set< std::size_t >&,
              const tk::UnsMesh::Coords& coord,
              const tk::Fields& U,
              const tk::Fields& P,
              const std::vector< std::size_t >& ndofel,
              tk::Fields& R ) const
    {
      const auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
      const auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();

      Assert( U.nunk() == P.nunk(), "Number of unknowns in solution "
              "vector and primitive vector at recent time step incorrect" );
      Assert( U.nunk() == R.nunk(), "Number of unknowns in solution "
              "vector and right-hand side at recent time step incorrect" );
      Assert( U.nprop() == rdof*m_ncomp, "Number of components in solution "
              "vector must equal "+ std::to_string(rdof*m_ncomp) );
      Assert( P.nprop() == 0, "Number of components in primitive "
              "vector must equal "+ std::to_string(0) );
      Assert( R.nprop() == ndof*m_ncomp, "Number of components in right-hand "
              "side vector must equal "+ std::to_string(ndof*m_ncomp) );
      Assert( fd.Inpofa().size()/3 == fd.Esuf().size()/2,
              "Mismatch in inpofa size" );

      // set rhs to zero
      R.fill(0.0);

      // empty vector for non-conservative terms. This vector is unused for
      // linear transport since, there are no non-conservative terms in the
      // system of PDEs.
      std::vector< std::vector < tk::real > > riemannDeriv;

      // compute internal surface flux integrals
      tk::surfInt( m_system, 1, m_offset, ndof, rdof, inpoel, coord,
                   fd, geoFace, Upwind::flux, Problem::prescribedVelocity, U, P,
                   ndofel, R, riemannDeriv );

      if(ndof > 1)
        // compute volume integrals
        tk::volInt( m_system, m_ncomp, m_offset, ndof, fd.Esuel().size()/4,
                    inpoel, coord, geoElem, flux, Problem::prescribedVelocity,
                    U, ndofel, R );

      // compute boundary surface flux integrals
      for (const auto& b : m_bc)
        tk::bndSurfInt( m_system, 1, m_offset, ndof, rdof, b.first, fd,
          geoFace, inpoel, coord, t, Upwind::flux, Problem::prescribedVelocity,
          b.second, U, P, ndofel, R, riemannDeriv );
    }

    //! Compute the minimum time step size
//     //! \param[in] U Solution vector at recent time step
//     //! \param[in] coord Mesh node coordinates
//     //! \param[in] inpoel Mesh element connectivity
    //! \return Minimum time step size
    tk::real dt( const std::array< std::vector< tk::real >, 3 >& /*coord*/,
                 const std::vector< std::size_t >& /*inpoel*/,
                 const inciter::FaceData& /*fd*/,
                 const tk::Fields& /*geoFace*/,
                 const tk::Fields& /*geoElem*/,
                 const std::vector< std::size_t >& /*ndofel*/,
                 const tk::Fields& /*U*/,
                 const tk::Fields&,
                 const std::size_t /*nielem*/ ) const
    {
      tk::real mindt = std::numeric_limits< tk::real >::max();
      return mindt;
    }

    //! Return field names to be output to file
    //! \return Vector of strings labelling fields output in file
    //! \details This functions should be written in conjunction with
    //!   fieldOutput(), which provides the vector of fields to be output
    std::vector< std::string > fieldNames() const {
      const auto pref = g_inputdeck.get< tag::pref, tag::pref >();
      std::vector< std::string > n;
      const auto& depvar =
      g_inputdeck.get< tag::param, eq, tag::depvar >().at(m_system);
      // will output numerical solution for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        n.push_back( depvar + std::to_string(c) + "_numerical" );
      // will output analytic solution for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        n.push_back( depvar + std::to_string(c) + "_analytic" );
      // will output error for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        n.push_back( depvar + std::to_string(c) + "_error" );
      if(pref)           // Adaptive DG on
        n.push_back( "ndof" );
      return n;
    }

    //! Return field names to be output to file
    //! \return Vector of strings labelling fields output in file
    std::vector< std::string > nodalFieldNames() const
    { return fieldNames(); }

    //! Return surface field output going to file
    std::vector< std::vector< tk::real > >
    surfOutput( const std::map< int, std::vector< std::size_t > >&,
                tk::Fields& ) const
    {
      std::vector< std::vector< tk::real > > s; // punt for now
      return s;
    }

    //! Return time history field names to be output to file
    //! \return Vector of strings labelling time history fields output in file
    std::vector< std::string > histNames() const {
      std::vector< std::string > s; // punt for now
      return s;
    }

    //! Return field output going to file
    //! \param[in] t Physical time
    //! \param[in] nunk Number of unknowns to extract
    //! \param[in] vol Volumes associated to elements (or nodes)
    //! \param[in] coord Coordinates at which to evaluate the solution
    //! \param[in,out] U Solution vector at recent time step
    //! \return Vector of vectors to be output to file
    //! \details This functions should be written in conjunction with names(),
    //!   which provides the vector of field names
    //! \note U is overwritten
    std::vector< std::vector< tk::real > >
    fieldOutput( tk::real t,
                 tk::real,
                 std::size_t nunk,
                 std::size_t rdof,
                 const std::vector< tk::real >& vol,
                 const std::array< std::vector< tk::real >, 3 >& coord,
                 const tk::Fields& U,
                 [[maybe_unused]] const tk::Fields& = tk::Fields() ) const
    {
      Assert( U.nunk() >= nunk, "Size mismatch" );
      std::vector< std::vector< tk::real > > out;

      // will output numerical solution for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        out.push_back( U.extract( c*rdof, m_offset ) );

      // mesh node coordinates
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // evaluate analytic solution at time t
      auto E = U;
      for (std::size_t i=0; i<nunk; ++i)
      {
        auto s =
          Problem::solution( m_system, m_ncomp, x[i], y[i], z[i], t );
        for (ncomp_t c=0; c<m_ncomp; ++c)
          E( i, c*rdof, m_offset ) = s[c];
      }

      // will output analytic solution for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        out.push_back( E.extract( c*rdof, m_offset ) );

      // will output error for all components
      for (ncomp_t c=0; c<m_ncomp; ++c) {
        auto mark = c*rdof;
        auto u = U.extract( mark, m_offset );
        auto e = E.extract( mark, m_offset );
        for (std::size_t i=0; i<nunk; ++i)
          e[i] = std::pow( e[i] - u[i], 2.0 ) * vol[i];
        out.push_back( e );
      }

      return out;
    }

    //! Return names of integral variables to be output to diagnostics file
    //! \return Vector of strings labelling integral variables output
    std::vector< std::string > names() const {
      std::vector< std::string > n;
      const auto& depvar =
      g_inputdeck.get< tag::param, eq, tag::depvar >().at(m_system);
      // construct the name of the numerical solution for all components
      for (ncomp_t c=0; c<m_ncomp; ++c)
        n.push_back( depvar + std::to_string(c) );
      return n;
    }

    //! Return analytic solution (if defined by Problem) at xi, yi, zi, t
    //! \param[in] xi X-coordinate at which to evaluate the analytic solution
    //! \param[in] yi Y-coordinate at which to evaluate the analytic solution
    //! \param[in] zi Z-coordinate at which to evaluate the analytic solution
    //! \param[in] t Physical time at which to evaluate the analytic solution
    //! \return Vector of analytic solution at given spatial location and time
    std::vector< tk::real >
    analyticSolution( tk::real xi, tk::real yi, tk::real zi, tk::real t ) const
    {
      return Problem::solution( m_system, m_ncomp, xi, yi, zi, t );
    }

    //! Compute nodal field output
    //! \param[in] t Physical time
    //! \param[in] V Total mesh volume
    //! \param[in] coord Node coordinates
    //! \param[in] inpoel Mesh connectivity
    //! \param[in] esup Elements surrounding points
    //! \param[in] geoElem Element geometry array
    //! \param[in,out] U Solution vector at recent time step
    //! \return Vector of vectors to be output to file
    std::vector< std::vector< tk::real > >
    nodeFieldOutput( tk::real t,
                     tk::real V,
                     const tk::UnsMesh::Coords& coord,
                     const std::vector< std::size_t >& inpoel,
                     const std::pair< std::vector< std::size_t >,
                                      std::vector< std::size_t > >& esup,
                     const tk::Fields& geoElem,
                     const tk::Fields& U,
                     const tk::Fields& ) const
    {
      // Evaluate solution in nodes
      auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();
      auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
      auto [Un,Pn] =
        tk::nodeEval( m_offset, ndof, rdof, coord, inpoel, esup, U );
      // Extract nodal fields
      auto f = fieldOutput( t, V, coord[0].size(), 1, geoElem.extract(0,0),
                            coord, Un );
      return f;
    }

    //! Return time history field output evaluated at time history points
    //! \param[in] h History point data
    std::vector< std::vector< tk::real > >
    histOutput( const std::vector< HistData >& h,
                const std::vector< std::size_t >&,
                const tk::UnsMesh::Coords&,
                const tk::Fields& ) const
    {
      std::vector< std::vector< tk::real > > Up(h.size()); //punt for now
      return Up;
    }

  private:
    const Physics m_physics;            //!< Physics policy
    const Problem m_problem;            //!< Problem policy
    const ncomp_t m_system;             //!< Equation system index
    const ncomp_t m_ncomp;              //!< Number of components in this PDE
    const ncomp_t m_offset;             //!< Offset this PDE operates from
    //! BC configuration
    BCStateFn m_bc;

    //! Evaluate physical flux function for this PDE system
    //! \param[in] ncomp Number of scalar components in this PDE system
    //! \param[in] ugp Numerical solution at the Gauss point at which to
    //!   evaluate the flux
    //! \param[in] v Prescribed velocity evaluated at the Gauss point at which
    //!   to evaluate the flux
    //! \return Flux vectors for all components in this PDE system
    //! \note The function signature must follow tk::FluxFn
    static tk::FluxFn::result_type
    flux( ncomp_t,
          ncomp_t ncomp,
          const std::vector< tk::real >& ugp,
          const std::vector< std::array< tk::real, 3 > >& v )
    {
      Assert( ugp.size() == ncomp, "Size mismatch" );
      Assert( v.size() == ncomp, "Size mismatch" );

      std::vector< std::array< tk::real, 3 > > fl( ugp.size() );

      for (ncomp_t c=0; c<ncomp; ++c)
        fl[c] = {{ v[c][0] * ugp[c], v[c][1] * ugp[c], v[c][2] * ugp[c] }};

      return fl;
    }

    //! \brief Boundary state function providing the left and right state of a
    //!   face at extrapolation boundaries
    //! \param[in] ul Left (domain-internal) state
    //! \return Left and right states for all scalar components in this PDE
    //!   system
    //! \note The function signature must follow tk::StateFn
    static tk::StateFn::result_type
    extrapolate( ncomp_t, ncomp_t, const std::vector< tk::real >& ul,
                 tk::real, tk::real, tk::real, tk::real,
                 const std::array< tk::real, 3 >& )
    {
      return {{ ul, ul }};
    }

    //! \brief Boundary state function providing the left and right state of a
    //!   face at extrapolation boundaries
    //! \param[in] ul Left (domain-internal) state
    //! \return Left and right states for all scalar components in this PDE
    //!   system
    //! \note The function signature must follow tk::StateFn
    static tk::StateFn::result_type
    inlet( ncomp_t, ncomp_t, const std::vector< tk::real >& ul,
           tk::real, tk::real, tk::real, tk::real,
           const std::array< tk::real, 3 >& )
    {
      auto ur = ul;
      std::fill( begin(ur), end(ur), 0.0 );
      return {{ ul, std::move(ur) }};
    }

    //! \brief Boundary state function providing the left and right state of a
    //!   face at outlet boundaries
    //! \param[in] ul Left (domain-internal) state
    //! \return Left and right states for all scalar components in this PDE
    //!   system
    //! \note The function signature must follow tk::StateFn
    static tk::StateFn::result_type
    outlet( ncomp_t, ncomp_t, const std::vector< tk::real >& ul,
            tk::real, tk::real, tk::real, tk::real,
            const std::array< tk::real, 3 >& )
    {
      return {{ ul, ul }};
    }

    //! \brief Boundary state function providing the left and right state of a
    //!   face at Dirichlet boundaries
    //! \param[in] system Equation system index
    //! \param[in] ncomp Number of scalar components in this PDE system
    //! \param[in] ul Left (domain-internal) state
    //! \param[in] x X-coordinate at which to compute the states
    //! \param[in] y Y-coordinate at which to compute the states
    //! \param[in] z Z-coordinate at which to compute the states
    //! \param[in] t Physical time
    //! \return Left and right states for all scalar components in this PDE
    //!   system
    //! \note The function signature must follow tk::StateFn
    static tk::StateFn::result_type
    dirichlet( ncomp_t system, ncomp_t ncomp, const std::vector< tk::real >& ul,
               tk::real x, tk::real y, tk::real z, tk::real t,
               const std::array< tk::real, 3 >& )
    {
      return {{ ul, Problem::solution( system, ncomp, x, y, z, t ) }};
    }
};

} // dg::
} // inciter::

#endif // DGTransport_h
