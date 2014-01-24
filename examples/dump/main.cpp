// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "main.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/CmdLine.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_entry.hpp>

#include "bound_imports.hpp"
#include "exports.hpp"
#include "filesystem.hpp"
#include "headers.hpp"
#include "imports.hpp"
#include "print.hpp"
#include "relocations.hpp"
#include "sections.hpp"
#include "tls.hpp"
#include "warning.hpp"

// TODO: Add functionality to make this tool more useful for reversing, such as
// basic heuristics to detect suspicious files, packer/container detection,
// diassembling the EP, compiler detection, dumping more of the file format
// (requires PeLib work), .NET/VB6/etc detection, hashing, etc.

// TODO: Add new warnings for strange cases which require more investigation.
// This includes both new cases, and also existing cases which are currently
// being ignored (including those ignored inside PeLib itself, like a lot of the
// corner cases in RvaToVa). Examples include a virtual or null EP, invalid
// number of data dirs, unknown DOS stub, strange RVAs which lie outside of the
// image or inside the headers, non-standard alignments, no sections, more than
// 96 sections, etc. Add some extra consistentcy checking to help detect strange
// PE files.

// TODO: Investigate places where we have a try/catch because it's probably a
// hack rather then the 'correct' solution. Fix or document all cases.

// TODO: Improve this tool against edge cases like those in the Corkami PE
// corpus. First step is to check each trick and document it if it's already
// handled, or add a note if it's not yet handled. Also add documentation for
// tricks from the ReversingLabs "Undocumented PECOFF" whitepaper.

// TODO: Multi-threading support.

// TODO: Add helper functions such as "HasExportDir" etc. to avoid the
// unnecessary memory allocations and exception handling.

// TODO: Dump string representation of data where possible, such as bitmasks
// (Charateristics etc.), data dir names, etc.

// TODO: Warn for unusual time stamps (very old, in the future, etc.).

// TODO: Check and use the value of the data directory sizes? (e.g. To limit
// IAT/EAT enumeration where it's available... Or does the Windows loader ignore
// it even in this case?)

// TODO: Add a new 'hostile' warning type for things that are not just
// suspicious, but are actively hostile and never found in 'legitimate' modules,
// like the AOI trick?

// TODO: Clean up this tool. It's a disaster of spaghetti code (spaghetti is
// delicious, but we should clean this up anyway...).

// TODO: Add support for warning when a file takes longer than N
// minutes/sections to analyze, with an optional forced timeout.

// TODO: Use backup semantics flags and try to get backup privilege in order to
// make directory enumeration find more files.

// TODO: XML output mode (easier to port to a GUI in future).

// TODO: GUI wrapper.

// TODO: Fix all cases both in Dump and in PeLib where we're potentially reading
// outside of the file/image.

// TODO: Move as much corner-case logic as possible into PeLib itself.

// TODO: Move all the "warnings" etc to PeLib itself (probably store inside
// PeFile class? or make it specific to each part of the file e.g. exports,
// imports, etc.). Need to think of the best way to handle it. Perhaps a set of
// flags per part of the file?

// TODO: Refactor out the warning code to operate separately from the dumping
// code where possible.

// TODO: Add warnings for cases like that are currently being detected and
// swallowed entirely in PeLib.

namespace
{

// TODO: Clean up this hack.
std::wstring g_current_file_path;

void DumpRegions(hadesmem::Process const& process)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Regions:", 0);

  hadesmem::RegionList const regions(process);
  for (auto const& region : regions)
  {
    WriteNewline(out);
    WriteNamedHex(
      out, L"Base", reinterpret_cast<std::uintptr_t>(region.GetBase()), 1);
    WriteNamedHex(out,
                  L"Allocation Base",
                  reinterpret_cast<std::uintptr_t>(region.GetAllocBase()),
                  1);
    WriteNamedHex(out, L"Allocation Protect", region.GetAllocProtect(), 1);
    WriteNamedHex(out, L"Size", region.GetSize(), 1);
    WriteNamedHex(out, L"State", region.GetState(), 1);
    WriteNamedHex(out, L"Protect", region.GetProtect(), 1);
    WriteNamedHex(out, L"Type", region.GetType(), 1);
  }
}

void DumpModules(hadesmem::Process const& process)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Modules:", 0);

  hadesmem::ModuleList const modules(process);
  for (auto const& module : modules)
  {
    WriteNewline(out);
    WriteNamedHex(
      out, L"Handle", reinterpret_cast<std::uintptr_t>(module.GetHandle()), 1);
    WriteNamedHex(out, L"Size", module.GetSize(), 1);
    WriteNamedNormal(out, L"Name", module.GetName(), 1);
    WriteNamedNormal(out, L"Path", module.GetPath(), 1);

    hadesmem::PeFile const pe_file(
      process, module.GetHandle(), hadesmem::PeFileType::Image, 0);

    try
    {
      hadesmem::DosHeader const dos_header(process, pe_file);
      hadesmem::NtHeaders const nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Not a valid PE file or architecture.", 1);
      continue;
    }

    DumpPeFile(process, pe_file, module.GetPath());
  }
}

