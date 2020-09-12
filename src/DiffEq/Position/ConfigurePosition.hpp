// *****************************************************************************
/*!
  \file      src/DiffEq/Position/ConfigurePosition.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Register and compile configuration on the position SDE
  \details   Register and compile configuration on the position SDE.
*/
// *****************************************************************************
#ifndef ConfigurePosition_h
#define ConfigurePosition_h

#include <set>
#include <map>
#include <string>
#include <utility>

#include "DiffEqFactory.hpp"
#include "Walker/Options/DiffEq.hpp"

namespace walker {

//! Register position SDE into DiffEq factory
void registerPosition( DiffEqFactory& f, std::set< ctr::DiffEqType >& t );

//! Return information on the position SDE
std::vector< std::pair< std::string, std::string > >
infoPosition( std::map< ctr::DiffEqType, tk::ctr::ncomp_t >& cnt );

} // walker::

#endif // ConfigurePosition_h
