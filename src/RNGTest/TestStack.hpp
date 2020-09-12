// *****************************************************************************
/*!
  \file      src/RNGTest/TestStack.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Stack collecting all types of random number generator statistical
     tests
  \details   Stack collecting all types of statistical tests. Currently, on
    TestU01 is interfaced. More might in the future.
*/
// *****************************************************************************
#ifndef TestStack_h
#define TestStack_h

#include "TestU01Stack.hpp"

namespace rngtest {

//! Stack collecting all types of random RNG statistical tests
struct TestStack {
  TestStack() : TestU01() {}
  TestU01Stack TestU01;
};

} // rngtest::

#endif // TestStack_h
