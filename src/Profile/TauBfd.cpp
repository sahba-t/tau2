#ifdef TAU_BFD

#if (defined(TAU_BGP) || defined(TAU_BGQ)) && defined(TAU_XLC)
// *CWL* - This is required to handle the different prototype for
//         asprintf and vasprintf between gnu and xlc compilers
//         on the BGP.
#define HAVE_DECL_VASPRINTF 1
#define HAVE_DECL_ASPRINTF 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>

#include <TAU.h>
#include <Profile/TauBfd.h>
#include <bfd.h>
#include <dirent.h>
#include <stdint.h>

#if defined(HAVE_GNU_DEMANGLE) && HAVE_GNU_DEMANGLE
#define HAVE_DECL_BASENAME 1
#include <demangle.h>
#define DEFAULT_DEMANGLE_FLAGS DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE | DMGL_TYPES
#ifdef __PGI
#define DEMANGLE_FLAGS DEFAULT_DEMANGLE_FLAGS | DMGL_ARM
#else
#define DEMANGLE_FLAGS DEFAULT_DEMANGLE_FLAGS
#endif // __PGI
#endif // HAVE_GNU_DEMANGLE

#if (defined(TAU_BGP) || defined(TAU_BGQ))
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <link.h>
#endif /* TAU_BGP || TAU_BGQ */

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#if defined(TAU_WINDOWS) && defined(TAU_MINGW)
#include <windows.h>
#include <psapi.h>
#endif

using namespace std;

static char const * Tau_bfd_internal_getExecutablePath();

struct TauBfdModule
{
  TauBfdModule() :
      bfdImage(NULL), syms(NULL), nr_all_syms(0), bfdOpen(false),
      lastResolveFailed(false), processCode(TAU_BFD_SYMTAB_NOT_LOADED)
  { }

  ~TauBfdModule() {
    free(syms);
    delete bfdImage;
  }

#ifdef TAU_INTEL12
  // Meant for consumption by the Intel12 workaround only.
  void markLastResult(bool success) {
    lastResolveFailed = !success;
  }
#endif

  bool loadSymbolTable(char const * path)
  {
#ifdef TAU_INTEL12
    // Nasty hack because Intel 12 is broken with Bfd 2.2x and
    //   requires a complete reset of BFD. The latter's internals
    //   becomes corrupted on a bad address from Intel 12 binaries.
    if (lastResolveFailed) {
      bfd_init();
      bfdOpen = false;
    }
#endif /* TAU_INTEL12 */

    // Executable symbol table is already loaded.
    if (bfdOpen) return true;

    Tau_bfd_initializeBfd();

    if (!(bfdImage = bfd_openr(path, 0))) {
      TAU_VERBOSE("loadSymbolTable: Failed to open [%s]\n", path);
      return (bfdOpen = false);
    }

    if (!bfd_check_format(bfdImage, bfd_object)) {
      TAU_VERBOSE("loadSymbolTable: bfd format check failed [%s]\n", path);
      return (bfdOpen = false);
    }

    if (!(bfd_get_file_flags(bfdImage) & HAS_SYMS)) {
      TAU_VERBOSE("loadSymbolTable: bfd has no symbols [%s]\n", path);
      return (bfdOpen = false);
    }

    size_t size = bfd_get_symtab_upper_bound(bfdImage);
    if (size < 1) {
      TAU_VERBOSE("loadSymbolTable: bfd_get_symtab_upper_bound() < 1 [%s]\n", path);
      return (bfdOpen = false);
    }

    syms = (asymbol **)malloc(size);
    nr_all_syms = bfd_canonicalize_symtab(bfdImage, syms);
    bfdOpen = nr_all_syms > 0;

    TAU_VERBOSE("loadSymbolTable: %s contains %d canonical symbols\n", path, nr_all_syms);

    return bfdOpen;
  }

  bfd *bfdImage;
  asymbol **syms;
  size_t nr_all_syms;

