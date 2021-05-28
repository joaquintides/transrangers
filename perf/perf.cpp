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
#include <transrangers.hpp>
#include <vector>

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

int n=rng.size()+rng.size()/2;

static void test2_handwritten(benchmark::State& st)
{
  for (auto _:st){
    int res=0;
    int m=n;
    auto f=[&]{
      for(auto first=std::begin(rng),last=std::end(rng);
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
    auto rgr=transform(x3,filter(is_even,take(n,concat(all(rng),all(rng)))));
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
      concat(rng,rng)|take(n)|filter(is_even)|transform(x3),0);
  }
}
BENCHMARK(test2_rangev3);

BENCHMARK_MAIN();
