// *****************************************************************************
/*!
  \file      src/Control/Options/PDFFile.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     PDF output file type options
  \details   PDF output file type options
*/
// *****************************************************************************
#ifndef PDFFileOptions_h
#define PDFFileOptions_h

#include <brigand/sequences/list.hpp>

#include "Toggle.hpp"
#include "Keywords.hpp"
#include "PUPUtil.hpp"

namespace tk {
namespace ctr {

//! PDF output file types
enum class PDFFileType : uint8_t { TXT=0,
                                   GMSHTXT,
                                   GMSHBIN,
                                   EXODUSII };

//! \brief Pack/Unpack PDFFileType: forward overload to generic enum class
//!   packer
inline void operator|( PUP::er& p, PDFFileType& e ) { PUP::pup( p, e ); }

//! \brief PDFFileType options: outsource searches to base templated on enum
//!   type
class PDFFile : public tk::Toggle< PDFFileType > {

  public:
    //! Valid expected choices to make them also available at compile-time
    using keywords = brigand::list< kw::txt
                                  , kw::gmshtxt
                                  , kw::gmshbin
                                  , kw::exodusii
                                  >;

    //! \brief Options constructor
    //! \details Simply initialize in-line and pass associations to base, which
    //!    will handle client interactions
    explicit PDFFile() :
      tk::Toggle< PDFFileType >(
        //! Group, i.e., options, name 
        "PDF output file type",
        //! Enums -> names
        { { PDFFileType::TXT, kw::txt::name() },
          { PDFFileType::GMSHTXT, kw::gmshtxt::name() },
          { PDFFileType::GMSHBIN, kw::gmshbin::name() },
          { PDFFileType::EXODUSII, kw::exodusii::name() } },
        //! keywords -> Enums
        { { kw::txt::string(), PDFFileType::TXT },
          { kw::gmshtxt::string(), PDFFileType::GMSHTXT },
          { kw::gmshbin::string(), PDFFileType::GMSHBIN },
          { kw::exodusii::string(), PDFFileType::EXODUSII } } ) {}
};

} // ctr::
} // tk:::

#endif // PDFFileOptions_h
