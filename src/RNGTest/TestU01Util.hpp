// *****************************************************************************
/*!
  \file      src/RNGTest/TestU01Util.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Interfacing the TestU01 random number generator test suite
  \details   Interfacing the TestU01 random number generator test suite.
*/
// *****************************************************************************
#ifndef TestU01Util_h
#define TestU01Util_h

#include <memory>

extern "C" {
  #include <unif01.h>
}

namespace rngtest {

//! Custom deleter binding a raw TestU01 pointer to its TestU01 deleter
template< typename RawPtr, void (*Deleter)(RawPtr *) >
struct Eraser {
  void operator()( RawPtr* ptr ) {
    Deleter( ptr );
  }
};

//! TestU01 pointer type with a custom deleter
template< class Ptr, void (*Deleter)(Ptr *) >
using TestU01Ptr = std::unique_ptr< Ptr, Eraser< Ptr, Deleter > >;

//! TestU01 external generator type with a custom deleter by TestU01
using Gen01Ptr = TestU01Ptr< unif01_Gen, unif01_DeleteExternGen01 >;

} // rngtest::

#endif // TestU01Util_h