  // For EBS book-keeping
  bool bfdOpen;    // once open, symtabs are loaded and never released
  bool lastResolveFailed;

  // Remember the result of the last process to avoid reprocessing
  int processCode;
};

struct TauBfdUnit
{
  TauBfdUnit() : objopen_counter(-1) {
    executablePath = Tau_bfd_internal_getExecutablePath();
    executableModule = new TauBfdModule;
  }

  void ClearMaps() {
    for (size_t i = 0; i < addressMaps.size(); ++i) {
      delete addressMaps[i];
    }
    addressMaps.clear();
  }

  void ClearModules() {
    for (size_t i = 0; i < modules.size(); ++i) {
      delete modules[i];
    }
    modules.clear();
  }

  int objopen_counter;
  char const * executablePath;
  TauBfdModule * executableModule;
  vector<TauBfdAddrMap*> addressMaps;
  vector<TauBfdModule*> modules;

};

struct LocateAddressData
{
  LocateAddressData(TauBfdModule * _module, TauBfdInfo & _info) :
      found(false), module(_module), info(_info)
  { }

  bool found;
  TauBfdModule * module;
  TauBfdInfo & info;
};

// Internal function prototypes
static bool Tau_bfd_internal_loadSymTab(TauBfdUnit *unit, int moduleIndex);
static bool Tau_bfd_internal_loadExecSymTab(TauBfdUnit *unit);
static int Tau_bfd_internal_getModuleIndex(TauBfdUnit *unit, unsigned long probe_addr);
static TauBfdModule * Tau_bfd_internal_getModuleFromIdx(TauBfdUnit *unit, int moduleIndex);
static void Tau_bfd_internal_locateAddress(bfd *bfdptr, asection *section, void *data ATTRIBUTE_UNUSED);
static void Tau_bfd_internal_updateProcSelfMaps(TauBfdUnit *unit);

#if (defined(TAU_BGP) || defined(TAU_BGQ))
static void Tau_bfd_internal_updateBGPMaps(TauBfdUnit *unit);
#endif /* TAU_BGP || TAU_BGQ */

//////////////////////////////////////////////////////////////////////
// Instead of using a global var., use static inside a function  to
// ensure that non-local static variables are initialised before being
// used (Ref: Scott Meyers, Item 47 Eff. C++).
//////////////////////////////////////////////////////////////////////
typedef std::vector<TauBfdUnit*> bfd_unit_vector_t;
static bfd_unit_vector_t & ThebfdUnits(void)
{
  // BFD units (e.g. executables and their dynamic libraries)
  static bfd_unit_vector_t internal_bfd_units;
  return internal_bfd_units;
}

typedef int * (*objopen_counter_t)(void);
objopen_counter_t objopen_counter = NULL;

int get_objopen_counter(void)
{
  if (objopen_counter) {
    return *(objopen_counter());
  }
  return 0;
}

void set_objopen_counter(int value)
{
  if (objopen_counter) {
    *(objopen_counter()) = value;
  }
}

extern "C"
void Tau_bfd_register_objopen_counter(int * (*)(void) handle)
{
  objopen_counter = handle;
}

//
// Main interface functions
//

void Tau_bfd_initializeBfd()
{
  static bool bfdInitialized = false;
  if (!bfdInitialized) {
    bfd_init();
    bfdInitialized = true;
  }
}

tau_bfd_handle_t Tau_bfd_registerUnit()
{
  tau_bfd_handle_t ret = ThebfdUnits().size();
  ThebfdUnits().push_back(new TauBfdUnit);

  TAU_VERBOSE("Tau_bfd_registerUnit: Unit %d registered and initialized\n", ret);

  // Initialize the first address maps for the unit.
  Tau_bfd_updateAddressMaps(ret);

  return ret;
}

