// *****************************************************************************
/*!
  \file      src/Inciter/Discretization.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \details   Data and functionality common to all discretization schemes
     The Discretization class contains data and functionality common to all
     discretization schemes.
*/
// *****************************************************************************
#ifndef Discretization_h
#define Discretization_h

#include <brigand/algorithms/for_each.hpp>

#include "Types.hpp"
#include "Timer.hpp"
#include "Keywords.hpp"
#include "Fields.hpp"
#include "PUPUtil.hpp"
#include "PDFReducer.hpp"
#include "UnsMesh.hpp"
#include "CommMap.hpp"
#include "History.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

#include "NoWarning/discretization.decl.h"
#include "NoWarning/refiner.decl.h"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

//! \brief Discretization Charm++ chare array holding common functinoality to
//!   all discretization schemes
class Discretization : public CBase_Discretization {

  public:
    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wunused-parameter"
      #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wunused-parameter"
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__INTEL_COMPILER)
      #pragma warning( push )
      #pragma warning( disable: 1478 )
    #endif
    // Include Charm++ SDAG code. See http://charm.cs.illinois.edu/manuals/html/
    // charm++/manual.html, Sec. "Structured Control Flow: Structured Dagger".
    Discretization_SDAG_CODE
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic pop
    #elif defined(__INTEL_COMPILER)
      #pragma warning( pop )
    #endif

    //! Constructor
    explicit
      Discretization(
        const CProxy_DistFCT& fctproxy,
        const CProxy_Transporter& transporter,
        const tk::CProxy_MeshWriter& meshwriter,
        const std::vector< std::size_t >& ginpoel,
        const tk::UnsMesh::CoordMap& coordmap,
        const tk::CommMaps& msum,
        int nc );

    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
    #endif
    //! Migrate constructor
    // cppcheck-suppress uninitMemberVar
    explicit Discretization( CkMigrateMessage* ) {}
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #endif

    //! Configure Charm++ reduction types
    static void registerReducers();

    //! Resize mesh data structures (e.g., after mesh refinement)
    void resizePostAMR( const tk::UnsMesh::Chunk& chunk,
                        const tk::UnsMesh::Coords& coord,
                        const tk::NodeCommMap& nodeCommMap );

    //! Get ready for (re-)computing/communicating nodal volumes
    void startvol();

    //! Sum mesh volumes to nodes, start communicating them on chare-boundaries
    void vol();

    //! Set Refiner Charm++ proxy
    void setRefiner( const CProxy_Refiner& ref );

    //! Collect nodal volumes across chare boundaries
    void comvol( const std::vector< std::size_t >& gid,
                 const std::vector< tk::real >& nodevol );

    //! Sum mesh volumes and contribute own mesh volume to total volume
    void totalvol();

    //! Compute mesh cell statistics
    void stat( tk::real mesh_volume );

    //! Compute total box IC volume
    void boxvol( const std::unordered_set< std::size_t >& boxnodes );

    /** @name Accessors */
    ///@{
    //! Coordinates accessors as const-ref
    const tk::UnsMesh::Coords& Coord() const{ return m_coord; }

    //! Global ids accessors as const-ref
    const std::vector< std::size_t >& Gid() const { return m_gid; }

    //! Local ids accessors as const-ref
     const std::unordered_map< std::size_t, std::size_t >& Lid() const
    { return m_lid; }

    //! Tetrahedron element connectivity (with local ids) accessors as const-ref
    const std::vector< std::size_t >& Inpoel() const { return m_inpoel; }

    //! Mesh chunk accessor as const-ref
    const tk::UnsMesh::Chunk& Chunk() const { return m_el; }

    //! Total mesh volume accessor
    tk::real meshvol() const { return m_meshvol; }

    //! Nodal mesh volume accessors const-ref
    const std::vector< tk::real >& V() const { return m_v; }

    //! Nodal mesh volumes accessors as const-ref
    const std::vector< tk::real >& Vol() const { return m_vol; }

    //! History points data accessor as const-ref
    const std::vector< HistData >& Hist() const { return m_histdata; }

    //! Box volume accessor
    tk::real& Boxvol() { return m_boxvol; }

    //! Time step size accessor
    tk::real Dt() const { return m_dt; }
    //! Physical time accessor
    tk::real T() const { return m_t; }
    //! Iteration count accessor
    uint64_t It() const { return m_it; }

    //! Non-const-ref refinement iteration count accessor
    uint64_t& Itr() { return m_itr; }
    //! Non-const-ref field-output iteration count accessor
    uint64_t& Itf() { return m_itf; }

    //! Non-const-ref number of restarts accessor
    int& Nrestart() { return m_nrestart; }

    //! Timer accessor as const-ref
    const tk::Timer& Timer() const { return m_timer; }
    //! Timer accessor as non-const-ref
    tk::Timer& Timer() { return m_timer; }

    //! Accessor to flag indicating if the mesh was refined as a value
    int refined() const { return m_refined; }
    //! Accessor to flag indicating if the mesh was refined as non-const-ref
    int& refined() { return m_refined; }

    //! Transporter proxy accessor as const-ref
    const CProxy_Transporter& Tr() const { return m_transporter; }
    //! Transporter proxy accessor as non-const-ref
    CProxy_Transporter& Tr() { return m_transporter; }

    //! Access bound Refiner class pointer
    Refiner* Ref() const {
      Assert( m_refiner[ thisIndex ].ckLocal() != nullptr,
              "Refiner ckLocal() null" );
      return m_refiner[ thisIndex ].ckLocal();
    }

    //! Access bound DistFCT class pointer
    DistFCT* FCT() const {
      Assert(m_fct[ thisIndex ].ckLocal() != nullptr, "DistFCT ckLocal() null");
      return m_fct[ thisIndex ].ckLocal();
    }

    //! Boundary node ids accessor as const-ref
    const std::unordered_map< std::size_t, std::size_t >& Bid() const
    { return m_bid; }

    //! Node communication map accessor as const-ref
    const tk::NodeCommMap& NodeCommMap() const { return m_nodeCommMap; }

    //! Edge communication map accessor as const-ref
    const tk::EdgeCommMap& EdgeCommMap() const { return m_edgeCommMap; }
    //@}

    //! Set time step size
    void setdt( tk::real newdt );

    //! Prepare for next step
    void next();

    //! Otput one-liner status report
    void status();

    //! Construct history output filename
    std::string histfilename( const std::string& id,
                              kw::precision::info::expect::type precision );

    //! Output headers for time history files (one for each point)
    void histheader( std::vector< std::string >&& names );

    //! Output time history for a time step
    void history( std::vector< std::vector< tk::real > >&& data );

    //! Output mesh and fields data (solution dump) to file(s)
    void write( const std::vector< std::size_t >& inpoel,
                const tk::UnsMesh::Coords& coord,
                const std::map< int, std::vector< std::size_t > >& bface,
                const std::map< int, std::vector< std::size_t > >& bnode,
                const std::vector< std::size_t >& triinpoel,
                const std::vector< std::string>& elemfieldnames,
                const std::vector< std::string>& nodefieldnames,
                const std::vector< std::string>& nodesurfnames,
                const std::vector< std::vector< tk::real > >& elemfields,
                const std::vector< std::vector< tk::real > >& nodefields,
                const std::vector< std::vector< tk::real > >& nodesurfs,
                CkCallback c );

    //! Zero grind-timer
    void grindZero();

    //! Detect if just returned from a checkpoint and if so, zero timers
    bool restarted( int nrestart );

    //! Remap mesh data due to new local ids
    void remap( const std::unordered_map< std::size_t, std::size_t >& map );

    //! \brief Function object for querying the node ids at which a particular
    //!   BCType BC is configured by the user, called for each PDE type
    template< typename BCType >
    struct BCNodes {

      const std::map< int, std::vector< std::size_t > >& m_bface;
      const std::vector< std::size_t >& m_triinpoel;
      std::unordered_map< int, std::unordered_set< std::size_t > >& m_nodes;

      explicit
        BCNodes( const std::map< int, std::vector< std::size_t > >& bface,
                 const std::vector< std::size_t >& triinpoel,
                 std::unordered_map< int,
                   std::unordered_set< std::size_t > >& nodes )
        : m_bface(bface), m_triinpoel(triinpoel), m_nodes(nodes) {}

      template< typename Eq > void operator()( brigand::type_<Eq> ) {
        const auto& bc =
          g_inputdeck.template get< tag::param, Eq, tag::bc, BCType >();
        for (const auto& eq : bc) {
          for (const auto& s : eq) {
            auto k = m_bface.find( std::stoi(s) );
            if (k != end(m_bface)) {
              auto& n = m_nodes[ k->first ];  // associate set id
              for (auto f : k->second) {      // face ids on BCType side set
                n.insert( m_triinpoel[f*3+0] );
                n.insert( m_triinpoel[f*3+1] );
                n.insert( m_triinpoel[f*3+2] );
              }
            }
          }
        }
      }
    };

    //! \brief Query nodes at which BCType boundary conditions are set for all
    //!   PDE types
    //! \tparam BCType Boundary condition type
    //! \param[in] bface Boundary-faces mapped to side set ids
    //! \param[in] triinpoel Boundary-face connectivity
    //! \return Node ids at which BCType BCs are set (value),
    //!    associated to sides set id on which the BC is set (key)
    template< typename BCType >
    std::unordered_map< int, std::unordered_set< std::size_t > >
    bcnodes( const std::map< int, std::vector< std::size_t > >& bface,
             const std::vector< std::size_t >& triinpoel ) const
    {
      using PDETypes = ctr::parameters::Keys;
      std::unordered_map< int, std::unordered_set< std::size_t > > nodes;
      brigand::for_each< PDETypes >(
        BCNodes< BCType >( bface, triinpoel, nodes ) );
      return nodes;
    }

    //! Find elements along our mesh chunk boundary
    std::vector< std::size_t > bndel() const;

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) override {
      p | m_nchare;
      p | m_it;
      p | m_itr;
      p | m_itf;
      p | m_initial;
      p | m_t;
      p | m_lastDumpTime;
      p | m_dt;
      p | m_nvol;
      p | m_fct;
      p | m_transporter;
      p | m_meshwriter;
      p | m_refiner;
      p | m_el;
      if (p.isUnpacking()) {
        m_inpoel = std::get< 0 >( m_el );
        m_gid = std::get< 1 >( m_el );
        m_lid = std::get< 2 >( m_el );
      }
      p | m_coord;
      p | m_nodeCommMap;
      p | m_edgeCommMap;
      p | m_meshvol;
      p | m_v;
      p | m_vol;
      p | m_volc;
      p | m_boxvol;
      p | m_bid;
      p | m_timer;
      p | m_refined;
      p( reinterpret_cast<char*>(&m_prevstatus), sizeof(Clock::time_point) );
      p | m_nrestart;
      p | m_histdata;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i Discretization object reference
    friend void operator|( PUP::er& p, Discretization& i ) { i.pup(p); }
    //@}

  private:
    // Shorthand for clock, setting an internal clock type
    using Clock = std::chrono::high_resolution_clock;

    //! Total number of Discretization chares
    int m_nchare;
    //! Iteration count
    uint64_t m_it;
    //! Iteration count with mesh refinement
    //! \details Used as the restart sequence number {RS} in saving output in
    //!    an ExodusII sequence
    //! \see https://www.paraview.org/Wiki/Restarted_Simulation_Readers
    uint64_t m_itr;
    //! Field output iteration count without mesh refinement
    //! \details Counts the number of field outputs to file during two
    //!   time steps with mesh efinement
    uint64_t m_itf;
    //! Flag that is nonzero during setup and zero during time stepping
    tk::real m_initial;
    //! Physical time
    tk::real m_t;
    //! Physical time at last field output
    tk::real m_lastDumpTime;
    //! Physical time step size
    tk::real m_dt;
    //! \brief Number of chares from which we received nodal volume
    //!   contributions on chare boundaries
    std::size_t m_nvol;
    //! Distributed FCT proxy
    CProxy_DistFCT m_fct;
    //! Transporter proxy
    CProxy_Transporter m_transporter;
    //! Mesh writer proxy
    tk::CProxy_MeshWriter m_meshwriter;
    //! Mesh refiner proxy
    CProxy_Refiner m_refiner;
    //! \brief Elements of the mesh chunk we operate on
    //! \details Initialized by the constructor. The first vector is the element
    //!   connectivity (local IDs), the second vector is the global node IDs of
    //!   owned elements, while the third one is a map of global->local node
    //!   IDs.
    tk::UnsMesh::Chunk m_el;
    //! Alias to element connectivity
    std::vector< std::size_t >& m_inpoel = std::get< 0 >( m_el );
    //! Alias to global node IDs of owned elements
    std::vector< std::size_t >& m_gid = std::get< 1 >( m_el );
    //! \brief Alias to local node ids associated to the global ones of owned
    //!    elements
    std::unordered_map< std::size_t, std::size_t >& m_lid = std::get< 2 >( m_el );
    //! Mesh point coordinates
    tk::UnsMesh::Coords m_coord;
    //! \brief Global mesh node IDs bordering the mesh chunk held by fellow
    //!   Discretization chares associated to their chare IDs
    tk::NodeCommMap m_nodeCommMap;
    //! \brief Edges with global node IDs bordering the mesh chunk held by
    //!   fellow Discretization chares associated to their chare IDs
    tk::EdgeCommMap m_edgeCommMap;
    //! Total mesh volume
    tk::real m_meshvol;
    //! Nodal mesh volumes
    //! \details This is the volume of the mesh associated to nodes of owned
    //!   elements (sum of surrounding cell volumes / 4) without contributions
    //!   from other chares on chare-boundaries
    std::vector< tk::real > m_v;
    //! Volume of nodes
    //! \details This is the volume of the mesh associated to nodes of owned
    //!   elements (sum of surrounding cell volumes / 4) with contributions from
    //!   other chares on chare-boundaries
    std::vector< tk::real > m_vol;
    //! Receive buffer for volume of nodes (with global node id as key)
    //! \details This is a communication buffer used to compute the volume of
    //!   the mesh associated to nodes of owned elements (sum of surrounding
    //!   cell volumes / 4) with contributions from other chares on
    //!   chare-boundaries.
    std::unordered_map< std::size_t, tk::real > m_volc;
    //! Volume of user-defined box IC
    tk::real m_boxvol;
    //! \brief Local chare-boundary mesh node IDs at which we receive
    //!   contributions associated to global mesh node IDs of elements we
    //!   contribute to
    std::unordered_map< std::size_t, std::size_t > m_bid;
    //! Timer measuring a time step
    tk::Timer m_timer;
    //! 1 if mesh was refined in a time step, 0 if it was not
    int m_refined;
    //! Time point storing clock state at status()
    Clock::time_point m_prevstatus;
    //! Number of times restarted
    int m_nrestart;
    //! Data at history point locations
    std::vector< HistData > m_histdata;

    //! Set mesh coordinates based on coordinates map
    tk::UnsMesh::Coords setCoord( const tk::UnsMesh::CoordMap& coordmap );
};

} // inciter::

#endif // Discretization_h
