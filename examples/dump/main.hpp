// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <locale>
#include <ostream>
#include <string>
#include <vector>

#include <hadesmem/detail/str_conv.hpp>

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path);

enum class WarningType : int
{
  kSuspicious,
  kUnsupported,
  kAll = -1
};

void SetCurrentFilePath(std::wstring const& path);

void WarnForCurrentFile(WarningType warned_type);

void ClearWarnForCurrentFile();

// TODO: Clean up this header. It contains a lot of random code that doesn't
// belong here.

template <typename CharT> class StreamFlagSaver
{
public:
  explicit StreamFlagSaver(std::basic_ios<CharT>& str)
    : str_(&str), flags_(str.flags()), width_(str.width()), fill_(str.fill())
  {
  }

  ~StreamFlagSaver()
  {
    str_->flags(flags_);
    str_->width(width_);
    str_->fill(fill_);
  }

  StreamFlagSaver(StreamFlagSaver const&) = delete;
  StreamFlagSaver& operator=(StreamFlagSaver const&) = delete;

private:
  std::basic_ios<CharT>* str_;
  std::ios_base::fmtflags flags_;
  std::streamsize width_;
  CharT fill_;
};

template <typename T>
inline void WriteNamedHex(std::wostream& out,
                          std::wstring const& name,
                          T const& num,
                          std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": 0x" << std::hex
      << std::setw(sizeof(num) * 2) << std::setfill(L'0') << num << '\n';
}

template <typename T>
inline void WriteNamedHexSuffix(std::wostream& out,
                                std::wstring const& name,
                                T const& num,
                                std::wstring const& suffix,
                                std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": 0x" << std::hex
      << std::setw(sizeof(num) * 2) << std::setfill(L'0') << num << L" ("
      << suffix << L")" << '\n';
}

template <typename C>
inline void WriteNamedHexContainer(std::wostream& out,
                                   std::wstring const& name,
                                   C const& c,
                                   std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << std::hex
      << std::setw(sizeof(typename C::value_type) * 2) << std::setfill(L'0');
  for (auto const& e : c)
  {
    out << " 0x" << e;
  }
  out << '\n';
}

template <typename T>
inline void WriteNamedNormal(std::wostream& out,
                             std::wstring const& name,
                             T const& t,
                             std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << t << '\n';
}

template <typename T>
inline void WriteNormal(std::wostream& out, T const& t, std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << t << '\n';
}

inline void WriteNewline(std::wostream& out)
{
  out << L'\n';
}

inline bool IsPrintableClassicLocale(std::string const& s)
{
  auto const i =
    std::find_if(std::begin(s),
                 std::end(s),
                 [](char c)
                 { return !std::isprint(c, std::locale::classic()); });
  return i == std::end(s);
}

inline bool ConvertTimeStamp(std::time_t time, std::wstring& str)
{
  // Using ctime rather than ctime_s because MinGW-w64 is apparently missing it.
  // WARNING! The ctime function is not thread safe.
  // TODO: Fix this.
  auto const conv = std::ctime(&time);
  if (conv)
  {
    // MSDN documents the ctime class of functions as returning a string that is
    // exactly 26 characters long, of the form "Wed Jan 02 02:03:55 1980\n\0".
    // Don't copy the newline or the extra null terminator.
    str = hadesmem::detail::MultiByteToWideChar(std::string(conv, conv + 24));
    return true;
  }
  else
  {
    str = L"Invalid";
    return false;
  }
}
