/* Illustrational code for annex A: rangers are as expressive as range adaptors.
 *
 * Copyright 2021 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://github.com/joaquintides/transrangers for project home page.
 */

#include <cassert>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/unique.hpp>
#include <transrangers.hpp>
#include <transranger_view.hpp>
#include <type_traits>
#include <utility>
#include <vector>

template<typename RangeAdaptor,typename Range,typename=void>
struct is_compatible:std::false_type{};

template<typename RangeAdaptor,typename Range>
struct is_compatible<
  RangeAdaptor,Range,
  std::void_t<decltype(std::declval<RangeAdaptor>()(std::declval<Range&>()))>>:
  std::true_type{};

template<typename RangeAdaptor,typename Range>
constexpr auto is_compatible_v=is_compatible<RangeAdaptor,Range>::value;
                        
template<typename RangeAdaptor>
auto equivalent_transranger(RangeAdaptor ra)
{
  using namespace transrangers;
    
  return [=](auto rgr){
    if constexpr(is_compatible_v<RangeAdaptor,decltype(input_view(rgr))>){
      return all(ra(input_view(rgr)));
    }
    else{
      static_assert(is_compatible_v<RangeAdaptor,decltype(forward_view(rgr))>);
      return all(ra(forward_view(rgr)));
    }
  };
}

template<typename Range,typename RangeAdaptor,typename Transranger>
bool same_output(Range& rng,RangeAdaptor ra,Transranger tr)
{
  auto v=ra(rng);
  using view_value_type=std::remove_cvref_t<decltype(*std::begin(v))>;
  std::vector<view_value_type> res1;
  for(const auto& x:v)res1.push_back(x);
    
  auto rgr=tr(transrangers::all(rng));
  using ranger_value_type=std::remove_cvref_t<
    decltype(*std::declval<typename decltype(rgr)::cursor>())>;
  std::vector<ranger_value_type> res2;
  rgr([&](auto p){res2.push_back(*p);return true;});
      
  return res1==res2;
}

template<typename Range,typename RangeAdaptor>
void check_equivalent_transranger(Range& rng,RangeAdaptor ra)
{
  assert(same_output(rng,ra,equivalent_transranger(ra)));
}

int main()
{
  auto is_even=[](int x){return x%2==0;};
  auto x3=[](int x){return 3*x;};
  std::vector<int> rng={0,0,1,1,2,3,4,5,5,6,7,9};
    
  using namespace ranges::views;
    
  check_equivalent_transranger(rng,filter(is_even));
  check_equivalent_transranger(rng,transform(x3));
  check_equivalent_transranger(rng,unique);
    
  std::vector rng2={rng,rng,rng};

  check_equivalent_transranger(rng2,join|take(20)|transform(x3)|unique);
}