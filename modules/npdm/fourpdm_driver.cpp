/*                                                                           
Developed by Sandeep Sharma and Garnet K.-L. Chan, 2012                      
Copyright (c) 2012, Garnet K.-L. Chan                                        
                                                                             
This program is integrated in Molpro with the permission of 
Sandeep Sharma and Garnet K.-L. Chan
*/

#include <algorithm>
#include "fourpdm_driver.h"
#include "npdm_epermute.h"

namespace SpinAdapted{

//===========================================================================================================================================================

Fourpdm_driver::Fourpdm_driver( int sites ) : Npdm_driver(4) 
{
  store_full_spin_array_ = true;
  store_full_spatial_array_ = true;

  if ( store_full_spin_array_ ) {
    fourpdm.resize(2*sites,2*sites,2*sites,2*sites,2*sites,2*sites,2*sites,2*sites);
    fourpdm.Clear();
  } 
  if ( store_full_spatial_array_ ) {
    spatial_fourpdm.resize(sites,sites,sites,sites,sites,sites,sites,sites);
    spatial_fourpdm.Clear();
  } 

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::save_npdms(const int& i, const int& j)
{
//  save_sparse_array(i,j);

//  if ( use_full_array_ ) {

//FIXME
boost::mpi::communicator world;
world.barrier();
    Timer timer;
    // Combine NPDM elements from all mpi ranks and dump to file
    accumulate_npdm();
    save_npdm_text(i, j);
    save_npdm_binary(i, j);
    build_spatial_npdm(i, j);
    save_spatial_npdm_text(i, j);
    save_spatial_npdm_binary(i, j);
world.barrier();
    pout << "4PDM save full array time " << timer.elapsedwalltime() << " " << timer.elapsedcputime() << endl;

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::save_npdm_text(const int &i, const int &j)
{
  if( mpigetrank() == 0)
  {
    char file[5000];
    sprintf (file, "%s%s%d.%d%s", dmrginp.save_prefix().c_str(),"/fourpdm.", i, j,".txt");
    ofstream ofs(file);
    ofs << fourpdm.dim1() << endl;

    double trace = 0.0;
    for(int i=0; i<fourpdm.dim1(); ++i)
      for(int j=0; j<fourpdm.dim2(); ++j)
        for(int k=0; k<fourpdm.dim3(); ++k)
          for(int l=0; l<fourpdm.dim4(); ++l)
            for(int m=0; m<fourpdm.dim5(); ++m)
              for(int n=0; n<fourpdm.dim6(); ++n)
                for(int p=0; p<fourpdm.dim7(); ++p)
                  for(int q=0; q<fourpdm.dim8(); ++q) {
                    if ( abs(fourpdm(i,j,k,l,m,n,p,q)) > 1e-14 ) {
                      ofs << boost::format("%d %d %d %d %d %d %d %d %20.14e\n") % i % j % k % l % m % n % p % q % fourpdm(i,j,k,l,m,n,p,q);
                      if ( (i==q) && (j==p) && (k==n) && (l==m) ) trace += fourpdm(i,j,k,l,m,n,p,q);
                    }
                  }
    ofs.close();
    std::cout << "Spin-orbital 4PDM trace = " << trace << "\n";
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::save_spatial_npdm_text(const int &i, const int &j)
{
  if( mpigetrank() == 0)
  {
    char file[5000];
    sprintf (file, "%s%s%d.%d%s", dmrginp.save_prefix().c_str(),"/spatial_fourpdm.", i, j,".txt");
    ofstream ofs(file);
    ofs << spatial_fourpdm.dim1() << endl;

    double trace = 0.0;
    for(int i=0; i<spatial_fourpdm.dim1(); ++i)
      for(int j=0; j<spatial_fourpdm.dim2(); ++j)
        for(int k=0; k<spatial_fourpdm.dim3(); ++k)
          for(int l=0; l<spatial_fourpdm.dim4(); ++l)
            for(int m=0; m<spatial_fourpdm.dim5(); ++m)
              for(int n=0; n<spatial_fourpdm.dim6(); ++n)
                for(int p=0; p<spatial_fourpdm.dim7(); ++p)
                  for(int q=0; q<spatial_fourpdm.dim8(); ++q) {
                    if ( abs(spatial_fourpdm(i,j,k,l,m,n,p,q)) > 1e-14 ) {
                      ofs << boost::format("%d %d %d %d %d %d %d %d %20.14e\n") % i % j % k % l % m % n % p % q % spatial_fourpdm(i,j,k,l,m,n,p,q);
                      if ( (i==q) && (j==p) && (k==n) && (l==m) ) trace += spatial_fourpdm(i,j,k,l,m,n,p,q);
                    }
                  }
    ofs.close();
    std::cout << "Spatial      4PDM trace = " << trace << "\n";
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::save_npdm_binary(const int &i, const int &j)
{
  if( mpigetrank() == 0)
  {
    char file[5000];
    sprintf (file, "%s%s%d.%d%s", dmrginp.save_prefix().c_str(),"/fourpdm.", i, j,".bin");
    std::ofstream ofs(file, std::ios::binary);
    boost::archive::binary_oarchive save(ofs);
    save << fourpdm;
    ofs.close();
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::save_spatial_npdm_binary(const int &i, const int &j)
{
  if( mpigetrank() == 0)
  {
    char file[5000];
    sprintf (file, "%s%s%d.%d%s", dmrginp.save_prefix().c_str(),"/spatial_fourpdm.", i, j,".bin");
    std::ofstream ofs(file, std::ios::binary);
    boost::archive::binary_oarchive save(ofs);
    save << spatial_fourpdm;
    ofs.close();
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::load_npdm_binary(const int &i, const int &j) { assert(false); }

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::accumulate_npdm()
{
#ifndef SERIAL
  array_8d<double> tmp_recv;
  mpi::communicator world;
  if( mpigetrank() == 0)
  {
    for(int u=1; u<world.size(); ++u) {
      world.recv(u, u, tmp_recv);
      for(int i=0; i<fourpdm.dim1(); ++i)
        for(int j=0; j<fourpdm.dim2(); ++j)
          for(int k=0; k<fourpdm.dim3(); ++k)
            for(int l=0; l<fourpdm.dim4(); ++l)
              for(int m=0; m<fourpdm.dim5(); ++m)
                for(int n=0; n<fourpdm.dim6(); ++n) 
                  for(int p=0; p<fourpdm.dim7(); ++p) 
                    for(int q=0; q<fourpdm.dim8(); ++q) 
                      if ( tmp_recv(i,j,k,l,m,n,p,q) != 0.0 ) fourpdm(i,j,k,l,m,n,p,q) = tmp_recv(i,j,k,l,m,n,p,q);
    }
  }
  else 
  {
    world.send(0, mpigetrank(), fourpdm);
  }
#endif
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::build_spatial_npdm(const int &i, const int &j)
{
  double factor = 1.0;
  if( mpigetrank() == 0)
  {
    for(int i=0; i<fourpdm.dim1()/2; ++i)
      for(int j=0; j<fourpdm.dim2()/2; ++j)
        for(int k=0; k<fourpdm.dim3()/2; ++k)
          for(int l=0; l<fourpdm.dim4()/2; ++l)
            for(int m=0; m<fourpdm.dim5()/2; ++m)
              for(int n=0; n<fourpdm.dim6()/2; ++n)
                for(int p=0; p<fourpdm.dim7()/2; ++p)
                  for(int q=0; q<fourpdm.dim8()/2; ++q) {

                    double pdm = 0.0;
                    for (int s=0; s<2; s++) {
                      for (int t=0; t<2; t++) {
                        for (int u=0; u<2; u++) {
                          for (int v=0; v<2; v++) {
                            pdm += fourpdm(2*i+s, 2*j+t, 2*k+u, 2*l+v, 2*m+v, 2*n+u, 2*p+t, 2*q+s);
                          }
                        }
                      }
                    }
                    if ( abs(pdm) > 1e-14 ) spatial_fourpdm(i,j,k,l,m,n,p,q) = factor * pdm;
                  }
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::get_even_and_odd_perms( const std::vector<int> mnpq, 
                                             std::vector< std::vector<int> > & even_perms, 
                                             std::vector< std::vector<int> > & odd_perms )
{

  // Get all even and odd mnpq permutations
  bool even = false;

  // Must sort them to get all possible permutations
  std::vector<int> foo = mnpq;
  std::sort( foo.begin(), foo.end() );

  // Get first set
  std::vector< std::vector<int> > perms1;
  do { 
    perms1.push_back( foo ); 
    if (foo == mnpq) even = true; 
  } while ( next_even_permutation(foo.begin(), foo.end()) );

  // Re-sort and swap LAST TWO elements to ensure we get all the remaining permutations
  std::sort( foo.begin(), foo.end() );
  assert( foo.size() == 4 );
  std::swap( foo[2], foo[3] );

  // Get second set
  std::vector< std::vector<int> > perms2;
  do { 
    perms2.push_back( foo ); 
  } while ( next_even_permutation(foo.begin(), foo.end()) );

  // Assign as even or odd permutations
  even_perms = perms1;
  odd_perms = perms2;
  if (!even) std::swap( even_perms, odd_perms );

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------
// The number of possible combinations are (4!)**2

void Fourpdm_driver::assign_npdm_antisymmetric(const int i, const int j, const int k, const int l, 
                                               const int m, const int n, const int p, const int q, const double val)
{
//if ( abs(val) > 1e-8 ) {
//  pout << "so-fourpdm val: i,j,k,l,m,n,p,q = " 
//       << i << "," << j << "," << k << "," << l << "," << m << "," << n << "," << p << "," << q
//       << "\t\t" << val << endl;
//}

  // Test for duplicates
  if ( fourpdm(i,j,k,l,m,n,p,q) != 0.0 && abs(fourpdm(i,j,k,l,m,n,p,q)-val) > 1e-6) {
    void *array[10];
    size_t size;
    size = backtrace(array, 10);
    cout << "WARNING: Already calculated "<<i<<" "<<j<<" "<<k<<" "<<l<<" "<<m<<" "<<n<<" "<<p<<" "<<q<<endl;
    //backtrace_symbols_fd(array, size, 2);
    cout << "earlier value: " << fourpdm(i,j,k,l,m,n,p,q) << endl << "new value:     " <<val<<endl;
    cout.flush();
    assert( false );
    return;
  }

  if ( abs(val) < 1e-14 ) return;

  // If indices are not all unique, then all elements should be zero (and next_even_permutation fails)
  std::vector<int> v = {i,j,k,l};
  std::sort( v.begin(), v.end() );
  if ( (v[0]==v[1]) || (v[1]==v[2]) || (v[2]==v[3]) ) { if (abs(val) > 1e-15) { std::cout << abs(val) << std::endl; assert(false); } }
  std::vector<int> w = {m,n,p,q};
  std::sort( w.begin(), w.end() );
  if ( (w[0]==w[1]) || (w[1]==w[2]) || (w[2]==w[3]) ) { if (abs(val) > 1e-15) { std::cout << abs(val) << std::endl; assert(false); } }

  // Get all even and odd permutations
  const std::vector<int> ijkl = {i,j,k,l};
  std::vector< std::vector<int> > ijkl_even, ijkl_odd;
  get_even_and_odd_perms( ijkl, ijkl_even, ijkl_odd );
  assert ( ijkl_even.size() + ijkl_odd.size() == 24 );

  const std::vector<int> mnpq = {m,n,p,q};
  std::vector< std::vector<int> > mnpq_even, mnpq_odd;
  get_even_and_odd_perms( mnpq, mnpq_even, mnpq_odd );
  assert ( mnpq_even.size() + mnpq_odd.size() == 24 );

  // Even-Even terms
  for ( auto u = ijkl_even.begin(); u != ijkl_even.end(); ++u )
    for ( auto v = mnpq_even.begin(); v != mnpq_even.end(); ++v )
      fourpdm( (*u)[0],(*u)[1],(*u)[2],(*u)[3], (*v)[0],(*v)[1],(*v)[2],(*v)[3] ) = val;
  // Even-Odd terms
  for ( auto u = ijkl_even.begin(); u != ijkl_even.end(); ++u )
    for ( auto v = mnpq_odd.begin(); v != mnpq_odd.end(); ++v )
      fourpdm( (*u)[0],(*u)[1],(*u)[2],(*u)[3], (*v)[0],(*v)[1],(*v)[2],(*v)[3] ) = -val;
  // Odd-Even terms
  for ( auto u = ijkl_odd.begin(); u != ijkl_odd.end(); ++u )
    for ( auto v = mnpq_even.begin(); v != mnpq_even.end(); ++v )
      fourpdm( (*u)[0],(*u)[1],(*u)[2],(*u)[3], (*v)[0],(*v)[1],(*v)[2],(*v)[3] ) = -val;
  // Odd-Odd terms
  for ( auto u = ijkl_odd.begin(); u != ijkl_odd.end(); ++u )
    for ( auto v = mnpq_odd.begin(); v != mnpq_odd.end(); ++v )
      fourpdm( (*u)[0],(*u)[1],(*u)[2],(*u)[3], (*v)[0],(*v)[1],(*v)[2],(*v)[3] ) = val;

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void Fourpdm_driver::store_npdm_elements( std::vector< std::pair< std::vector<int>, double > > & new_spin_orbital_elements )
{
  for (int i=0; i < new_spin_orbital_elements.size(); ++i) {
    assert( new_spin_orbital_elements[i].first.size() == 8 );
    int ix = new_spin_orbital_elements[i].first[0];
    int jx = new_spin_orbital_elements[i].first[1];
    int kx = new_spin_orbital_elements[i].first[2];
    int lx = new_spin_orbital_elements[i].first[3];
    int mx = new_spin_orbital_elements[i].first[4];
    int nx = new_spin_orbital_elements[i].first[5];
    int px = new_spin_orbital_elements[i].first[6];
    int qx = new_spin_orbital_elements[i].first[7];
    double x = new_spin_orbital_elements[i].second;
    assign_npdm_antisymmetric(ix, jx, kx, lx, mx, nx, px, qx, x);
  }

  for (int i=0; i < new_spin_orbital_elements.size(); ++i) {
    assert( new_spin_orbital_elements[i].first.size() == 8 );
    int ix = new_spin_orbital_elements[i].first[7];
    int jx = new_spin_orbital_elements[i].first[6];
    int kx = new_spin_orbital_elements[i].first[5];
    int lx = new_spin_orbital_elements[i].first[4];
    int mx = new_spin_orbital_elements[i].first[3];
    int nx = new_spin_orbital_elements[i].first[2];
    int px = new_spin_orbital_elements[i].first[1];
    int qx = new_spin_orbital_elements[i].first[0];
    double x = new_spin_orbital_elements[i].second;
    assign_npdm_antisymmetric(ix, jx, kx, lx, mx, nx, px, qx, x);
  }
}

//===========================================================================================================================================================

}
