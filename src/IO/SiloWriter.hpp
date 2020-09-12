// *****************************************************************************
/*!
  \file      src/IO/SiloWriter.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Silo writer declaration.
  \details   Silo writer declaration. This class currently only supports writing
    an STL triangulation into a Silo file. See also
    https://wci.llnl.gov/codes/silo.
*/
// *****************************************************************************
#ifndef SiloWriter_h
#define SiloWriter_h

#include <string>

#include <silo.h>

namespace tk {

//! Silo error handler function type
typedef void (*SiloErrorHandler)( char* );

//! Silo error handler
void SiloError( char* msg );

class STLMesh;

//! \brief SiloWriter
//! \details Mesh reader class facilitating reading a mesh from a file in
//!   Silo format. See also https://wci.llnl.gov/codes/silo.
class SiloWriter {

  public:
    //! Constructor
    explicit SiloWriter( const std::string& filename,
                         const STLMesh& mesh,
                         int errLevel );
 
    //! Destructor: Release glob file handle
    ~SiloWriter() noexcept;

    //! Write out silo file
    void write();

  private:
    const std::string m_filename;       //!< Silo filename
    const STLMesh& m_mesh;              //!< Mesh

    SiloErrorHandler m_errFunc;         //!< Silo error handler function ptr
    int m_errLevel;                     //!< Silo error reporting level
    DBfile* m_dbfile;                   //!< Silo DB file
};

} // tk::

#endif // SiloWriter_h
