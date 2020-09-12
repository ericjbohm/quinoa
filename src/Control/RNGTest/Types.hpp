// *****************************************************************************
/*!
  \file      src/Control/RNGTest/Types.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Types for RNGTest's parsers
  \details   Types for RNGTest's parsers. This file defines the components of
    the tagged tuple that stores heteroegeneous objects in a hierarchical way.
    These components are therefore part of the grammar stack that is filled
    during parsing (both command-line argument parsing and control file
    parsing).
*/
// *****************************************************************************
#ifndef RNGTestTypes_h
#define RNGTestTypes_h

#include "Tags.hpp"
#include "Base/Types.hpp"
#include "RNGParam.hpp"
#include "RNGTest/Options/Battery.hpp"
#include "Options/RNG.hpp"
#include "QuinoaConfig.hpp"

namespace rngtest {
namespace ctr {

using namespace tao;

using tk::ctr::RNGType;

//! Storage of selected options
using selects = tk::TaggedTuple< brigand::list<
    tag::battery, BatteryType                    //!< Battery
  , tag::rng,     std::vector< RNGType >         //!< Random number generators
> >;

//! IO parameters storage
using ios = tk::TaggedTuple< brigand::list<
    tag::nrestart,  int                             //!< Number of restarts
  , tag::control,  std::string                      //!< Control filename
  , tag::screen,    kw::screen::info::expect::type  //!< Screen output filename
> >;

//! Parameters storage
using parameters = tk::TaggedTuple< brigand::list<
    #ifdef HAS_MKL
    tag::rngmkl,    tk::ctr::RNGMKLParameters,        //!< MKL RNG parameters
    #endif
    tag::rngsse,    tk::ctr::RNGSSEParameters        //!< RNGSSE parameters
  , tag::rng123,    tk::ctr::RNGRandom123Parameters  //!< Random123 parameters
> >;

//! PEGTL location/position type to use throughout all of RNGTest's parsers
using Location = pegtl::position;

} // ctr::
} // rngtest::

#endif // RNGTestTypes_h
