#pragma once

#include <cstdint>

// Auxiliary vector info for the Linux process stack.
// Populated from the ELF headers and passed to Setup() so that _start / glibc
// can find program header info, page size, entry point, etc.
struct ElfAuxvInfo
{
    uint32_t AtPhdr;      // AT_PHDR  (3) - address of program headers in memory
    uint32_t AtPhent;     // AT_PHENT (4) - size of one program header entry
    uint32_t AtPhnum;     // AT_PHNUM (5) - number of program headers
    uint32_t AtPageSz;    // AT_PAGESZ(6) - system page size
    uint32_t AtEntry;     // AT_ENTRY (9) - entry point address
};

class LinuxStack {
public:
    // Simple stack setup like Windy project
    // Returns the ESP pointer after building the stack
    static uint32_t Setup(uint32_t size, int argc, char** argv, uint32_t* outStackBase, uint32_t* outStackLimit, uint32_t reserveSize = 0x4000, const ElfAuxvInfo* auxvInfo = nullptr);
};
