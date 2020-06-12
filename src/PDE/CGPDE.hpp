// *****************************************************************************
/*!
  \file      src/PDE/CGPDE.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Partial differential equation base for continuous Galerkin PDEs
  \details   This file defines a generic partial differential equation (PDE)
    class for PDEs that use continuous Galerkin spatial discretization.
    The class uses runtime polymorphism without client-side inheritance:
    inheritance is confined to the internals of the class, invisible to
    client-code. The class exclusively deals with ownership enabling client-side
    value semantics. Credit goes to Sean Parent at Adobe:
    https://github.com/sean-parent/sean-parent.github.com/wiki/
    Papers-and-Presentations.
*/
// *****************************************************************************
#ifndef CGPDE_h
#define CGPDE_h

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "Types.hpp"
#include "Fields.hpp"
#include "UnsMesh.hpp"
#include "FunctionPrototypes.hpp"
#include "Mesh/CommMap.hpp"
#include "History.hpp"

namespace inciter {

namespace cg {

using ncomp_t = kw::ncomp::info::expect::type;

//! \brief Evaluate the increment from t to t+dt of an analytical solution at
//!   (x,y,z) for all components
std::vector< tk::real >
solinc( tk::ncomp_t system, tk::ncomp_t ncomp, tk::real x, tk::real y,
        tk::real z, tk::real t, tk::real dt, tk::SolutionFn solution );

} // cg::

//! \brief Partial differential equation base for continuous Galerkin PDEs
//! \details This class uses runtime polymorphism without client-side
//!   inheritance: inheritance is confined to the internals of the this class,
//!   invisible to client-code. The class exclusively deals with ownership
//!   enabling client-side value semantics. Credit goes to Sean Parent at Adobe:
//!   https://github.com/sean-parent/sean-parent.github.com/wiki/
//!   Papers-and-Presentations. For example client code that models a CGPDE,
//!   see inciter::CompFlow.
class CGPDE {

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Default constructor taking no arguments for Charm++
    explicit CGPDE() = default;

    //! Constructor taking an object modeling Concept.
    //! \details The object of class T comes pre-constructed.
    //! \param[in] x Instantiated object of type T given by the template
    //!   argument.
    template< typename T > explicit CGPDE( T x ) :
      self( std::make_unique< Model<T> >( std::move(x) ) ) {}

    //! \brief Constructor taking a function pointer to a constructor of an
    //!   object modeling Concept.
    //! \details Passing std::function allows late execution of the constructor,
    //!   i.e., as late as inside this class' constructor, and thus usage from
    //!   a factory. Note that there are at least two different ways of using
    //!   this constructor:
    //!   - Bind T's constructor arguments and place it in std::function<T()>
    //!   and passing no arguments as args.... This case then instantiates the
    //!   model via its constructor and stores it in here.
    //!   - Bind a single placeholder argument to T's constructor and pass it in
    //!   as host's args..., which then forwards it to model's constructor. This
    //!   allows late binding, i.e., binding the argument only here.
    //! \see See also the wrapper tk::recordModel() which does the former and
    //!   tk::recordModelLate() which does the latter, both defined in
    //!   src/Base/Factory.h.
    //! \param[in] x Function pointer to a constructor of an object modeling
    //!    Concept.
    //! \param[in] args Zero or more constructor arguments
    template< typename T, typename...Args >
    explicit CGPDE( std::function<T(Args...)> x, Args&&... args ) :
      self( std::make_unique< Model<T> >(
              std::move( x( std::forward<Args>(args)... ) ) ) ) {}

    //! Public interface to setting the initial conditions for the diff eq
    void initialize( const std::array< std::vector< tk::real >, 3 >& coord,
                     tk::Fields& unk,
                     tk::real t,
                     std::vector< std::size_t >& inbox )
    { self->initialize( coord, unk, t, inbox ); }

    //! Public interface to updating the initial conditions in box ICs
    void box( tk::real v, const std::vector< std::size_t >& boxnodes,
              tk::Fields& unk ) const { self->box( v, boxnodes, unk ); }

    //! Public interface to computing the nodal gradients for ALECG
    void grad( const std::array< std::vector< tk::real >, 3 >& coord,
               const std::vector< std::size_t >& inpoel,
               const std::vector< std::size_t >& bndel,
               const std::vector< std::size_t >& gid,
               const std::unordered_map< std::size_t, std::size_t >& bid,
               const tk::Fields& U,
               tk::Fields& G ) const
    { self->grad( coord, inpoel, bndel, gid, bid, U, G ); }

    //! Public interface to computing the right-hand side vector for DiagCG
    void rhs( tk::real t,
              tk::real deltat,
              const std::array< std::vector< tk::real >, 3 >& coord,
              const std::vector< std::size_t >& inpoel,
              const tk::Fields& U,
              tk::Fields& Ue,
              tk::Fields& R ) const
    { self->rhs( t, deltat, coord, inpoel, U, Ue, R ); }

