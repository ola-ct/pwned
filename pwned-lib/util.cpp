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

#include "util.hpp"

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

#include <string>
#include <memory>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <cmath>
#include <fstream>
#include <iostream>

#ifndef NO_POPCNT
#include <popcntintrin.h>
#endif

#if defined(__linux__)
#include <map>
#include <regex>
#include <algorithm>
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace pwned
{

// see https://gist.github.com/gunkmogo/5d54f9fb4579768d9c7d5c41293cc784
int getMemoryStat(MemoryStat &memoryStat)
{
#if defined(__APPLE__)
  xsw_usage vmusage = {0, 0, 0, 0, 0};
  size_t size = sizeof(vmusage);
  if (sysctlbyname("vm.swapusage", &vmusage, &size, NULL, 0) != 0)
  {
    return -1;
  }
  memoryStat.virt.total = vmusage.xsu_total;
  memoryStat.virt.avail = vmusage.xsu_avail;
  memoryStat.virt.used = vmusage.xsu_used;

  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  size_t length = sizeof(memoryStat.phys.total);
  sysctl(mib, 2, &memoryStat.phys.total, &length, NULL, 0);

  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
  vm_statistics_data_t vmstat;
  if (KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count))
  {
    return -2;
  }
  memoryStat.phys.avail = vmstat.free_count * vmusage.xsu_pagesize;
  memoryStat.phys.used = (vmstat.active_count + vmstat.inactive_count + vmstat.wire_count) * vmusage.xsu_pagesize;

  // Get App Memory Stats
  struct task_basic_info t_info;
  mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

  if (KERN_SUCCESS != task_info(mach_task_self(),
                                TASK_BASIC_INFO, (task_info_t)&t_info,
                                &t_info_count))
  {
    return -3;
  }
  memoryStat.virt.app = t_info.virtual_size;
  memoryStat.phys.app = t_info.resident_size;
#elif defined(__linux__)
  std::map<std::string, uint64_t*> memTable{
    {"MemTotal", &memoryStat.phys.total},
    {"MemAvailable", &memoryStat.phys.avail},
    {"VmallocTotal", &memoryStat.virt.total},
    {"VmallocUsed", &memoryStat.virt.used}
  };
  std::ifstream meminfo("/proc/meminfo");
  while (!meminfo.eof())
  {
    std::string line;
    std::getline(meminfo, line);
    for (const auto &m : memTable)
    {
      if (line.size() >= m.first.size() && std::equal(m.first.begin(), m.first.end(), line.begin()))
      {
        std::smatch fields;
        const std::regex reFields("\\w+:\\s+(\\d+)\\s+(\\w+)");
        if (std::regex_match(line, fields, reFields))
        {
          if (fields[2] == "GB")
          {
            *m.second = std::stoull(fields[1]) * 1024ULL * 1024ULL * 1024ULL;
          }
          else if (fields[2] == "MB")
          {
            *m.second = std::stoull(fields[1]) * 1024ULL * 1024ULL;
          }
          else if (fields[2] == "kB")
          {
            *m.second = std::stoull(fields[1]) * 1024ULL;
          }
          else if (fields[2] == "B")
          {
            *m.second = std::stoull(fields[1]);
          }
        }
      }
    }
  }
  memoryStat.phys.used = memoryStat.phys.total - memoryStat.phys.avail;
  memoryStat.virt.avail = memoryStat.virt.total - memoryStat.virt.used;
#elif defined(WIN32)
  #error Windows currently unsupported
#else
  #error Unsupported platform
#endif
  return 0;
}