void DumpThreadEntry(hadesmem::ThreadEntry const& thread_entry)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNamedHex(out, L"Usage", thread_entry.GetUsage(), 1);
  WriteNamedHex(out, L"ID", thread_entry.GetId(), 1);
  WriteNamedHex(out, L"Owner ID", thread_entry.GetOwnerId(), 1);
  WriteNamedHex(out, L"Base Priority", thread_entry.GetBasePriority(), 1);
  WriteNamedHex(out, L"Delta Priority", thread_entry.GetDeltaPriority(), 1);
  WriteNamedHex(out, L"Flags", thread_entry.GetFlags(), 1);
}

void DumpThreads(DWORD pid)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Threads:", 0);

  hadesmem::ThreadList threads(pid);
  for (auto const& thread_entry : threads)
  {
    DumpThreadEntry(thread_entry);
  }
}

void DumpProcessEntry(hadesmem::ProcessEntry const& process_entry)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNamedHex(out, L"ID", process_entry.GetId(), 0);
  WriteNamedHex(out, L"Threads", process_entry.GetThreads(), 0);
  WriteNamedHex(out, L"Parent", process_entry.GetParentId(), 0);
  WriteNamedHex(out, L"Priority", process_entry.GetPriority(), 0);
  WriteNamedNormal(out, L"Normal", process_entry.GetName(), 0);

  DumpThreads(process_entry.GetId());

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process = std::make_unique<hadesmem::Process>(process_entry.GetId());
  }
  catch (std::exception const& /*e*/)
  {
    WriteNewline(out);
    WriteNormal(out, L"Could not open process for further inspection.", 0);
    WriteNewline(out);
    return;
  }

  // Using the Win32 API to get a processes path can fail for 'zombie'
  // processes. (QueryFullProcessImageName fails with ERROR_GEN_FAILURE.)
  // TODO: Remove this once GetPath is fixed.
  try
  {
    WriteNewline(out);
    WriteNormal(out, L"Path (Win32): " + hadesmem::GetPath(*process), 0);
  }
  catch (std::exception const& /*e*/)
  {
    WriteNewline(out);
    WriteNormal(out, L"WARNING! Could not get Win32 path to process.", 0);
  }
  WriteNormal(out, L"Path (NT): " + hadesmem::GetPathNative(*process), 0);
  WriteNormal(out,
              L"WoW64: " +
                std::wstring(hadesmem::IsWoW64(*process) ? L"Yes" : L"No"),
              0);

  DumpModules(*process);

  DumpRegions(*process);
}

void DumpProcesses()
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Processes:", 0);

  hadesmem::ProcessList const processes;
  for (auto const& process_entry : processes)
  {
    DumpProcessEntry(process_entry);
  }
}
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path)
{
  std::wostream& out = std::wcout;

  ClearWarnForCurrentFile();

  std::uint32_t const k1MB = (1U << 20);
  std::uint32_t const k100MB = k1MB * 100;
  if (pe_file.GetSize() > k100MB)
  {
    // Not actually unsupported, just want to flag large files.
    WriteNormal(out, L"WARNING! File is over 100MB.", 0);
    WarnForCurrentFile(WarningType::kUnsupported);
  }

  DumpHeaders(process, pe_file);

  DumpSections(process, pe_file);

  DumpTls(process, pe_file);

  DumpExports(process, pe_file);

  // TODO: Fix the app/library so this is no longer necessary... Should the
  // bound import dumper simply perform an extra validation pass on the import
  // dir? What about perf? Needs more investigation.
  bool has_new_bound_imports_any = false;
  DumpImports(process, pe_file, has_new_bound_imports_any);

  DumpBoundImports(process, pe_file, has_new_bound_imports_any);

  DumpRelocations(process, pe_file);

  HandleWarnings(path);
}

void SetCurrentFilePath(std::wstring const& path)
{
  g_current_file_path = path;
}

// TODO: Fix perf for extremely long names. Instead of reading
// indefinitely and then checking the size after the fact, we should
// perform a bounded read.
void HandleLongOrUnprintableString(std::wstring const& name,
                                   std::wstring const& description,
                                   std::size_t tabs,
                                   WarningType warning_type,
                                   std::string value)
{
  std::wostream& out = std::wcout;

  auto const unprintable = FindFirstUnprintableClassicLocale(value);
  std::size_t const kMaxNameLength = 1024;
  if (unprintable != std::string::npos)
  {
    WriteNormal(out,
                L"WARNING! Detected unprintable " + description +
                  L". Truncating.",
                tabs);
    WarnForCurrentFile(warning_type);
    value.erase(unprintable);
  }
  else if (value.size() > kMaxNameLength)
  {
    WriteNormal(out,
                L"WARNING! Detected suspiciously long " + description +
                  L". Truncating.",
                tabs);
    WarnForCurrentFile(warning_type);
    value.erase(kMaxNameLength);
  }
  WriteNamedNormal(out, name, value.c_str(), tabs);
}

