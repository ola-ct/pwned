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

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "hash.hpp"
#include "util.hpp"

namespace pwned
{

Hash::Hash(const Hash &o)
    : quad(o.quad), isValid(o.isValid)
{
}

Hash::Hash(uint64_t upper, uint64_t lower)
    : quad({upper, lower}), isValid(true)
{
}

Hash::Hash(const std::string &pwd)
{
  MD5((const unsigned char *)pwd.c_str(), pwd.size(), data);
  toHostByteOrder();
  isValid = true;
}

std::string Hash::toString(bool uppercase) const
{
  std::ostringstream ss;
  if (uppercase)
  {
    ss << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << quad.upper << std::setw(16) << std::setfill('0') << std::uppercase << quad.lower;
  }
  else
  {
    ss << std::setw(16) << std::setfill('0') << std::hex << quad.upper << std::setw(16) << std::setfill('0') << quad.lower;
  }
  return ss.str();
}

Hash Hash::fromHex(const std::string &seq)
{
  Hash hash;
  if (seq.size() == 2 * Hash::size)
  {
    size_t j = 0;
    for (size_t i = 0; i < seq.size(); i += 2)
    {
      const int hi = decodeHex(seq.at(i));
      const int lo = decodeHex(seq.at(i + 1));
      if (hi >= 0 && lo >= 0)
      {
        const int b = (hi << 4) + lo;
        hash.data[j] = (uint8_t)b;
        ++j;
      }
      else
      {
        break;
      }
    }
    hash.isValid = j == Hash::size;
  }
  hash.toHostByteOrder();
  return hash;
}

std::ostream &operator<<(std::ostream &os, Hash const &h)
{
  return os << h.toString();
}
} // namespace pwned
