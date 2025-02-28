// *****************************************************************************
/*!
  \file      src/PDE/MultiMat/MiscMultiMatFns.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Misc multi-material system functions
  \details   This file defines functions that required for multi-material
    compressible hydrodynamics.
*/
// *****************************************************************************

#include "MiscMultiMatFns.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"
#include "Integrate/Basis.hpp"
#include "MultiMat/MultiMatIndexing.hpp"
#include "EoS/EoS.hpp"
#include "EoS/StiffenedGas.hpp"
#include "EoS/JWL.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

void initializeMaterialEoS( std::size_t system,
  std::vector< EoS_Base* >& mat_blk )
// *****************************************************************************
//  Initialize the material block with configured EOS
//! \param[in] system Index of system being solved
//! \param[in,out] mat_blk Material block that gets initialized
// *****************************************************************************
{
  // EoS initialization
  auto nmat =
    g_inputdeck.get< tag::param, tag::multimat, tag::nmat >()[system];
  const auto& matprop = g_inputdeck.get< tag::param, tag::multimat,
    tag::material >()[system];
  const auto& matidxmap = g_inputdeck.get< tag::param, tag::multimat,
    tag::matidxmap >();
  for (std::size_t k=0; k<nmat; ++k) {
    auto mateos = matprop[matidxmap.get< tag::eosidx >()[k]].get<tag::eos>();
    if (mateos == inciter::ctr::MaterialType::STIFFENEDGAS) {
      // query input deck to get gamma, p_c, cv
      auto g = gamma< tag::multimat >(system, k);
      auto ps = pstiff< tag::multimat >(system, k);
      auto c_v = cv< tag::multimat >(system, k);
      mat_blk.push_back(new StiffenedGas(g, ps, c_v));
    }
    else if (mateos == inciter::ctr::MaterialType::JWL) {
      // query input deck to get jwl parameters
      auto w = getmatprop< tag::multimat, tag::w_gru >(system, k);
      auto c_v = cv< tag::multimat >(system, k);
      auto A_jwl = getmatprop< tag::multimat, tag::A_jwl >(system, k);
      auto B_jwl = getmatprop< tag::multimat, tag::B_jwl >(system, k);
      //[[maybe_unused]] auto C_jwl =
      //  getmatprop< tag::multimat, tag::C_jwl >(system, k);
      auto R1_jwl = getmatprop< tag::multimat, tag::R1_jwl >(system, k);
      auto R2_jwl = getmatprop< tag::multimat, tag::R2_jwl >(system, k);
      auto rho0_jwl = getmatprop< tag::multimat, tag::rho0_jwl >(system, k);
      auto e0_jwl = getmatprop< tag::multimat, tag::e0_jwl >(system, k);
      mat_blk.push_back(new JWL(w, c_v, rho0_jwl, e0_jwl, A_jwl, B_jwl, R1_jwl,
        R2_jwl));
    }
  }
}

bool
cleanTraceMultiMat(
  std::size_t nelem,
  std::size_t system,
  const std::vector< EoS_Base* >& mat_blk,
  const tk::Fields& geoElem,
  std::size_t nmat,
  tk::Fields& U,
  tk::Fields& P )
