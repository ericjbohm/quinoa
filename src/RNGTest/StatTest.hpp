// *****************************************************************************
/*!
  \file      src/RNGTest/StatTest.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Random number generator statistical test
  \details   This file defines a generic random number generator statistical
    test class. The class uses runtime polymorphism without client-side
    inheritance: inheritance is confined to the internals of the class,
    invisible to client-code. The class exclusively deals with ownership
    enabling client-side value semantics. Credit goes to Sean Parent at Adobe:
    https://github.com/sean-parent/
    sean-parent.github.com/wiki/Papers-and-Presentations.
*/
// *****************************************************************************
#ifndef StatTest_h
#define StatTest_h

#include <functional>
#include <memory>

#include "NoWarning/charm++.hpp"

#include "Macro.hpp"
#include "Options/RNG.hpp"

namespace rngtest {

//! \brief Random number generator statistical test
//! \details This class uses runtime polymorphism without client-side
//!   inheritance: inheritance is confined to the internals of the this class,
//!   invisible to client-code. The class exclusively deals with ownership
//!   enabling client-side value semantics. Credit goes to Sean Parent at Adobe:
//!   https://github.com/sean-parent/sean-parent.github.com/wiki/
//!   Papers-and-Presentations. For example client code that models a Battery,
//!   see rngtest::TestU01.
class StatTest {

  public:
    //! \brief Constructor taking a function pointer to a constructor of an
    //!    object modeling Concept
    //! \details Passing std::function allows late execution of the constructor
    //!   of T, i.e., at some future time, and thus usage from a factory. Note
    //!   that the value of the first function argument, std::function<T()>, is
    //!   not used here, but its constructor type, T, is used to enable the
    //!   compiler to deduce the model constructor type, used to create its
    //!   Charm proxy, defined by T::Proxy. The actual constructor of T is not
    //!   called here but at some future time by the Charm++ runtime system,
    //!   here only an asynchrounous ckNew() is called, i.e., a message (or
    //!   request) for a future call to T's constructor. This overload can only
    //!   be used for Charm++ chare objects defining typedef 'Proxy', which must
    //!   define the Charm++ proxy. All optional constructor arguments are
    //!   forwarded to ckNew() and thus to T's constructor. If it was somehow
    //!   possible to obtain all bound arguments' types and values from an
    //!   already-bound std::function, we could use those instead of having to
    //!   explicitly forward the model constructor arguments via this host
    //!   constructor.
    //! \param[in] c Function pointer to a constructor of an object modeling
    //!    Concept
    //! \param[in] args Constructor arguments
    //! \see See also tk::recordCharmModel().
    template< typename T, typename... CtrArgs >
    explicit StatTest( std::function<T()> c [[maybe_unused]], CtrArgs... args )
      : self( std::make_unique< Model< typename T::Proxy > >
              (std::move(T::Proxy::ckNew(std::forward<CtrArgs>(args)...))) ) {
      Assert( c == nullptr, "std::function argument to StatTest Charm "
                            "constructor must be nullptr" );
    }

    //! Public interface to contribute number of results/test, i.e., p-values
    void npval() const { self->npval(); }

    //! Public interface to contribute test name(s)
    void names() const { self->names(); }

    //! Public interface to running a test
    void run() const { self->run(); }

    //! Public interface to contributing a test's run time measured in seconds
    void time() const { self->time(); }

    //! Copy assignment
    StatTest& operator=( const StatTest& x )
    { StatTest tmp(x); *this = std::move(tmp); return *this; }
    //! Copy constructor
    StatTest( const StatTest& x ) : self( x.self->copy() ) {}
    //! Move assignment
    StatTest& operator=( StatTest&& ) noexcept = default;
    //! Move constructor
    StatTest( StatTest&& ) noexcept = default;

  private:
    //! Concept is a pure virtual base class specifying the requirements of
    //! polymorphic objects deriving from it
    struct Concept {
      Concept() = default;
      Concept( const Concept& ) = default;
      virtual ~Concept() = default;
      virtual Concept* copy() const = 0;
      virtual void npval() = 0;
      virtual void names() = 0;
      virtual void run() = 0;
      virtual void time() = 0;
    };

    //! Model models the Concept above by deriving from it and overriding the
    //! the virtual functions required by Concept
    template< typename T >
    struct Model : Concept {
      explicit Model( T x ) : data( std::move(x) ) {}
      Concept* copy() const override { return new Model( *this ); }
      void npval() override { data.npval(); }
      void names() override { data.names(); }
      void run() override { data.run(); }
      void time() override { data.time(); }
      T data;
    };

    std::unique_ptr< Concept > self;    //!< Base pointer used polymorphically
};

} // rngtest::

#endif // StatTest_h
