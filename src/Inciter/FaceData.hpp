// *****************************************************************************
/*!
  \file      src/Inciter/FaceData.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \details   Face-data used only in discontinuous Galerkin discretization scheme
*/
// *****************************************************************************
#ifndef FaceData_h
#define FaceData_h

#include <vector>
#include <tuple>
#include <map>
#include <unordered_map>

#include "Types.hpp"
#include "PUPUtil.hpp"
#include "ContainerUtil.hpp"

namespace inciter {

//! Data associated to a tetrahedron cell id used to communicate across faces
using GhostData =
  std::unordered_map< std::size_t, // tet id
                      std::tuple<
                        // 3 node ids for potentially multiple faces
                        std::vector< std::size_t >,
                        // elem geometry, see tk::genGeoElemTet()
                        std::vector< tk::real >,
                        // coordinates of vertex of tet that is not on face
                        std::array< tk::real, 3 >,
                        // relative position of above vertex in inpoel
                        std::size_t,
                        // inpoel of said tet
                        std::array< std::size_t, 4 > > >;

//! FaceData class holding face-connectivity data useful for DG discretization
class FaceData {

  public:
    //! Empty constructor for Charm++
    explicit FaceData() {}

    //! \brief Constructor: compute (element-face) data for internal and
    //!   domain-boundary faces
    explicit
    FaceData( const std::vector< std::size_t >& inpoel,
              const std::map< int, std::vector< std::size_t > >& bface,
              const std::vector< std::size_t >& triinpoel );

    /** @name Accessors
      * */
    ///@{
    const std::map< int, std::vector< std::size_t > >& Bface() const
    { return m_bface; }
    const std::vector< std::size_t >& Triinpoel() const { return m_triinpoel; }
    std::size_t Nbfac() const { return tk::sumvalsize( m_bface ); }
    const std::vector< int >& Esuel() const { return m_esuel; }
    std::vector< int >& Esuel() { return m_esuel; }
    std::size_t Nipfac() const { return m_nipfac; }
    const std::vector< std::size_t >& Inpofa() const { return m_inpofa; }
    std::vector< std::size_t >& Inpofa() { return m_inpofa; }
    const std::vector< std::size_t >& Belem() const { return m_belem; }
    const std::vector< int >& Esuf() const { return m_esuf; }
    std::vector< int >& Esuf() { return m_esuf; }
    //@}

    /** @name Charm++ pack/unpack (serialization) routines
      * */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) {
      p | m_bface;
      p | m_triinpoel;
      p | m_esuel;
      p | m_nipfac;
      p | m_inpofa;
      p | m_belem;
      p | m_esuf;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i FaceData object reference
    friend void operator|( PUP::er& p, FaceData& i ) { i.pup(p); }
    //@}

  private:
    //! Boundary faces side-set information
    std::map< int, std::vector< std::size_t > > m_bface;
    //! Triangle face connecitivity
    std::vector< std::size_t > m_triinpoel;
    //! Elements surrounding elements
    std::vector< int > m_esuel;
    //! Number of internal and physical-boundary faces
    std::size_t m_nipfac;
    //! Face-node connectivity
    std::vector< std::size_t > m_inpofa;
    //! Boundary element vector
    std::vector< std::size_t > m_belem;
    //! Element surrounding faces
    std::vector< int > m_esuf;
};

} // inciter::

#endif // Discretization_h