// see https://stackoverflow.com/a/8098080
std::string string_format(const std::string fmt_str, ...)
{
  int n = (int)(2 * fmt_str.size());
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (true)
  {
    formatted.reset(new char[size_t(n)]);
    strcpy(&formatted[0], fmt_str.c_str());
    va_start(ap, fmt_str);
    int final_n = vsnprintf(&formatted[0], (size_t)n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
    {
      n += abs(final_n - n + 1);
    }
    else
    {
      break;
    }
  }
  return std::string(formatted.get());
}

std::string readableSize(uint64_t size)
{
  double sz = (double)size;
  if (sz > 1024LL * 1024LL * 1024LL * 1024LL)
  {
    return string_format("%.1f TB", sz / 1024 / 1024 / 1024 / 1024);
  }
  else if (size > 1024 * 1024 * 1024)
  {
    return string_format("%.1f GB", sz / 1024 / 1024 / 1024);
  }
  else if (size > 1024 * 1024)
  {
    return string_format("%.1f MB", sz / 1024 / 1024);
  }
  else if (size > 1024)
  {
    return string_format("%.1f KB", sz / 1024);
  }
  return string_format("%ld B", size);
}

std::string readableTime(double t)
{
  double x = t;
  const int days = int(x / 60 / 60 / 24);
  x -= double(days * 60 * 60 * 24);
  const int hours = int(x / 60 / 60);
  x -= double(hours * 60 * 60);
  const int mins = int(x / 60);
  x -= double(mins * 60);
  const double secs = x;
  const int isecs = int(round(secs));
  if (t > 60 * 60 * 24)
  {
    return string_format("%dd %dh %dm %ds", days, hours, mins, isecs);
  }
  if (t > 60 * 60)
  {
    return string_format("%dh %dm %ds", hours, mins, isecs);
  }
  if (t > 60)
  {
    return string_format("%dm %ds", mins, isecs);
  }
  return string_format("%.4fs", secs);
}

int decodeHex(const char c)
{
  if ('0' <= c && c <= '9')
  {
    return c - '0';
  }
  if ('a' <= c && c <= 'f')
  {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'F')
  {
    return c - 'A' + 10;
  }
  return -1;
}

bool hexToCharSeq(const std::string &seq, std::string &result)
{
  result.clear();
  if (seq.size() % 2 == 0)
  {
    for (size_t i = 0; i < seq.size(); i += 2)
    {
      const int hi = decodeHex(seq.at(i));
      const int lo = decodeHex(seq.at(i + 1));
      if (lo >= 0 && hi >= 0)
      {
        const int b = hi * 16 + lo;
        result.push_back(char(b));
      }
      else
      {
        return false;
      }
    }
  }
  return true;
}

unsigned int popcnt64(uint64_t x)
{
#ifndef NO_POPCNT
  return (unsigned int)_mm_popcnt_u64(x);
#else
  // see https://en.wikipedia.org/wiki/Hamming_weight
  const uint64_t m1  = 0x5555555555555555;
  const uint64_t m2  = 0x3333333333333333;
  const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
  const uint64_t h01 = 0x0101010101010101;
  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2); 
  x = (x + (x >> 4)) & m4; 
  return (unsigned int)((x * h01) >> 56);
#endif
}

TermIO::TermIO()
{
  tcgetattr(STDIN_FILENO, &old_t);
}

TermIO::~TermIO()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
}

void TermIO::disableEcho()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag &= tcflag_t(~ICANON);
  t.c_lflag &= tcflag_t(~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void TermIO::enableEcho()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag |= tcflag_t(ICANON);
  t.c_lflag |= tcflag_t(~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void TermIO::disableBreak()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag &= tcflag_t(~ISIG);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void TermIO::enableBreak()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag |= tcflag_t(ISIG);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}



void setStdinEcho(bool enable)
{
#ifdef WIN32
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(hStdin, &mode);
  if (!enable)
  {
    mode &= ~ENABLE_ECHO_INPUT;
  }
  else
  {
    mode |= ENABLE_ECHO_INPUT;
  }
  SetConsoleMode(hStdin, mode);
#else
  struct termios tty;
  tcgetattr(STDIN_FILENO, &tty);
  if (!enable)
  {
    tty.c_lflag &= tcflag_t(~ECHO);
  }
  else
  {
    tty.c_lflag |= tcflag_t(ECHO);
  }
  (void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}



} // namespace pwned
