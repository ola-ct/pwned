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

#ifndef __progresscallback_hpp__
#define __progresscallback_hpp__

#include <cstdint>

class ProgressCallback
{
public:
  ProgressCallback() = default;
  virtual void operator()(uint64_t value) noexcept
  {
    update(value);
  }
  virtual void update(uint64_t) = 0;
};

#endif // __progresscallback_hpp__