bool Tau_bfd_checkHandle(tau_bfd_handle_t handle)
{
  if (handle == TAU_BFD_NULL_HANDLE) {
    TAU_VERBOSE("TauBfd: Warning - attempt to use uninitialized BFD handle\n");
    return false;
  }
  if (handle >= ThebfdUnits().size()) {
    TAU_VERBOSE("TauBfd: Warning - invalid BFD unit handle %d, max value %d\n", handle, ThebfdUnits().size());
    return false;
  }
  return (handle >= 0);
}

static void Tau_bfd_internal_updateProcSelfMaps(TauBfdUnit *unit)
{
  // *CWL* - This is important! We DO NOT want to use /proc/self/maps on
  //         the BGP because the information acquired comes from the I/O nodes
  //         and not the compute nodes. You could end up with an overlapping
  //         range for address resolution if used!
#if (defined (TAU_BGP) || defined(TAU_BGQ) || (TAU_WINDOWS))
  /* do nothing */
  // *JCL* - Windows has no /proc filesystem, so don't try to use it
#else 

  // Note: Linux systems only.
  FILE * mapsfile = fopen("/proc/self/maps", "r");
  if(!mapsfile) {
    TAU_VERBOSE("Tau_bfd_internal_updateProcSelfMaps: Warning - /proc/self/maps could not be opened.\n");
    return;
  }

  char line[4096];
  // count is used for TAU_VERBOSE only
  int count = 0;
  while (!feof(mapsfile)) {
    fgets(line, 4096, mapsfile);
    unsigned long start, end, offset;
    char module[4096];
    char perms[5];
    module[0] = 0;

    sscanf(line, "%lx-%lx %s %lx %*s %*u %[^\n]",
        &start, &end, perms, &offset, module);
    if (*module && ((strcmp(perms, "r-xp") == 0) ||
            (strcmp(perms, "rwxp") == 0)))
    {
      TAU_VERBOSE("[%d] Module: %s, %p-%p (%d)\n",
          count++, module, start, end, offset);
      unit->addressMaps.push_back(new TauBfdAddrMap(start, end, offset, module));
      unit->modules.push_back(new TauBfdModule);
    }
  }
  fclose(mapsfile);
#endif /* TAU_BGP || TAU_BGQ || TAU_WINDOWS */
}

#if (defined(TAU_BGP) || defined(TAU_BGQ))
static int Tau_bfd_internal_BGP_dl_iter_callback(struct dl_phdr_info * info, size_t size, void * data)
{
  if (strlen(info->dlpi_name) == 0) {
    TAU_VERBOSE("Tau_bfd_internal_BGP_dl_iter_callback: Nameless module. Ignored.\n");
    return 0;
  }
  TAU_VERBOSE("Tau_bfd_internal_BGP_dl_iter_callback: Processing module [%s]\n", info->dlpi_name);

  TauBfdUnit * unit = (TauBfdUnit *)data;

  // assuming the max of the physical addresses of each segment added to the
  // memory size yields the end of the address range.
  unsigned long max_addr = 0;
  for (int j = 0; j < info->dlpi_phnum; j++) {
    unsigned long local_max = (unsigned long)info->dlpi_phdr[j].p_paddr + (unsigned long)info->dlpi_phdr[j].p_memsz;
    if (local_max > max_addr) {
      max_addr = local_max;
    }
  }
  unsigned long start = (unsigned long)info->dlpi_addr;
  TauBfdAddrMap * map = new TauBfdAddrMap(start, start + max_addr, 0, info->dlpi_name);
  TAU_VERBOSE("BG Module: %s, %p-%p (%d)\n", map->name, map->start, map->end, map->offset);
  unit->addressMaps.push_back(map);
  unit->modules.push_back(new TauBfdModule);
  return 0;
}

static void Tau_bfd_internal_updateBGPMaps(TauBfdUnit *unit)
{
  dl_iterate_phdr(Tau_bfd_internal_BGP_dl_iter_callback, (void *)unit);
}
#endif /* TAU_BGP || TAU_BGQ */