std::string::size_type FindFirstUnprintableClassicLocale(std::string const& s)
{
  auto const i =
    std::find_if(std::begin(s),
                 std::end(s),
                 [](char c)
                 { return !std::isprint(c, std::locale::classic()); });
  return i == std::end(s) ? std::string::npos : std::distance(std::begin(s), i);
}

bool ConvertTimeStamp(std::time_t time, std::wstring& str)
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

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Dumper [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd("PE file format dumper", ' ', HADESMEM_VERSION_STRING);
    TCLAP::ValueArg<DWORD> pid_arg(
      "p", "pid", "Target process id", false, 0, "DWORD");
    TCLAP::ValueArg<std::string> file_arg(
      "f", "file", "Target file", false, "", "string");
    TCLAP::ValueArg<std::string> dir_arg(
      "d", "dir", "Target directory", false, "", "string");
    TCLAP::SwitchArg all_arg("a", "all", "No target, dump everything");
    std::vector<TCLAP::Arg*> xor_args{&pid_arg, &file_arg, &dir_arg, &all_arg};
    cmd.xorAdd(xor_args);
    TCLAP::SwitchArg warned_arg(
      "w", "warned", "Dump list of files which cause warnings", cmd);
    TCLAP::ValueArg<std::string> warned_file_arg(
      "x",
      "warned-file",
      "Dump warned list to file instead of stdout",
      false,
      "",
      "string",
      cmd);
    TCLAP::SwitchArg warned_file_dynamic_arg(
      "y",
      "warned-file-dynamic",
      "Dump warnings to file on the fly rather than at the end",
      cmd);
    TCLAP::ValueArg<int> warned_type_arg("t",
                                         "warned-type",
                                         "Filter warned file using warned type",
                                         false,
                                         -1,
                                         "int",
                                         cmd);
    cmd.parse(argc, argv);

    SetWarningsEnabled(warned_arg.getValue());
    SetDynamicWarningsEnabled(warned_file_dynamic_arg.getValue());
    if (warned_file_arg.isSet())
    {
      SetWarnedFilePath(
        hadesmem::detail::MultiByteToWideChar(warned_file_arg.getValue()));
    }

    if (GetDynamicWarningsEnabled() && GetWarnedFilePath().empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error()
        << hadesmem::ErrorString(
             "Please specify a file path for dynamic warnings."));
    }

    int const warned_type = warned_type_arg.getValue();
    switch (warned_type)
    {
    case static_cast<int>(WarningType::kSuspicious) :
      SetWarnedType(WarningType::kSuspicious);
      break;
    case static_cast<int>(WarningType::kUnsupported) :
      SetWarnedType(WarningType::kUnsupported);
      break;
    case static_cast<int>(WarningType::kAll) :
      SetWarnedType(WarningType::kAll);
      break;
    default:
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString("Unknown warned type."));
      break;
    }

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    if (pid_arg.isSet())
    {
      DWORD const pid = pid_arg.getValue();

      hadesmem::ProcessList const processes;
      auto iter =
        std::find_if(std::begin(processes),
                     std::end(processes),
                     [pid](hadesmem::ProcessEntry const& process_entry)
                     { return process_entry.GetId() == pid; });
      if (iter != std::end(processes))
      {
        DumpProcessEntry(*iter);
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error()
          << hadesmem::ErrorString("Failed to find requested process."));
      }
    }
    else if (file_arg.isSet())
    {
      DumpFile(hadesmem::detail::MultiByteToWideChar(file_arg.getValue()));
    }
    else if (dir_arg.isSet())
    {
      DumpDir(hadesmem::detail::MultiByteToWideChar(dir_arg.getValue()));
    }
    else
    {
      DumpThreads(static_cast<DWORD>(-1));

      DumpProcesses();

      std::wcout << "\nFiles:\n";

      std::wstring const self_path = hadesmem::detail::GetSelfPath();
      std::wstring const root_path = hadesmem::detail::GetRootPath(self_path);
      DumpDir(root_path);
    }

    if (GetWarningsEnabled())
    {
      if (!GetWarnedFilePath().empty() && !GetDynamicWarningsEnabled())
      {
        std::unique_ptr<std::wfstream> warned_file_ptr(
          hadesmem::detail::OpenFileWide(GetWarnedFilePath(), std::ios::out));
        std::wfstream& warned_file = *warned_file_ptr;
        if (!warned_file)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            hadesmem::Error()
            << hadesmem::ErrorString("Failed to open warned file for output."));
        }

        DumpWarned(warned_file);
      }
      else
      {
        DumpWarned(std::wcout);
      }
    }

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n"
              << boost::current_exception_diagnostic_information() << '\n';

    // TODO: Clean up this hack.
    if (!g_current_file_path.empty())
    {
      std::wcerr << "\nCurrent file: " << g_current_file_path << "\n";
    }

    return 1;
  }
}
