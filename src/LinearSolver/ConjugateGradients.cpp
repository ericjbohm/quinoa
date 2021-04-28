// *****************************************************************************
/*!
  \file      src/LinearSolver/ConjugateGradients.cpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2021 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Charm++ chare array for distributed conjugate gradients.
  \details   Charm++ chare array for asynchronous distributed
    conjugate gradients linear solver.
  \see Y. Saad, Iterative Methods for Sparse Linear Systems: Second Edition,
    ISBN 9780898718003, 2003, Algorithm 6.18, conjugate gradients to solve the
    linear system A * x = b, reproduced here:

    Compute r0:=b-A*x0, p0:=r0                  see residual(), normb()
    For j=0,1,..., until convergence, do
      alpha_j := (r_j,r_j) / (Ap_j,p_j)         see next(), qAp(), q(), pq()
      x_{j+1} := x_j + alpha_j p_j              see normres()
      r_{j+1} := r_j - alpha_j A p_j            see pq()
      beta_j := (r_{j+1},r_{j+1}) / (r_j,r_j)   see normres()
      p_{j+1} := r_{j+1} + beta_j p_j           see rho()
    end
*/
// *****************************************************************************

#include <numeric>
#include <iostream>     // NOT NEEDED AFTER DEBUGGED, LEAVE FOR NOW

#include "Exception.hpp"
#include "ConjugateGradients.hpp"
#include "Vector.hpp"

using tk::ConjugateGradients;

ConjugateGradients::ConjugateGradients(
  const CSR& A,
  const std::vector< tk::real >& x,
  const std::vector< tk::real >& b,
  const std::vector< std::size_t >& gid,
  const std::unordered_map< std::size_t, std::size_t >& lid,
  const NodeCommMap& nodecommmap ) :
  m_A( A ),
  m_x( x ),
  m_b( b ),
  m_gid( gid ),
  m_lid( lid ),
  m_nodeCommMap( nodecommmap ),
  m_r( m_A.rsize(), 0.0 ),
  m_rc(),
  m_nr( 0 ),
  m_p( m_A.rsize(), 0.0 ),
  m_q( m_A.rsize(), 0.0 ),
  m_qc(),
  m_nq( 0 ),
  m_initialized(),
  m_solved(),
  m_normb( 0.0 ),
  m_it( 0 ),
  m_rho( 0.0 ),
  m_rho0( 0.0 ),
  m_alpha( 0.0 )
// *****************************************************************************
//  Constructor
//! \param[in] A Left hand side matrix of the linear system to solve in Ax=b
//! \param[in] x Solution (initial guess) of the linear system to solve in Ax=b
//! \param[in] b Right hand side of the linear system to solve in Ax=b
//! \param[in] gid Global node ids
//! \param[in] lid Local node ids associated to global ones
//! \param[in] nodecommmap Global mesh node IDs shared with other chares
//!   associated to their chare IDs
// *****************************************************************************
{
  // Fill in gid and lid for serial solve
  if (gid.empty() || lid.empty() || nodecommmap.empty()) {
    m_gid.resize( m_x.size() );
    std::iota( begin(m_gid), end(m_gid), 0 );
    for (auto g : m_gid) m_lid[g] = g;
  }

  Assert( m_A.rsize() == m_gid.size()*A.Ncomp(), "Size mismatch" );
  Assert( m_x.size() == m_gid.size()*A.Ncomp(), "Size mismatch" );
  Assert( m_b.size() == m_gid.size()*A.Ncomp(), "Size mismatch" );
}

void
ConjugateGradients::init( CkCallback c )
// *****************************************************************************
//  Initialize solver
//! \param[in] c Call to continue with after initialization is complete
// *****************************************************************************
{
  m_initialized = c;

  // initiate computing A * x (for the initial residual)
  thisProxy[ thisIndex ].wait4res();
  residual();

  // initiate computing norm of right hand side
  dot( m_b, m_b,
       CkCallback( CkReductionTarget(ConjugateGradients,normb), thisProxy ) );
}

void
ConjugateGradients::dot( const std::vector< tk::real >& a,
                         const std::vector< tk::real >& b,
                         CkCallback c )