// Executables compiled by MinGW are strange beasts in that
// they use GNU debugger symbols, but are Windows executables.
// BFD support for windows is incomplete (e.g. dl_iterate_phdr
// is not implemented and probably never will be), so we must
// use the Windows API to walk through the PE imports directory
// to discover our external modules (e.g. DLLs).  However, we
// still need BFD to parse the GNU debugger symbols.  In fact,
// the DEBUG PE header of an executable produced by MinGW is
// just an empty table.
static void Tau_bfd_internal_updateWindowsMaps(TauBfdUnit *unit)
{
#if defined(TAU_WINDOWS) && defined(TAU_MINGW)

  // Use Windows Process API to find modules
  // This is preferable to walking the PE file headers with
  // Windows API calls because it provides a more complete
  // and accurate picture of the process memory layout, and
  // memory addresses aren't truncated on 64-bit Windows.

  HMODULE hMod[1024];// Handles for each module
  HANDLE hProc;// A handle on the current process
  DWORD cbNeeded;// Bytes needed to store all handles
  MODULEINFO modInfo;// Information about a module
  int count = 0;// for TAU_VERBOSE only

  // Get the process handle
  hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
      FALSE, GetCurrentProcessId());
  if (hProc == NULL) {
    TAU_VERBOSE("Tau_bfd_internal_updateWindowsMaps: Cannot get process handle.\n");
    return;
  }

  // Get handles on all modules in this process
  if (EnumProcessModules(hProc, hMod, sizeof(hMod), &cbNeeded) == 0) {
    TAU_VERBOSE("Tau_bfd_internal_updateWindowsMaps: Cannot enumerate process modules.\n");
    return;
  }

  // Calculate number of handles enumerated
  size_t const nModHandle = cbNeeded / sizeof(HMODULE);

  // Iterate over module handles
  for(size_t i=0; i<nModHandle; ++i) {

    // Get the module information structure
    if(GetModuleInformation(hProc, hMod[i], &modInfo, sizeof(modInfo)) == 0) {
      TAU_VERBOSE("Tau_bfd_internal_updateWindowsMaps: Cannot get module info (handle 0x%x).\n", hMod[i]);
      continue;
    }

    // Create a new BFD map for this module
    TauBfdAddrMap * map = new TauBfdAddrMap;
    map->start = Tau_convert_ptr_to_unsigned_long(modInfo.lpBaseOfDll);
    map->end = map->start + modInfo.SizeOfImage;
    map->offset = 0;

    // Get the full module path name for the map
    if(GetModuleFileNameEx(hProc, hMod[i], map->name, sizeof(map->name)) == 0) {
      TAU_VERBOSE("Tau_bfd_internal_updateWindowsMaps: Cannot get absolute path to module (handle 0x%x).\n", hMod[i]);
      continue;
    }

    TAU_VERBOSE("[%d] Module: %s, %p-%p (%d)\n", count++, map->name, map->start, map->end, map->offset);

    unit->addressMaps.push_back(map);
    unit->modules.push_back(new TauBfdModule);
  }

  // Release the process handle
  CloseHandle(hProc);

#endif /* TAU_WINDOWS && TAU_MINGW */
}

void Tau_bfd_updateAddressMaps(tau_bfd_handle_t handle)
{
  if (!Tau_bfd_checkHandle(handle)) return;

  TauBfdUnit * unit = ThebfdUnits()[handle];

  if (unit->objopen_counter != get_objopen_counter())

  unit->ClearMaps();
  unit->ClearModules();

#if defined(TAU_BGP) || defined(TAU_BGQ)
  Tau_bfd_internal_updateBGPMaps(unit);
#elif defined(TAU_WINDOWS) && defined(TAU_MINGW)
  Tau_bfd_internal_updateWindowsMaps(unit);
#else
  Tau_bfd_internal_updateProcSelfMaps(unit);
#endif

  AddressMapsAreStale() = 0;

  TAU_VERBOSE("Tau_bfd_updateAddressMaps: %d modules discovered\n", unit->modules.size());
}

