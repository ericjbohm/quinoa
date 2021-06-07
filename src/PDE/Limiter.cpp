// *****************************************************************************
/*!
  \file      src/PDE/Limiter.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Limiters for discontiunous Galerkin methods
  \details   This file contains functions that provide limiter function
    calculations for maintaining monotonicity near solution discontinuities
    for the DG discretization.
*/
// *****************************************************************************

#include <array>
#include <vector>

#include "Vector.hpp"
#include "Limiter.hpp"
#include "DerivedData.hpp"
#include "Integrate/Quadrature.hpp"
#include "Integrate/Basis.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"
#include "Inciter/PrefIndicator.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

void
WENO_P1( const std::vector< int >& esuel,
         inciter::ncomp_t offset,
         tk::Fields& U )
// *****************************************************************************
//  Weighted Essentially Non-Oscillatory (WENO) limiter for DGP1
//! \param[in] esuel Elements surrounding elements
//! \param[in] offset Index for equation systems
//! \param[in,out] U High-order solution vector which gets limited
//! \details This WENO function should be called for transport and compflow
//! \note This limiter function is experimental and untested. Use with caution.
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto cweight = inciter::g_inputdeck.get< tag::discr, tag::cweight >();
  auto nelem = esuel.size()/4;
  std::array< std::vector< tk::real >, 3 >
    limU {{ std::vector< tk::real >(nelem),
            std::vector< tk::real >(nelem),
            std::vector< tk::real >(nelem) }};

  std::size_t ncomp = U.nprop()/rdof;

  for (inciter::ncomp_t c=0; c<ncomp; ++c)
  {
    for (std::size_t e=0; e<nelem; ++e)
    {
      WENOFunction(U, esuel, e, c, rdof, offset, cweight, limU);
    }

    auto mark = c*rdof;

    for (std::size_t e=0; e<nelem; ++e)
    {
      U(e, mark+1, offset) = limU[0][e];
      U(e, mark+2, offset) = limU[1][e];
      U(e, mark+3, offset) = limU[2][e];
    }
  }
}

void
WENOMultiMat_P1( const std::vector< int >& esuel,
                 inciter::ncomp_t offset,
                 tk::Fields& U,
                 tk::Fields& P,
                 std::size_t nmat )
// *****************************************************************************
//  Weighted Essentially Non-Oscillatory (WENO) limiter for multi-material DGP1
//! \param[in] esuel Elements surrounding elements
//! \param[in] offset Index for equation systems
//! \param[in,out] U High-order solution vector which gets limited
//! \param[in,out] P High-order vector of primitives which gets limited
//! \param[in] nmat Number of materials in this PDE system
//! \details This WENO function should be called for multimat
//! \note This limiter function is experimental and untested. Use with caution.
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto cweight = inciter::g_inputdeck.get< tag::discr, tag::cweight >();
  auto nelem = esuel.size()/4;
  std::array< std::vector< tk::real >, 3 >
    limU {{ std::vector< tk::real >(nelem),
            std::vector< tk::real >(nelem),
            std::vector< tk::real >(nelem) }};

  std::size_t ncomp = U.nprop()/rdof;
  std::size_t nprim = P.nprop()/rdof;

  // limit conserved quantities
  for (inciter::ncomp_t c=0; c<ncomp; ++c)
  {
    for (std::size_t e=0; e<nelem; ++e)
    {
      WENOFunction(U, esuel, e, c, rdof, offset, cweight, limU);
    }

    auto mark = c*rdof;

    for (std::size_t e=0; e<nelem; ++e)
    {
      U(e, mark+1, offset) = limU[0][e];
      U(e, mark+2, offset) = limU[1][e];
      U(e, mark+3, offset) = limU[2][e];
    }
  }

  // limit primitive quantities
  for (inciter::ncomp_t c=0; c<nprim; ++c)
  {
    for (std::size_t e=0; e<nelem; ++e)
    {
      WENOFunction(P, esuel, e, c, rdof, offset, cweight, limU);
    }

    auto mark = c*rdof;

    for (std::size_t e=0; e<nelem; ++e)
    {
      P(e, mark+1, offset) = limU[0][e];
      P(e, mark+2, offset) = limU[1][e];
      P(e, mark+3, offset) = limU[2][e];
    }
  }

  std::vector< tk::real > phic(ncomp, 1.0), phip(nprim, 1.0);
  for (std::size_t e=0; e<nelem; ++e)
  {
    consistentMultiMatLimiting_P1(nmat, offset, rdof, e, U, P, phic, phip);
  }
}

void
Superbee_P1( const std::vector< int >& esuel,
             const std::vector< std::size_t >& inpoel,
             const std::vector< std::size_t >& ndofel,
             inciter::ncomp_t offset,
             const tk::UnsMesh::Coords& coord,
             tk::Fields& U )
// *****************************************************************************
//  Superbee limiter for DGP1
//! \param[in] esuel Elements surrounding elements
//! \param[in] inpoel Element connectivity
//! \param[in] ndofel Vector of local number of degrees of freedom
//! \param[in] offset Index for equation systems
//! \param[in] coord Array of nodal coordinates
//! \param[in,out] U High-order solution vector which gets limited
//! \details This Superbee function should be called for transport and compflow
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto ndof = inciter::g_inputdeck.get< tag::discr, tag::ndof >();
  std::size_t ncomp = U.nprop()/rdof;

  auto beta_lim = 2.0;

  for (std::size_t e=0; e<esuel.size()/4; ++e)
  {
    // If an rDG method is set up (P0P1), then, currently we compute the P1
    // basis functions and solutions by default. This implies that P0P1 is
    // unsupported in the p-adaptive DG (PDG). This is a workaround until we
    // have rdofel, which is needed to distinguish between ndofs and rdofs per
    // element for pDG.
    std::size_t dof_el;
    if (rdof > ndof)
    {
      dof_el = rdof;
    }
    else
    {
      dof_el = ndofel[e];
    }

    if (dof_el > 1)
    {
      auto phi = SuperbeeFunction(U, esuel, inpoel, coord, e, ndof, rdof,
                   dof_el, offset, ncomp, beta_lim);

      // apply limiter function
      for (inciter::ncomp_t c=0; c<ncomp; ++c)
      {
        auto mark = c*rdof;
        U(e, mark+1, offset) = phi[c] * U(e, mark+1, offset);
        U(e, mark+2, offset) = phi[c] * U(e, mark+2, offset);
        U(e, mark+3, offset) = phi[c] * U(e, mark+3, offset);
      }
    }
  }
}

void
SuperbeeMultiMat_P1(
  const std::vector< int >& esuel,
  const std::vector< std::size_t >& inpoel,
  const std::vector< std::size_t >& ndofel,
  std::size_t system,
  inciter::ncomp_t offset,
  const tk::UnsMesh::Coords& coord,
  tk::Fields& U,
  tk::Fields& P,
  std::size_t nmat )
// *****************************************************************************
//  Superbee limiter for multi-material DGP1
//! \param[in] esuel Elements surrounding elements
//! \param[in] inpoel Element connectivity
//! \param[in] ndofel Vector of local number of degrees of freedom
//! \param[in] system Index for equation systems
//! \param[in] offset Offset this PDE system operates from
//! \param[in] coord Array of nodal coordinates
//! \param[in,out] U High-order solution vector which gets limited
//! \param[in,out] P High-order vector of primitives which gets limited
//! \param[in] nmat Number of materials in this PDE system
//! \details This Superbee function should be called for multimat
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto ndof = inciter::g_inputdeck.get< tag::discr, tag::ndof >();
  const auto intsharp = inciter::g_inputdeck.get< tag::param, tag::multimat,
    tag::intsharp >()[system];
  std::size_t ncomp = U.nprop()/rdof;
  std::size_t nprim = P.nprop()/rdof;

  auto beta_lim = 2.0;

  for (std::size_t e=0; e<esuel.size()/4; ++e)
  {
    // If an rDG method is set up (P0P1), then, currently we compute the P1
    // basis functions and solutions by default. This implies that P0P1 is
    // unsupported in the p-adaptive DG (PDG). This is a workaround until we
    // have rdofel, which is needed to distinguish between ndofs and rdofs per
    // element for pDG.
    std::size_t dof_el;
    if (rdof > ndof)
    {
      dof_el = rdof;
    }
    else
    {
      dof_el = ndofel[e];
    }

    if (dof_el > 1)
    {
      // limit conserved quantities
      auto phic = SuperbeeFunction(U, esuel, inpoel, coord, e, ndof, rdof,
                    dof_el, offset, ncomp, beta_lim);
      // limit primitive quantities
      auto phip = SuperbeeFunction(P, esuel, inpoel, coord, e, ndof, rdof,
                    dof_el, offset, nprim, beta_lim);

      if(ndof > 1)
        BoundPreservingLimiting(nmat, offset, ndof, e, inpoel, coord, U, phic);

      // limits under which compression is to be performed
      std::vector< std::size_t > matInt(nmat, 0);
      std::vector< tk::real > alAvg(nmat, 0.0);
      for (std::size_t k=0; k<nmat; ++k)
        alAvg[k] = U(e, volfracDofIdx(nmat,k,rdof,0), offset);
      auto intInd = interfaceIndicator(nmat, alAvg, matInt);
      if ((intsharp > 0) && intInd)
      {
        for (std::size_t k=0; k<nmat; ++k)
        {
          if (matInt[k])
            phic[volfracIdx(nmat,k)] = 1.0;
        }
      }
      else
      {
        consistentMultiMatLimiting_P1(nmat, offset, rdof, e, U, P, phic, phip);
      }

      // apply limiter function
      for (inciter::ncomp_t c=0; c<ncomp; ++c)
      {
        auto mark = c*rdof;
        U(e, mark+1, offset) = phic[c] * U(e, mark+1, offset);
        U(e, mark+2, offset) = phic[c] * U(e, mark+2, offset);
        U(e, mark+3, offset) = phic[c] * U(e, mark+3, offset);
      }
      for (inciter::ncomp_t c=0; c<nprim; ++c)
      {
        auto mark = c*rdof;
        P(e, mark+1, offset) = phip[c] * P(e, mark+1, offset);
        P(e, mark+2, offset) = phip[c] * P(e, mark+2, offset);
        P(e, mark+3, offset) = phip[c] * P(e, mark+3, offset);
      }
    }
  }
}