// *****************************************************************************
//  Initiate computation of dot product of two vectors
//! \param[in] a 1st vector of dot product
//! \param[in] b 2nd vector of dot product
//! \param[in] c Callback to target with the final result
// *****************************************************************************
{
  Assert( a.size() == b.size(), "Size mismatch" );

  tk::real d = 0.0;
  for (std::size_t i=0; i<a.size(); ++i)
    if (!slave(m_nodeCommMap,m_gid[i],thisIndex))
      d += a[i]*b[i];

  contribute( sizeof(tk::real), &d, CkReduction::sum_double, c );
}

void
ConjugateGradients::normb( tk::real n )
// *****************************************************************************
// Compute the norm of the right hand side
//! \param[in] n Norm of right hand side (aggregated across all chares)
// *****************************************************************************
{
  m_normb = std::sqrt(n);
  normb_complete();
}

void
ConjugateGradients::residual()
// *****************************************************************************
//  Initiate A * x for computing the initial residual, r = b - A * x
// *****************************************************************************
{
  // Compute own contribution to r = A * x
  m_A.mult( m_x, m_r );

  // Send partial product on chare-boundary nodes to fellow chares
  if (m_nodeCommMap.empty()) {
    comres_complete();
  } else {
    auto dof = m_A.Ncomp();
    for (const auto& [c,n] : m_nodeCommMap) {
      std::vector< std::vector< tk::real > > rc( n.size() );
      std::size_t j = 0;
      for (auto g : n) {
        std::vector< tk::real > nr( dof );
        auto lid = tk::cref_find( m_lid, g );
        for (std::size_t d=0; d<dof; ++d) nr[d] = m_r[ lid*dof+d ];
        rc[j++] = std::move(nr);
      }
      thisProxy[c].comres( std::vector<std::size_t>(begin(n),end(n)), rc );
    }
  }

  ownres_complete();
}

void
ConjugateGradients::comres( const std::vector< std::size_t >& gid,
                            const std::vector< std::vector< tk::real > >& rc )
// *****************************************************************************
//  Receive contributions to A * x on chare-boundaries
//! \param[in] gid Global mesh node IDs at which we receive contributions
//! \param[in] rc Partial contributions at chare-boundary nodes
// *****************************************************************************
{
  Assert( rc.size() == gid.size(), "Size mismatch" );

  using tk::operator+=;

  for (std::size_t i=0; i<gid.size(); ++i)
    m_rc[ gid[i] ] += rc[i];

  if (++m_nr == m_nodeCommMap.size()) {
    m_nr = 0;
    comres_complete();
  }
}

void
ConjugateGradients::initres()
// *****************************************************************************
// Compute the initial residual, r = b - A * x
// *****************************************************************************
{
  // Combine own and communicated contributions to r = A * x
  auto dof = m_A.Ncomp();
  for (const auto& [gid,r] : m_rc) {
    auto lid = tk::cref_find( m_lid, gid );
    for (std::size_t c=0; c<dof; ++c) m_r[lid*dof+c] += r[c];
  }
  tk::destroy( m_rc );

  // Finish computing initial residual, r = b - A * x
  for (auto& r : m_r) r *= -1.0;
  m_r += m_b;

  m_initialized.send( CkDataMsg::buildNew( sizeof(tk::real), &m_normb ) );
}

void
ConjugateGradients::solve( std::size_t maxit, tk::real tol, CkCallback c )
// *****************************************************************************
//  Solve linear system
//! \param[in] maxit Max iteration count
//! \param[in] tol Stop tolerance
//! \param[in] c Call to continue with after solve is complete
// *****************************************************************************
{
  m_maxit = maxit;
  m_tol = tol;
  m_solved = c;
  m_it = 0;
  next();
}

void
ConjugateGradients::next()
// *****************************************************************************
//  Start next linear solver iteration
// *****************************************************************************
{
  // initiate computing rho = (r,r)
  dot( m_r, m_r,
       CkCallback( CkReductionTarget(ConjugateGradients,rho), thisProxy ) );
}

void
ConjugateGradients::rho( tk::real r )
// *****************************************************************************
// Compute rho = (r,r)
//! \param[in] r Dot product, rho = (r,r) (aggregated across all chares)
// *****************************************************************************
{
  m_rho = r;
  if (m_it == 0) m_alpha = 0.0; else m_alpha = m_rho/m_rho0;
  m_rho0 = m_rho;

  // compute p = r + alpha * p
  for (std::size_t i=0; i<m_p.size(); ++i) m_p[i] = m_r[i] + m_alpha * m_p[i];

  // initiate computing q = A * p
  thisProxy[ thisIndex ].wait4q();
  qAp();
}