// *****************************************************************************
//  Clean up the state of trace materials for multi-material PDE system
//! \param[in] nelem Number of elements
//! \param[in] system Index for equation systems
//! \param[in] mat_blk EOS material block
//! \param[in] geoElem Element geometry array
//! \param[in] nmat Number of materials in this PDE system
//! \param[in/out] U High-order solution vector which gets modified
//! \param[in/out] P High-order vector of primitives which gets modified
//! \return Boolean indicating if an unphysical material state was found
// *****************************************************************************
{
  const auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();
  auto al_eps = 1.0e-02;
  auto neg_density = false;

  for (std::size_t e=0; e<nelem; ++e)
  {
    // find material in largest quantity, and determine if pressure
    // relaxation is needed. If it is, determine materials that need
    // relaxation, and the total volume of these materials.
    std::vector< int > relaxInd(nmat, 0);
    auto almax(0.0), relaxVol(0.0);
    std::size_t kmax = 0;
    for (std::size_t k=0; k<nmat; ++k)
    {
      auto al = U(e, volfracDofIdx(nmat, k, rdof, 0));
      if (al > almax)
      {
        almax = al;
        kmax = k;
      }
      else if (al < al_eps)
      {
        relaxInd[k] = 1;
        relaxVol += al;
      }
    }
    relaxInd[kmax] = 1;
    relaxVol += almax;

    auto u = P(e, velocityDofIdx(nmat, 0, rdof, 0));
    auto v = P(e, velocityDofIdx(nmat, 1, rdof, 0));
    auto w = P(e, velocityDofIdx(nmat, 2, rdof, 0));
    auto pmax = P(e, pressureDofIdx(nmat, kmax, rdof, 0))/almax;
    auto tmax = mat_blk[kmax]->eos_temperature(
      U(e, densityDofIdx(nmat, kmax, rdof, 0)), u, v, w,
      U(e, energyDofIdx(nmat, kmax, rdof, 0)), almax );

    tk::real p_target(0.0), d_al(0.0), d_arE(0.0);
    //// get equilibrium pressure
    //std::vector< tk::real > kmat(nmat, 0.0);
    //tk::real ratio(0.0);
    //for (std::size_t k=0; k<nmat; ++k)
    //{
    //  auto arhok = U(e, densityDofIdx(nmat,k,rdof,0));
    //  auto alk = U(e, volfracDofIdx(nmat,k,rdof,0));
    //  auto apk = P(e, pressureDofIdx(nmat,k,rdof,0));
    //  auto ak = eos_soundspeed< tag::multimat >(system, arhok, apk, alk, k );
    //  kmat[k] = arhok * ak * ak;

    //  p_target += alk * apk / kmat[k];
    //  ratio += alk * alk / kmat[k];
    //}
    //p_target /= ratio;
    //p_target = std::max(p_target, 1e-14);
    p_target = std::max(pmax, 1e-14);

    // 1. Correct minority materials and store volume/energy changes
    for (std::size_t k=0; k<nmat; ++k)
    {
      auto alk = U(e, volfracDofIdx(nmat, k, rdof, 0));
      auto pk = P(e, pressureDofIdx(nmat, k, rdof, 0)) / alk;
      auto Pck = pstiff< tag::multimat >(system, k);
      // for positive volume fractions
      if (matExists(alk))
      {
        // check if volume fraction is lesser than threshold (al_eps) and
        // if the material (effective) pressure is negative. If either of
        // these conditions is true, perform pressure relaxation.
        if ((alk < al_eps) || (pk+Pck < 0.0)/*&& (std::fabs((pk-pmax)/pmax) > 1e-08)*/)
        {
          //auto gk = gamma< tag::multimat >(system, k);

          tk::real alk_new(0.0);
          //// volume change based on polytropic expansion/isentropic compression
          //if (pk > p_target)
          //{
          //  alk_new = std::pow((pk/p_target), (1.0/gk)) * alk;
          //}
          //else
          //{
          //  auto arhok = U(e, densityDofIdx(nmat, k, rdof, 0));
          //  auto ck = eos_soundspeed< tag::multimat >(system, arhok, alk*pk,
          //    alk, k);
          //  auto kk = arhok * ck * ck;
          //  alk_new = alk - (alk*alk/kk) * (p_target-pk);
          //}
          alk_new = alk;

          // energy change
          auto rhomat = U(e, densityDofIdx(nmat, k, rdof, 0))
            / alk_new;
          auto rhoEmat = mat_blk[k]->eos_totalenergy( rhomat, u, v, w,
                                                      p_target);

          // volume-fraction and total energy flux into majority material
          d_al += (alk - alk_new);
          d_arE += (U(e, energyDofIdx(nmat, k, rdof, 0))
            - alk_new * rhoEmat);

          // update state of trace material
          U(e, volfracDofIdx(nmat, k, rdof, 0)) = alk_new;
          U(e, energyDofIdx(nmat, k, rdof, 0)) = alk_new*rhoEmat;
          P(e, pressureDofIdx(nmat, k, rdof, 0)) = alk_new*p_target;
        }
      }
      // check for unbounded volume fractions
      else if (alk < 0.0)
      {
        auto rhok = mat_blk[k]->eos_density(p_target, tmax);
        d_al += (alk - 1e-14);
        // update state of trace material
        U(e, volfracDofIdx(nmat, k, rdof, 0)) = 1e-14;
        U(e, densityDofIdx(nmat, k, rdof, 0)) = 1e-14 * rhok;
        U(e, energyDofIdx(nmat, k, rdof, 0)) = 1e-14
          * mat_blk[k]->eos_totalenergy(rhok, u, v, w, p_target );
        P(e, pressureDofIdx(nmat, k, rdof, 0)) = 1e-14 *
          p_target;
        for (std::size_t i=1; i<rdof; ++i) {
          U(e, volfracDofIdx(nmat, k, rdof, i)) = 0.0;
          U(e, densityDofIdx(nmat, k, rdof, i)) = 0.0;
          U(e, energyDofIdx(nmat, k, rdof, i)) = 0.0;
          P(e, pressureDofIdx(nmat, k, rdof, i)) = 0.0;
        }
      }
      else {
        auto rhok = U(e, densityDofIdx(nmat, k, rdof, 0)) / alk;
        // update state of trace material
        U(e, energyDofIdx(nmat, k, rdof, 0)) = alk
          * mat_blk[k]->eos_totalenergy( rhok, u, v, w, p_target );
        P(e, pressureDofIdx(nmat, k, rdof, 0)) = alk *
          p_target;
        for (std::size_t i=1; i<rdof; ++i) {
          U(e, energyDofIdx(nmat, k, rdof, i)) = 0.0;
          P(e, pressureDofIdx(nmat, k, rdof, i)) = 0.0;
        }
      }
    }

    // 2. Based on volume change in majority material, compute energy change
    //auto gmax = gamma< tag::multimat >(system, kmax);
    //auto pmax_new = pmax * std::pow(almax/(almax+d_al), gmax);
    //auto rhomax_new = U(e, densityDofIdx(nmat, kmax, rdof, 0))
    //  / (almax+d_al);
    //auto rhoEmax_new = eos_totalenergy< tag::multimat >(system, rhomax_new, u,
    //  v, w, pmax_new, kmax);
    //auto d_arEmax_new = (almax+d_al) * rhoEmax_new
    //  - U(e, energyDofIdx(nmat, kmax, rdof, 0));

    U(e, volfracDofIdx(nmat, kmax, rdof, 0)) += d_al;
    //U(e, energyDofIdx(nmat, kmax, rdof, 0)) += d_arEmax_new;

    // 2. Flux energy change into majority material
    U(e, energyDofIdx(nmat, kmax, rdof, 0)) += d_arE;
    P(e, pressureDofIdx(nmat, kmax, rdof, 0)) =
      mat_blk[kmax]->eos_pressure(
      U(e, densityDofIdx(nmat, kmax, rdof, 0)), u, v, w,
      U(e, energyDofIdx(nmat, kmax, rdof, 0)),
      U(e, volfracDofIdx(nmat, kmax, rdof, 0)) );

    // enforce unit sum of volume fractions
    auto alsum = 0.0;
    for (std::size_t k=0; k<nmat; ++k)
      alsum += U(e, volfracDofIdx(nmat, k, rdof, 0));

    for (std::size_t k=0; k<nmat; ++k) {
      U(e, volfracDofIdx(nmat, k, rdof, 0)) /= alsum;
      U(e, densityDofIdx(nmat, k, rdof, 0)) /= alsum;
      U(e, energyDofIdx(nmat, k, rdof, 0)) /= alsum;
      P(e, pressureDofIdx(nmat, k, rdof, 0)) /= alsum;
    }

    //// bulk quantities
    //auto rhoEb(0.0), rhob(0.0), volb(0.0);
    //for (std::size_t k=0; k<nmat; ++k)
    //{
    //  if (relaxInd[k] > 0.0)
    //  {
    //    rhoEb += U(e, energyDofIdx(nmat,k,rdof,0));
    //    volb += U(e, volfracDofIdx(nmat,k,rdof,0));
    //    rhob += U(e, densityDofIdx(nmat,k,rdof,0));
    //  }
    //}

    //// 2. find mixture-pressure
    //tk::real pmix(0.0), den(0.0);
    //pmix = rhoEb - 0.5*rhob*(u*u+v*v+w*w);
    //for (std::size_t k=0; k<nmat; ++k)
    //{
    //  auto gk = gamma< tag::multimat >(system, k);
    //  auto Pck = pstiff< tag::multimat >(system, k);

    //  pmix -= U(e, volfracDofIdx(nmat,k,rdof,0)) * gk * Pck *
    //    relaxInd[k] / (gk-1.0);
    //  den += U(e, volfracDofIdx(nmat,k,rdof,0)) * relaxInd[k]
    //    / (gk-1.0);
    //}
    //pmix /= den;

    //// 3. correct energies
    //for (std::size_t k=0; k<nmat; ++k)
    //{
    //  if (relaxInd[k] > 0.0)
    //  {
    //    auto alk_new = U(e, volfracDofIdx(nmat,k,rdof,0));
    //    U(e, energyDofIdx(nmat,k,rdof,0)) = alk_new *
    //      eos_totalenergy< tag::multimat >(system, rhomat[k], u, v, w, pmix,
    //      k);
    //    P(e, pressureDofIdx(nmat, k, rdof, 0)) = alk_new * pmix;
    //  }
    //}

    pmax = P(e, pressureDofIdx(nmat, kmax, rdof, 0)) /
      U(e, volfracDofIdx(nmat, kmax, rdof, 0));

    // check for unphysical state
    for (std::size_t k=0; k<nmat; ++k)
    {
      auto alpha = U(e, volfracDofIdx(nmat, k, rdof, 0));
      auto arho = U(e, densityDofIdx(nmat, k, rdof, 0));
      auto apr = P(e, pressureDofIdx(nmat, k, rdof, 0));

      // lambda for screen outputs
      auto screenout = [&]()
      {
        std::cout << "Element centroid: " << geoElem(e,1) << ", "
          << geoElem(e,2) << ", " << geoElem(e,3) << std::endl;
        std::cout << "Material-id:      " << k << std::endl;
        std::cout << "Volume-fraction:  " << alpha << std::endl;
        std::cout << "Partial density:  " << arho << std::endl;
        std::cout << "Partial pressure: " << apr << std::endl;
        std::cout << "Major pressure:   " << pmax << std::endl;
        std::cout << "Major temperature:" << tmax << std::endl;
        std::cout << "Velocity:         " << u << ", " << v << ", " << w
          << std::endl;
      };

      if (arho < 0.0)
      {
        neg_density = true;
        screenout();
      }
    }
  }
  return neg_density;
}