void
VertexBasedTransport_P1(
  const std::map< std::size_t, std::vector< std::size_t > >& esup,
  const std::vector< std::size_t >& inpoel,
  const std::vector< std::size_t >& ndofel,
  std::size_t nelem,
  std::size_t system,
  std::size_t offset,
  const tk::UnsMesh::Coords& coord,
  const std::vector< std::size_t >& gid,
  const std::unordered_map< std::size_t, std::size_t >& bid,
  const tk::Fields& uNodalExtrm,
  tk::Fields& U )
// *****************************************************************************
//  Kuzmin's vertex-based limiter for transport DGP1
//! \param[in] esup Elements surrounding points
//! \param[in] inpoel Element connectivity
//! \param[in] ndofel Vector of local number of degrees of freedom
//! \param[in] nelem Number of elements
//! \param[in] system Index for equation systems
//! \param[in] offset Index for equation systems
//! \param[in] coord Array of nodal coordinates
//! \param[in] gid Local->global node id map
//! \param[in] bid Local chare-boundary node ids (value) associated to
//!   global node ids (key)
//! \param[in] uNodalExtrm Chare-boundary nodal extrema for conservative
//!   variables
//! \param[in,out] U High-order solution vector which gets limited
//! \details This vertex-based limiter function should be called for transport.
//!   For details see: Kuzmin, D. (2010). A vertex-based hierarchical slope
//!   limiter for p-adaptive discontinuous Galerkin methods. Journal of
//!   computational and applied mathematics, 233(12), 3077-3085.
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto ndof = inciter::g_inputdeck.get< tag::discr, tag::ndof >();
  const auto intsharp = inciter::g_inputdeck.get< tag::param, tag::transport,
    tag::intsharp >()[system];
  std::size_t ncomp = U.nprop()/rdof;

  for (std::size_t e=0; e<nelem; ++e)
  {
    // If an rDG method is set up (P0P1), then, currently we compute the P1
    // basis functions and solutions by default. This implies that P0P1 is
    // unsupported in the p-adaptive DG (PDG). This is a workaround until we
    // have rdofel, which is needed to distinguish between ndofs and rdofs per
    // element for pDG.
    std::size_t dof_el;
    if (rdof > ndof)
    {
      dof_el = rdof;
    }
    else
    {
      dof_el = ndofel[e];
    }

    if (dof_el > 1)
    {
      std::vector< std::vector< tk::real > > unk;
      unk.resize(ncomp, std::vector< tk::real >(dof_el, 0.0));
      TransformBasis(ncomp, offset, e, dof_el, U, inpoel, coord, unk);

      // limit conserved quantities
      auto phi = VertexBasedFunction(U, esup, inpoel, coord, e, rdof, dof_el,
        offset, ncomp, gid, bid, uNodalExtrm);

      // limits under which compression is to be performed
      std::vector< std::size_t > matInt(ncomp, 0);
      std::vector< tk::real > alAvg(ncomp, 0.0);
      for (std::size_t k=0; k<ncomp; ++k)
        alAvg[k] = U(e,k*rdof,offset);
      auto intInd = interfaceIndicator(ncomp, alAvg, matInt);
      if ((intsharp > 0) && intInd)
      {
        for (std::size_t k=0; k<ncomp; ++k)
        {
          if (matInt[k]) phi[k] = 1.0;
        }
      }

      // apply limiter function
      for (std::size_t c=0; c<ncomp; ++c)
      {
        auto mark = c*rdof;
        U(e, mark+1, offset) = phi[c] * U(e, mark+1, offset);
        U(e, mark+2, offset) = phi[c] * U(e, mark+2, offset);
        U(e, mark+3, offset) = phi[c] * U(e, mark+3, offset);
      }
    }
  }
}

void
VertexBased_P1(
  const std::map< std::size_t, std::vector< std::size_t > >& esup,
  const std::vector< std::size_t >& inpoel,
  const std::vector< std::size_t >& ndofel,
  std::size_t nelem,
  std::size_t offset,
  const tk::UnsMesh::Coords& coord,
  const std::vector< std::size_t >& gid,
  const std::unordered_map< std::size_t, std::size_t >& bid,
  const tk::Fields& uNodalExtrm,
  tk::Fields& U )
// *****************************************************************************
//  Kuzmin's vertex-based limiter for single-material DGP1
//! \param[in] esup Elements surrounding points
//! \param[in] inpoel Element connectivity
//! \param[in] ndofel Vector of local number of degrees of freedom
//! \param[in] nelem Number of elements
//! \param[in] offset Index for equation systems
//! \param[in] coord Array of nodal coordinates
//! \param[in] gid Local->global node id map
//! \param[in] bid Local chare-boundary node ids (value) associated to
//!   global node ids (key)
//! \param[in] uNodalExtrm Chare-boundary nodal extrema for conservative
//!   variables
//! \param[in,out] U High-order solution vector which gets limited
//! \details This vertex-based limiter function should be called for compflow.
//!   For details see: Kuzmin, D. (2010). A vertex-based hierarchical slope
//!   limiter for p-adaptive discontinuous Galerkin methods. Journal of
//!   computational and applied mathematics, 233(12), 3077-3085.
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto ndof = inciter::g_inputdeck.get< tag::discr, tag::ndof >();
  std::size_t ncomp = U.nprop()/rdof;

  // Copy field data U to U_lim. U_lim will store the limited solution
  // temperally to avoid the limited solution will be used when finding
  // the min/max bounds for the limiting function
  auto U_lim = U;

  for (std::size_t e=0; e<nelem; ++e)
  {
    // If an rDG method is set up (P0P1), then, currently we compute the P1
    // basis functions and solutions by default. This implies that P0P1 is
    // unsupported in the p-adaptive DG (PDG). This is a workaround until we
    // have rdofel, which is needed to distinguish between ndofs and rdofs per
    // element for pDG.
    std::size_t dof_el;
    if (rdof > ndof)
    {
      dof_el = rdof;
    }
    else
    {
      dof_el = ndofel[e];
    }

    bool shock_detec(false);

    // Evaluate the shock detection indicator
    auto Ind = eval_indicator(e, ncomp, dof_el, ndofel, U);
    if(Ind > 1e-6)
      shock_detec = true;

    if (dof_el > 1 && shock_detec == true)
    {
      // The vector used to store the numerical solution with Taylor basis
      std::vector< std::vector< tk::real > > unk;
      unk.resize(ncomp, std::vector< tk::real >(dof_el, 0.0));

      // Transform the solution with Dubiner basis to Taylor basis so that the
      // limiting function could be applied to physical derivatives in a
      // hierarchical manner
      TransformBasis(ncomp, offset, e, dof_el, U, inpoel, coord, unk);

      // The vector of limiting coefficients for P1 and P2 coefficients
      std::vector< tk::real > phic_p1(ncomp, 1.0);
      std::vector< tk::real > phic_p2(ncomp, 1.0);

      // If DGP2 is applied, apply the limiter function to the first derivative
      // to obtain the limiting coefficient for P2 coefficients
      if(dof_el > 4)
        phic_p2 = VertexBasedFunction_P2(unk, U, esup, inpoel, coord, geoElem, e, rdof, dof_el,
          offset, ncomp);

      // limit conserved quantities
      phic_p1 = VertexBasedFunction(unk, U, esup, inpoel, coord, e, rdof, dof_el,
        offset, ncomp, gid, bid, uNodalExtrm);

      if(dof_el > 4)
        for (std::size_t c=0; c<ncomp; ++c)
          phic_p1[c] = std::max(phic_p1[c], phic_p2[c]);

      // apply limiter function to the solution with Taylor basis
      for (std::size_t c=0; c<ncomp; ++c)
      {
        unk[c][1] = phic_p1[c] * unk[c][1];
        unk[c][2] = phic_p1[c] * unk[c][2];
        unk[c][3] = phic_p1[c] * unk[c][3];
      }
      if(dof_el > 4)
      {
        for (std::size_t c=0; c<ncomp; ++c)
        {
          unk[c][4] = phic_p2[c] * unk[c][4];
          unk[c][5] = phic_p2[c] * unk[c][5];
          unk[c][6] = phic_p2[c] * unk[c][6];
          unk[c][7] = phic_p2[c] * unk[c][7];
          unk[c][8] = phic_p2[c] * unk[c][8];
          unk[c][9] = phic_p2[c] * unk[c][9];
        }
      }

      // Convert the solution with Taylor basis to the solution with Dubiner basis
      InverseBasis(ncomp, offset, e, dof_el, inpoel, coord, geoElem, U_lim, unk);
    }
  }

  for (std::size_t e=0; e<nelem; ++e)
  {
    for (std::size_t c=0; c<ncomp; ++c)
    {
      auto mark = c*rdof;
      U(e, mark+1, offset) = U_lim(e, mark+1, offset);
      U(e, mark+2, offset) = U_lim(e, mark+2, offset);
      U(e, mark+3, offset) = U_lim(e, mark+3, offset);

      if(ndof > 4)
      {
        U(e, mark+4, offset) = U_lim(e, mark+4, offset);
        U(e, mark+5, offset) = U_lim(e, mark+5, offset);
        U(e, mark+6, offset) = U_lim(e, mark+6, offset);
        U(e, mark+7, offset) = U_lim(e, mark+7, offset);
        U(e, mark+8, offset) = U_lim(e, mark+8, offset);
        U(e, mark+9, offset) = U_lim(e, mark+9, offset);
      }
    }
  }
}

