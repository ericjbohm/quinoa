// *****************************************************************************
/*!
  \file      src/PDE/Integrate/Mass.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Functions for computing the mass matrix for a system of PDEs
  \details   This file contains functionality for computing the mass matrix for
     a system of PDEs used in discontinuous and continuous Galerkin methods for
     various orders of numerical representation.
*/
// *****************************************************************************

#include "Mass.hpp"
#include "Vector.hpp"

void
tk::mass( ncomp_t ncomp,
          ncomp_t offset,
          ncomp_t ndof,
          const Fields& geoElem,
          Fields& l )
// *****************************************************************************
//  Compute the block-diagonal mass matrix for DG
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] offset Offset this PDE system operates from
//! \param[in] ndof Number of solution degrees of freedom
//! \param[in] geoElem Element geometry array
//! \param[in,out] l Block diagonal mass matrix
// *****************************************************************************
{
  Assert( geoElem.nunk() == l.nunk(), "Size mismatch" );
  const auto nelem = geoElem.nunk();
  Assert( l.nprop() == ndof*ncomp, "lhs mass matrix size incorrect" );

  // Compute LHS for DG(P0)
  for (std::size_t e=0; e<nelem; ++e)
    for (ncomp_t c=0; c<ncomp; ++c)
      l(e, c*ndof, offset) = geoElem(e,0,0);

  // Augment LHS for DG(P1)
  if (ndof > 1) {
    for (std::size_t e=0; e<nelem; ++e) {
      for (ncomp_t c=0; c<ncomp; ++c) {
        const auto mark = c * ndof;
        l(e, mark+1, offset) = geoElem(e,0,0) / 10.0;
        l(e, mark+2, offset) = geoElem(e,0,0) * 3.0/10.0;
        l(e, mark+3, offset) = geoElem(e,0,0) * 3.0/5.0;
      }
    }
  }

  // Augment LHS for DG(P2)
  if (ndof > 4){
    for (std::size_t e=0; e<nelem; ++e) {
      for (ncomp_t c=0; c<ncomp; ++c) {
        const auto mark = c * ndof;
        l(e, mark+4, offset) = geoElem(e,0,0) / 35.0;
        l(e, mark+5, offset) = geoElem(e,0,0) / 21.0;
        l(e, mark+6, offset) = geoElem(e,0,0) / 14.0;
        l(e, mark+7, offset) = geoElem(e,0,0) / 7.0;
        l(e, mark+8, offset) = geoElem(e,0,0) * 3.0/14.0;
        l(e, mark+9, offset) = geoElem(e,0,0) * 3.0/7.0;
      }
    }
  }
}

tk::Fields
tk::lump( ncomp_t ncomp,
          const std::array< std::vector< tk::real >, 3 >& coord,
          const std::vector< std::size_t >& inpoel )
// *****************************************************************************
//  Compute lumped mass matrix for CG
//! \param[in] ncomp Total number of scalar components in the eq system
//! \param[in] coord Mesh node coordinates
//! \param[in] inpoel Mesh element connectivity
//! \return Lumped mass matrix
// *****************************************************************************
{
  const auto& x = coord[0];
  const auto& y = coord[1];
  const auto& z = coord[2];

  tk::Fields L( coord[0].size(), ncomp );
  L.fill( 0.0 );

  for (std::size_t e=0; e<inpoel.size()/4; ++e) {
    const std::array< std::size_t, 4 > N{{ inpoel[e*4+0], inpoel[e*4+1],
                                           inpoel[e*4+2], inpoel[e*4+3] }};
    // compute element Jacobi determinant
    const std::array< tk::real, 3 >
      ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
      ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
      da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
    const auto J = tk::triple( ba, ca, da ) * 5.0 / 120.0;
    Assert( J > 0, "Element Jacobian non-positive" );

    // access pointer to lumped mass left hand side at element nodes
    std::vector< const tk::real* > l( ncomp );
    for (ncomp_t c=0; c<ncomp; ++c) l[c] = L.cptr( c, 0 );

    // scatter-add lumped mass element contributions to lhs nodes
    for (ncomp_t c=0; c<ncomp; ++c)
      for (std::size_t j=0; j<4; ++j)
        L.var(l[c],N[j]) += J;
  }

  return L;
}
