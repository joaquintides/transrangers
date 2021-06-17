/* Transrangers performance benchmark.
 *
 * Copyright 2021 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://github.com/joaquintides/transrangers for project home page.
 */

#include <algorithm>
#include <functional>
#include <iostream>
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <numeric>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/unique.hpp>
#include <range/v3/view/zip.hpp>
#include <transrangers.hpp>
#include <vector>

volatile int ret;

auto is_even=[](int x){return x%2==0;};
auto x3=[](int x){return 3*x;};

auto rng1=[]{
  std::vector<int> rng1(1000000);
  std::iota(rng1.begin(),rng1.end(),0);
  return rng1;
}();

auto test1_handwritten=[]
{
  int res=0;
  for(auto x:rng1){
    if(is_even(x))res+=x3(x);
  }
  ret=res;
};

auto test1_transrangers=[]
{
  using namespace transrangers;
    
  ret=accumulate(
    transform(x3,filter(is_even,all(rng1))),0);
};

auto test1_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    rng1|filter(is_even)|transform(x3),0);
};

auto rng2=rng1;
int n=rng2.size()+rng2.size()/2;

auto test2_handwritten=[]
{
  int res=0;
  int m=n;
  auto f=[&]{
    for(auto first=std::begin(rng2),last=std::end(rng2);
        m&&first!=last;--m,++first){
      auto&& x=*first;
      if(is_even(x))res+=x3(x);
    }
  };
  f();f();
  ret=res;
};

auto test2_transrangers=[]
{
  using namespace transrangers;
    
  ret=accumulate(
    transform(x3,filter(is_even,take(n,concat(all(rng2),all(rng2))))),0);
};

auto test2_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    concat(rng2,rng2)|take(n)|filter(is_even)|transform(x3),0);
};

auto rng3=[]{
  std::vector<int> rng3;
  for(int i=0;i<100000/4;++i){
    rng3.push_back(i);
    rng3.push_back(i);
    rng3.push_back(i);
    rng3.push_back(i);
  }
  return rng3;
}();

auto test3_handwritten=[]
{
  int res=0;
  int x=rng3[0]+1;
  for(int y:rng3){
    if(y!=x){
      x=y;
      if(is_even(x))res+=x;
    }
  }
  ret=res;
};

auto test3_transrangers=[]
{
  using namespace transrangers;
    
  ret=accumulate(
    filter(is_even,unique(all(rng3))),0);
};

auto test3_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    rng3|unique|filter(is_even),0);
};

auto rng4=[]{
  std::vector<int> srng;
  for(int i=0;i<100000/4;++i){
    srng.push_back(i);
    srng.push_back(i);
    srng.push_back(i);
    srng.push_back(i);
  }
  std::vector<std::vector<int>> rng4(10,srng);
  return rng4;
}();

auto test4_handwritten=[]
{
  int res=0;
  int x=rng4[0][0]+1;
  for(auto&& srng:rng4){
    for(int y:srng){
      if(y!=x){
        x=y;
        if(is_even(x))res+=x3(x);
      }
    }
  }
  ret=res;
};

auto test4_transrangers=[]
{
  using namespace transrangers;
    
  ret=accumulate(
    transform(x3,filter(is_even,unique(ranger_join(all(rng4))))),0);
};

auto test4_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    rng4|join|unique|filter(is_even)|transform(x3),0);
};

auto rng5=rng4;

auto test5_handwritten=[]
{
  int res=0;
  for(auto&& srng:rng5){
    int x=srng[0]+1;
    for(int y:srng){
      if(y!=x){
        x=y;
        if(is_even(x))res+=x3(x);
      }
    }
  }
  ret=res;
};

auto rangev3_unique_adaptor=[](auto&& srng)
{
  return srng|ranges::views::unique;
};

auto test5_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    rng5|transform(rangev3_unique_adaptor)|join|filter(is_even)|transform(x3),
    0);
};


auto test5_rangev3=[]
{
  using namespace ranges::views;

  auto unique_adaptor=[](auto&& srng){return srng|unique;};
  ret=ranges::accumulate(
    rng5|transform(unique_adaptor)|join|filter(is_even)|transform(x3),0);
};

auto divisible_by_3=[](int x){return x%3==0;};
auto sum=[](const auto& p){return std::get<0>(p)+std::get<1>(p);};
auto rng6=rng1;

auto test6_handwritten=[]
{
  int res=0;
  for(auto x:rng6){
    auto y=x+x3(x);
    if(divisible_by_3(y))res+=y;
  }
  ret=res;
};

auto test6_transrangers=[]
{
  using namespace transrangers;
    
  ret=accumulate(
    filter(divisible_by_3,
      transform(sum,zip(all(rng6),transform(x3,all(rng6))))),0);
};

auto test6_rangev3=[]
{
  using namespace ranges::views;

  ret=ranges::accumulate(
    zip(rng6,rng6|transform(x3))|transform(sum)|filter(divisible_by_3),0);
};

int main()
{
  auto bench=ankerl::nanobench::Bench().minEpochIterations(100);
  
  bench.run("test1_handwritten",test1_handwritten);
  bench.run("test1_transrangers",test1_transrangers);
  bench.run("test1_rangev3",test1_rangev3);

  bench.run("test2_handwritten",test2_handwritten);
  bench.run("test2_transrangers",test2_transrangers);
  bench.run("test2_rangev3",test2_rangev3);

  bench.run("test3_handwritten",test3_handwritten);
  bench.run("test3_transrangers",test3_transrangers);
  bench.run("test3_rangev3",test3_rangev3);

  bench.run("test4_handwritten",test4_handwritten);
  bench.run("test4_transrangers",test4_transrangers);
  bench.run("test4_rangev3",test4_rangev3);

  bench.run("test5_handwritten",test5_handwritten);
  bench.run("test5_transrangers",test5_transrangers);
  bench.run("test5_rangev3",test5_rangev3);

  bench.run("test6_handwritten",test6_handwritten);
  bench.run("test6_transrangers",test6_transrangers);
  bench.run("test6_rangev3",test6_rangev3);
  
  ankerl::nanobench::render(
    ankerl::nanobench::templates::csv(),bench,std::cout);
}