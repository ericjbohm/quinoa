// *****************************************************************************
/*!
  \file      src/Control/Walker/CmdLine/Parser.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Walker's command line parser
  \details   Walker's command line parser
*/
// *****************************************************************************
#ifndef WalkerCmdLineParser_h
#define WalkerCmdLineParser_h

#include "StringParser.hpp"
#include "Walker/CmdLine/CmdLine.hpp"

namespace tk { class Print; }

namespace walker {

//! CmdLineParser : StringParser
class CmdLineParser : public tk::StringParser {

  public:
    //! Constructor
    explicit CmdLineParser( int argc, char** argv,
                            const tk::Print& print,
                            ctr::CmdLine& cmdline );
};

} // walker::

#endif // WalkerCmdLineParser_h
