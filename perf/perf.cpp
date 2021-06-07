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
#include <range/v3/view/transform.hpp>
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

auto rng1=[]{
  std::vector<int> rng1(2000000);
  std::iota(rng1.begin(),rng1.end(),0);
  return rng1;
}();
auto rng2=[]{
  std::vector<int> rng2(1000000);
  std::iota(rng2.begin(),rng2.end(),1000);
  return rng1;
}();
auto rng3=[]{
  std::vector<int> rng3(1000000);
  std::iota(rng3.begin(),rng3.end(),2000);
  return rng1;
}();

auto sum=[](const auto& p){return std::get<0>(p)+std::get<1>(p);};

static void test1_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    int res=0;
    auto rgr=
      transform(sum,zip(all(rng1),concat(all(rng2),all(rng3))));
    rgr([&](const auto& p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test1_transrangers)->Name("transrangers zip(1,concat(2,3))");

static void test2_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    int res=0;
    auto rgr=
      transform(sum,zip(concat(all(rng2),all(rng3)),all(rng1)));
    rgr([&](const auto& p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test2_transrangers)->Name("transrangers zip(concat(2,3),1)");

static void test3_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    int res=0;
    auto rgr=
      transform(sum,push_zip(all(rng1),concat(all(rng2),all(rng3))));
    rgr([&](const auto& p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test3_transrangers)->Name("transrangers push_zip(1,concat(2,3))");

static void test4_transrangers(benchmark::State& st)
{
  for(auto _:st){
    using namespace transrangers;
      
    int res=0;
    auto rgr=
      transform(sum,push_zip(concat(all(rng2),all(rng3)),all(rng1)));
    rgr([&](const auto& p){res+=*p;return true;});
    volatile auto res2=res;
  }
}
BENCHMARK(test4_transrangers)->Name("transrangers push_zip(concat(2,3),1)");

static void test5_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    volatile auto res=ranges::accumulate(
      zip(rng1,concat(rng2,rng3))|transform(sum),0);
  }
}
BENCHMARK(test5_rangev3)->Name("Range-v3 zip(1,concat(2,3))");

static void test6_rangev3(benchmark::State& st)
{
  for(auto _:st){
    using namespace ranges::views;

    volatile auto res=ranges::accumulate(
      zip(concat(rng2,rng3),rng1)|transform(sum),0);
  }
}
BENCHMARK(test6_rangev3)->Name("Range-v3 zip(concat(2,3),1)");

BENCHMARK_MAIN();