vector<TauBfdAddrMap*> const & Tau_bfd_getAddressMaps(tau_bfd_handle_t handle)
{
  Tau_bfd_checkHandle(handle);
  return ThebfdUnits()[handle]->addressMaps;
}

tau_bfd_module_handle_t Tau_bfd_getModuleHandle(tau_bfd_handle_t handle, unsigned long probeAddr)
{
  if (!Tau_bfd_checkHandle(handle)) {
    return TAU_BFD_INVALID_MODULE;
  }
  TauBfdUnit *unit = ThebfdUnits()[handle];

  int matchingIdx = Tau_bfd_internal_getModuleIndex(unit, probeAddr);
  if (matchingIdx != -1) {
    return (tau_bfd_module_handle_t)matchingIdx;
  }
  return TAU_BFD_NULL_MODULE_HANDLE;
}

TauBfdAddrMap const * Tau_bfd_getAddressMap(tau_bfd_handle_t handle, unsigned long probe_addr)
{
  if (!Tau_bfd_checkHandle(handle)) {
    return NULL;
  }

  TauBfdUnit *unit = ThebfdUnits()[handle];
  int matchingIdx = Tau_bfd_internal_getModuleIndex(unit, probe_addr);
  if (matchingIdx == -1) {
    return NULL;
  }

  return unit->addressMaps[matchingIdx];
}

static char const * Tau_bfd_internal_tryDemangle(bfd * bfdImage, char const * funcname)
{
  char const * demangled = NULL;
#if defined(HAVE_GNU_DEMANGLE) && HAVE_GNU_DEMANGLE
  if (funcname && bfdImage) {
    // Some compilers prepend .text. to the symbol name
    if (strncmp(funcname, ".text.", 6) == 0) {
      funcname += 6;
    }

    // Sampling sometimes gives the names as a long branch offset
    char const * substr = strstr(funcname, ".long_branch_r2off.");
    if (substr) {
      char * tmp = strdup(substr+19);
      // Trim offset address from end of name
      char * p = tmp + strlen(tmp) - 1;
      while (p != tmp && isdigit(*p)) --p;
      if (*p == '+') *p = '\0';
      demangled = bfd_demangle(bfdImage, tmp, DEMANGLE_FLAGS);
      free(tmp);
    } else {
      demangled = bfd_demangle(bfdImage, funcname, DEMANGLE_FLAGS);
    }
  }
#endif
  if (demangled) return demangled;
  return funcname;
}

