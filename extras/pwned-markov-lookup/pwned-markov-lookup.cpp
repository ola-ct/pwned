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

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <exception>
#include <boost/locale/encoding_utf.hpp>

#include <pwned-lib/markovnode.hpp>
#include <pwned-lib/markovchain.hpp>

namespace markov = pwned::markov;

using symbol_type = wchar_t;
using prob_value_type = double;
using chain_type = markov::Chain<symbol_type, prob_value_type>;

int main(int argc, char *argv[])
{
  if (argc < 2)
    return EXIT_FAILURE;
  const std::string &inputFilename = argv[1];
  std::ifstream inputFile(inputFilename, std::ios::binary);
  chain_type chain;
  chain.loadBinary(inputFile);
  while (true)
  {
    std::cout << "Type password: " << std::flush;
    std::string pwd;
    pwned::setStdinEcho(false);
    std::cin >> pwd;
    pwned::setStdinEcho(true);
    if (pwd.empty())
      break;
    const std::basic_string<symbol_type> &wPwd = boost::locale::conv::utf_to_utf<symbol_type>(pwd);
    std::cout << "p = " << chain.totalProbability(wPwd) << std::endl;
  }
  return EXIT_SUCCESS;
}
