// *****************************************************************************
/*!
  \file      src/Control/Walker/Options/InitPolicy.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Differential equation initialization policy options for walker
  \details   Differential equation initialization policy options for walker
*/
// *****************************************************************************
#ifndef InitPolicyOptions_h
#define InitPolicyOptions_h

#include <brigand/sequences/list.hpp>
#include <brigand/algorithms/for_each.hpp>

#include "Toggle.hpp"
#include "Keywords.hpp"
#include "PUPUtil.hpp"

namespace walker {
namespace ctr {

//! Differential equation initializion policy types
enum class InitPolicyType : uint8_t { RAW=0,
                                      ZERO,
                                      JOINTDELTA,
                                      JOINTGAUSSIAN,
                                      JOINTCORRGAUSSIAN,
                                      JOINTBETA,
                                      JOINTGAMMA,
                                      JOINTDIRICHLET };

//! Pack/Unpack InitPolicyType: forward overload to generic enum class packer
inline void operator|( PUP::er& p, InitPolicyType& e ) { PUP::pup( p, e ); }

//! InitPolicy options: outsource searches to base templated on enum type
class InitPolicy : public tk::Toggle< InitPolicyType > {

  public:
    //! Valid expected choices to make them also available at compile-time
    using keywords = brigand::list< kw::raw
                                  , kw::zero
                                  , kw::jointdelta
                                  , kw::jointgaussian
                                  , kw::jointcorrgaussian
                                  , kw::jointbeta
                                  , kw::jointgamma
                                  , kw::jointdirichlet
                                  >;

    //! \brief Options constructor
    //! \details Simply initialize in-line and pass associations to base, which
    //!    will handle client interactions
    explicit InitPolicy() :
      tk::Toggle< InitPolicyType >(
        //! Group, i.e., options, name
        "Initialization Policy",
        //! Enums -> names
        { { InitPolicyType::RAW, kw::raw::name() },
          { InitPolicyType::ZERO, kw::zero::name() },
          { InitPolicyType::JOINTDELTA, kw::jointdelta::name() },
          { InitPolicyType::JOINTGAUSSIAN, kw::jointgaussian::name() },
          { InitPolicyType::JOINTCORRGAUSSIAN, kw::jointcorrgaussian::name() },
          { InitPolicyType::JOINTBETA, kw::jointbeta::name() },
          { InitPolicyType::JOINTGAMMA, kw::jointgamma::name() },
          { InitPolicyType::JOINTDIRICHLET, kw::jointdirichlet::name() } },
        //! keywords -> Enums
        { { kw::raw::string(), InitPolicyType::RAW },
          { kw::zero::string(), InitPolicyType::ZERO },
          { kw::jointdelta::string(), InitPolicyType::JOINTDELTA },
          { kw::jointgaussian::string(), InitPolicyType::JOINTGAUSSIAN },
          { kw::jointcorrgaussian::string(),
            InitPolicyType::JOINTCORRGAUSSIAN },
          { kw::jointbeta::string(), InitPolicyType::JOINTBETA },
          { kw::jointgamma::string(), InitPolicyType::JOINTGAMMA },
          { kw::jointdirichlet::string(), InitPolicyType::JOINTDIRICHLET } } )
    {
       brigand::for_each< keywords >( assertPolicyCodes() );
    }

    //! \brief Return policy code based on Enum
    //! \param[in] p Enum value of the option requested
    //! \return Policy code of the option
    const std::string& code( InitPolicyType p ) const {
      using tk::operator<<;
      auto it = policy.find( p );
      Assert( it != end(policy),
              std::string("Cannot find policy code for physics \"") << p <<
                "\"" );
      return it->second;
    }

  private:
    //! Function object for ensuring the existence of policy codes
    struct assertPolicyCodes {
      //! \brief Function call operator templated on the type to assert the
      //!   existence of a policy code
      template< typename U > void operator()( brigand::type_<U> ) {
        static_assert( tk::HasTypedef_code_v< typename U::info >,
                       "Policy code undefined for keyword" );
      }
    };

    //! Enums -> policy code
    std::map< InitPolicyType, std::string > policy {
        { InitPolicyType::RAW, *kw::raw::code() }
      , { InitPolicyType::ZERO, *kw::zero::code() }
      , { InitPolicyType::JOINTDELTA, *kw::jointdelta::code() }
      , { InitPolicyType::JOINTGAUSSIAN, *kw::jointgaussian::code() }
      , { InitPolicyType::JOINTCORRGAUSSIAN, *kw::jointcorrgaussian::code() }
      , { InitPolicyType::JOINTBETA, *kw::jointbeta::code() }
      , { InitPolicyType::JOINTGAMMA, *kw::jointgamma::code() }
      , { InitPolicyType::JOINTDIRICHLET, *kw::jointdirichlet::code() }
    };
};

} // ctr::
} // walker::

#endif // InitPolicyOptions_h