// Probe for BFD information given a single address.
bool Tau_bfd_resolveBfdInfo(tau_bfd_handle_t handle, unsigned long probeAddr, TauBfdInfo & info)
{
  if (!Tau_bfd_checkHandle(handle)) {
    info.secure(probeAddr);
    return false;
  }

  TauBfdUnit * unit = ThebfdUnits()[handle];
  TauBfdModule * module;
  unsigned long addr0;
  unsigned long addr1;

  if (AddressMapsAreStale()) {
    Tau_bfd_updateAddressMaps(handle);
  }

  // Discover if we are searching in the executable or a module
  int matchingIdx = Tau_bfd_internal_getModuleIndex(unit, probeAddr);
printf("%p: matchingIdx: %d\n", probeAddr, matchingIdx);
  if (matchingIdx != -1) {
    if (!Tau_bfd_internal_loadSymTab(unit, matchingIdx)) {
      info.secure(probeAddr);
      return false;
    }
    module = Tau_bfd_internal_getModuleFromIdx(unit, matchingIdx);

    // Calculate search addresses for module search
#if defined(TAU_WINDOWS) && defined(TAU_MINGW)
    addr0 = probeAddr;
    addr1 = probeAddr - unit->addressMaps[matchingIdx]->start;
#else
    addr0 = probeAddr - unit->addressMaps[matchingIdx]->start;
    addr1 = probeAddr;
#endif
  } else {
    if (!Tau_bfd_internal_loadExecSymTab(unit)) {
      info.secure(probeAddr);
      return false;
    }
    module = unit->executableModule;

    // Calculate search addresses for executable search
    // Only the first address is valid for the executable
    addr0 = probeAddr;
    addr1 = 0;
  }

  // Convert address to something bfd can use.
  char hex_pc_string[100];
  sprintf(hex_pc_string, "%p", addr0);

  // Search BFD sections for address
  LocateAddressData data(module, info);
  info.probeAddr = bfd_scan_vma(hex_pc_string, NULL, 16);
  bfd_map_over_sections(module->bfdImage, Tau_bfd_internal_locateAddress, &data);

  // If the data wasn't found where we expected and we are searching
  // in a module, try a few more addresses
  if (!data.found && (module != unit->executableModule)) {
    // Try the second address
    if (addr0 != addr1) {
      sprintf(hex_pc_string, "%p", addr1);
      info.probeAddr = bfd_scan_vma(hex_pc_string, NULL, 16);
      bfd_map_over_sections(module->bfdImage, Tau_bfd_internal_locateAddress, &data);
    }
    // Try the executable
    if (!data.found && Tau_bfd_internal_loadExecSymTab(unit)) {
      sprintf(hex_pc_string, "%p", probeAddr);
      info.probeAddr = bfd_scan_vma(hex_pc_string, NULL, 16);
      bfd_map_over_sections(unit->executableModule->bfdImage, Tau_bfd_internal_locateAddress, &data);
    }
  }

  bool resolved = data.found && (info.funcname != NULL);
  if (resolved) {
#ifdef TAU_INTEL12
    // For Intel 12 workaround. Inform the module that the previous resolve was successful.
    module->markLastResult(true);
#endif /* TAU_INTEL12 */
    info.funcname = Tau_bfd_internal_tryDemangle(module->bfdImage, info.funcname);
    if (info.filename == NULL) {
      info.filename = "(unknown)";
#ifdef TAU_INTEL12
      module->markLastResult(false);
#endif /* TAU_INTEL12 */
    }
  } else {
printf("Couldn't resolve %p\n", probeAddr);
#ifdef TAU_INTEL12
    // For Intel 12 workaround. Inform the module that the previous resolve failed.
    module->markLastResult(false);
#endif /* TAU_INTEL12 */
    // Couldn't resolve the address.
    // Fill in fields as best we can.
    if (info.funcname == NULL) {
      info.funcname = (char*)malloc(128);
      sprintf((char*)info.funcname, "addr=<%p>", probeAddr);
    }
    if (info.filename == NULL) {
      if (matchingIdx != -1) {
        info.filename = unit->addressMaps[matchingIdx]->name;
      } else {
        info.filename = unit->executablePath;
      }
    }
    info.probeAddr = probeAddr;
    info.lineno = 0;
  }

  return resolved;
}

static void Tau_bfd_internal_iterateOverSymtab(TauBfdModule * module, TauBfdIterFn fn, unsigned long offset)
{
  // Apply the iterator function to all symbols in the table
  for (asymbol ** s = module->syms; *s; s++) {
    asymbol const & asym = **s;

    // Skip useless symbols (e.g. line numbers)
    // It would be easier to use BFD_FLAGS, but those aren't reliable
    // since the debug symbol format is unpredictable
    if (!asym.name || !asym.section->size) {
      continue;
    }

    // Calculate symbol address
    unsigned long addr = asym.section->vma + asym.value;

    // Get apprixmate symbol name
    char const * name = asym.name;
    if (name[0] == '.') {
      char const * mark = strchr((char*)name, '$');
      if (mark) name = mark + 1;
    }

    // Apply the iterator function
    // Names will be resolved and demangled later
    fn(addr + offset, name);
  }
}

