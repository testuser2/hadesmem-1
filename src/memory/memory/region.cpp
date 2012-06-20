// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/region.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"

namespace hadesmem
{

Region::Region(Process const* process, LPCVOID address)
  : process_(process), 
  mbi_(detail::Query(*process, address))
{ }

Region::Region(Process const* process, MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT
  : process_(process), 
  mbi_(mbi)
{ }

Region::Region(Region const& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  mbi_(other.mbi_)
{ }

Region& Region::operator=(Region const& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  mbi_ = other.mbi_;
  
  return *this;
}

Region::Region(Region&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  mbi_(other.mbi_)
{
  other.process_ = nullptr;
  ZeroMemory(&other.mbi_, sizeof(other.mbi_));
}

Region& Region::operator=(Region&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  mbi_ = other.mbi_;
  
  other.process_ = nullptr;
  ZeroMemory(&other.mbi_, sizeof(other.mbi_));
  
  return *this;
}

Region::~Region() BOOST_NOEXCEPT
{ }

PVOID Region::GetBase() const BOOST_NOEXCEPT
{
  return mbi_.BaseAddress;
}

PVOID Region::GetAllocBase() const BOOST_NOEXCEPT
{
  return mbi_.AllocationBase;
}

DWORD Region::GetAllocProtect() const BOOST_NOEXCEPT
{
  return mbi_.AllocationProtect;
}

SIZE_T Region::GetSize() const BOOST_NOEXCEPT
{
  return mbi_.RegionSize;
}

DWORD Region::GetState() const BOOST_NOEXCEPT
{
  return mbi_.State;
}

DWORD Region::GetProtect() const BOOST_NOEXCEPT
{
  return mbi_.Protect;
}

DWORD Region::GetType() const BOOST_NOEXCEPT
{
  return mbi_.Type;
}

bool Region::operator==(Region const& other) const BOOST_NOEXCEPT
{
  return this->GetBase() == other.GetBase();
}

bool Region::operator!=(Region const& other) const BOOST_NOEXCEPT
{
  return !(*this == other);
}

bool Region::operator<(Region const& other) const BOOST_NOEXCEPT
{
  return this->GetBase() < other.GetBase();
}

bool Region::operator<=(Region const& other) const BOOST_NOEXCEPT
{
  return this->GetBase() <= other.GetBase();
}

bool Region::operator>(Region const& other) const BOOST_NOEXCEPT
{
  return this->GetBase() > other.GetBase();
}

bool Region::operator>=(Region const& other) const BOOST_NOEXCEPT
{
  return this->GetBase() >= other.GetBase();
}

}
