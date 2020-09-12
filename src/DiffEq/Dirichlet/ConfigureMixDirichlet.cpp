// *****************************************************************************
/*!
  \file      src/DiffEq/Dirichlet/ConfigureMixDirichlet.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Register and compile configuration on the MixDirichlet SDE
  \details   Register and compile configuration on the MixDirichlet SDE.
*/
// *****************************************************************************

#include <set>
#include <map>
#include <vector>
#include <string>
#include <utility>

#include <brigand/algorithms/for_each.hpp>

#include "Tags.hpp"
#include "CartesianProduct.hpp"
#include "DiffEqFactory.hpp"
#include "Walker/Options/DiffEq.hpp"
#include "Walker/Options/InitPolicy.hpp"
#include "Walker/Options/Normalization.hpp"

#include "ConfigureMixDirichlet.hpp"
#include "MixDirichlet.hpp"
#include "MixDirichletCoeffPolicy.hpp"

namespace walker {

void
registerMixDirichlet( DiffEqFactory& f, std::set< ctr::DiffEqType >& t )
// *****************************************************************************
// Register MixDirichlet SDE into DiffEq factory
//! \param[in,out] f Differential equation factory to register to
//! \param[in,out] t Counters for equation types registered
// *****************************************************************************
{
  // Construct vector of vectors for all possible policies for SDE
  using MixDirPolicies =
    tk::cartesian_product< InitPolicies, MixDirichletCoeffPolicies >;
  // Register SDE for all combinations of policies
  brigand::for_each< MixDirPolicies >(
    registerDiffEq< MixDirichlet >( f, ctr::DiffEqType::MIXDIRICHLET, t ) );
}

std::vector< std::pair< std::string, std::string > >
infoMixDirichlet( std::map< ctr::DiffEqType, tk::ctr::ncomp_t >& cnt )
// *****************************************************************************
//  Return information on the MixDirichlet SDE
//! \param[inout] cnt std::map of counters for all differential equation types
//! \return vector of string pairs describing the SDE configuration
// *****************************************************************************
{
  using eq = tag::mixdirichlet;

  auto c = ++cnt[ ctr::DiffEqType::MIXDIRICHLET ];       // count eqs
  --c;  // used to index vectors starting with 0

  std::vector< std::pair< std::string, std::string > > nfo;

  nfo.emplace_back( ctr::DiffEq().name( ctr::DiffEqType::MIXDIRICHLET ), "" );

  nfo.emplace_back( "start offset in particle array", std::to_string(
    g_inputdeck.get< tag::component >().offset< eq >(c) ) );
  const auto ncomp = g_inputdeck.get< tag::component >().get< eq >()[c];
  nfo.emplace_back( "number of components", std::to_string( ncomp ) );

  nfo.emplace_back( "kind", "stochastic" );
  nfo.emplace_back( "dependent variable", std::string( 1,
    g_inputdeck.get< tag::param, eq, tag::depvar >()[c] ) );
  nfo.emplace_back( "initialization policy", ctr::InitPolicy().name(
    g_inputdeck.get< tag::param, eq, tag::initpolicy >()[c] ) );
  nfo.emplace_back( "coefficients policy", ctr::CoeffPolicy().name(
    g_inputdeck.get< tag::param, eq, tag::coeffpolicy >()[c] ) );
  nfo.emplace_back( "random number generator", tk::ctr::RNG().name(
    g_inputdeck.get< tag::param, eq, tag::rng >()[c] ) );
  nfo.emplace_back( "initialization policy", ctr::InitPolicy().name(
    g_inputdeck.get< tag::param, eq, tag::initpolicy >()[c] ) );

  auto norm = g_inputdeck.get< tag::param, eq, tag::normalization >()[c];
  nfo.emplace_back( "normalization", ctr::Normalization().name(norm)+"-fluid" );

  auto numderived = MixDirichlet<InitZero,MixDirichletCoeffConst>::NUMDERIVED;
  auto K = ncomp - numderived;
  auto N = K + 1;

  nfo.emplace_back( "coeff b [" + std::to_string(K) + "]",
    parameters( g_inputdeck.get< tag::param, eq, tag::b >().at(c) )
  );
  nfo.emplace_back( "coeff S [" + std::to_string(K) + "]",
    parameters( g_inputdeck.get< tag::param, eq, tag::S >().at(c) )
  );
  nfo.emplace_back( "coeff kappaprime [" + std::to_string(K) + "]",
    parameters( g_inputdeck.get< tag::param, eq, tag::kappaprime >().at(c) )
  );

  const auto& rho = g_inputdeck.get< tag::param, eq, tag::rho >();
  if (!rho.empty()) {
    nfo.emplace_back( "coeff rho [" + std::to_string(N) + "]",
                      parameters( rho.at(c) ) );
    nfo.emplace_back( "coeff r [" + std::to_string(K) + "]",
                      parameters( MixDir_r( rho[c], norm ) ) );
  }

  return nfo;
}

}  // walker::
