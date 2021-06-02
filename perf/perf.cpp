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
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/unique.hpp>
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

auto rng=[]{
  std::vector<int> rng(1000000);
  std::iota(rng.begin(),rng.end(),0);
  return rng;
}();

auto is_even=[](int x){return x%2==0;};
auto x3=[](int x){return 3*x;};

static void test1_handwritten(benchmark::State& st)
{
  for (auto _:st){
    int res=0;
    for(auto x:rng){
      if(is_even(x))res+=x3(x);
    }
    volatile auto res2=res;
  }
}
BENCHMARK(test1_handwritten);

static void test1_transrangers(benchmark::State& st)
{
  for (auto _:st){
    using namespace transrangers;
      
    int  res=0;
    auto rgr=transform(x3,filter(is_even,all(rng)));
    rgr([&](auto p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test1_transrangers);

static void test1_rangev3(benchmark::State& st)
{
  for (auto _:st){
    using namespace ranges::views;

    volatile auto res=ranges::accumulate(
      rng|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test1_rangev3);

auto rng2=rng;
int n=rng.size()+rng.size()/2;

static void test2_handwritten(benchmark::State& st)
{
  for (auto _:st){
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
    volatile auto res2=res;
  }
}
BENCHMARK(test2_handwritten);

static void test2_transrangers(benchmark::State& st)
{
  for (auto _:st){
    using namespace transrangers;
      
    int  res=0;
    auto rgr=transform(x3,filter(is_even,take(n,concat(all(rng2),all(rng2)))));
    rgr([&](auto p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test2_transrangers);

static void test2_rangev3(benchmark::State& st)
{
  for (auto _:st){
    using namespace ranges::views;

    volatile auto res=ranges::accumulate(
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
  for (auto _:st){
    int res=0;
    int x=rng3[0]+1;
    for(int y:rng3){
      if(y!=x){
        x=y;
        if(is_even(x))res+=x;
      }
    }
    volatile auto res2=res;
  }
}
BENCHMARK(test3_handwritten);

static void test3_transrangers(benchmark::State& st)
{
  for (auto _:st){
    using namespace transrangers;
      
    int  res=0;
    auto rgr=filter(is_even,unique(all(rng3)));
    rgr([&](auto p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test3_transrangers);

static void test3_rangev3(benchmark::State& st)
{
  for (auto _:st){
    using namespace ranges::views;

    volatile auto res=ranges::accumulate(
      rng3|unique|filter(is_even),0);
  }
}
BENCHMARK(test3_rangev3);

BENCHMARK_MAIN();
