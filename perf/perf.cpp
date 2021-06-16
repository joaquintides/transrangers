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
#include <benchmark/benchmark.h>
#include <functional>
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

#ifdef _WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#ifdef _DEBUG
#pragma comment ( lib, "benchmarkd.lib" )
#else
#pragma comment ( lib, "benchmark.lib" )
#endif
#endif

volatile int ret;

auto is_even=[](int x){return x%2==0;};
auto x3=[](int x){return 3*x;};

auto rng1=[]{
  std::vector<int> rng1(1000000);
  std::iota(rng1.begin(),rng1.end(),0);
  return rng1;
}();

static void test1_handwritten(benchmark::State& st)
{
  for(auto _:st){
    int res=0;
    for(auto x:rng1){
      if(is_even(x))res+=x3(x);
    }
    ret=res;
  }
}
BENCHMARK(test1_handwritten);

static void test1_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    ret=accumulate(
      transform(x3,filter(is_even,all(rng1))),0);
  }
}
BENCHMARK(test1_transrangers);

static void test1_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    ret=ranges::accumulate(
      rng1|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test1_rangev3);

auto rng2=rng1;
int n=rng2.size()+rng2.size()/2;

static void test2_handwritten(benchmark::State& st)
{
  for(auto _:st){
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
  }
}
BENCHMARK(test2_handwritten);

static void test2_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    ret=accumulate(
      transform(x3,filter(is_even,take(n,concat(all(rng2),all(rng2))))),0);
  }
}
BENCHMARK(test2_transrangers);

static void test2_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    ret=ranges::accumulate(
      concat(rng2,rng2)|take(n)|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test2_rangev3);

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

static void test3_handwritten(benchmark::State& st)
{
  for(auto _:st){
    int res=0;
    int x=rng3[0]+1;
    for(int y:rng3){
      if(y!=x){
        x=y;
        if(is_even(x))res+=x;
      }
    }
    ret=res;
  }
}
BENCHMARK(test3_handwritten);

static void test3_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    ret=accumulate(
      filter(is_even,unique(all(rng3))),0);
  }
}
BENCHMARK(test3_transrangers);

static void test3_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    ret=ranges::accumulate(
      rng3|unique|filter(is_even),0);
  }
}
BENCHMARK(test3_rangev3);

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

static void test4_handwritten(benchmark::State& st)
{
  for(auto _:st){
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
  }
}
BENCHMARK(test4_handwritten);

static void test4_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    ret=accumulate(
      transform(x3,filter(is_even,unique(ranger_join(all(rng4))))),0);
  }
}
BENCHMARK(test4_transrangers);

static void test4_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    ret=ranges::accumulate(
      rng4|join|unique|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test4_rangev3);

auto rng5=rng4;

static void test5_handwritten(benchmark::State& st)
{
  for(auto _:st){
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
  }
}
BENCHMARK(test5_handwritten);

static void test5_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    auto unique_adaptor=[](auto&& srng){
      return unique(all(std::forward<decltype(srng)>(srng)));
    };
    ret=accumulate(
      transform(x3,filter(is_even,join(transform(unique_adaptor,all(rng5))))),
      0);
  }
}
BENCHMARK(test5_transrangers);

static void test5_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    auto unique_adaptor=[](auto&& srng){return srng|unique;};
    ret=ranges::accumulate(
      rng5|transform(unique_adaptor)|join|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test5_rangev3);

auto divisible_by_3=[](int x){return x%3==0;};
auto sum=[](const auto& p){return std::get<0>(p)+std::get<1>(p);};
auto rng6=rng1;

static void test6_handwritten(benchmark::State& st)
{
  for(auto _:st){
    int res=0;
    for(auto x:rng6){
      auto y=x+x3(x);
      if(divisible_by_3(y))res+=y;
    }
    ret=res;
  }
}
BENCHMARK(test6_handwritten);

static void test6_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    ret=accumulate(
      filter(divisible_by_3,
        transform(sum,zip(all(rng6),transform(x3,all(rng6))))),0);
  }
}
BENCHMARK(test6_transrangers);

static void test6_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    ret=ranges::accumulate(
      zip(rng6,rng6|transform(x3))|transform(sum)|filter(divisible_by_3),0);
  }
}
BENCHMARK(test6_rangev3);

BENCHMARK_MAIN();
