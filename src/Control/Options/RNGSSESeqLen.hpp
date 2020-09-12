// *****************************************************************************
/*!
  \file      src/Control/Options/RNGSSESeqLen.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     RNGSSE sequence length options
  \details   RNGSSE sequence length options
*/
// *****************************************************************************
#ifndef RNGSSESeqLenOptions_h
#define RNGSSESeqLenOptions_h

#include <brigand/sequences/list.hpp>

#include "Toggle.hpp"
#include "Keywords.hpp"
#include "PUPUtil.hpp"

namespace tk {
namespace ctr {

//! RNGSSE sequence length options
enum class RNGSSESeqLenType : uint8_t { SHORT,
                                        MEDIUM,
                                        LONG };

//! \brief Pack/Unpack RNGSSESeqLenType: forward overload to generic enum class
//!   packer
inline void operator|( PUP::er& p, RNGSSESeqLenType& e ) { PUP::pup( p, e ); }

//! \brief RNGSSESeqLen options: outsource searches to base templated on enum
//!   type
class RNGSSESeqLen : public tk::Toggle< RNGSSESeqLenType > {

  public:
    //! Valid expected choices to make them also available at compile-time
    using keywords = brigand::list< kw::seq_short
                                  , kw::seq_med
                                  , kw::seq_long
                                  >;

    //! \brief Options constructor
    //! \details Simply initialize in-line and pass associations to base, which
    //!    will handle client interactions
    explicit RNGSSESeqLen() :
      tk::Toggle< RNGSSESeqLenType >(
        //! Group, i.e., options, name
        "sequence length",
        //! Enums -> names
        { { RNGSSESeqLenType::SHORT, kw::seq_short::name() },
          { RNGSSESeqLenType::MEDIUM, kw::seq_med::name() },
          { RNGSSESeqLenType::LONG, kw::seq_long::name() } },
        //! keywords -> Enums
        { { kw::seq_short::string(), RNGSSESeqLenType::SHORT },
          { kw::seq_med::string(), RNGSSESeqLenType::MEDIUM },
          { kw::seq_long::string(), RNGSSESeqLenType::LONG } } ) {}
};

} // ctr::
} // tk::

#endif // RNGSSESeqLenOptions_h