    //! Public interface to computing the right-hand side vector for ALECG
    void rhs(
      tk::real t,
      const std::array< std::vector< tk::real >, 3 >& coord,
      const std::vector< std::size_t >& inpoel,
      const std::vector< std::size_t >& triinpoel,
      const std::vector< std::size_t >& gid,
      const std::unordered_map< std::size_t, std::size_t >& bid,
      const std::unordered_map< std::size_t, std::size_t >& lid,
      const std::vector< tk::real >& dfn,
      const std::pair< std::vector< std::size_t >,
                       std::vector< std::size_t > >& psup,
      const std::pair< std::vector< std::size_t >,
                       std::vector< std::size_t > >& esup,
      const std::vector< int >& symbcnode,
      const std::vector< tk::real >& vol,
      const std::vector< std::size_t >& edgenode,
      const std::vector< std::size_t >& edgeid,
      const tk::Fields& G,
      const tk::Fields& U,
      const std::vector< tk::real >& tp,
      tk::Fields& R ) const
    { self->rhs( t, coord, inpoel, triinpoel, gid, bid, lid, dfn, psup, esup,
                 symbcnode, vol, edgenode, edgeid, G, U, tp, R ); }

    //! Public interface for computing the minimum time step size
    tk::real dt( const std::array< std::vector< tk::real >, 3 >& coord,
                 const std::vector< std::size_t >& inpoel,
                 const tk::Fields& U ) const
    { return self->dt( coord, inpoel, U ); }

    //! Public interface for computing a time step size for each mesh node
    void dt( uint64_t it,
             const std::vector< tk::real >& vol,
             const tk::Fields& U,
             std::vector< tk::real >& dtp ) const
    { self->dt( it, vol, U, dtp ); }

    //! \brief Public interface for querying Dirichlet boundary condition values
    //!  set by the user on a given side set for all components in a PDE system
    std::map< std::size_t, std::vector< std::pair<bool,tk::real> > >
    dirbc( tk::real t,
           tk::real deltat,
           const std::vector< tk::real >& tp,
           const std::vector< tk::real >& dtp,
           const std::pair< const int, std::vector< std::size_t > >& sides,
           const std::array< std::vector< tk::real >, 3 >& coord ) const
    { return self->dirbc( t, deltat, tp, dtp, sides, coord ); }

    //! Public interface to set symmetry boundary conditions at nodes
    void
    symbc( tk::Fields& U,
           const std::unordered_map<std::size_t,std::array<tk::real,4>>& bnorm )
    const { self->symbc( U, bnorm ); }

    //! Public interface to querying symmetry boundary nodes
    void
    symbcnodes( const std::map< int, std::vector< std::size_t > >& bface,
                const std::vector< std::size_t >& triinpoel,
                std::unordered_set< std::size_t >& nodes ) const
    { self->symbcnodes( bface, triinpoel, nodes ); }

    //! Public interface to returning field output labels
    std::vector< std::string > fieldNames() const { return self->fieldNames(); }

    //! Public interface to returning surface field output labels
    std::vector< std::string > surfNames() const { return self->surfNames(); }

    //! Public interface to returning time history field output labels
    std::vector< std::string > histNames() const { return self->histNames(); }

    //! Public interface to returning variable names
    std::vector< std::string > names() const { return self->names(); }

    //! Public interface to returning field output
    std::vector< std::vector< tk::real > > fieldOutput(
      tk::real t,
      tk::real V,
      std::size_t nunk,
      const std::array< std::vector< tk::real >, 3 >& coord,
      const std::vector< tk::real >& v,
      tk::Fields& U ) const
    { return self->fieldOutput( t, V, nunk, coord, v, U ); }

    //! Public interface to returning surface field output
    std::vector< std::vector< tk::real > >
    surfOutput( const std::map< int, std::vector< std::size_t > >& bnd,
                tk::Fields& U ) const
    { return self->surfOutput( bnd, U ); }

    //! Public interface to returning time history output
    std::vector< std::vector< tk::real > >
    histOutput( const std::vector< HistData >& h,
                const std::vector< std::size_t >& inpoel,
                const tk::Fields& U ) const
    { return self->histOutput( h, inpoel, U ); }

    //! Public interface to returning analytic solution
    std::vector< tk::real >
    analyticSolution( tk::real xi, tk::real yi, tk::real zi, tk::real t ) const
    { return self->analyticSolution( xi, yi, zi, t ); }

    //! Copy assignment
    CGPDE& operator=( const CGPDE& x )
    { CGPDE tmp(x); *this = std::move(tmp); return *this; }
    //! Copy constructor
    CGPDE( const CGPDE& x ) : self( x.self->copy() ) {}
    //! Move assignment
    CGPDE& operator=( CGPDE&& ) noexcept = default;
    //! Move constructor
    CGPDE( CGPDE&& ) noexcept = default;