tk::real
timeStepSizeMultiMat(
  const std::vector< EoS_Base* >& mat_blk,
  const std::vector< int >& esuf,
  const tk::Fields& geoFace,
  const tk::Fields& geoElem,
  const std::size_t nelem,
  std::size_t nmat,
  const tk::Fields& U,
  const tk::Fields& P )
// *****************************************************************************
//  Time step restriction for multi material cell-centered schemes
//! \param[in] mat_blk EOS material block
//! \param[in] esuf Elements surrounding elements array
//! \param[in] geoFace Face geometry array
//! \param[in] geoElem Element geometry array
//! \param[in] nelem Number of elements
//! \param[in] nmat Number of materials in this PDE system
//! \param[in] U High-order solution vector
//! \param[in] P High-order vector of primitives
//! \return Maximum allowable time step based on cfl criterion
// *****************************************************************************
{
  const auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
  const auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();
  std::size_t ncomp = U.nprop()/rdof;
  std::size_t nprim = P.nprop()/rdof;

  tk::real u, v, w, a, vn, dSV_l, dSV_r;
  std::vector< tk::real > delt(U.nunk(), 0.0);
  std::vector< tk::real > ugp(ncomp, 0.0), pgp(nprim, 0.0);

  // compute maximum characteristic speed at all internal element faces
  for (std::size_t f=0; f<esuf.size()/2; ++f)
  {
    std::size_t el = static_cast< std::size_t >(esuf[2*f]);
    auto er = esuf[2*f+1];

    // left element

    // Compute the basis function for the left element
    std::vector< tk::real > B_l(rdof, 0.0);
    B_l[0] = 1.0;

    // get conserved quantities
    ugp = eval_state(ncomp, rdof, ndof, el, U, B_l, {0, ncomp-1});
    // get primitive quantities
    pgp = eval_state(nprim, rdof, ndof, el, P, B_l, {0, nprim-1});

    // advection velocity
    u = pgp[velocityIdx(nmat, 0)];
    v = pgp[velocityIdx(nmat, 1)];
    w = pgp[velocityIdx(nmat, 2)];

    vn = u*geoFace(f,1) + v*geoFace(f,2) + w*geoFace(f,3);

    // acoustic speed
    a = 0.0;
    for (std::size_t k=0; k<nmat; ++k)
    {
      if (ugp[volfracIdx(nmat, k)] > 1.0e-04) {
        a = std::max( a, mat_blk[k]->eos_soundspeed( ugp[densityIdx(nmat, k)],
          pgp[pressureIdx(nmat, k)], ugp[volfracIdx(nmat, k)] ) );
      }
    }

    dSV_l = geoFace(f,0) * (std::fabs(vn) + a);

    // right element
    if (er > -1) {
      std::size_t eR = static_cast< std::size_t >( er );

      // Compute the basis function for the right element
      std::vector< tk::real > B_r(rdof, 0.0);
      B_r[0] = 1.0;

      // get conserved quantities
      ugp = eval_state( ncomp, rdof, ndof, eR, U, B_r, {0, ncomp-1});
      // get primitive quantities
      pgp = eval_state( nprim, rdof, ndof, eR, P, B_r, {0, nprim-1});

      // advection velocity
      u = pgp[velocityIdx(nmat, 0)];
      v = pgp[velocityIdx(nmat, 1)];
      w = pgp[velocityIdx(nmat, 2)];

      vn = u*geoFace(f,1) + v*geoFace(f,2) + w*geoFace(f,3);

      // acoustic speed
      a = 0.0;
      for (std::size_t k=0; k<nmat; ++k)
      {
        if (ugp[volfracIdx(nmat, k)] > 1.0e-04) {
          a = std::max( a, mat_blk[k]->eos_soundspeed( ugp[densityIdx(nmat, k)],
            pgp[pressureIdx(nmat, k)], ugp[volfracIdx(nmat, k)] ) );
        }
      }

      dSV_r = geoFace(f,0) * (std::fabs(vn) + a);

      delt[eR] += std::max( dSV_l, dSV_r );
    } else {
      dSV_r = dSV_l;
    }

    delt[el] += std::max( dSV_l, dSV_r );
  }

  tk::real mindt = std::numeric_limits< tk::real >::max();

  // compute allowable dt
  for (std::size_t e=0; e<nelem; ++e)
  {
    mindt = std::min( mindt, geoElem(e,0)/delt[e] );
  }

  return mindt;
}