int Tau_bfd_processBfdExecInfo(tau_bfd_handle_t handle, TauBfdIterFn fn)
{
  if (!Tau_bfd_checkHandle(handle)) {
    return TAU_BFD_SYMTAB_LOAD_FAILED;
  }
  TauBfdUnit * unit = ThebfdUnits()[handle];

  char const * execName = unit->executablePath;
  TauBfdModule * module = unit->executableModule;

  // Only process the executable once.
  if (module->processCode != TAU_BFD_SYMTAB_NOT_LOADED) {
    TAU_VERBOSE("Tau_bfd_processBfdExecInfo: "
        "%s already processed (code %d).  Will not reprocess.\n", execName, module->processCode);
    return module->processCode;
  }
  TAU_VERBOSE("Tau_bfd_processBfdExecInfo: processing executable %s\n", execName);

  // Make sure executable symbol table is loaded
  if (!Tau_bfd_internal_loadExecSymTab(unit)) {
    module->processCode = TAU_BFD_SYMTAB_LOAD_FAILED;
    return module->processCode;
  }

  // Process the symbol table
  Tau_bfd_internal_iterateOverSymtab(module, fn, 0);

  module->processCode = TAU_BFD_SYMTAB_LOAD_SUCCESS;
  return module->processCode;
}

int Tau_bfd_processBfdModuleInfo(tau_bfd_handle_t handle, tau_bfd_module_handle_t moduleHandle, TauBfdIterFn fn)
{
  if (!Tau_bfd_checkHandle(handle)) {
    return TAU_BFD_SYMTAB_LOAD_FAILED;
  }
  TauBfdUnit * unit = ThebfdUnits()[handle];

  unsigned int moduleIdx = (unsigned int)moduleHandle;
  TauBfdModule * module = Tau_bfd_internal_getModuleFromIdx(unit, moduleIdx);
  char const * name = unit->addressMaps[moduleIdx]->name;

  // Only process the module once.
  if (module->processCode != TAU_BFD_SYMTAB_NOT_LOADED) {
    TAU_VERBOSE("Tau_bfd_processBfdModuleInfo: "
        "%s already processed (code %d).  Will not reprocess.\n", name, module->processCode);
    return module->processCode;
  }
  TAU_VERBOSE("Tau_bfd_processBfdModuleInfo: processing module %s\n", name);

  // Make sure symbol table is loaded
  if (!Tau_bfd_internal_loadSymTab(unit, moduleHandle)) {
    module->processCode = TAU_BFD_SYMTAB_LOAD_FAILED;
    return module->processCode;
  }

  unsigned int offset;
#if defined(TAU_WINDOWS) && defined(TAU_MINGW)
  offset = 0;
#else
  offset = unit->addressMaps[moduleIdx]->start;
#endif

  // Process the symbol table
  Tau_bfd_internal_iterateOverSymtab(module, fn, offset);

  module->processCode = TAU_BFD_SYMTAB_LOAD_SUCCESS;
  return module->processCode;
}

static bool Tau_bfd_internal_loadSymTab(TauBfdUnit *unit, int moduleIndex)
{
  if ((moduleIndex == TAU_BFD_NULL_MODULE_HANDLE) || (moduleIndex == TAU_BFD_INVALID_MODULE)) {
    return false;
  }

  char const * name = unit->addressMaps[moduleIndex]->name;
  TauBfdModule * module = Tau_bfd_internal_getModuleFromIdx(unit, moduleIndex);

  return module->loadSymbolTable(name);
}

static bool Tau_bfd_internal_loadExecSymTab(TauBfdUnit *unit)
{
  char const * name = unit->executablePath;
  TauBfdModule * module = unit->executableModule;

  return module->loadSymbolTable(name);
}