  private:
    //! \brief Concept is a pure virtual base class specifying the requirements
    //!   of polymorphic objects deriving from it
    struct Concept {
      Concept() = default;
      Concept( const Concept& ) = default;
      virtual ~Concept() = default;
      virtual Concept* copy() const = 0;
      virtual void initialize( const std::array< std::vector< tk::real >, 3 >&,
                               tk::Fields&,
                               tk::real,
                               std::vector< std::size_t >& inbox ) = 0;
      virtual void box( tk::real, const std::vector< std::size_t >&,
                        tk::Fields& unk ) const = 0;
      virtual void grad( const std::array< std::vector< tk::real >, 3 >&,
                         const std::vector< std::size_t >&,
                         const std::vector< std::size_t >&,
                         const std::vector< std::size_t >&,
                         const std::unordered_map< std::size_t, std::size_t >&,
                         const tk::Fields&,
                         tk::Fields& ) const = 0;
      virtual void rhs( tk::real,
                        tk::real,
                        const std::array< std::vector< tk::real >, 3 >&,
                        const std::vector< std::size_t >&,
                        const tk::Fields&,
                        tk::Fields&,
                        tk::Fields& ) const = 0;
      virtual void rhs(
        tk::real,
        const std::array< std::vector< tk::real >, 3 >&,
        const std::vector< std::size_t >&,
        const std::vector< std::size_t >&,
        const std::vector< std::size_t >&,
        const std::unordered_map< std::size_t, std::size_t >&,
        const std::unordered_map< std::size_t, std::size_t >&,
        const std::vector< tk::real >&,
        const std::pair< std::vector< std::size_t >,
                         std::vector< std::size_t > >&,
        const std::pair< std::vector< std::size_t >,
                         std::vector< std::size_t > >&,
        const std::vector< int >&,
        const std::vector< tk::real >&,
        const std::vector< std::size_t >&,
        const std::vector< std::size_t >&,
        const tk::Fields&,
        const tk::Fields&,
        const std::vector< tk::real >&,
        tk::Fields& ) const = 0;
      virtual tk::real dt( const std::array< std::vector< tk::real >, 3 >&,
                           const std::vector< std::size_t >&,
                           const tk::Fields& ) const = 0;
      virtual void dt( uint64_t,
                       const std::vector< tk::real > &,
                       const tk::Fields&,
                       std::vector< tk::real >& ) const = 0;
      virtual std::map< std::size_t, std::vector< std::pair<bool,tk::real> > >
      dirbc( tk::real,
             tk::real,
             const std::vector< tk::real >&,
             const std::vector< tk::real >&,
             const std::pair< const int, std::vector< std::size_t > >&,
             const std::array< std::vector< tk::real >, 3 >& ) const = 0;
      virtual void symbc( tk::Fields& U,
         const std::unordered_map< std::size_t, std::array< tk::real, 4 > >& )
         const = 0;
      virtual void symbcnodes(
         const std::map< int, std::vector< std::size_t > >&,
         const std::vector< std::size_t >&,
         std::unordered_set< std::size_t >& ) const = 0;
      virtual std::vector< std::string > fieldNames() const = 0;
      virtual std::vector< std::string > surfNames() const = 0;
      virtual std::vector< std::string > histNames() const = 0;
      virtual std::vector< std::string > names() const = 0;
      virtual std::vector< std::vector< tk::real > > fieldOutput(
        tk::real,
        tk::real,
        std::size_t,
        const std::array< std::vector< tk::real >, 3 >&,
        const std::vector< tk::real >&,
        tk::Fields& ) const = 0;
      virtual std::vector< std::vector< tk::real > > surfOutput(
        const std::map< int, std::vector< std::size_t > >&,
        tk::Fields& ) const = 0;
      virtual std::vector< std::vector< tk::real > > histOutput(
        const std::vector< HistData >&,
        const std::vector< std::size_t >&,
        const tk::Fields& ) const = 0;
      virtual std::vector< tk::real > analyticSolution(
        tk::real xi, tk::real yi, tk::real zi, tk::real t ) const = 0;
    };

