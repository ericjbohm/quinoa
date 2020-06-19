// *****************************************************************************
/*!
  \file      src/PDE/Integrate/Volume.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Functions for computing volume integrals for a system of PDEs in DG
     methods
  \details   This file contains functionality for computing volume integrals for
     a system of PDEs used in discontinuous Galerkin methods for various orders
     of numerical representation.
*/
// *****************************************************************************

#include "Volume.hpp"
#include "Vector.hpp"
#include "Quadrature.hpp"

void
tk::volInt( ncomp_t system,
            ncomp_t ncomp,
            ncomp_t offset,
            const std::size_t ndof,
            const std::size_t nelem,
            const std::vector< std::size_t >& inpoel,
            const UnsMesh::Coords& coord,
            const Fields& geoElem,
            const FluxFn& flux,
            const VelFn& vel,
            const Fields& U,
            const std::vector< std::size_t >& ndofel,
            Fields& R )
// *****************************************************************************
//  Compute volume integrals for DG
//! \param[in] system Equation system index
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] offset Offset this PDE system operates from
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] nelem Maximum number of elements
//! \param[in] inpoel Element-node connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] geoElem Element geometry array
//! \param[in] flux Flux function to use
//! \param[in] vel Function to use to query prescribed velocity (if any)
//! \param[in] U Solution vector at recent time step
//! \param[in] ndofel Vector of local number of degrees of freedome
//! \param[in,out] R Right-hand side vector added to
// *****************************************************************************
{
  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  // compute volume integrals
  for (std::size_t e=0; e<nelem; ++e)
  {
    if(ndofel[e] > 1)
    {
      auto ng = tk::NGvol(ndofel[e]);

      // arrays for quadrature points
      std::array< std::vector< real >, 3 > coordgp;
      std::vector< real > wgp;

      coordgp[0].resize( ng );
      coordgp[1].resize( ng );
      coordgp[2].resize( ng );
      wgp.resize( ng );

      GaussQuadratureTet( ng, coordgp, wgp );

      // Extract the element coordinates
      std::array< std::array< real, 3>, 4 > coordel {{
        {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
        {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
        {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
        {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }}
      }};

      auto jacInv =
              inverseJacobian( coordel[0], coordel[1], coordel[2], coordel[3] );

      // Compute the derivatives of basis function for DG(P1)
      auto dBdx = eval_dBdx_p1( ndofel[e], jacInv );

      // Gaussian quadrature
      for (std::size_t igp=0; igp<ng; ++igp)
      {
        if (ndofel[e] > 4)
          eval_dBdx_p2( igp, coordgp, jacInv, dBdx );

        // Compute the coordinates of quadrature point at physical domain
        auto gp = eval_gp( igp, coordel, coordgp );

        // Compute the basis function
        auto B =
          eval_basis( ndofel[e], coordgp[0][igp], coordgp[1][igp], coordgp[2][igp] );

        auto wt = wgp[igp] * geoElem(e, 0, 0);

        auto state = eval_state( ncomp, offset, ndof, ndofel[e], e, U, B );

        // evaluate prescribed velocity (if any)
        auto v = vel( system, ncomp, gp[0], gp[1], gp[2] );

        // comput flux
        auto fl = flux( system, ncomp, state, v );

        update_rhs( ncomp, offset, ndof, ndofel[e], wt, e, dBdx, fl, R );
      }
    }
  }
}

void
tk::update_rhs( ncomp_t ncomp,
                ncomp_t offset,
                const std::size_t ndof,
                const std::size_t ndof_el,
                const tk::real wt,
                const std::size_t e,
                const std::array< std::vector<tk::real>, 3 >& dBdx,
                const std::vector< std::array< tk::real, 3 > >& fl,
                Fields& R )
// *****************************************************************************
//  Update the rhs by adding the source term integrals
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] offset Offset this PDE system operates from
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] ndof_el Number of degrees of freedom for local element
//! \param[in] wt Weight of gauss quadrature point
//! \param[in] e Element index
//! \param[in] dBdx Vector of basis function derivatives
//! \param[in] fl Vector of numerical flux
//! \param[in,out] R Right-hand side vector computed
// *****************************************************************************
{
  Assert( dBdx[0].size() == ndof_el, "Size mismatch for basis function derivatives" );
  Assert( dBdx[1].size() == ndof_el, "Size mismatch for basis function derivatives" );
  Assert( dBdx[2].size() == ndof_el, "Size mismatch for basis function derivatives" );
  Assert( fl.size() == ncomp, "Size mismatch for flux term" );

  for (ncomp_t c=0; c<ncomp; ++c)
  {
    auto mark = c*ndof;
    R(e, mark+1, offset) +=
      wt * (fl[c][0]*dBdx[0][1] + fl[c][1]*dBdx[1][1] + fl[c][2]*dBdx[2][1]);
    R(e, mark+2, offset) +=
      wt * (fl[c][0]*dBdx[0][2] + fl[c][1]*dBdx[1][2] + fl[c][2]*dBdx[2][2]);
    R(e, mark+3, offset) +=
      wt * (fl[c][0]*dBdx[0][3] + fl[c][1]*dBdx[1][3] + fl[c][2]*dBdx[2][3]);

    if( ndof_el > 4 )
    {
      R(e, mark+4, offset) +=
        wt * (fl[c][0]*dBdx[0][4] + fl[c][1]*dBdx[1][4] + fl[c][2]*dBdx[2][4]);
      R(e, mark+5, offset) +=
        wt * (fl[c][0]*dBdx[0][5] + fl[c][1]*dBdx[1][5] + fl[c][2]*dBdx[2][5]);
      R(e, mark+6, offset) +=
        wt * (fl[c][0]*dBdx[0][6] + fl[c][1]*dBdx[1][6] + fl[c][2]*dBdx[2][6]);
      R(e, mark+7, offset) +=
        wt * (fl[c][0]*dBdx[0][7] + fl[c][1]*dBdx[1][7] + fl[c][2]*dBdx[2][7]);
      R(e, mark+8, offset) +=
        wt * (fl[c][0]*dBdx[0][8] + fl[c][1]*dBdx[1][8] + fl[c][2]*dBdx[2][8]);
      R(e, mark+9, offset) +=
        wt * (fl[c][0]*dBdx[0][9] + fl[c][1]*dBdx[1][9] + fl[c][2]*dBdx[2][9]);
    }
  }
}
