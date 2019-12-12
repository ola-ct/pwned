/*
 Copyright © 2019 Oliver Lau <ola@ct.de>, Heise Medien GmbH & Co. KG - Redaktion c't

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __markovchain_hpp__
#define __markovchain_hpp__

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include "markovnode.hpp"

namespace pwned {

namespace markov {

template <typename T>
static inline T read(std::istream &is)
{
  T data;
  is.read(reinterpret_cast<char*>(&data), sizeof(data));
  return data;
}

template <typename T>
static inline void write(std::ostream &os, T data)
{
  os.write(reinterpret_cast<const char*>(&data), sizeof(data));
}  

template <typename SymbolType = wchar_t, typename ProbabilityType = double, typename CountType = uint64_t>
class Chain
{
public:
  using symbol_type = SymbolType;
  using prob_value_type = ProbabilityType;
  using count_type = CountType;
  using node_type = Node<symbol_type, prob_value_type>;
  using map_type = std::unordered_map<symbol_type, node_type>;
  using pair_type = std::pair<symbol_type, prob_value_type>;

  Chain() = default;
  void update()
  {
    using elem_type = typename decltype(mFirstSymbolCounts)::value_type;
    const count_type sum = std::accumulate(std::cbegin(mFirstSymbolCounts), std::cend(mFirstSymbolCounts), 0ULL,
                                           [](count_type a, const elem_type &b) {
                                             return b.second + a;
                                           });
    for (const auto &p : mFirstSymbolCounts)
    {
      mFirstSymbolProbs[p.first] = (double)p.second / (double)sum;
    }
    mFirstSymbolSortedProbs.clear();
    mFirstSymbolSortedProbs.reserve(mFirstSymbolProbs.size());
    std::copy(std::cbegin(mFirstSymbolProbs), std::cend(mFirstSymbolProbs), std::begin(mFirstSymbolSortedProbs));
    using prob_type = std::pair<Chain::symbol_type, double>;
    struct {
      bool operator()(const prob_type &a, const prob_type &b) const
      {
        return a.second < b.second;
      }
    } probLess;
    std::sort(std::begin(mFirstSymbolSortedProbs), std::end(mFirstSymbolSortedProbs), probLess);
    for (auto &node : mNodes)
    {
      node.second.update();
    }
  }
  void clear()
  {
    mFirstSymbolProbs.clear();
    mFirstSymbolSortedProbs.clear();
  }
  void addFirst(symbol_type symbol)
  {
    if (mFirstSymbolCounts.find(symbol) == mFirstSymbolCounts.end())
    {
      mFirstSymbolCounts.emplace(symbol, 0);
    }
    ++mFirstSymbolCounts[symbol];
  }
  void addPair(symbol_type current, symbol_type successor)
  {
    if (mNodes.find(current) == mNodes.end())
    {
      mNodes.emplace(current, node_type());
    }
    mNodes[current].increment(successor);
  }
  const map_type &nodes() const
  {
    return mNodes;
  }
  void writeBinary(std::ostream &os)
  {
    if (mNodes.empty())
      return;
    os.write(FileHeader, 4);
    write(os, (uint32_t)mFirstSymbolSortedProbs.size());
    for (const auto &prob : mFirstSymbolSortedProbs)
    {
      write(os, prob.first);
      write(os, prob.second);
    }
    write(os, (uint32_t)mNodes.size());
    for (const auto &node : mNodes)
    {
      write(os, node.first);
      write(os, (uint32_t)node.second.successors().size());
      for (const auto &successor : node.second.successors())
      {
        write(os, successor.first);
        write(os, successor.second);
      }
    }
  }
  bool readBinary(std::istream &is, bool doClear = true)
  {
    if (doClear)
    {
      mNodes.clear();
      mFirstSymbolCounts.clear();
      mFirstSymbolProbs.clear();
      mFirstSymbolSortedProbs.clear();
    }
    while (!is.eof())
    {
      char hdr[4] = {0, 0, 0, 0};
      is.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
      if (is.eof())
        return false;
      if (memcmp(hdr, FileHeader, sizeof(FileHeader)) != 0)
        return false;
      const uint32_t firstSymbolCount = read<uint32_t>(is);
      if (is.eof())
        return false;
      mFirstSymbolSortedProbs.reserve(firstSymbolCount);
      for (auto i = 0; i < firstSymbolCount; ++i)
      {
        const symbol_type c = read<symbol_type>(is);
        if (is.eof())
          return false;
        const prob_value_type p = read<prob_value_type>(is);
        if (is.eof())
          return false;
        mFirstSymbolSortedProbs.emplace_back(c, p);
      }
      const uint32_t symbolCount = read<uint32_t>(is);
      if (is.eof())
        return false;
      for (auto i = 0; i < symbolCount; ++i)
      {
        symbol_type c = read<symbol_type>(is);
        if (is.eof())
          return false;
        if (mNodes.find(c) == mNodes.end())
        {
          mNodes.emplace(c, node_type());
        }
        if (is.eof())
          return false;
        const uint32_t nodeCount = read<uint32_t>(is);
        if (is.eof())
          return false;
        node_type &currentNode = mNodes[c];
        for (auto j = 0; j < nodeCount; ++j)
        {
          const symbol_type symbol = read<symbol_type>(is);
          if (is.eof())
            return false;
          const prob_value_type probability = read<prob_value_type>(is);
          if (is.eof())
            return false;
          currentNode.addSuccessor(symbol, probability);
        }
      }
    }
    return true;
  }
  const std::vector<pair_type>& firstSymbolProbs() const
  {
    return mFirstSymbolSortedProbs;
  }
  symbol_type randomFirstSymbol() const
  {
    const prob_value_type p = pwned::random();
    prob_value_type pAccumulated = 0.0;
    for (const auto &symbol : mFirstSymbolSortedProbs)
    {
      pAccumulated += symbol.second;
      if (p > pAccumulated)
        return symbol.first;
    }
    return 0;
  }

private:
  map_type mNodes;
  std::unordered_map<symbol_type, count_type> mFirstSymbolCounts;
  std::unordered_map<symbol_type, prob_value_type> mFirstSymbolProbs;
  std::vector<pair_type> mFirstSymbolSortedProbs;
  const char FileHeader[4]{'M', 'R', 'K', 'V'};
};

} // namespace markov

} // namespace pwned

#endif // __markovnode_hpp__