    //! \brief Model models the Concept above by deriving from it and overriding
    //!   the virtual functions required by Concept
    template< typename T >
    struct Model : Concept {
      explicit Model( T x ) : data( std::move(x) ) {}
      Concept* copy() const override { return new Model( *this ); }
      void initialize( const std::array< std::vector< tk::real >, 3 >& coord,
                       tk::Fields& unk,
                       tk::real t,
                       std::vector< std::size_t >& inbox )
      override { data.initialize( coord, unk, t, inbox ); }
      void box( tk::real v, const std::vector< std::size_t >& boxnodes,
                tk::Fields& unk ) const override
      { data.box( v, boxnodes, unk ); }
      void grad( const std::array< std::vector< tk::real >, 3 >& coord,
                 const std::vector< std::size_t >& inpoel,
                 const std::vector< std::size_t >& bndel,
                 const std::vector< std::size_t >& gid,
                 const std::unordered_map< std::size_t, std::size_t >& bid,
                 const tk::Fields& U,
                 tk::Fields& G ) const override
      { data.grad( coord, inpoel, bndel, gid, bid, U, G ); }
      void rhs( tk::real t,
                tk::real deltat,
                const std::array< std::vector< tk::real >, 3 >& coord,
                const std::vector< std::size_t >& inpoel,
                const tk::Fields& U,
                tk::Fields& Ue,
                tk::Fields& R ) const override
      { data.rhs( t, deltat, coord, inpoel, U, Ue, R ); }
      void rhs(
        tk::real t,
        const std::array< std::vector< tk::real >, 3 >& coord,
        const std::vector< std::size_t >& inpoel,
        const std::vector< std::size_t >& triinpoel,
        const std::vector< std::size_t >& gid,
        const std::unordered_map< std::size_t, std::size_t >& bid,
        const std::unordered_map< std::size_t, std::size_t >& lid,
        const std::vector< tk::real >& dfn,
        const std::pair< std::vector< std::size_t >,
                         std::vector< std::size_t > >& psup,
        const std::pair< std::vector< std::size_t >,
                         std::vector< std::size_t > >& esup,
        const std::vector< int >& symbcnode,
        const std::vector< tk::real >& vol,
        const std::vector< std::size_t >& edgenode,
        const std::vector< std::size_t >& edgeid,
        const tk::Fields& G,
        const tk::Fields& U,
        const std::vector< tk::real >& tp,
        tk::Fields& R ) const override
      { data.rhs( t, coord, inpoel, triinpoel, gid, bid, lid, dfn, psup, esup,
                  symbcnode, vol, edgenode, edgeid, G, U, tp, R ); }
      tk::real dt( const std::array< std::vector< tk::real >, 3 >& coord,
                   const std::vector< std::size_t >& inpoel,
                   const tk::Fields& U ) const override
      { return data.dt( coord, inpoel, U ); }
      void dt( uint64_t it,
               const std::vector< tk::real > & vol,
               const tk::Fields& U,
               std::vector< tk::real >& dtp ) const override
      { data.dt( it, vol, U, dtp ); }
      std::map< std::size_t, std::vector< std::pair<bool,tk::real> > >
      dirbc( tk::real t,
             tk::real deltat,
             const std::vector< tk::real >& tp,
             const std::vector< tk::real >& dtp,
             const std::pair< const int, std::vector< std::size_t > >& sides,
             const std::array< std::vector< tk::real >, 3 >& coord ) const
        override { return data.dirbc( t, deltat, tp, dtp, sides, coord ); }
      void symbc( tk::Fields& U,
        const std::unordered_map<std::size_t,std::array<tk::real,4>>& bnorm )
        const override { data.symbc( U, bnorm ); }
      void symbcnodes(
         const std::map< int, std::vector< std::size_t > >& bface,
         const std::vector< std::size_t >& triinpoel,
         std::unordered_set< std::size_t >& nodes ) const override
      { data.symbcnodes( bface, triinpoel, nodes ); }
      std::vector< std::string > fieldNames() const override
      { return data.fieldNames(); }
      std::vector< std::string > surfNames() const override
      { return data.surfNames(); }
      std::vector< std::string > histNames() const override
      { return data.histNames(); }
      std::vector< std::string > names() const override
      { return data.names(); }
      std::vector< std::vector< tk::real > > fieldOutput(
        tk::real t,
        tk::real V,
        std::size_t nunk,
        const std::array< std::vector< tk::real >, 3 >& coord,
        const std::vector< tk::real >& v,
        tk::Fields& U ) const override
      { return data.fieldOutput( t, V, nunk, coord, v, U ); }
      std::vector< std::vector< tk::real > > surfOutput(
        const std::map< int, std::vector< std::size_t > >& bnd,
        tk::Fields& U ) const override
      { return data.surfOutput( bnd, U ); }
      std::vector< std::vector< tk::real > > histOutput(
        const std::vector< HistData >& h,
        const std::vector< std::size_t >& inpoel,
        const tk::Fields& U ) const override
      { return data.histOutput( h, inpoel, U ); }
      std::vector< tk::real >
      analyticSolution( tk::real xi, tk::real yi, tk::real zi, tk::real t )
       const override { return data.analyticSolution( xi, yi, zi, t ); }
      T data;
    };

    std::unique_ptr< Concept > self;    //!< Base pointer used polymorphically
};

} // inciter::

#endif // CGPDE_h