void
VertexBasedMultiMat_P1(
  const std::map< std::size_t, std::vector< std::size_t > >& esup,
  const std::vector< std::size_t >& inpoel,
  const std::vector< std::size_t >& ndofel,
  std::size_t nelem,
  std::size_t system,
  std::size_t offset,
  const tk::UnsMesh::Coords& coord,
  const std::vector< std::size_t >& gid,
  const std::unordered_map< std::size_t, std::size_t >& bid,
  const tk::Fields& uNodalExtrm,
  const tk::Fields& pNodalExtrm,
  tk::Fields& U,
  tk::Fields& P,
  std::size_t nmat )
// *****************************************************************************
//  Kuzmin's vertex-based limiter for multi-material DGP1
//! \param[in] esup Elements surrounding points
//! \param[in] inpoel Element connectivity
//! \param[in] ndofel Vector of local number of degrees of freedom
//! \param[in] nelem Number of elements
//! \param[in] system Index for equation systems
//! \param[in] offset Offset this PDE system operates from
//! \param[in] coord Array of nodal coordinates
//! \param[in] gid Local->global node id map
//! \param[in] bid Local chare-boundary node ids (value) associated to
//!   global node ids (key)
//! \param[in] uNodalExtrm Chare-boundary nodal extrema for conservative
//!   variables
//! \param[in] pNodalExtrm Chare-boundary nodal extrema for primitive
//!   variables
//! \param[in,out] U High-order solution vector which gets limited
//! \param[in,out] P High-order vector of primitives which gets limited
//! \param[in] nmat Number of materials in this PDE system
//! \details This vertex-based limiter function should be called for multimat.
//!   For details see: Kuzmin, D. (2010). A vertex-based hierarchical slope
//!   limiter for p-adaptive discontinuous Galerkin methods. Journal of
//!   computational and applied mathematics, 233(12), 3077-3085.
// *****************************************************************************
{
  const auto rdof = inciter::g_inputdeck.get< tag::discr, tag::rdof >();
  const auto ndof = inciter::g_inputdeck.get< tag::discr, tag::ndof >();
  const auto intsharp = inciter::g_inputdeck.get< tag::param, tag::multimat,
    tag::intsharp >()[system];
  std::size_t ncomp = U.nprop()/rdof;
  std::size_t nprim = P.nprop()/rdof;

  for (std::size_t e=0; e<nelem; ++e)
  {
    // If an rDG method is set up (P0P1), then, currently we compute the P1
    // basis functions and solutions by default. This implies that P0P1 is
    // unsupported in the p-adaptive DG (PDG). This is a workaround until we
    // have rdofel, which is needed to distinguish between ndofs and rdofs per
    // element for pDG.
    std::size_t dof_el;
    if (rdof > ndof)
    {
      dof_el = rdof;
    }
    else
    {
      dof_el = ndofel[e];
    }

    if (dof_el > 1)
    {
      std::vector< std::vector< tk::real > > unk;
      unk.resize(ncomp, std::vector< tk::real >(dof_el, 0.0));
      TransformBasis(ncomp, offset, e, dof_el, U, inpoel, coord, unk);

      // limit conserved quantities
      auto phic = VertexBasedFunction(unk, U, esup, inpoel, coord, e,
        rdof, dof_el, offset, ncomp, gid, bid, uNodalExtrm);
      // limit primitive quantities
      auto phip = VertexBasedFunction(unk, P, esup, inpoel, coord, e,
        rdof, dof_el, offset, nprim, gid, bid, uNodalExtrm);

      if(ndof > 1 && intsharp == 0)
        BoundPreservingLimiting(nmat, offset, ndof, e, inpoel, coord, U, phic);

      // limits under which compression is to be performed
      std::vector< std::size_t > matInt(nmat, 0);
      std::vector< tk::real > alAvg(nmat, 0.0);
      for (std::size_t k=0; k<nmat; ++k)
        alAvg[k] = U(e, volfracDofIdx(nmat,k,rdof,0), offset);
      auto intInd = interfaceIndicator(nmat, alAvg, matInt);
      if ((intsharp > 0) && intInd)
      {
        for (std::size_t k=0; k<nmat; ++k)
        {
          if (matInt[k])
            phic[volfracIdx(nmat,k)] = 1.0;
        }
      }
      else
      {
        consistentMultiMatLimiting_P1(nmat, offset, rdof, e, U, P, phic, phip);
      }

      // apply limiter function
      for (std::size_t c=0; c<ncomp; ++c)
      {
        auto mark = c*rdof;
        U(e, mark+1, offset) = phic[c] * U(e, mark+1, offset);
        U(e, mark+2, offset) = phic[c] * U(e, mark+2, offset);
        U(e, mark+3, offset) = phic[c] * U(e, mark+3, offset);
      }
      for (std::size_t c=0; c<nprim; ++c)
      {
        auto mark = c*rdof;
        P(e, mark+1, offset) = phip[c] * P(e, mark+1, offset);
        P(e, mark+2, offset) = phip[c] * P(e, mark+2, offset);
        P(e, mark+3, offset) = phip[c] * P(e, mark+3, offset);
      }
    }
  }
}

void
WENOFunction( const tk::Fields& U,
              const std::vector< int >& esuel,
              std::size_t e,
              inciter::ncomp_t c,
              std::size_t rdof,
              inciter::ncomp_t offset,
              tk::real cweight,
              std::array< std::vector< tk::real >, 3 >& limU )
