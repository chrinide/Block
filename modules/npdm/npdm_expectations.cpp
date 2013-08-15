/*                                                                           
Developed by Sandeep Sharma and Garnet K.-L. Chan, 2012                      
Copyright (c) 2012, Garnet K.-L. Chan                                        
                                                                             
This program is integrated in Molpro with the permission of 
Sandeep Sharma and Garnet K.-L. Chan
*/

#include <boost/lexical_cast.hpp>
#include "MatrixBLAS.h"
#include "pario.h"
//FIXME use forward declaration for spinExpectation
#include "npdm_expectations_engine.h"
#include "npdm_expectations.h"
#include "npdm_operators.h"
#include "npdm_patterns.h"

namespace SpinAdapted{
namespace Npdm{

// Forward declaration
void npdm_set_up_linear_equations(std::string& s, std::vector<double>& b0, Matrix& A, ColumnVector& b, std::vector< std::vector<int> >& so_indices);

//===========================================================================================================================================================

Npdm_expectations::Npdm_expectations( Wavefunction & wavefunction, 
                                      const SpinBlock & big, 
                                      NpdmSpinOps & lhsOps,
                                      NpdmSpinOps & dotOps,
                                      NpdmSpinOps & rhsOps )
: wavefunction_(wavefunction),
  big_(big),
  lhsOps_(lhsOps),
  dotOps_(dotOps),
  rhsOps_(rhsOps)
{ }

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

std::string Npdm_expectations::get_op_string()
{

  // Set up npdm element indices
  std::vector<int> indices;
  indices.reserve( lhsOps_.indices_.size() + dotOps_.indices_.size() + rhsOps_.indices_.size() );
  indices.insert( indices.end(), lhsOps_.indices_.begin(), lhsOps_.indices_.end() );
  indices.insert( indices.end(), dotOps_.indices_.begin(), dotOps_.indices_.end() );
  indices.insert( indices.end(), rhsOps_.indices_.begin(), rhsOps_.indices_.end() );
//FIXME test for 2PDM or 3PDM
  assert( (indices.size() == 4) || (indices.size() == 6) || (indices.size() == 8) );
  cout << "dot indices = "; for (auto it = dotOps_.indices_.begin(); it != dotOps_.indices_.end(); ++it) { cout << *it << " "; } cout << std::endl;
  cout << "spatial indices = "; for (auto it = indices.begin(); it != indices.end(); ++it) { cout << *it << " "; } cout << std::endl;

  // Set up how tensor operator is constructed from (compound) block operators
  std::string build_pattern = "(";
  build_pattern.reserve( lhsOps_.build_pattern_.size() + dotOps_.build_pattern_.size() + rhsOps_.build_pattern_.size() + 2 );
  build_pattern.insert( build_pattern.end(), lhsOps_.build_pattern_.begin(), lhsOps_.build_pattern_.end() );
  build_pattern.insert( build_pattern.end(), dotOps_.build_pattern_.begin(), dotOps_.build_pattern_.end() );
  build_pattern.push_back( ')' );
  build_pattern.insert( build_pattern.end(), rhsOps_.build_pattern_.begin(), rhsOps_.build_pattern_.end() );

  // Combine indices and build_pattern into one string
  std::string op_string;
  for (auto it = build_pattern.begin(); it != build_pattern.end(); ++it) {
    op_string.push_back(*it);
    if ( (*it == 'C') || (*it == 'D') ) {
      string index = boost::lexical_cast<string>( indices.at(0) );
      op_string.append(index);
      indices.erase( indices.begin() );  
    }
  }
  cout << op_string << std::endl;

  return op_string;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------
//
//Oporder Npdm_expectations::old_parse_build_pattern( std::vector<char> build_pattern )
//{
//
//  std::vector<char> test;
//
//for (auto it = build_pattern.begin(); it != build_pattern.end(); ++it) {
//  pout << *it;
//}
//pout << std::endl;
//
//  // 2,2,0
//  test = { '(','(','C','C',')','(','D','D',')',')' };
//  if ( build_pattern == test ) return CC_DD;  // (C2 VERIFIED)
//  test = { '(','(','C','D',')','(','C','D',')',')' };
//  if ( build_pattern == test ) return CD_CD;
//
//  // 2,1,1
//  test = { '(','(','C','C',')','(','D',')',')','(','D',')' };
//  if ( build_pattern == test ) return CC_D_D;
//  test = { '(','(','C','D',')','(','C',')',')','(','D',')' };
//  if ( build_pattern == test ) return CD_CD;  // ???
//  test = { '(','(','C','D',')','(','D',')',')','(','C',')' };
//  if ( build_pattern == test ) return CD_D_C; 
//
//  // 1,3,0  (C2 VERIFIED)
//  test = { '(','(','C',')','(','C','(','D','D',')',')',')' };
//  if ( build_pattern == test ) return CC_D_D;
//
//  // 1,2,1 
//  test = { '(','(','C',')','(','C','D',')',')','(','D',')' };
//  if ( build_pattern == test ) return C_CD_D;  // (H2O verified)
//  test = { '(','(','C',')','(','D','D',')',')','(','C',')' };
//  if ( build_pattern == test ) return D_CC_D; // ????
//
//  // 0,4,0  (VERIFIED)
//  test = { '(','(','(','C','C',')','(','D','D',')',')',')' };
//  if ( build_pattern == test ) return CC_DD;
//
//  // 0,3,1  (C2 VERIFIED)
//  test = { '(','(','(','C','C',')','D',')',')','(','D',')' };
//  if ( build_pattern == test ) return CC_D_D;
//
//  for (auto i = build_pattern.begin(); i != build_pattern.end(); ++i) {
//    pout << *i;
//  }
//  pout << std::endl;
//  assert( false );
//
//}
//
////-----------------------------------------------------------------------------------------------------------------------------------------------------------
//
//void Npdm_expectations::old_transform_spin_adapt_to_nonspin_adapt( array_4d<double> & twopdm ) 
//{
//
//  // Set-up npdm element indices
//  std::vector<int> indices;
//  indices.reserve( lhsOps_.indices_.size() + dotOps_.indices_.size() + rhsOps_.indices_.size() );
//  indices.insert( indices.end(), lhsOps_.indices_.begin(), lhsOps_.indices_.end() );
//  indices.insert( indices.end(), dotOps_.indices_.begin(), dotOps_.indices_.end() );
//  indices.insert( indices.end(), rhsOps_.indices_.begin(), rhsOps_.indices_.end() );
//  assert (indices.size() == 4);
//pout << "indices = " << indices[0] << "," << indices[1] << "," << indices[2] << "," << indices[3] << std::endl;
//
//  // Set-up how tensor operator is constructed from (compound) block operators
//  std::vector<char> build_pattern = { '(' };
//  build_pattern.reserve( lhsOps_.build_pattern_.size() + dotOps_.build_pattern_.size() + rhsOps_.build_pattern_.size() + 2 );
//  build_pattern.insert( build_pattern.end(), lhsOps_.build_pattern_.begin(), lhsOps_.build_pattern_.end() );
//  build_pattern.insert( build_pattern.end(), dotOps_.build_pattern_.begin(), dotOps_.build_pattern_.end() );
//  build_pattern.push_back( ')' );
//  build_pattern.insert( build_pattern.end(), rhsOps_.build_pattern_.begin(), rhsOps_.build_pattern_.end() );
//
//  // Call old code for transforming spin-adapted expectation values and twopdm update
//  // Translate our format for the build pattern into the types used by the old twopdm implementation
//  Oporder build_pattern_type = old_parse_build_pattern( build_pattern );  
//  assert( expectations_.size() > 0 );
////if ( build_pattern_type == CD_CD )
//  spin_to_nonspin( indices, expectations_, twopdm, build_pattern_type, true );
//
//}
//
////-----------------------------------------------------------------------------------------------------------------------------------------------------------

double Npdm_expectations::contract_spin_adapted_operators( int ilhs, int idot, int irhs )
{
//FIXME is null_deleter() necessary below?
  SparseMatrix* null = 0; 
  double expectation;

  boost::shared_ptr<SparseMatrix> lhsOp, dotOp, rhsOp;
  if ( lhsOps_.opReps_.size() > 0 ) lhsOp = lhsOps_.opReps_.at(ilhs);
  if ( dotOps_.opReps_.size() > 0 ) dotOp = dotOps_.opReps_.at(idot);
  if ( rhsOps_.opReps_.size() > 0 ) rhsOp = rhsOps_.opReps_.at(irhs);

//if ( lhsOps_.opReps_.size() > 0 ) pout << "lhsOp:\n" << *lhsOp;
//if ( dotOps_.opReps_.size() > 0 ) pout << "dotOp:\n" << *dotOp;
//if ( rhsOps_.opReps_.size() > 0 ) pout << "rhsOp:\n" << *rhsOp;

  // We need to distinguish cases where one or more blocks has an empty operator string
  // X_X_X
  if ( (lhsOps_.opReps_.size() > 0) && (dotOps_.opReps_.size() > 0) && (rhsOps_.opReps_.size() > 0) ) {
//pout << "hello X_X_X\n";
    Transposeview lhsOpTr = Transposeview(*lhsOp);
    if ( lhsOps_.transpose_ ) lhsOp = boost::shared_ptr<SparseMatrix>( &lhsOpTr, boostutils::null_deleter() );
    Transposeview dotOpTr = Transposeview(*dotOp);
    if ( dotOps_.transpose_ ) dotOp = boost::shared_ptr<SparseMatrix>( &dotOpTr, boostutils::null_deleter() );
    Transposeview rhsOpTr = Transposeview(*rhsOp);
    if ( rhsOps_.transpose_ ) rhsOp = boost::shared_ptr<SparseMatrix>( &rhsOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *lhsOp, *dotOp, *rhsOp, big_);
  }
  // X_X_0
  else if ( (lhsOps_.opReps_.size() > 0) && (dotOps_.opReps_.size() > 0) && (rhsOps_.opReps_.size() == 0) ) {
//pout << "hello X_X_0\n";
    Transposeview lhsOpTr = Transposeview(*lhsOp);
    if ( lhsOps_.transpose_ ) lhsOp = boost::shared_ptr<SparseMatrix>( &lhsOpTr, boostutils::null_deleter() );
    Transposeview dotOpTr = Transposeview(*dotOp);
    if ( dotOps_.transpose_ ) dotOp = boost::shared_ptr<SparseMatrix>( &dotOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *lhsOp, *dotOp, *null, big_);
  }
  // X_0_X
  else if ( (lhsOps_.opReps_.size() > 0) && (dotOps_.opReps_.size() == 0) && (rhsOps_.opReps_.size() > 0) ) {
//pout << "hello X_0_X\n";
    Transposeview lhsOpTr = Transposeview(*lhsOp);
    if ( lhsOps_.transpose_ ) lhsOp = boost::shared_ptr<SparseMatrix>( &lhsOpTr, boostutils::null_deleter() );
    Transposeview rhsOpTr = Transposeview(*rhsOp);
    if ( rhsOps_.transpose_ ) rhsOp = boost::shared_ptr<SparseMatrix>( &rhsOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *lhsOp, *null, *rhsOp, big_);
  }
  // 0_X_X
  else if ( (lhsOps_.opReps_.size() == 0) && (dotOps_.opReps_.size() > 0) && (rhsOps_.opReps_.size() > 0) ) {
//pout << "hello 0_X_X\n";
    Transposeview dotOpTr = Transposeview(*dotOp);
    if ( dotOps_.transpose_ ) dotOp = boost::shared_ptr<SparseMatrix>( &dotOpTr, boostutils::null_deleter() );
    Transposeview rhsOpTr = Transposeview(*rhsOp);
    if ( rhsOps_.transpose_ ) rhsOp = boost::shared_ptr<SparseMatrix>( &rhsOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *null, *dotOp, *rhsOp, big_);
  }
  // X_0_0
  else if ( (lhsOps_.opReps_.size() > 0) && (dotOps_.opReps_.size() == 0) && (rhsOps_.opReps_.size() == 0) ) {
//pout << "hello X_0_0\n";
    Transposeview lhsOpTr = Transposeview(*lhsOp);
    if ( lhsOps_.transpose_ ) lhsOp = boost::shared_ptr<SparseMatrix>( &lhsOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *lhsOp, *null, *null, big_);
  }
  // 0_X_0
  else if ( (lhsOps_.opReps_.size() == 0) && (dotOps_.opReps_.size() > 0) && (rhsOps_.opReps_.size() == 0) ) {
//pout << "hello 0_X_0\n";
    Transposeview dotOpTr = Transposeview(*dotOp);
    if ( dotOps_.transpose_ ) dotOp = boost::shared_ptr<SparseMatrix>( &dotOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *null, *dotOp, *null, big_);
  }
  // 0_0_X
  else if ( (lhsOps_.opReps_.size() == 0) && (dotOps_.opReps_.size() == 0) && (rhsOps_.opReps_.size() > 0) ) {
//pout << "hello 0_0_X\n";
    Transposeview rhsOpTr = Transposeview(*rhsOp);
    if ( rhsOps_.transpose_ ) rhsOp = boost::shared_ptr<SparseMatrix>( &rhsOpTr, boostutils::null_deleter() );
    expectation = spinExpectation(wavefunction_, wavefunction_, *null, *null, *rhsOp, big_);
  }
  else assert(false);

  // Modify new element with sign factors and return
  double factor = lhsOps_.factor_ * dotOps_.factor_ * rhsOps_.factor_;
//pout << "expectation, factor = " << expectation << ", " << factor  << std::endl;
  return expectation*factor;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

bool Npdm_expectations::test_for_singlet( int lhs_mult, int dot_mult, int rhs_mult )
{
  // Work with 2*S instead of multiplicities
  int lhs2S = lhs_mult -1;
  int dot2S = dot_mult -1;
  int rhs2S = rhs_mult -1;
//pout << "2S(lhs,dot,rhs) =   " << lhs2S << " " << dot2S << " " << rhs2S << std::endl;

//if ( (lhs2S == 0) && (dot2S == 0) && (rhs2S == 0) ) return true;
//else return false;

  // Couple LHS and Dot spin angular momenta and see if any equal RHS  
  for (int twoS = std::abs(lhs2S - dot2S); twoS <= ( lhs2S + dot2S ); twoS += 2 ) {
    if ( twoS == rhs2S ) {
//      pout << "\nsinglet found!\n";
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Npdm_expectations::build_spin_adapted_singlet_expectations()
{
  expectations_.clear();

  // IMPORTANT: generate spin-components in the same order as RHS of linear equation solver in npdm_set_up_linear_equations routine
  // i.e. in accordance with the operator string build_pattern
  for (int irhs = 0; irhs < rhsOps_.mults_.size(); ++irhs) {
    for (int idot = 0; idot < dotOps_.mults_.size(); ++idot) {
      for (int ilhs = 0; ilhs < lhsOps_.mults_.size(); ++ilhs) {
//pout << "spin comp: ilhs, idot, irhs = " << ilhs << idot << irhs << std::endl;

        // Check that the spin multiplicities of the actual operators we've got are what we think they are!
//if ( lhsOps_.opReps_.size() > 0 ) {
//pout << lhsOps_.mults_.at(ilhs) -1 << "          " << lhsOps_.opReps_.at(ilhs)->get_deltaQuantum().totalSpin << std::endl;
//}
        if ( lhsOps_.opReps_.size() > 0 ) assert( lhsOps_.mults_.at(ilhs) -1 == lhsOps_.opReps_.at(ilhs)->get_deltaQuantum().totalSpin );
        if ( dotOps_.opReps_.size() > 0 ) assert( dotOps_.mults_.at(idot) -1 == dotOps_.opReps_.at(idot)->get_deltaQuantum().totalSpin );
        if ( rhsOps_.opReps_.size() > 0 ) assert( rhsOps_.mults_.at(irhs) -1 == rhsOps_.opReps_.at(irhs)->get_deltaQuantum().totalSpin );

        // Screen operator combinations that do not combine to give a singlet
        bool singlet = test_for_singlet( lhsOps_.mults_.at(ilhs), dotOps_.mults_.at(idot), rhsOps_.mults_.at(irhs) );
        if ( singlet ) expectations_.push_back( contract_spin_adapted_operators( ilhs, idot, irhs ) );
      }
    }
  }

assert (expectations_.size() > 0);
cout << "---------------------------------\n";
cout << "spin-adapted expectations =\n";
cout << "mpirank = " << mpigetrank() << endl;
for (auto it = expectations_.begin(); it != expectations_.end(); ++it) {
  cout << *it << std::endl;
}
cout << "---------------------------------\n";

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

std::vector< std::pair< std::vector<int>, double > > Npdm_expectations::get_nonspin_adapted_expectations( int dim )
{
  assert(dim > 0);
  // Contract spin-adapted spatial operators and build singlet expectation values
  build_spin_adapted_singlet_expectations();
  
  // Now transform to non-spin-adapted spin-orbital representation
  std::string op_string;
  // Set up operator string  
  op_string = get_op_string();

  // b holds the spin-adapted expectation values (we only care about the singlets)
  ColumnVector x(dim), b(dim);
  // x holds the non-spin-adapted expectation values
  x=0.0;

  // Transformation matrix
  Matrix A(dim,dim);
  // Vector of spin-orbital indices ordered according to A
  std::vector< std::vector<int> > so_indices(dim);

  // Parse operator string and set up linear equations
  npdm_set_up_linear_equations(op_string, expectations_, A, b, so_indices );

//pout << "A matrix:\n";
//for (int i=1; i<(dim+1); ++i) { 
//  pout << i << "\t\t";
//  for (int j=1; j<(dim+1); ++j) {
//    pout << "  " << A(i,j);
//  }
//  pout << std::endl;
//}

//pout << "b vector:\n";
//for (int i=1; i<(dim+1); ++i) { 
//    pout << i << "\t\t" << b(i) << std::endl;
//}

  // Solve A.x = b to get non-spin-adapted expectations in x
  xsolve_AxeqB(A, b, x);

  // Package transformed elements into container and return
  std::vector< std::pair< std::vector<int>, double > > new_pdm_elements;
  for (int i=0; i < so_indices.size(); ++i) {
    new_pdm_elements.push_back( std::make_pair(so_indices[i], x(i+1)) );
  } 

//pout << "x vector:\n";
//for (int i=1; i<(dim+1); ++i) { 
//    pout << i << "\t\t" << x(i) << std::endl;
//}
  
  return new_pdm_elements;
}

//===========================================================================================================================================================

}
}