// Internal BFD helper functions
static int Tau_bfd_internal_getModuleIndex(TauBfdUnit *unit, unsigned long probe_addr)
{
  vector<TauBfdAddrMap*> const & addressMaps = unit->addressMaps;
  for (int i = 0; i < addressMaps.size(); i++) {
    if (probe_addr >= addressMaps[i]->start && probe_addr <= addressMaps[i]->end) return i;
  }
  return -1;
}

static TauBfdModule *
Tau_bfd_internal_getModuleFromIdx(TauBfdUnit * unit, int moduleIndex)
{
  if (moduleIndex == -1) {
    return unit->executableModule;
  }
  return unit->modules[moduleIndex];
}

static int Tau_bfd_internal_getBGPExePath(char * path)
{
  DIR * pdir = opendir("/jobs");
  if (!pdir) {
    TAU_VERBOSE("TAU: ERROR - Failed to open /jobs\n");
    return -1;
  }

  struct dirent * pent;
  for (int i = 0; i < 3; ++i) {
    pent = readdir(pdir);
    if (!pent) {
      TAU_VERBOSE("TAU: ERROR - readdir failed on /jobs (i=%d)\n", i);
      return -1;
    }
  }
  sprintf(path, "/jobs/%s/exe", pent->d_name);
  closedir(pdir);

  TAU_VERBOSE("Tau_bfd_internal_getBGPExePath: [%s]\n", path);
  return 0;
}

static char const * Tau_bfd_internal_getExecutablePath()
{
  static char path[4096];
  static bool init = false;

  if (!init) {
    RtsLayer::LockEnv();
    if (!init) {
#if defined(TAU_AIX)
      sprintf(path, "/proc/%d/object/a.out", getpid());
#elif defined(TAU_BGP)
      if (Tau_bfd_internal_getBGPExePath(path) != 0) {
        fprintf(stderr, "Tau_bfd_internal_getExecutablePath: "
            "Warning! Cannot find BG/P executable path [%s], "
            "symbols will not be resolved\n", path);
      }
#elif defined(TAU_BGQ)
      sprintf(path, "%s", "/proc/self/exe");
#elif defined(__APPLE__)
      uint32_t size = sizeof(path);
      _NSGetExecutablePath(path, &size);
#elif defined(TAU_WINDOWS) && defined(TAU_MINGW)
      GetModuleFileName(NULL, path, sizeof(path));
#else
      // Default: Linux systems
      sprintf(path, "%s", "/proc/self/exe");
#endif
      init = true;
    }
    RtsLayer::UnLockEnv();
  }

  return path;
}

static void Tau_bfd_internal_locateAddress(bfd * bfdptr, asection * section, void * dataPtr)
{
  // Assume dataPtr != NULL because if that parameter is
  // NULL then we've got bigger problems elsewhere in the code
  LocateAddressData & data = *(LocateAddressData*)dataPtr;

  // Skip this section if we've already resolved the address data
  if (data.found) return;

  // Skip this section if it isn't a debug info section
  if ((bfd_get_section_flags(bfdptr, section) & SEC_ALLOC) == 0) return;

  // Skip this section if the address is before the section start
  bfd_vma vma = bfd_get_section_vma(bfdptr, section);
  if (data.info.probeAddr < vma) return;

  // Skip this section if the address is after the section end
  bfd_size_type size = bfd_get_section_size(section);
  if (data.info.probeAddr >= vma + size) return;

  // The section contains this address, so try to resolve info
  // Note that data.info is a reference, so this call sets the
  // TauBfdInfo fields without an extra copy.  This also means
  // that the pointers in TauBfdInfo must never be deleted
  // since they point directly into the module's BFD.
  data.found = bfd_find_nearest_line(bfdptr, section,
      data.module->syms, (data.info.probeAddr - vma),
      &data.info.filename, &data.info.funcname,
      (unsigned int*)&data.info.lineno);
}

#endif /* TAU_BFD */
