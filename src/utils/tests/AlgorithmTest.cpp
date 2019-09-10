#include "testing/Testing.hpp"
#include "utils/algorithm.hpp"
#include <Eigen/Core>

namespace pu = precice::utils;

BOOST_AUTO_TEST_SUITE(UtilsTests)

BOOST_AUTO_TEST_CASE(MakeArray)
{
    auto a = pu::make_array(1,2,3);
    BOOST_TEST(a.size() == 3);
    BOOST_TEST(a[0] == 1);
    BOOST_TEST(a[1] == 2);
    BOOST_TEST(a[2] == 3);
}

BOOST_AUTO_TEST_CASE(UniqueElements)
{
    std::vector<int> y{1,2,3,4,5,6,7,8,9};
    BOOST_TEST(pu::unique_elements(y));
    BOOST_TEST(pu::unique_elements(y, [](int l, int r){ return l == r; }));

    std::vector<int> n1{1,2,3,4,5,6,7,8,9,9};
    BOOST_TEST(!pu::unique_elements(n1));
    BOOST_TEST(!pu::unique_elements(n1, [](int l, int r){ return l == r; }));

    std::vector<int> n2{1,1,3,4,5,6,7,8,9};
    BOOST_TEST(!pu::unique_elements(n2));
    BOOST_TEST(!pu::unique_elements(n2, [](int l, int r){ return l == r; }));

    std::vector<int> e;
    BOOST_TEST(pu::unique_elements(e));
    BOOST_TEST(pu::unique_elements(e, [](int l, int r){ return l == r; }));
}

BOOST_AUTO_TEST_CASE(UniqueEigenElements)
{
    Eigen::VectorXd v1(3);
    v1 << 1.0, 0.1, 0.2;
    Eigen::VectorXd v2(3);
    v2 << 0.1, 1.0, 0.2;
    Eigen::VectorXd v3(3);
    v3 << 0.1, 0.2, 1.0;

    std::vector<Eigen::VectorXd> case1{};
    BOOST_TEST(pu::unique_elements(case1));

    std::vector<Eigen::VectorXd> case2{v1};
    BOOST_TEST(pu::unique_elements(case2));

    std::vector<Eigen::VectorXd> case3{v1, v2};
    BOOST_TEST(pu::unique_elements(case3));

    std::vector<Eigen::VectorXd> case4{v1, v2, v1};
    BOOST_TEST(!pu::unique_elements(case4));

    std::vector<Eigen::VectorXd> case5{v1, v2, v3};
    BOOST_TEST(pu::unique_elements(case5));

    std::vector<Eigen::VectorXd> case6{v1, v2, v3, v1};
    BOOST_TEST(!pu::unique_elements(case6));
}

BOOST_AUTO_TEST_CASE(Mismatch)
{
    std::vector<int> a{1,2,3,4,5,6,7,8,9};
    std::vector<int> b{1,2,3,4,5,0,9};

    auto aa = pu::mismatch(
            a.begin(), a.end(),
            a.begin(), a.end());
    BOOST_TEST((aa.first == aa.second));
    BOOST_TEST((aa.first == a.end()));

    auto ab = pu::mismatch(
            a.begin(), a.end(),
            b.begin(), b.end());
    BOOST_TEST((ab.first != a.end()));
    BOOST_TEST(*ab.first == 6);
    BOOST_TEST(*ab.second == 0);
}




BOOST_AUTO_TEST_SUITE_END()