void
ConjugateGradients::qAp()
// *****************************************************************************
//  Initiate computing q = A * p
// *****************************************************************************
{
  // Compute own contribution to q = A * p
  m_A.mult( m_p, m_q );

  // Send partial product on chare-boundary nodes to fellow chares
  if (m_nodeCommMap.empty()) {
    comq_complete();
  } else {
    auto dof = m_A.Ncomp();
    for (const auto& [c,n] : m_nodeCommMap) {
      std::vector< std::vector< tk::real > > qc( n.size() );
      std::size_t j = 0;
      for (auto g : n) {
        std::vector< tk::real > nq( dof );
        auto lid = tk::cref_find( m_lid, g );
        for (std::size_t d=0; d<dof; ++d) nq[d] = m_q[ lid*dof+d ];
        qc[j++] = std::move(nq);
      }
      thisProxy[c].comq( std::vector<std::size_t>(begin(n),end(n)), qc );
    }
  }

  ownq_complete();
}

void
ConjugateGradients::comq( const std::vector< std::size_t >& gid,
                          const std::vector< std::vector< tk::real > >& qc )
// *****************************************************************************
//  Receive contributions to q = A * p on chare-boundaries
//! \param[in] gid Global mesh node IDs at which we receive contributions
//! \param[in] qc Partial contributions at chare-boundary nodes
// *****************************************************************************
{
  Assert( qc.size() == gid.size(), "Size mismatch" );

  using tk::operator+=;

  for (std::size_t i=0; i<gid.size(); ++i)
    m_qc[ gid[i] ] += qc[i];

  if (++m_nq == m_nodeCommMap.size()) {
    m_nq = 0;
    comq_complete();
  }
}

void
ConjugateGradients::q()
// *****************************************************************************
// Finish computing q = A * p
// *****************************************************************************
{
  // Combine own and communicated contributions to r = A * x
  auto dof = m_A.Ncomp();
  for (const auto& [gid,q] : m_qc) {
    auto lid = tk::cref_find( m_lid, gid );
    for (std::size_t c=0; c<dof; ++c) m_q[lid*dof+c] += q[c];
  }
  tk::destroy( m_qc );

  // initiate computing (p,q)
  dot( m_p, m_q,
       CkCallback( CkReductionTarget(ConjugateGradients,pq), thisProxy ) );
}

void
ConjugateGradients::pq( tk::real d )
// *****************************************************************************
// Compute the dot product (p,q)
//! \param[in] d Dot product of (p,q) (aggregated across all chares)
// *****************************************************************************
{
  Assert( d > 1.0e-14, "Conjugate Gradients: (p,q) orthogonal, wrong/no BC?" );

  m_alpha = m_rho / d;

  // compute r = r - alpha * q
  for (std::size_t i=0; i<m_r.size(); ++i) m_r[i] -= m_alpha * m_q[i];

  // initiate computing norm of residual: (r,r)
  dot( m_r, m_r,
       CkCallback( CkReductionTarget(ConjugateGradients,normres), thisProxy ) );
}

void
ConjugateGradients::normres( tk::real r )
// *****************************************************************************
// Compute norm of residual: (r,r)
//! \param[in] r Dot product, (r,r) (aggregated across all chares)
// *****************************************************************************
{
  auto norm = std::sqrt( r );

  // advance solution: x = x + alpha * p
  for (std::size_t i=0; i<m_x.size(); ++i) m_x[i] += m_alpha * m_p[i];

  ++m_it;

  if (m_it < m_maxit && norm > m_tol*m_normb) {
    //std::cout << "ch:" << thisIndex << ", it:" << m_it << ": " << norm << " >? " << m_tol*m_normb << '\n';
    next();
  } else {
     //std::cout << thisIndex << "x: ";
     ////std::size_t j=0; for (auto i : m_x) std::cout << m_gid[j++] << ':' << i << ' ';
     //for (auto i : m_x) std::cout << i << ' ';
     //std::cout << '\n';
     //std::cout << "ch:" << thisIndex << ", it:" << m_it << ": " << norm << '\n';
     m_solved.send( CkDataMsg::buildNew( sizeof(tk::real), &norm ) );
  }
}


#include "NoWarning/conjugategradients.def.h"
