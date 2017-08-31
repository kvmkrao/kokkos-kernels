/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "gtest/gtest.h"
#include "Kokkos_Core.hpp"
#include "Kokkos_Random.hpp"

//#include "KokkosBatched_Vector.hpp"

#include "KokkosBatched_Trsm_Decl.hpp"
#include "KokkosBatched_Trsm_Serial_Impl.hpp"
#include "KokkosBatched_Trsm_Team_Impl.hpp"

#include "KokkosKernels_TestUtils.hpp"

using namespace KokkosBatched::Experimental;

namespace Test {

  template<typename S, typename U, typename T, typename D>
  struct ParamTag {
    typedef S side;
    typedef U uplo;
    typedef T trans;
    typedef D diag;
  };

  template<typename DeviceType,
           typename ViewType,
           typename ScalarType,
           typename ParamTagType,
           typename AlgoTagType>
  struct Functor {
    ViewType _a, _b;
    
    ScalarType _alpha;

    KOKKOS_INLINE_FUNCTION
    Functor(const ScalarType alpha, 
            const ViewType &a,
            const ViewType &b) 
      : _a(a), _b(b), _alpha(alpha) {}

    template<typename MemberType>
    KOKKOS_INLINE_FUNCTION
    void operator()(const ParamTagType &, const MemberType &member) const {
      const int k = member.league_rank();
      
      auto aa = Kokkos::subview(_a, k, Kokkos::ALL(), Kokkos::ALL());
      auto bb = Kokkos::subview(_b, k, Kokkos::ALL(), Kokkos::ALL());

      TeamTrsm<MemberType,
        typename ParamTagType::side,
        typename ParamTagType::uplo,
        typename ParamTagType::trans,
        typename ParamTagType::diag,
        AlgoTagType>::
        invoke(member, _alpha, aa, bb);
    }

    inline
    void run() {
      const int league_size = _b.dimension_0();
      Kokkos::TeamPolicy<DeviceType,ParamTagType> policy(league_size, Kokkos::AUTO);
      Kokkos::parallel_for(policy, *this);
    }
  };

  template<typename DeviceType,
           typename ViewType,
           typename ScalarType,
           typename ParamTagType,
           typename AlgoTagType>
  void impl_test_batched_trsm(const int N, const int BlkSize, const int NumCols) {
    typedef typename ViewType::value_type value_type;
    typedef Kokkos::Details::ArithTraits<value_type> ats;

    /// randomized input testing views
    ScalarType alpha(1.0);

    const bool is_side_right = std::is_same<typename ParamTagType::side,Side::Right>::value;
    const int b_nrows = is_side_right ? NumCols : BlkSize;
    const int b_ncols = is_side_right ? BlkSize : NumCols;
    ViewType
      a0("a0", N, BlkSize,BlkSize), a1("a1", N, BlkSize, BlkSize),
      b0("b0", N, b_nrows,b_ncols), b1("b1", N, b_nrows, b_ncols);

    Kokkos::Random_XorShift64_Pool<typename DeviceType::execution_space> random(13718);
    Kokkos::fill_random(a0, random, value_type(1.0));
    Kokkos::fill_random(b0, random, value_type(1.0));

    Kokkos::deep_copy(a1, a0);
    Kokkos::deep_copy(b1, b0);

    Functor<DeviceType,ViewType,ScalarType,
      ParamTagType,Algo::Trsm::Unblocked>(alpha, a0, b0).run();
    Functor<DeviceType,ViewType,ScalarType,
      ParamTagType,AlgoTagType>(alpha, a1, b1).run();

    /// for comparison send it to host
    typename ViewType::HostMirror b0_host = Kokkos::create_mirror_view(b0);
    typename ViewType::HostMirror b1_host = Kokkos::create_mirror_view(b1);

    Kokkos::deep_copy(b0_host, b0);
    Kokkos::deep_copy(b1_host, b1);

    /// check b0 = b1 ; this eps is about 10^-14
    typedef typename ats::mag_type mag_type;
    mag_type sum(1), diff(0);
    const mag_type eps = 1.0e3 * ats::epsilon();

    for (int k=0;k<N;++k)
      for (int i=0;i<b_nrows;++i)
        for (int j=0;j<b_ncols;++j) {
          sum  += ats::abs(b0_host(k,i,j));
          diff += ats::abs(b0_host(k,i,j)-b1_host(k,i,j));
        }
    EXPECT_NEAR_KK( diff/sum, 0.0, eps);
  }
}


template<typename DeviceType,
         typename ValueType,
         typename ScalarType,
         typename ParamTagType,
         typename AlgoTagType>
int test_batched_trsm() {
#if defined(KOKKOSKERNELS_INST_LAYOUTLEFT)
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutLeft,DeviceType> ViewType;
    Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(     0, 10, 4);
    for (int i=0;i<10;++i) {
      printf("Testing: LayoutLeft,  Blksize %d\n", i);  
      Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(1024,  i, 4);
      Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(1024,  i, 1);
    }
  }
#endif
#if defined(KOKKOSKERNELS_INST_LAYOUTRIGHT)
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutRight,DeviceType> ViewType;
    Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(     0, 10, 4);
    for (int i=0;i<10;++i) {
      printf("Testing: LayoutRight, Blksize %d\n", i);  
      Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(1024,  i, 4);
      Test::impl_test_batched_trsm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(1024,  i, 1);
    }
  }
#endif

  return 0;
}

#if defined(KOKKOSKERNELS_INST_FLOAT)
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_u_float_float ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_n_float_float ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_u_float_float ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_n_float_float ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_u_float_float ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_n_float_float ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
#endif


#if defined(KOKKOSKERNELS_INST_DOUBLE)
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_u_double_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_n_double_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_u_double_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_n_double_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_u_double_double ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_n_double_double ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
#endif


#if defined(KOKKOSKERNELS_INST_COMPLEX_DOUBLE)
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_u_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_n_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_u_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_n_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_u_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_n_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}



TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_u_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_l_nt_n_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Lower,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_u_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_l_u_nt_n_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Left,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_u_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::Unit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_team_trsm_r_u_nt_n_dcomplex_double ) {
  typedef ::Test::ParamTag<Side::Right,Uplo::Upper,Trans::NoTranspose,Diag::NonUnit> param_tag_type;
  typedef Algo::Trsm::Blocked algo_tag_type;
  test_batched_trsm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
#endif