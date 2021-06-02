/* Copyright 2021 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See https://github.com/joaquintides/transrangers for project home page.
 */
 
#ifndef JOAQUINTIDES_TRANSRANGER_VIEW_HPP
#define JOAQUINTIDES_TRANSRANGER_VIEW_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/semiregular_box.hpp>
#include <type_traits>
#include <utility>

namespace transrangers{

/* Ranger -> Range-v3 view adaptors */

namespace detail::view{

struct sentinel{};

template<typename Iterator,typename Ranger>
class iterator_base
{
public:
  using value_type=
    std::remove_cvref_t<decltype(*std::declval<typename Ranger::cursor>())>;
  using difference_type=std::ptrdiff_t;
    
  iterator_base()=default;
  iterator_base(const Ranger& rgr):rgr{rgr}{} /* invalid till first ++ */ 
  iterator_base(const iterator_base&)=default;
  
  iterator_base& operator=(const iterator_base& x)
  {
    /* Direct assignment fails to compile sometimes (e.g. when
     * ranges::views::unique is part of the chain), no idea why.
     */
    this->rgr=x.rgr.get();
    
    this->end=x.end;
    this->p=x.p;
    return *this;
  }
  
  decltype(auto) operator*()const{return *p;}
  Iterator& operator++(){final().advance();return final();}
  Iterator operator++(int){auto x=final();final().advance();return x;}
    
  friend bool operator==(const iterator_base& x,const sentinel&)
    {return x.end;}
  friend bool operator!=(const iterator_base& x,const sentinel& y)
    {return !(x==y);}

protected:
  ranges::semiregular_box<Ranger> rgr;
  bool                            end;
  typename Ranger::cursor         p;

private:
  Iterator& final(){return static_cast<Iterator&>(*this);}
};

template<typename Ranger>
class input_iterator:
  public iterator_base<input_iterator<Ranger>,Ranger>
{
  using super=iterator_base<input_iterator,Ranger>;

public:
  using super::super;
    
private:
  friend super;

  void advance()
  {
    this->end=this->rgr([&](auto q){this->p=q;return false;}); 
  }
};

template<typename Ranger>
class forward_iterator:
  public iterator_base<forward_iterator<Ranger>,Ranger>
{
  using super=iterator_base<forward_iterator,Ranger>;

public:
  using super::super;
  
  friend bool operator==(const forward_iterator& x,const forward_iterator& y)
    {return x.n==y.n;}
  friend bool operator!=(const forward_iterator& x,const forward_iterator& y)
    {return !(x==y);}
  
private:
  friend super;

  void advance()
  {
    this->end=this->rgr([&](auto q){this->p=q;++n;return false;}); 
  }
  
  std::size_t n=0;
};

template<typename Ranger,typename Iterator>
class view:public ranges::view_base
{
public:
  view()=default;
  view(Ranger rgr):rgr{std::move(rgr)}{}
  
  auto begin(){return ++Iterator{rgr};} /* note ++ */
  auto end(){return sentinel{};}
  
private:
  ranges::semiregular_box<Ranger> rgr;
};

} /* detail::view */

template<typename Ranger>
auto input_view(Ranger rgr)
{
  using namespace detail::view;
  return view<Ranger,input_iterator<Ranger>>{std::move(rgr)};
}

template<typename Ranger>
auto forward_view(Ranger rgr)
{
  using namespace detail::view;
  return view<Ranger,forward_iterator<Ranger>>{std::move(rgr)};
}

} /* transrangers */

#endif