tk::real
timeStepSizeMultiMatFV(
  const std::vector< EoS_Base* >& mat_blk,
  const tk::Fields& geoElem,
  const std::size_t nelem,
  std::size_t system,
  std::size_t nmat,
  const int engSrcAd,
  const tk::Fields& U,
  const tk::Fields& P )
// *****************************************************************************
//  Time step restriction for multi material cell-centered FV scheme
//! \param[in] mat_blk Material EOS block
//! \param[in] geoElem Element geometry array
//! \param[in] nelem Number of elements
//! \param[in] system Index for equation systems
//! \param[in] nmat Number of materials in this PDE system
//! \param[in] engSrcAd Whether the energy source was added
//! \param[in] U High-order solution vector
//! \param[in] P High-order vector of primitives
//! \return Maximum allowable time step based on cfl criterion
// *****************************************************************************
{
  const auto ndof = g_inputdeck.get< tag::discr, tag::ndof >();
  const auto rdof = g_inputdeck.get< tag::discr, tag::rdof >();
  std::size_t ncomp = U.nprop()/rdof;
  std::size_t nprim = P.nprop()/rdof;

  std::vector< tk::real > ugp(ncomp, 0.0), pgp(nprim, 0.0);
  tk::real mindt = std::numeric_limits< tk::real >::max();

  // determine front propagation speed if relevant energy sources were added
  tk::real v_front(0.0);
  if (engSrcAd == 1) {
    const auto& icmbk = g_inputdeck.get< tag::param, tag::multimat, tag::ic,
      tag::meshblock >();
    if (icmbk.size() > system) {
      for (const auto& b : icmbk[system]) { // for all blocks
        auto inittype = b.template get< tag::initiate, tag::init >();
        if (inittype == ctr::InitiateType::LINEAR) {
          v_front = std::max(v_front,
            b.template get< tag::initiate, tag::velocity >());
        }
      }
    }
  }

  // loop over all elements
  for (std::size_t e=0; e<nelem; ++e)
  {
    // basis function at centroid
    std::vector< tk::real > B(rdof, 0.0);
    B[0] = 1.0;

    // get conserved quantities
    ugp = eval_state(ncomp, rdof, ndof, e, U, B, {0, ncomp-1});
    // get primitive quantities
    pgp = eval_state(nprim, rdof, ndof, e, P, B, {0, nprim-1});

    // magnitude of advection velocity
    auto u = pgp[velocityIdx(nmat, 0)];
    auto v = pgp[velocityIdx(nmat, 1)];
    auto w = pgp[velocityIdx(nmat, 2)];
    auto vmag = std::sqrt(tk::dot({u, v, w}, {u, v, w}));

    // acoustic speed
    tk::real a = 0.0;
    for (std::size_t k=0; k<nmat; ++k)
    {
      if (ugp[volfracIdx(nmat, k)] > 1.0e-04) {
        a = std::max( a, mat_blk[k]->eos_soundspeed( ugp[densityIdx(nmat, k)],
          pgp[pressureIdx(nmat, k)], ugp[volfracIdx(nmat, k)] ) );
      }
    }

    // characteristic wave speed
    auto v_char = vmag + a + v_front;

    // characteristic length (radius of insphere)
    auto dx = std::min(std::cbrt(geoElem(e,0)), geoElem(e,4))
      /std::sqrt(24.0);

    // element dt
    mindt = std::min(mindt, dx/v_char);
  }

  return mindt;
}

} //inciter::