// *****************************************************************************
//  WENO limiter function calculation for P1 dofs
//! \param[in] U High-order solution vector which is to be limited
//! \param[in] esuel Elements surrounding elements
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] c Index of component which is to be limited
//! \param[in] rdof Maximum number of reconstructed degrees of freedom
//! \param[in] offset Index for equation systems
//! \param[in] cweight Weight of the central stencil
//! \param[in,out] limU Limited gradients of component c
// *****************************************************************************
{
  std::array< std::array< tk::real, 3 >, 5 > gradu;
  std::array< tk::real, 5 > wtStencil, osc, wtDof;

  auto mark = c*rdof;

  // reset all stencil values to zero
  for (auto& g : gradu) g.fill(0.0);
  osc.fill(0);
  wtDof.fill(0);
  wtStencil.fill(0);

  // The WENO limiter uses solution data from the neighborhood in the form
  // of stencils to enforce non-oscillatory conditions. The immediate
  // (Von Neumann) neighborhood of a tetrahedral cell consists of the 4
  // cells that share faces with it. These are the 4 neighborhood-stencils
  // for the tetrahedron. The primary stencil is the tet itself. Weights are
  // assigned to these stencils, with the primary stencil usually assigned
  // the highest weight. The lower the primary/central weight, the more
  // dissipative the limiting effect. This central weight is usually problem
  // dependent. It is set higher for relatively weaker discontinuities, and
  // lower for stronger discontinuities.

  // primary stencil
  gradu[0][0] = U(e, mark+1, offset);
  gradu[0][1] = U(e, mark+2, offset);
  gradu[0][2] = U(e, mark+3, offset);
  wtStencil[0] = cweight;

  // stencils from the neighborhood
  for (std::size_t is=1; is<5; ++is)
  {
    auto nel = esuel[ 4*e+(is-1) ];

    // ignore physical domain ghosts
    if (nel == -1)
    {
      gradu[is].fill(0.0);
      wtStencil[is] = 0.0;
      continue;
    }

    std::size_t n = static_cast< std::size_t >( nel );
    gradu[is][0] = U(n, mark+1, offset);
    gradu[is][1] = U(n, mark+2, offset);
    gradu[is][2] = U(n, mark+3, offset);
    wtStencil[is] = 1.0;
  }

  // From these stencils, an oscillation indicator is calculated, which
  // determines the effective weights for the high-order solution DOFs.
  // These effective weights determine the contribution of each of the
  // stencils to the high-order solution DOFs of the current cell which are
  // being limited. If this indicator detects a large oscillation in the
  // solution of the current cell, it reduces the effective weight for the
  // central stencil contribution to its high-order DOFs. This results in
  // a more dissipative and well-behaved solution in the troubled cell.

  // oscillation indicators
  for (std::size_t is=0; is<5; ++is)
    osc[is] = std::sqrt( tk::dot(gradu[is], gradu[is]) );

  tk::real wtotal = 0;

  // effective weights for dofs
  for (std::size_t is=0; is<5; ++is)
  {
    // A small number (1.0e-8) is needed here to avoid dividing by a zero in
    // the case of a constant solution, where osc would be zero. The number
    // is not set to machine zero because it is squared, and a number
    // between 1.0e-8 to 1.0e-6 is needed.
    wtDof[is] = wtStencil[is] * pow( (1.0e-8 + osc[is]), -2 );
    wtotal += wtDof[is];
  }

  for (std::size_t is=0; is<5; ++is)
  {
    wtDof[is] = wtDof[is]/wtotal;
  }

  limU[0][e] = 0.0;
  limU[1][e] = 0.0;
  limU[2][e] = 0.0;

  // limiter function
  for (std::size_t is=0; is<5; ++is)
  {
    limU[0][e] += wtDof[is]*gradu[is][0];
    limU[1][e] += wtDof[is]*gradu[is][1];
    limU[2][e] += wtDof[is]*gradu[is][2];
  }
}

std::vector< tk::real >
SuperbeeFunction( const tk::Fields& U,
                  const std::vector< int >& esuel,
                  const std::vector< std::size_t >& inpoel,
                  const tk::UnsMesh::Coords& coord,
                  std::size_t e,
                  std::size_t ndof,
                  std::size_t rdof,
                  std::size_t dof_el,
                  inciter::ncomp_t offset,
                  inciter:: ncomp_t ncomp,
                  tk::real beta_lim )
