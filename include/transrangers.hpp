/* Transrangers: an efficient, composable design pattern for range processing.
 *
 * Copyright 2021 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://github.com/joaquintides/transrangers for project home page.
 */
 
#ifndef JOAQUINTIDES_TRANSRANGERS_HPP
#define JOAQUINTIDES_TRANSRANGERS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <iterator>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#if defined(__GNUC__)
#if defined(__clang__)
#define TRANSANGERS_MUTABLE_FLATTEN __attribute__((flatten)) mutable
#else
#define TRANSANGERS_MUTABLE_FLATTEN mutable __attribute__((flatten))
#endif
#else
#define TRANSANGERS_MUTABLE_FLATTEN
#endif

namespace transrangers{

template<typename Cursor,typename F> 
struct ranger_class:F
{
  using cursor=Cursor;
};
    
template<typename Cursor,typename F>
auto ranger(F f)
{
  return ranger_class<Cursor,F>{f};
}
    
template<typename Range>
auto all(Range&& rng)
{
  using std::begin;
  using std::end;
  using cursor=decltype(begin(rng));
  
  return ranger<cursor>(
    [first=begin(rng),last=end(rng)](auto dst)TRANSANGERS_MUTABLE_FLATTEN{
    auto it=first;
    while(it!=last)if(!dst(it++)){first=it;return false;}
    return true;
  });
}

template<typename Range>
struct all_copy
{
  using ranger=decltype(all(std::declval<Range&>()));
  using cursor=typename ranger::cursor;
  
  auto operator()(const auto& p){return rgr(p);}

  Range  rng;
  ranger rgr=all(rng);
};

template<typename Range>
auto all(Range&& rng) requires(std::is_rvalue_reference_v<Range&&>)
{
  return all_copy<Range>{std::move(rng)};
}

template<typename Pred>
auto pred_box(Pred pred)
{
  return [=](auto&&... x)->int{
    return pred(std::forward<decltype(x)>(x)...);
  };
}

template<typename Pred,typename Ranger>
auto filter(Pred pred_,Ranger rgr)
{
  using cursor=typename Ranger::cursor;
    
  return ranger<cursor>([=,pred=pred_box(pred_)](auto dst)mutable{
    return rgr([&](const auto& p){
      return pred(*p)?dst(p):true;
    });
  });
}

template<typename Cursor,typename F,typename=void>
struct deref_fun
{
  decltype(auto) operator*()const{return (*pf)(*p);} 
    
  Cursor p;
  F*     pf;
};

template<typename Cursor,typename F>
struct deref_fun<
  Cursor,F,
  std::enable_if_t<
    std::is_trivially_default_constructible_v<F>&&std::is_empty_v<F>
  >
>
{
  deref_fun(Cursor p={},F* =nullptr):p{p}{}

  decltype(auto) operator*()const{return F()(*p);} 
    
  Cursor p;
};
    
template<typename F,typename Ranger>
auto transform(F f,Ranger rgr)
{
  using cursor=deref_fun<typename Ranger::cursor,F>;
    
  return ranger<cursor>([=](auto dst)mutable{
    return rgr([&](const auto& p){return dst(cursor{p,&f});});
  });
}

template<typename Ranger>
auto take(int n,Ranger rgr)
{
  using cursor=typename Ranger::cursor;
    
  return ranger<cursor>([=](auto dst)mutable{
    if(n)return rgr([&](const auto& p){
      --n;
      return dst(p)&&(n!=0);
    })||(n==0);
    else return true;
  });
}

template<typename Ranger>
auto concat(Ranger rgr)
{
  return rgr;
}

template<typename Ranger,typename... Rangers>
auto concat(Ranger rgr,Rangers... rgrs)
{
  using cursor=typename Ranger::cursor;
    
  return ranger<cursor>(
    [=,cont=false,next=concat(rgrs...)](auto dst)mutable{
      if(!cont){
        if(!(cont=rgr(dst)))return false;
      }
      return next(dst);
    }
  );
}
    
template<typename Ranger>
auto unique(Ranger rgr)
{
  using cursor=typename Ranger::cursor;
    
  return ranger<cursor>([=,start=true,p=cursor{}](auto dst)mutable{
    if(start){
      start=false;
      bool cont=false;
      if(rgr([&](const auto& q){
        p=q;
        cont=dst(q);
        return false;
      }))return true;
      if(!cont)return false;
    }
    return rgr([&,prev=p](const auto& q)mutable{
      if((*prev==*q)||dst(q)){prev=q;return true;}
      else{p=q;return false;}
    });
  });
}

template<typename Ranger>
auto join(Ranger rgr)
{
  using cursor=typename Ranger::cursor;
  using subranger=std::remove_cvref_t<decltype(*std::declval<cursor>())>; 
  using subranger_cursor=typename subranger::cursor;
    
  return ranger<subranger_cursor>(
    [=,osrgr=std::optional<subranger>{}](auto dst)mutable{
      if(osrgr){
        if(!(*osrgr)(dst))return false;
      }
      return(rgr([&](const auto& p){
        auto srgr=*p;
        auto cont=srgr(dst);
        if(!cont)osrgr.emplace(std::move(srgr));
        return cont;
      }));
  });    
}

template<typename Ranger>
auto ranger_join(Ranger rgr)
{
  auto all_adaptor=[](auto&& srng){
    return all(std::forward<decltype(srng)>(srng));
  };

  return join(transform(all_adaptor,rgr));
}

template<typename... Rangers>
struct zip_cursor
{
  auto operator*()const
  {
    return std::apply([](const auto&... ps){
      return std::tuple<decltype(*ps)...>{*ps...};
    },ps);
  }
  
  std::tuple<typename Rangers::cursor...> ps;
};

template<typename Ranger,typename... Rangers>
auto zip(Ranger rgr,Rangers... rgrs)
{
  using cursor=zip_cursor<Ranger,Rangers...>;

  return ranger<cursor>(
    [=,zp=cursor{}](auto dst)mutable{
      bool finished=false;
      return rgr([&](const auto& p){
        std::get<0>(zp.ps)=p;
        if([&]<std::size_t... I>(std::index_sequence<I...>
#ifdef _MSC_VER
          ,auto&... rgrs
#endif
        ){
          return (rgrs([&](const auto& p){
            std::get<I+1>(zp.ps)=p;
            return false;
          })||...); 
        }(std::index_sequence_for<Rangers...>{}
#ifdef _MSC_VER
          ,rgrs...
#endif
        )){
          finished=true;
          return false;
        }
        
        return dst(zp);
      })||finished;
    }
  );
}

} /* transrangers */

#undef TRANSANGERS_MUTABLE_FLATTEN
#endif
