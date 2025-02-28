// *****************************************************************************
/*!
  \file      src/Control/HelpFactory.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Command-line and input deck help factory
  \details   This file contains some types that facilitate the generation of
     on-screen help.
*/
// *****************************************************************************
#ifndef HelpFactory_h
#define HelpFactory_h

#include <brigand/sequences/list.hpp>
#include <brigand/algorithms/for_each.hpp>

#include "PUPUtil.hpp"
#include "Factory.hpp"
#include "Has.hpp"

namespace tk {
namespace ctr {

//! \brief Keyword information bundle
//! \details This bundle contains the information that is used to display
//!    on-screen help on all command-line arguments and control file keywords
//!    for an exectuable. This struct is stored in a container that associates
//!    keywords (used by a grammar and parser) to this struct. The container, an
//!    runtime, std::map, is filled by the CmdLine and InputDeck objects'
//!    constructors by one or more brigand::for_each which loops through the
//!    set of all keywords used in a grammar. The maps are stored in the CmdLine
//!    and InputDeck objects (which are tagged tuples) and thus can be migrated
//!    through the network, thus the Charm++ parck/unpack routines are defined.
//! \see Info functor used to fill the std::maps
struct KeywordInfo {
  std::string shortDescription;           //!< Short description
  std::string longDescription;            //!< Long description
  std::optional< std::string > alias;     //!< Keyword alias
  std::optional< std::string > expt;      //!< Expected type description
  std::optional< std::string > lower;     //!< Lower bound as string
  std::optional< std::string > upper;     //!< Upper bound as string
  std::optional< std::string > choices;   //!< Expected choices description

  /** @name Pack/Unpack: Serialize KeywordInfo object for Charm++ */
  ///@{
  //! \brief Pack/Unpack serialize member function
  //! \param[in,out] p Charm++'s PUP::er serializer object reference
  void pup( PUP::er& p ) {
    p | shortDescription;
    p | longDescription;
    p | alias;
    p | expt;
    p | lower;
    p | upper;
    p | choices;
  }
  //! \brief Pack/Unpack serialize operator|
  //! \param[in,out] p Charm++'s PUP::er serializer object reference
  //! \param[in,out] info KeywordInfo object reference
  friend void operator|( PUP::er& p, KeywordInfo& info ) { info.pup(p); }
  ///@}
};

//! \brief A typedef for associating a keyword-string with its associated
//!   information stored in a KeywordInfo struct
using HelpFactory = std::map< std::string, KeywordInfo >;

//! \brief Help bundle on a single keyword
//! \details This is used for delivering help on a single keyword. This struct
//!    also differentiates between command-line arguments and control file
//!    keywords.
struct HelpKw {
  HelpFactory::key_type keyword;        //!< Keyword string
  HelpFactory::mapped_type info;        //!< Keyword information
  bool cmd;                             //!< True if command-line keyword

  /** @name Pack/Unpack: Serialize HelpKw object for Charm++ */
  ///@{
  //! \brief Pack/Unpack serialize member function
  //! \param[in,out] p Charm++'s PUP::er serializer object reference
  void pup( PUP::er& p ) { p|keyword; p|info; p|cmd; }
  //! \brief Pack/Unpack serialize operator|
  //! \param[in,out] p Charm++'s PUP::er serializer object reference
  //! \param[in,out] h HelpKw object reference
  friend void operator|( PUP::er& p, HelpKw& h ) { h.pup(p); }
  ///@}
};

//! \brief Function object for filling a HelpFactory (std::map) with keywords
//!   and their associated information bundle
//! \details This struct is used as a functor to loop through a set of keywords
//!   at compile-time and generate code for filling up the std::map.
struct Info {
  //! Store reference to map we are filling
  tk::ctr::HelpFactory& m_factory;
  //! Constructor: store reference to map to fill
  explicit Info( tk::ctr::HelpFactory& factory ) : m_factory( factory ) {}
  //! \brief Function call operator templated on the type that does the filling
  template< typename U > void operator()( brigand::type_<U> ) {
    m_factory[ U::string() ] = { U::shortDescription(),
                                 U::longDescription(),
                                 U::alias(),
                                 U::expt(),
                                 U::lower(),
                                 U::upper(),
                                 U::choices() };
  }
};

} // ctr::
} // tk::

#endif // HelpFactory_h
