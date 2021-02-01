// *****************************************************************************
/*!
  \file      src/Mesh/CommMap.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Functions employing communication maps
  \details   Functions employing communication maps.
*/
// *****************************************************************************

#include "CommMap.hpp"

namespace tk {

bool slave( const NodeCommMap& map, std::size_t node, int chare )
// *****************************************************************************
//  Decide if a node is not counted by a chare
//! \detail If a node is found in the node communication map and is associated
//! to a lower chare id than the chare id given, it is counted by another chare
//! (and not this one), hence a "slave" (for the purpose of this count).
//! \return True if the node is a slave (counted by another chare with a lower
//!   chare id)
// *****************************************************************************
{
  return
    std::any_of( map.cbegin(), map.cend(),
      [&](const auto& s) {
        return s.second.find(node) != s.second.cend() && s.first > chare; } );
}

} // tk::
