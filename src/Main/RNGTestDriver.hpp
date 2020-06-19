// *****************************************************************************
/*!
  \file      src/Main/RNGTestDriver.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Random number generator test suite driver
  \details   Random number generator test suite driver.
*/
// *****************************************************************************
#ifndef RNGTestDriver_h
#define RNGTestDriver_h

#include <map>
#include <functional>

#include "RNGTest/Options/Battery.hpp"
#include "RNGTest/CmdLine/CmdLine.hpp"

//! Everything that contributes to the rngtest executable
namespace rngtest {

class Battery;
class RNGTestPrint;

//! Battery factory type
using BatteryFactory = std::map< ctr::BatteryType, std::function< Battery() > >;

//! \brief Random number generator test suite driver used polymorphically with
//!   tk::Driver
class RNGTestDriver {

  public:
    //! Constructor
    explicit RNGTestDriver( const ctr::CmdLine& cmdline, int nrestart );

    //! Execute driver
    void execute() const;

  private:
    const RNGTestPrint m_print;
};

} // rngtest::

#endif // RNGTestDriver_h
