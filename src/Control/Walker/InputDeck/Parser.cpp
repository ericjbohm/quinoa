// *****************************************************************************
/*!
  \file      src/Control/Walker/InputDeck/Parser.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Walker's input deck file parser
  \details   Walker's input deck file parser
*/
// *****************************************************************************

#include <ostream>
#include <vector>
#include <type_traits>

#include "NoWarning/pegtl.hpp"

#include "Print.hpp"
#include "Tags.hpp"
#include "ContainerUtil.hpp"
#include "Walker/Types.hpp"
#include "Walker/InputDeck/InputDeck.hpp"
#include "Walker/InputDeck/Parser.hpp"
#include "Walker/InputDeck/Grammar.hpp"

namespace tk {
namespace grm {

extern tk::Print g_print;

} // grm::
} // tk::

using walker::InputDeckParser;

InputDeckParser::InputDeckParser( const tk::Print& print,
                                  const ctr::CmdLine& cmdline,
                                  ctr::InputDeck& inputdeck ) :
  FileParser( cmdline.get< tag::io, tag::control >() )
// *****************************************************************************
//  Constructor
//! \param[in] print Pretty printer
//! \param[in] cmdline Command line stack
//! \param[inout] inputdeck Input deck stack where data is stored during parsing
// *****************************************************************************
{
  // Create InputDeck (a tagged tuple) to store parsed input
  ctr::InputDeck id( cmdline );

  // Reset parser's output stream to that of print's. This is so that mild
  // warnings emitted during parsing can be output using the pretty printer.
  // Usually, errors and warnings are simply accumulated during parsing and
  // printed during diagnostics after the parser has finished. Howver, in some
  // special cases we can provide a more user-friendly message right during
  // parsing since there is more information available to construct a more
  // sensible message. This is done in e.g., tk::grm::store_option. Resetting
  // the global g_print, to that of passed in as the constructor argument allows
  // not to have to create a new pretty printer, but use the existing one.
  tk::grm::g_print.reset( print.save() );

  // Parse input file and populate the underlying tagged tuple
  tao::pegtl::file_input<> in( m_filename );
  tao::pegtl::parse< deck::read_file, tk::grm::action >( in, id );

  // Echo errors and warnings accumulated during parsing
  diagnostics( print, id.get< tag::error >() );

  // Strip input deck (and its underlying tagged tuple) from PEGTL instruments
  // and transfer it out
  inputdeck = std::move( id );

  // Filter out repeated statistics
  tk::unique( inputdeck.get< tag::stat >() );
}
