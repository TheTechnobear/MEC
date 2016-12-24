/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PICROSS_PIC_STL__
#define __PICROSS_PIC_STL__

#include "pic_fastalloc.h"

#include <map>
#include <list>
#include <set>
#include <vector>
#include <iostream>
#include <sstream>

namespace pic
{
    template <class K,class V, class C=std::less<K> > struct lckmap_t
    {
        typedef std::map<K,V,C,pic::stlnballocator_t<std::pair<const K,V> > > nbtype;
        typedef std::map<K,V,C,pic::stllckallocator_t<std::pair<const K,V> > > lcktype;
        typedef std::map<K,V,C> stdtype;
        typedef stdtype type;
    };

    template <class K,class V,class C=std::less<K> > struct lckmultimap_t
    {
        typedef std::multimap<K,V,C,pic::stlnballocator_t<std::pair<const K,V> > > nbtype;
        typedef std::multimap<K,V,C,pic::stllckallocator_t<std::pair<const K,V> > > lcktype;
        typedef std::multimap<K,V,C> stdtype;
        typedef stdtype type;
    };

    template <class V> struct lcklist_t
    {
        typedef std::list<V,pic::stlnballocator_t<V> > nbtype;
        typedef std::list<V,pic::stllckallocator_t<V> > lcktype;
        typedef std::list<V> stdtype;
        typedef stdtype type;
    };

    template <class V> struct lckvector_t
    {
        typedef std::vector<V,pic::stlnballocator_t<V> > nbtype;
        typedef std::vector<V,pic::stllckallocator_t<V> > lcktype;
        typedef std::vector<V> stdtype;
        typedef stdtype type;
    };

    template <class V,class C=std::less<V> > struct lckset_t
    {
        typedef std::set<V,C,pic::stlnballocator_t<V> > nbtype;
        typedef std::set<V,C,pic::stllckallocator_t<V> > lcktype;
        typedef std::set<V,C> stdtype;
        typedef stdtype type;
    };

    template <class V,class C=std::less<V> > struct lckmultiset_t
    {
        typedef std::multiset<V,C,pic::stlnballocator_t<V> > nbtype;
        typedef std::multiset<V,C,pic::stllckallocator_t<V> > lcktype;
        typedef std::multiset<V,C> stdtype;
        typedef stdtype type;
    };

    typedef std::basic_ostringstream<char,std::char_traits<char>,stlnballocator_t<char> > nbostringstream_t;
    typedef std::basic_istringstream<char,std::char_traits<char>,stlnballocator_t<char> > nbistringstream_t;
    typedef std::basic_string<char,std::char_traits<char>,stlnballocator_t<char> > nbstring_t;
};

#endif