// *****************************************************************************
//  Superbee limiter function calculation for P1 dofs
//! \param[in] U High-order solution vector which is to be limited
//! \param[in] esuel Elements surrounding elements
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] rdof Maximum number of reconstructed degrees of freedom
//! \param[in] dof_el Local number of degrees of freedom
//! \param[in] offset Index for equation systems
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] beta_lim Parameter which is equal to 2 for Superbee and 1 for
//!   minmod limiter
//! \return phi Limiter function for solution in element e
// *****************************************************************************
{
  // Superbee is a TVD limiter, which uses min-max bounds that the
  // high-order solution should satisfy, to ensure TVD properties. For a
  // high-order method like DG, this involves the following steps:
  // 1. Find min-max bounds in the immediate neighborhood of cell.
  // 2. Calculate the Superbee function for all the points where solution
  //    needs to be reconstructed to (all quadrature points). From these,
  //    use the minimum value of the limiter function.

  std::vector< tk::real > uMin(ncomp, 0.0), uMax(ncomp, 0.0);

  for (inciter::ncomp_t c=0; c<ncomp; ++c)
  {
    auto mark = c*rdof;
    uMin[c] = U(e, mark, offset);
    uMax[c] = U(e, mark, offset);
  }

  // ----- Step-1: find min/max in the neighborhood
  for (std::size_t is=0; is<4; ++is)
  {
    auto nel = esuel[ 4*e+is ];

    // ignore physical domain ghosts
    if (nel == -1) continue;

    auto n = static_cast< std::size_t >( nel );
    for (inciter::ncomp_t c=0; c<ncomp; ++c)
    {
      auto mark = c*rdof;
      uMin[c] = std::min(uMin[c], U(n, mark, offset));
      uMax[c] = std::max(uMax[c], U(n, mark, offset));
    }
  }

  // ----- Step-2: loop over all quadrature points to get limiter function

  // to loop over all the quadrature points of all faces of element e,
  // coordinates of the quadrature points are needed.
  // Number of quadrature points for face integration
  auto ng = tk::NGfa(ndof);

  // arrays for quadrature points
  std::array< std::vector< tk::real >, 2 > coordgp;
  std::vector< tk::real > wgp;

  coordgp[0].resize( ng );
  coordgp[1].resize( ng );
  wgp.resize( ng );

  // get quadrature point weights and coordinates for triangle
  tk::GaussQuadratureTri( ng, coordgp, wgp );

  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  // Extract the element coordinates
  std::array< std::array< tk::real, 3>, 4 > coordel {{
    {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
    {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
    {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
    {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }} }};

  // Compute the determinant of Jacobian matrix
  auto detT =
    tk::Jacobian( coordel[0], coordel[1], coordel[2], coordel[3] );

  // initialize limiter function
  std::vector< tk::real > phi(ncomp, 1.0);
  for (std::size_t lf=0; lf<4; ++lf)
  {
    // Extract the face coordinates
    std::array< std::size_t, 3 > inpofa_l {{ inpoel[4*e+tk::lpofa[lf][0]],
                                             inpoel[4*e+tk::lpofa[lf][1]],
                                             inpoel[4*e+tk::lpofa[lf][2]] }};

    std::array< std::array< tk::real, 3>, 3 > coordfa {{
      {{ cx[ inpofa_l[0] ], cy[ inpofa_l[0] ], cz[ inpofa_l[0] ] }},
      {{ cx[ inpofa_l[1] ], cy[ inpofa_l[1] ], cz[ inpofa_l[1] ] }},
      {{ cx[ inpofa_l[2] ], cy[ inpofa_l[2] ], cz[ inpofa_l[2] ] }} }};

    // Gaussian quadrature
    for (std::size_t igp=0; igp<ng; ++igp)
    {
      // Compute the coordinates of quadrature point at physical domain
      auto gp = tk::eval_gp( igp, coordfa, coordgp );

      //Compute the basis functions
      auto B_l = tk::eval_basis( rdof,
            tk::Jacobian( coordel[0], gp, coordel[2], coordel[3] ) / detT,
            tk::Jacobian( coordel[0], coordel[1], gp, coordel[3] ) / detT,
            tk::Jacobian( coordel[0], coordel[1], coordel[2], gp ) / detT );

      auto state = tk::eval_state( ncomp, offset, rdof, dof_el, e, U, B_l );

      Assert( state.size() == ncomp, "Size mismatch" );

      // compute the limiter function
      for (inciter::ncomp_t c=0; c<ncomp; ++c)
      {
        auto phi_gp = 1.0;
        auto mark = c*rdof;
        auto uNeg = state[c] - U(e, mark, offset);
        if (uNeg > 1.0e-14)
        {
          uNeg = std::max(uNeg, 1.0e-08);
          phi_gp = std::min( 1.0, (uMax[c]-U(e, mark, offset))/(2.0*uNeg) );
        }
        else if (uNeg < -1.0e-14)
        {
          uNeg = std::min(uNeg, -1.0e-08);
          phi_gp = std::min( 1.0, (uMin[c]-U(e, mark, offset))/(2.0*uNeg) );
        }
        else
        {
          phi_gp = 1.0;
        }
        phi_gp = std::max( 0.0,
                           std::max( std::min(beta_lim*phi_gp, 1.0),
                                     std::min(phi_gp, beta_lim) ) );
        phi[c] = std::min( phi[c], phi_gp );
      }
    }
  }

  return phi;
}

std::vector< tk::real >
VertexBasedFunction( const std::vector< std::vector< tk::real > >& unk,
  const tk::Fields& U,
  const std::map< std::size_t, std::vector< std::size_t > >& esup,
  const std::vector< std::size_t >& inpoel,
  const tk::UnsMesh::Coords& coord,
  const tk::Fields& geoElem,
  std::size_t e,
  std::size_t rdof,
  std::size_t ,
  std::size_t offset,
  std::size_t ncomp,
  const std::vector< std::size_t >& gid,
  const std::unordered_map< std::size_t, std::size_t >& bid,
  const tk::Fields& NodalExtrm )
// *****************************************************************************
//  Kuzmin's vertex-based limiter function calculation for P1 dofs
//! \param[in] U High-order solution vector which is to be limited
//! \param[in] esup Elements surrounding points
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] rdof Maximum number of reconstructed degrees of freedom
//! \param[in] dof_el Local number of degrees of freedom
//! \param[in] offset Index for equation systems
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] gid Local->global node id map
//! \param[in] bid Local chare-boundary node ids (value) associated to
//!   global node ids (key)
//! \param[in] NodalExtrm Chare-boundary nodal extrema
//! \return phi Limiter function for solution in element e
// *****************************************************************************
{
  // Kuzmin's vertex-based TVD limiter uses min-max bounds that the
  // high-order solution should satisfy, to ensure TVD properties. For a
  // high-order method like DG, this involves the following steps:
  // 1. Find min-max bounds in the nodal-neighborhood of cell.
  // 2. Calculate the limiter function (Superbee) for all the vertices of cell.
  //    From these, use the minimum value of the limiter function.

  const auto nelem = inpoel.size() / 4;

  // Prepare for calculating Basis functions
  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  // Extract the element coordinates
  std::array< std::array< tk::real, 3>, 4 > coordel {{
    {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
    {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
    {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
    {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }} }};

  // Compute the determinant of Jacobian matrix
  //auto detT =
  //  tk::Jacobian( coordel[0], coordel[1], coordel[2], coordel[3] );

  std::vector< tk::real > uMin(ncomp, 0.0), uMax(ncomp, 0.0), phi(ncomp, 1.0);

  // loop over all nodes of the element e
  for (std::size_t lp=0; lp<4; ++lp)
  {
    // reset min/max
    for (std::size_t c=0; c<ncomp; ++c)
    {
      auto mark = c*rdof;
      uMin[c] = U(e, mark, offset);
      uMax[c] = U(e, mark, offset);
    }
    auto p = inpoel[4*e+lp];
    const auto& pesup = tk::cref_find(esup, p);

    // ----- Step-1: find min/max in the neighborhood of node p
    // loop over all the internal elements surrounding this node p
    for (auto er : pesup)
    {
      if(er < nelem)
      {
        for (std::size_t c=0; c<ncomp; ++c)
        {
          auto mark = c*rdof;
          uMin[c] = std::min(uMin[c], U(er, mark, offset));
          uMax[c] = std::max(uMax[c], U(er, mark, offset));
        }
      }
    }

    // If node p is the chare-boundary node, find min/max by comparing with
    // the chare-boundary nodal extrema from vector NodalExtrm
    auto gip = bid.find( gid[p] );
    if(gip != end(bid))
    {
      for (std::size_t c=0; c<ncomp; ++c)
      {
        uMax[c] = std::max(NodalExtrm(gip->second,c,0),       uMax[c]);
        uMin[c] = std::min(NodalExtrm(gip->second,c+ncomp,0), uMin[c]);
      }
    }

    // ----- Step-2: compute the limiter function at this node

    // The nodal and central coordinates
    std::array< tk::real, 3 > node{cx[p], cy[p], cz[p]};
    std::array< tk::real, 3 > x_center{geoElem(e,1,0), geoElem(e,2,0), geoElem(e,3,0)};

    // compute the basis functions
    auto B_p = eval_TaylorBasis( rdof, node, x_center, coordel );

    // find high-order solution
    std::vector< tk::real > state( ncomp, 0.0 );
    for (ncomp_t c=0; c<ncomp; ++c)
      for(std::size_t idof = 0; idof < 4; idof++)
        state[c] += unk[c][idof] * B_p[idof];

    Assert( state.size() == ncomp, "Size mismatch" );

    // compute the limiter function
    for (std::size_t c=0; c<ncomp; ++c)
    {
      auto phi_gp = 1.0;
      auto mark = c*rdof;
      auto uNeg = state[c] - U(e, mark, offset);
      auto uref = std::max(std::fabs(U(e,mark,offset)), 1e-14);
      if (uNeg > 1.0e-06*uref)
      {
        phi_gp = std::min( 1.0, (uMax[c]-U(e, mark, offset))/uNeg );
      }
      else if (uNeg < -1.0e-06*uref)
      {
        phi_gp = std::min( 1.0, (uMin[c]-U(e, mark, offset))/uNeg );
      }
      else
      {
        phi_gp = 1.0;
      }

    // ----- Step-3: take the minimum of the nodal-limiter functions
      phi[c] = std::min( phi[c], phi_gp );
    }
  }

  return phi;
}

std::vector< tk::real >
VertexBasedFunction_P2( const std::vector< std::vector< tk::real > >& unk,
  const tk::Fields& U,
  const std::map< std::size_t, std::vector< std::size_t > >& esup,
  const std::vector< std::size_t >& inpoel,
  const tk::UnsMesh::Coords& coord,
  const tk::Fields& geoElem,
  std::size_t e,
  std::size_t rdof,
  [[maybe_unused]] std::size_t dof_el,
  std::size_t offset,
  std::size_t ncomp )
// *****************************************************************************
//  Kuzmin's vertex-based limiter function calculation for P2 dofs
//! \param[in] U High-order solution vector which is to be limited
//! \param[in] esup Elements surrounding points
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] rdof Maximum number of reconstructed degrees of freedom
//! \param[in] dof_el Local number of degrees of freedom
//! \param[in] offset Index for equation systems
//! \param[in] ncomp Number of scalar components in this PDE system
//! \return phi Limiter function for solution in element e
// *****************************************************************************
{
    // Prepare for calculating Basis functions
  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  // Extract the element coordinates
  //std::array< std::array< tk::real, 3>, 4 > coordel {{
  //  {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
  //  {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
  //  {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
  //  {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }} }};

  std::vector< tk::real > phi(ncomp, 1.0);
  std::vector< std::vector< tk::real > > uMin, uMax;
  uMin.resize( ncomp, std::vector<tk::real>(3, 0.0) );
  uMax.resize( ncomp, std::vector<tk::real>(3, 0.0) );

  // The coordinates of centroid in the reference domain
  std::vector< tk::real > center{0.25, 0.25, 0.25};

  // loop over all nodes of the element e
  for (std::size_t lp=0; lp<4; ++lp)
  {
    for (std::size_t c=0; c<ncomp; ++c)
    {
      for (std::size_t idir=1; idir < 4; ++idir)
      {
        uMin[c][idir-1] = unk[c][idir];
        uMax[c][idir-1] = unk[c][idir];
      }
    }

    auto p = inpoel[4*e+lp];
    const auto& pesup = tk::cref_find(esup, p);

    // Step-1: find min/max first order derivative at the centroid in the
    // neighborhood of node p
    for (auto er : pesup)
    {
      // Coordinates of the neighboring element
      //std::array< std::array< tk::real, 3>, 4 > coorder {{
      // {{ cx[ inpoel[4*er  ] ], cy[ inpoel[4*er  ] ], cz[ inpoel[4*er  ] ] }},
      // {{ cx[ inpoel[4*er+1] ], cy[ inpoel[4*er+1] ], cz[ inpoel[4*er+1] ] }},
      // {{ cx[ inpoel[4*er+2] ], cy[ inpoel[4*er+2] ], cz[ inpoel[4*er+2] ] }},
      // {{ cx[ inpoel[4*er+3] ], cy[ inpoel[4*er+3] ], cz[ inpoel[4*er+3] ] }} }};

      std::array< std::array< tk::real, 3 >, 3 > jacInv_er;

      jacInv_er[0][0] = geoElem(er, 5, 0);
      jacInv_er[1][0] = geoElem(er, 6, 0);
      jacInv_er[2][0] = geoElem(er, 7, 0);

      jacInv_er[0][1] = geoElem(er, 8, 0);
      jacInv_er[1][1] = geoElem(er, 9, 0);
      jacInv_er[2][1] = geoElem(er, 10, 0);

      jacInv_er[0][2] = geoElem(er, 11, 0);
      jacInv_er[1][2] = geoElem(er, 12, 0);
      jacInv_er[2][2] = geoElem(er, 13, 0);


      // Compute the derivatives of basis function in the physical domain
      auto dBdx_er = tk::eval_dBdx_p1( rdof, jacInv_er );

      if(rdof > 4)
        tk::evaldBdx_p2(center, jacInv_er, dBdx_er);

      for (std::size_t c=0; c<ncomp; ++c)
      {
        auto mark = c*rdof;
        for (std::size_t idir=0; idir < 3; ++idir)
        {
          // The first order derivative at the centroid of element er
          tk::real slope_er(0.0);
          for(std::size_t idof = 1; idof < rdof; idof++)
            slope_er += U(er, mark+idof, offset) * dBdx_er[idir][idof];

          uMin[c][idir] = std::min(uMin[c][idir], slope_er);
          uMax[c][idir] = std::max(uMax[c][idir], slope_er);

        }
      }
    }

    //Step-2: compute the limiter function at this node
    std::array< tk::real, 3 > node{cx[p], cy[p], cz[p]};
    std::array< tk::real, 3 >
      centroid_physical{geoElem(e,1,0), geoElem(e,2,0), geoElem(e,3,0)};

    // find high-order solution
    std::vector< std::vector< tk::real > > state;
    state.resize( ncomp, std::vector<tk::real>(3, 0.0) );

    for (ncomp_t c=0; c<ncomp; ++c)
    {
      auto dx = node[0] - centroid_physical[0];
      auto dy = node[1] - centroid_physical[1];
      auto dz = node[2] - centroid_physical[2];

      state[c][0] = unk[c][1] + unk[c][4] * dx + unk[c][7] * dy + unk[c][8] * dz;
      state[c][1] = unk[c][2] + unk[c][5] * dy + unk[c][7] * dx + unk[c][9] * dz;
      state[c][2] = unk[c][3] + unk[c][6] * dz + unk[c][8] * dx + unk[c][9] * dy;
    }

    // compute the limiter function
    for (std::size_t c=0; c<ncomp; ++c)
    {
      tk::real phi_dir(1.0);
      for (std::size_t idir=1; idir < 3; ++idir)
      {
        phi_dir = 1.0;
        auto uNeg = state[c][idir-1] - unk[c][idir];
        if (uNeg > 1.0e-5)
        {
          //uNeg = std::max(uNeg, 1.0e-08);
          phi_dir =
            std::min( 1.0, ( uMax[c][idir-1] - unk[c][idir])/uNeg );
        }
        else if (uNeg < -1.0e-5)
        {
          //uNeg = std::min(uNeg, -1.0e-08);
          phi_dir =
            std::min( 1.0, ( uMin[c][idir-1] - unk[c][idir])/uNeg );
        }
        else
        {
          phi_dir = 1.0;
        }

        phi[c] = std::min( phi[c], phi_dir );
      }
    }
  }

  return phi;
}



void consistentMultiMatLimiting_P1(
  std::size_t nmat,
  ncomp_t offset,
  std::size_t rdof,
  std::size_t e,
  tk::Fields& U,
  [[maybe_unused]] tk::Fields& P,
  std::vector< tk::real >& phic,
  [[maybe_unused]] std::vector< tk::real >& phip )
// *****************************************************************************
//  Consistent limiter modifications for P1 dofs
//! \param[in] nmat Number of materials in this PDE system
//! \param[in] offset Index for equation system
//! \param[in] rdof Total number of reconstructed dofs
//! \param[in] e Element being checked for consistency
//! \param[in,out] U Second-order solution vector which gets modified near
//!   material interfaces for consistency
//! \param[in,out] P Second-order vector of primitive quantities which gets
//!   modified near material interfaces for consistency
//! \param[in,out] phic Vector of limiter functions for the conserved quantities
//! \param[in,out] phip Vector of limiter functions for the primitive quantities
// *****************************************************************************
{
  Assert(phic.size() == U.nprop()/rdof, "Number of unknowns in vector of "
    "conserved quantities incorrect");
  Assert(phip.size() == P.nprop()/rdof, "Number of unknowns in vector of "
    "primitive quantities incorrect");

  // find the limiter-function for volume-fractions
  auto phi_al(1.0), almax(0.0), dalmax(0.0);
  //std::size_t nmax(0);
  for (std::size_t k=0; k<nmat; ++k)
  {
    phi_al = std::min( phi_al, phic[volfracIdx(nmat, k)] );
    if (almax < U(e,volfracDofIdx(nmat, k, rdof, 0),offset))
    {
      //nmax = k;
      almax = U(e,volfracDofIdx(nmat, k, rdof, 0),offset);
    }
    auto dmax = std::max(
                  std::max(
                    std::abs(U(e,volfracDofIdx(nmat, k, rdof, 1),offset)),
                    std::abs(U(e,volfracDofIdx(nmat, k, rdof, 2),offset)) ),
                  std::abs(U(e,volfracDofIdx(nmat, k, rdof, 3),offset)) );
    dalmax = std::max( dalmax, dmax );
  }

  auto al_band = 1e-4;

  //phi_al = phic[nmax];

  // determine if cell is a material-interface cell based on ad-hoc tolerances.
  // if interface-cell, then modify high-order dofs of conserved unknowns
  // consistently and use same limiter for all equations.
  // Slopes of solution variables \alpha_k \rho_k and \alpha_k \rho_k E_k need
  // to be modified in interface cells, such that slopes in the \rho_k and
  // \rho_k E_k part are ignored and only slopes in \alpha_k are considered.
  // Ideally, we would like to not do this, but this is a necessity to avoid
  // limiter-limiter interactions in multiphase CFD (see "K.-M. Shyue, F. Xiao,
  // An Eulerian interface sharpening algorithm for compressible two-phase flow:
  // the algebraic THINC approach, Journal of Computational Physics 268, 2014,
  // 326–354. doi:10.1016/j.jcp.2014.03.010." and "A. Chiapolino, R. Saurel,
  // B. Nkonga, Sharpening diffuse interfaces with compressible fluids on
  // unstructured meshes, Journal of Computational Physics 340 (2017) 389–417.
  // doi:10.1016/j.jcp.2017.03.042."). This approximation should be applied in
  // as narrow a band of interface-cells as possible. The following if-test
  // defines this band of interface-cells. This tests checks the value of the
  // maximum volume-fraction in the cell (almax) and the maximum change in
  // volume-fraction in the cell (dalmax, calculated from second-order DOFs),
  // to determine the band of interface-cells where the aforementioned fix needs
  // to be applied. This if-test says that, the fix is applied when the change
  // in volume-fraction across a cell is greater than 0.1, *and* the
  // volume-fraction is between 0.1 and 0.9.
  if ( dalmax > al_band &&
       (almax > al_band && almax < (1.0-al_band)) )
  {
    // 1. consistent high-order dofs
    for (std::size_t k=0; k<nmat; ++k)
    {
      auto alk = std::max( 1.0e-14, U(e,volfracDofIdx(nmat, k, rdof, 0),offset) );
      auto rhok = U(e,densityDofIdx(nmat, k, rdof, 0),offset)/alk;
      for (std::size_t idir=1; idir<=3; ++idir)
      {
        U(e,densityDofIdx(nmat, k, rdof, idir),offset) = rhok *
          U(e,volfracDofIdx(nmat, k, rdof, idir),offset);
      }
    }

    // 2. same limiter for all volume-fractions and densities
    for (std::size_t k=0; k<nmat; ++k)
    {
      phic[volfracIdx(nmat, k)] = phi_al;
      phic[densityIdx(nmat, k)] = phi_al;
    }
  }
  else
  {
    // same limiter for all volume-fractions
    for (std::size_t k=0; k<nmat; ++k)
      phic[volfracIdx(nmat, k)] = phi_al;
  }
}

void BoundPreservingLimiting( std::size_t nmat,
                              ncomp_t offset,
                              std::size_t ndof,
                              std::size_t e,
                              const std::vector< std::size_t >& inpoel,
                              const tk::UnsMesh::Coords& coord,
                              const tk::Fields& U,
                              std::vector< tk::real >& phic )
// *****************************************************************************
//  Bound preserving limiter for P1 dofs when MulMat scheme is selected
//! \param[in] nmat Number of materials in this PDE system
//! \param[in] offset Index for equation system
//! \param[in] ndof Total number of reconstructed dofs
//! \param[in] e Element being checked for consistency
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in,out] U Second-order solution vector which gets modified near
//!   material interfaces for consistency
//! \param[in,out] phic Vector of limiter functions for the conserved quantities
// *****************************************************************************
{
  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  // Extract the element coordinates
  std::array< std::array< tk::real, 3>, 4 > coordel {{
    {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
    {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
    {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
    {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }} }};

  // Compute the determinant of Jacobian matrix
  auto detT =
    tk::Jacobian( coordel[0], coordel[1], coordel[2], coordel[3] );

  std::vector< tk::real > phi_bound(nmat, 1.0);

  // loop over all faces of the element e
  for (std::size_t lf=0; lf<4; ++lf)
  {
    // Extract the face coordinates
    std::array< std::size_t, 3 > inpofa_l {{ inpoel[4*e+tk::lpofa[lf][0]],
                                             inpoel[4*e+tk::lpofa[lf][1]],
                                             inpoel[4*e+tk::lpofa[lf][2]] }};

    std::array< std::array< tk::real, 3>, 3 > coordfa {{
      {{ cx[ inpofa_l[0] ], cy[ inpofa_l[0] ], cz[ inpofa_l[0] ] }},
      {{ cx[ inpofa_l[1] ], cy[ inpofa_l[1] ], cz[ inpofa_l[1] ] }},
      {{ cx[ inpofa_l[2] ], cy[ inpofa_l[2] ], cz[ inpofa_l[2] ] }} }};

    auto ng = tk::NGfa(ndof);

    // arrays for quadrature points
    std::array< std::vector< tk::real >, 2 > coordgp;
    std::vector< tk::real > wgp;

    coordgp[0].resize( ng );
    coordgp[1].resize( ng );
    wgp.resize( ng );

    // get quadrature point weights and coordinates for triangle
    tk::GaussQuadratureTri( ng, coordgp, wgp );

    // Compute the upper and lower bound for volume fraction
    tk::real min = 1e-14;
    tk::real max = 1.0 - min;

    // Gaussian quadrature
    for (std::size_t igp=0; igp<ng; ++igp)
    {
      // Compute the coordinates of quadrature point at physical domain
      auto gp = tk::eval_gp( igp, coordfa, coordgp );

      //Compute the basis functions
      auto B = tk::eval_basis( ndof,
            tk::Jacobian( coordel[0], gp, coordel[2], coordel[3] ) / detT,
            tk::Jacobian( coordel[0], coordel[1], gp, coordel[3] ) / detT,
            tk::Jacobian( coordel[0], coordel[1], coordel[2], gp ) / detT );

      auto state = eval_state( U.nprop()/ndof, offset, ndof, ndof, e, U, B );

      for(std::size_t imat = 0; imat < nmat; imat++)
      {
        tk::real phi(1.0);
        auto al = state[volfracIdx(nmat, imat)];
        if(al > 1.0)
        {
          phi = std::fabs(
                  (max - U(e,volfracDofIdx(nmat, imat, ndof, 0),offset))
                / (al  - U(e,volfracDofIdx(nmat, imat, ndof, 0),offset)) );
        }
        else if(al < 1e-14)
        {
          phi = std::fabs(
                    (min - U(e,volfracDofIdx(nmat, imat, ndof, 0),offset))
                  / (al  - U(e,volfracDofIdx(nmat, imat, ndof, 0),offset)) );
        }

        phi_bound[imat] = std::min( phi_bound[imat], phi );
      }
    }
  }

  for(std::size_t imat = 0; imat < nmat; imat++)
    phic[imat] = phi_bound[imat] * phic[imat];
}

bool
interfaceIndicator( std::size_t nmat,
  const std::vector< tk::real >& al,
  std::vector< std::size_t >& matInt )
// *****************************************************************************
//  Interface indicator function, which checks element for material interface
//! \param[in] nmat Number of materials in this PDE system
//! \param[in] al Cell-averaged volume fractions
//! \param[in] matInt Array indicating which material has an interface
//! \return Boolean which indicates if the element contains a material interface
// *****************************************************************************
{
  bool intInd = false;

  // limits under which compression is to be performed
  auto al_eps = 1e-08;
  auto loLim = 2.0 * al_eps;
  auto hiLim = 1.0 - loLim;

  auto almax = 0.0;
  for (std::size_t k=0; k<nmat; ++k)
  {
    almax = std::max(almax, al[k]);
    matInt[k] = 0;
    if ((al[k] > loLim) && (al[k] < hiLim)) matInt[k] = 1;
  }

  if ((almax > loLim) && (almax < hiLim)) intInd = true;

  return intInd;
}

void TransformBasis( ncomp_t ncomp,
                     ncomp_t offset,
                     const std::size_t e,
                     const std::size_t ndof,
                     const tk::Fields& U,
                     const std::vector< std::size_t >& inpoel,
                     const tk::UnsMesh::Coords& coord,
                     std::vector< std::vector< tk::real > >& unk)
// *****************************************************************************
//  Transform the solution with Dubiner basis to the solution with Taylor basis
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] offset Index for equation systems
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] U High-order solution vector with Dubiner basis
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] unk High-order solution vector with Taylor basis
// *****************************************************************************
{
  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  std::vector< tk::real > center{0.25, 0.25, 0.25};

  // Evaluate the cell center solution
  for(ncomp_t icomp = 0; icomp < ncomp; icomp++)
  {
    auto mark = icomp * ndof;
    unk[icomp][0] = U(e, mark, offset);
  }

  // Evaluate the first order derivative
  std::array< std::array< tk::real, 3>, 4 > coordel {{
    {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
    {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
    {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
    {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }}
  }};

  auto jacInv =
              tk::inverseJacobian( coordel[0], coordel[1], coordel[2], coordel[3] );

  // Compute the derivatives of basis function for DG(P1)
  auto dBdx = tk::eval_dBdx_p1( ndof, jacInv );

  if(ndof > 4)
    tk::evaldBdx_p2(center, jacInv, dBdx);

  for(ncomp_t icomp = 0; icomp < ncomp; icomp++)
  {
    auto mark = icomp * ndof;
    for(std::size_t idir = 0; idir < 3; idir++)
    {
      unk[icomp][idir+1] = 0;
      for(std::size_t idof = 1; idof < ndof; idof++)
        unk[icomp][idir+1] += U(e, mark+idof, offset) * dBdx[idir][idof];
    }
  }

  // Evaluate the second order derivative if DGP2 is applied
  if(ndof > 4)
  {
    std::array< std::array< tk::real, 6 >, 6 > dB2dxi2;
    dB2dxi2[0][0] = 12.0;
    dB2dxi2[1][0] =  2.0;
    dB2dxi2[2][0] =  2.0;
    dB2dxi2[3][0] =  6.0;
    dB2dxi2[4][0] =  6.0;
    dB2dxi2[5][0] =  2.0;

    dB2dxi2[0][1] =  0.0;
    dB2dxi2[1][1] = 10.0;
    dB2dxi2[2][1] =  2.0;
    dB2dxi2[3][1] = 10.0;
    dB2dxi2[4][1] =  2.0;
    dB2dxi2[5][1] =  6.0;

    dB2dxi2[0][2] =  0.0;
    dB2dxi2[1][2] =  0.0;
    dB2dxi2[2][2] = 12.0;
    dB2dxi2[3][2] =  0.0;
    dB2dxi2[4][2] = 12.0;
    dB2dxi2[5][2] =  6.0;

    dB2dxi2[0][3] =  0.0;
    dB2dxi2[1][3] = 20.0;
    dB2dxi2[2][3] =  2.0;
    dB2dxi2[3][3] =  0.0;
    dB2dxi2[4][3] =  0.0;
    dB2dxi2[5][3] =  8.0;

    dB2dxi2[0][4] =  0.0;
    dB2dxi2[1][4] =  0.0;
    dB2dxi2[2][4] = 12.0;
    dB2dxi2[3][4] =  0.0;
    dB2dxi2[4][4] =  0.0;
    dB2dxi2[5][4] = 18.0;

    dB2dxi2[0][5] =  0.0;
    dB2dxi2[1][5] =  0.0;
    dB2dxi2[2][5] = 30.0;
    dB2dxi2[3][5] =  0.0;
    dB2dxi2[4][5] =  0.0;
    dB2dxi2[5][5] =  0.0;

    std::vector< std::vector< tk::real > > d2Bdx2;
    d2Bdx2.resize(6, std::vector< tk::real>(6,0.0) );
    for(std::size_t ibasis = 0; ibasis < 6; ibasis++)
    {
      for(std::size_t idir = 0; idir < 3; idir++)
        d2Bdx2[idir][ibasis] += dB2dxi2[0][ibasis] * jacInv[0][idir] * jacInv[0][idir]
                              + dB2dxi2[1][ibasis] * jacInv[1][idir] * jacInv[1][idir]
                              + dB2dxi2[2][ibasis] * jacInv[2][idir] * jacInv[2][idir]
                              + 2.0 * ( dB2dxi2[3][ibasis] * jacInv[0][idir] * jacInv[1][idir]
                                      + dB2dxi2[4][ibasis] * jacInv[0][idir] * jacInv[2][idir]
                                      + dB2dxi2[5][ibasis] * jacInv[1][idir] * jacInv[2][idir] );
      d2Bdx2[3][ibasis] += dB2dxi2[0][ibasis] * jacInv[0][0] * jacInv[0][1]
                         + dB2dxi2[1][ibasis] * jacInv[1][0] * jacInv[1][1]
                         + dB2dxi2[2][ibasis] * jacInv[2][0] * jacInv[2][1]
                         + dB2dxi2[3][ibasis] * (jacInv[0][0] * jacInv[1][1] + jacInv[1][0] * jacInv[0][1])
                         + dB2dxi2[4][ibasis] * (jacInv[0][0] * jacInv[2][1] + jacInv[2][0] * jacInv[0][1])
                         + dB2dxi2[5][ibasis] * (jacInv[1][0] * jacInv[2][1] + jacInv[2][0] * jacInv[1][1]);
      d2Bdx2[4][ibasis] += dB2dxi2[0][ibasis] * jacInv[0][0] * jacInv[0][2]
                         + dB2dxi2[1][ibasis] * jacInv[1][0] * jacInv[1][2]
                         + dB2dxi2[2][ibasis] * jacInv[2][0] * jacInv[2][2]
                         + dB2dxi2[3][ibasis] * (jacInv[0][0] * jacInv[1][2] + jacInv[1][0] * jacInv[0][2])
                         + dB2dxi2[4][ibasis] * (jacInv[0][0] * jacInv[2][2] + jacInv[2][0] * jacInv[0][2])
                         + dB2dxi2[5][ibasis] * (jacInv[1][0] * jacInv[2][2] + jacInv[2][0] * jacInv[1][2]);
      d2Bdx2[5][ibasis] += dB2dxi2[0][ibasis] * jacInv[0][1] * jacInv[0][2]
                         + dB2dxi2[1][ibasis] * jacInv[1][1] * jacInv[1][2]
                         + dB2dxi2[2][ibasis] * jacInv[2][1] * jacInv[2][2]
                         + dB2dxi2[3][ibasis] * (jacInv[0][1] * jacInv[1][2] + jacInv[1][1] * jacInv[0][2])
                         + dB2dxi2[4][ibasis] * (jacInv[0][1] * jacInv[2][2] + jacInv[2][1] * jacInv[0][2])
                         + dB2dxi2[5][ibasis] * (jacInv[1][1] * jacInv[2][2] + jacInv[2][1] * jacInv[1][2]);
    }

    for(ncomp_t icomp = 0; icomp < ncomp; icomp++)
    {
      auto mark = icomp * ndof;
      for(std::size_t idir = 0; idir < 6; idir++)
      {
        unk[icomp][idir+4] = 0;
        for(std::size_t ibasis = 0; ibasis < 6; ibasis++)
          unk[icomp][idir+4] += U(e, mark+4+ibasis, offset) * d2Bdx2[idir][ibasis];
      }
    }
  }
}

void InverseBasis( ncomp_t ncomp,
                   ncomp_t offset,
                   std::size_t e,
                   std::size_t ndof,
                   const std::vector< std::size_t >& inpoel,
                   const tk::UnsMesh::Coords& coord,
                   const tk::Fields& geoElem,
                   tk::Fields& U,
                   std::vector< std::vector< tk::real > >& unk )
// *****************************************************************************
//  Convert the solution with Taylor basis to the solution with Dubiner basis
//! \param[in] ncomp Number of scalar components in this PDE system
//! \param[in] e Id of element whose solution is to be limited
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] inpoel Element connectivity
//! \param[in] coord Array of nodal coordinates
//! \param[in] U High-order solution vector with Dubiner basis
//! \param[in] unk High-order solution vector with Taylor basis
// *****************************************************************************
{
  // The diagonal of mass matrix
  std::vector< tk::real > L(ndof, 0.0);

  tk::real vol = 1.0 / 6.0;

  L[0] = vol;

  L[1] = vol / 10.0;
  L[2] = vol * 3.0/10.0;
  L[3] = vol * 3.0/5.0;

  if(ndof > 4)
  {
    L[4] = vol / 35.0;
    L[5] = vol / 21.0;
    L[6] = vol / 14.0;
    L[7] = vol / 7.0;
    L[8] = vol * 3.0/14.0;
    L[9] = vol * 3.0/7.0;
  }

  // Coordinates of the centroid in physical domain
  std::array< tk::real, 3 > x_c{geoElem(e,1,0), geoElem(e,2,0), geoElem(e,3,0),};

  const auto& cx = coord[0];
  const auto& cy = coord[1];
  const auto& cz = coord[2];

  std::array< std::array< tk::real, 3>, 4 > coordel {{
    {{ cx[ inpoel[4*e  ] ], cy[ inpoel[4*e  ] ], cz[ inpoel[4*e  ] ] }},
    {{ cx[ inpoel[4*e+1] ], cy[ inpoel[4*e+1] ], cz[ inpoel[4*e+1] ] }},
    {{ cx[ inpoel[4*e+2] ], cy[ inpoel[4*e+2] ], cz[ inpoel[4*e+2] ] }},
    {{ cx[ inpoel[4*e+3] ], cy[ inpoel[4*e+3] ], cz[ inpoel[4*e+3] ] }}
  }};

  // Number of quadrature points for volume integration
  auto ng = tk::NGvol(ndof);

  // arrays for quadrature points
  std::array< std::vector< tk::real >, 3 > coordgp;
  std::vector< tk::real > wgp;

  coordgp[0].resize( ng );
  coordgp[1].resize( ng );
  coordgp[2].resize( ng );
  wgp.resize( ng );

  // get quadrature point weights and coordinates for triangle
  tk::GaussQuadratureTet( ng, coordgp, wgp );

  // right hand side vector
  std::vector< tk::real > R( ncomp*ndof, 0.0 );

  // Gaussian quadrature
  for (std::size_t igp=0; igp<ng; ++igp)
  {
    auto wt = wgp[igp] * vol;

    auto gp = tk::eval_gp( igp, coordel, coordgp );

    auto B_taylor = eval_TaylorBasis( ndof, gp, x_c, coordel);

    // Compute high order solution at gauss point
    std::vector< tk::real > state( ncomp, 0.0 );
    for (ncomp_t c=0; c<ncomp; ++c)
    {
      state[c] = unk[c][0];
      state[c] += unk[c][1] * B_taylor[1] + unk[c][2] * B_taylor[2] + unk[c][3] * B_taylor[3];

      if(ndof > 4)
        state[c] += unk[c][4] * B_taylor[4] + unk[c][5] * B_taylor[5] + unk[c][6] * B_taylor[6]
                  + unk[c][7] * B_taylor[7] + unk[c][8] * B_taylor[8] + unk[c][9] * B_taylor[9];
    }

    auto B = tk::eval_basis( ndof, coordgp[0][igp], coordgp[1][igp], coordgp[2][igp] );

    for (ncomp_t c=0; c<ncomp; ++c)
    {
      auto mark = c*ndof;
      R[mark] += wt * state[c];

      if(ndof > 1)
      {
        R[mark+1] += wt * state[c] * B[1];
        R[mark+2] += wt * state[c] * B[2];
        R[mark+3] += wt * state[c] * B[3];

        if(ndof > 4)
        {
          R[mark+4] += wt * state[c] * B[4];
          R[mark+5] += wt * state[c] * B[5];
          R[mark+6] += wt * state[c] * B[6];
          R[mark+7] += wt * state[c] * B[7];
          R[mark+8] += wt * state[c] * B[8];
          R[mark+9] += wt * state[c] * B[9];
        }
      }
    }
  }

  for (ncomp_t c=0; c<ncomp; ++c)
  {
    auto mark = c*ndof;
    U(e, mark, offset) = R[mark] / L[0];

    if(ndof > 1)
    {
      U(e, mark+1, offset) = R[mark+1] / L[1];
      U(e, mark+2, offset) = R[mark+2] / L[2];
      U(e, mark+3, offset) = R[mark+3] / L[3];

      if(ndof > 4)
      {
        U(e, mark+4, offset) = R[mark+4] / L[4];
        U(e, mark+5, offset) = R[mark+5] / L[5];
        U(e, mark+6, offset) = R[mark+6] / L[6];
        U(e, mark+7, offset) = R[mark+7] / L[7];
        U(e, mark+8, offset) = R[mark+8] / L[8];
        U(e, mark+9, offset) = R[mark+9] / L[9];
      }
    }
  }
}

std::vector< tk::real >
eval_TaylorBasis( const std::size_t ndof,
                  const std::array< tk::real, 3 >& x,
                  const std::array< tk::real, 3 >& x_c,
                  const std::array< std::array< tk::real, 3>, 4 >& coordel )
// *****************************************************************************
//  Evaluate the Taylor basis at points
//! \param[in] ndof Maximum number of degrees of freedom
//! \param[in] x Nodal coordinates
//! \param[in] x_c Coordinates of the centroid
//! \param[in] coordel Array of nodal coordinates for the tetrahedron
// *****************************************************************************
{
  std::vector< tk::real > avg( 6, 0.0 );
  if(ndof > 4)
  {
    auto ng = tk::NGvol(ndof);

    std::array< std::vector< tk::real >, 3 > coordgp;
    std::vector< tk::real > wgp;

    coordgp[0].resize( ng );
    coordgp[1].resize( ng );
    coordgp[2].resize( ng );
    wgp.resize( ng );

    tk::GaussQuadratureTet( ng, coordgp, wgp );

    for (std::size_t igp=0; igp<ng; ++igp)
    {
      // Compute the coordinates of quadrature point at physical domain
      auto gp = tk::eval_gp( igp, coordel, coordgp );

      avg[0] += wgp[igp] * (gp[0] - x_c[0]) * (gp[0] - x_c[0]) * 0.5;
      avg[1] += wgp[igp] * (gp[1] - x_c[1]) * (gp[1] - x_c[1]) * 0.5;
      avg[2] += wgp[igp] * (gp[2] - x_c[2]) * (gp[2] - x_c[2]) * 0.5;
      avg[3] += wgp[igp] * (gp[0] - x_c[0]) * (gp[1] - x_c[1]);
      avg[4] += wgp[igp] * (gp[0] - x_c[0]) * (gp[2] - x_c[2]);
      avg[5] += wgp[igp] * (gp[1] - x_c[1]) * (gp[2] - x_c[2]);
    }
  }

  std::vector< tk::real > B( ndof, 1.0 );

  B[1] = x[0] - x_c[0];
  B[2] = x[1] - x_c[1];
  B[3] = x[2] - x_c[2];

  if( ndof > 4 )
  {
    B[4] = B[1] * B[1] * 0.5 - avg[0];
    B[5] = B[2] * B[2] * 0.5 - avg[1];
    B[6] = B[3] * B[3] * 0.5 - avg[2];
    B[7] = B[1] * B[2] - avg[3];
    B[8] = B[1] * B[3] - avg[4];
    B[9] = B[2] * B[3] - avg[5];
  }

  return B;
}

} // inciter::
