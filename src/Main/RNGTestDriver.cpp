// *****************************************************************************
/*!
  \file      src/Main/RNGTestDriver.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Random number generator test suite driver
  \details   Random number generator test suite driver.
*/
// *****************************************************************************

#include "Tags.hpp"
#include "Exception.hpp"
#include "Factory.hpp"
#include "Battery.hpp"
#include "TestU01Suite.hpp"
#include "RNGTestPrint.hpp"
#include "RNGTestDriver.hpp"
#include "RNGTest/InputDeck/InputDeck.hpp"
#include "RNGTest/InputDeck/Parser.hpp"
#include "TaggedTupleDeepPrint.hpp"
#include "Writer.hpp"

namespace rngtest {

extern ctr::InputDeck g_inputdeck;
extern ctr::InputDeck g_inputdeck_defaults;

} // rngtest::

using rngtest::RNGTestDriver;

RNGTestDriver::RNGTestDriver( const ctr::CmdLine& cmdline, int nrestart ) :
  m_print( cmdline.logname(
             g_inputdeck_defaults.get< tag::cmd, tag::io, tag::screen >(),
             nrestart ),
           cmdline.get< tag::verbose >() ? std::cout : std::clog,
           std::ios_base::app )
// *****************************************************************************
//  Constructor
//! \param[in] cmdline Command line object storing data parsed from the command
//!   line arguments
//! \param[in] nrestart Number of times restarted
// *****************************************************************************
{
  // All global-scope data to be migrated to all PEs initialized here

  // Parse input deck into g_inputdeck
  m_print.item( "Control file", cmdline.get< tag::io, tag::control >() );  
  InputDeckParser inputdeckParser( m_print, cmdline, g_inputdeck );
  m_print.item( "Parsed control file", "success" );  

  m_print.endpart();

  // Output input deck object to file
  auto logfilename = tk::rngtest_executable() + "_input.log";
  tk::Writer log( logfilename );
  tk::print( log.stream(), "inputdeck", g_inputdeck );
}

void
RNGTestDriver::execute() const
// *****************************************************************************
//  Run battery
// *****************************************************************************
{
  m_print.part( "Factory" );

  // Register batteries
  BatteryFactory bf;
  using ctr::BatteryType;
  // Note that TestU01Suite constructors take the BatteryType (enum class)
  // value as their argument, which happens to be the same as the key in the
  // factory - hence the double-specification of the battery type below.
  // Record all into a factory passing the last 0 means instantiate on PE 0.
  tk::recordCharmModel< Battery, TestU01Suite >
                    ( bf, BatteryType::SMALLCRUSH, BatteryType::SMALLCRUSH, 0 );
  tk::recordCharmModel< Battery, TestU01Suite >
                    ( bf, BatteryType::CRUSH, BatteryType::CRUSH, 0 );
  tk::recordCharmModel< Battery, TestU01Suite >
                    ( bf, BatteryType::BIGCRUSH, BatteryType::BIGCRUSH, 0 );
  m_print.list< ctr::Battery >( "Registered batteries", bf );
  m_print.endpart();

  m_print.part( "Problem" );
  if ( !g_inputdeck.get< tag::title >().empty() )
    m_print.title( g_inputdeck.get< tag::title >() );

  // Instantiate and run battery
  const auto s = bf.find( g_inputdeck.get< tag::selected, tag::battery >() );
  if (s != end(bf)) {
    s->second();
  } else Throw( "Battery not found in factory" );
}
