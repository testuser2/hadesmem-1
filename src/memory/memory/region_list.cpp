// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/region_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/query_region.hpp>

namespace hadesmem
{

struct RegionIterator::Impl
{
  explicit Impl(Process const& process) HADESMEM_NOEXCEPT
    : process_(&process), 
    region_()
  {
    MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, nullptr);
    region_ = Region(process, mbi);
  }

  Process const* process_;
  boost::optional<Region> region_;
};

RegionIterator::RegionIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

RegionIterator::RegionIterator(Process const& process)
  : impl_(new Impl(process))
{ }

RegionIterator::RegionIterator(RegionIterator const& other) HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

RegionIterator& RegionIterator::operator=(RegionIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

RegionIterator::RegionIterator(RegionIterator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

RegionIterator& RegionIterator::operator=(RegionIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

RegionIterator::~RegionIterator()
{ }

RegionIterator::reference RegionIterator::operator*() const HADESMEM_NOEXCEPT
{
  HADESMEM_ASSERT(impl_.get());
  return *impl_->region_;
}

RegionIterator::pointer RegionIterator::operator->() const HADESMEM_NOEXCEPT
{
  HADESMEM_ASSERT(impl_.get());
  return &*impl_->region_;
}

RegionIterator& RegionIterator::operator++()
{
  try
  {
    HADESMEM_ASSERT(impl_.get());
    
    void const* const base = impl_->region_->GetBase();
    SIZE_T const size = impl_->region_->GetSize();
    auto const next = static_cast<char const* const>(base) + size;
    MEMORY_BASIC_INFORMATION const mbi = detail::Query(*impl_->process_, next);
    impl_->region_ = Region(*impl_->process_, mbi);
  }
  catch (std::exception const& /*e*/)
  {
    // TODO: Check whether this is the right thing to do. We should only 
    // flag as the 'end' once we've actually reached the end of the list. If 
    // the iteration fails we should throw an exception.
    impl_.reset();
  }
  
  return *this;
}

RegionIterator RegionIterator::operator++(int)
{
  RegionIterator iter(*this);
  ++*this;
  return iter;
}

bool RegionIterator::operator==(RegionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool RegionIterator::operator!=(RegionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

struct RegionList::Impl
{
  Impl(Process const& process)
    : process_(&process)
  { }

  Process const* process_;
};

RegionList::RegionList(Process const& process)
  : impl_(new Impl(process))
{ }

RegionList::RegionList(RegionList const& other)
  : impl_(new Impl(*other.impl_))
{ }

RegionList& RegionList::operator=(RegionList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

RegionList::RegionList(RegionList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

RegionList& RegionList::operator=(RegionList&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

RegionList::~RegionList()
{ }

RegionList::iterator RegionList::begin()
{
  return RegionList::iterator(*impl_->process_);
}

RegionList::const_iterator RegionList::begin() const
{
  return RegionList::iterator(*impl_->process_);
}

RegionList::iterator RegionList::end() HADESMEM_NOEXCEPT
{
  return RegionList::iterator();
}

RegionList::const_iterator RegionList::end() const HADESMEM_NOEXCEPT
{
  return RegionList::iterator();
}

}
