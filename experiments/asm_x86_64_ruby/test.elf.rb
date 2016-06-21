def write_elf(filename, start_addr, *segments)
  required_keys = [:flags, :section_name, :base_addr, :data].sort
  known_flags = [:read, :write, :exec].sort
  segments.each_with_index do |s, i|
    raise "section #{i} misses required keys: #{required_keys.select{|k| not s.keys.include? k}.join(", ")}" if s.keys.sort != required_keys
    raise "section #{i} contains unknown keys: #{s.keys.select{|k| not required_keys.include? k}.join(", ")}" if s.keys.detect{|k| not required_keys.include? k}
    raise "section #{i} contains unknown flags: #{s[:flags].select{|f| not known_flags.include? f}.join(", ")}" if s[:flags].detect{|f| not known_flags.include? f}
  end
  
  elf_header_size = 64
  program_header_size = 56
  section_header_size = 64
  page_size = 4096
  
  open(filename, "wb") do |f|
    program_headers = []
    section_headers = []
    section_headers << "\0" * section_header_size  # first section header is unused (index 0 is invalid)
    string_table = "\0"  # first byte is invalid (hence padded with 0) since index 0 is invalid in section headers
    
    # Skip space for ELF and program headers
    f.pos = elf_header_size + segments.count * program_header_size
    
    segments.each do |segment|
      # Increment offset to a page boundary (all data blocks mapped into memory have to be at page boundaries, otherwise surrounding data leaks in)
      f.pos += page_size - (f.pos % page_size);
      
      # Create program header for segment
      flags = segment[:flags].inject(0) do |mask, flag|
        if    flag == :exec  then mask |= (1 << 0)
        elsif flag == :write then mask |= (1 << 1)
        elsif flag == :read  then mask |= (1 << 2)
        end
      end
      program_headers << [
        1,        # Segment type
                  # 0: Program header table entry unused (PT_NULL)
                  # 1: Loadable program segment (PT_LOAD)
                  # 2: Dynamic linking information (PT_DYNAMIC)
                  # 3: Program interpreter (PT_INTERP)
                  # 4: Auxiliary information (PT_NOTE)
                  # 5: Reserved (PT_SHLIB)
                  # 6: Entry for header table itself (PT_PHDR)
                  # 7: Thread-local storage segment (PT_TLS)
                  # 0x60000000: Start of OS-specific (PT_LOOS)
                  # 0x6474e550: GCC .eh_frame_hdr segment (PT_GNU_EH_FRAME)
                  # 0x6474e551: Indicates stack executability (PT_GNU_STACK)
                  # 0x6474e552: Read-only after relocation (PT_GNU_RELRO)
                  # 0x6ffffffa: ? (PT_LOSUNW)
                  # 0x6ffffffa: Sun Specific segment (PT_SUNWBSS)
                  # 0x6ffffffb: Stack segment (PT_SUNWSTACK)
                  # 0x6fffffff: ? (PT_HISUNW)
                  # 0x6fffffff: End of OS-specific (PT_HIOS)
        flags,    # Segment flags
                  # (1 << 0): Segment is executable (PF_X)
                  # (1 << 1): Segment is writable (PF_W)
                  # (1 << 2): Segment is readable (PF_R)
        f.pos,                    # Segment file offset
        segment[:base_addr],      # Segment virtual address
        0,                        # Segment physical address (unused)
        segment[:data].bytesize,  # Segment size in file
        segment[:data].bytesize,  # Segment size in memory (may be larger than size in file)
        page_size                 # Segment alignment
      ].pack("L L Q Q Q Q Q Q")
      
      # Add section name to string table
      name_index = string_table.bytesize
      string_table += segment[:section_name] + "\0"
      
      # Create section header for segment
      flags = (1 << 1)
      flags |= (1 << 0) if segment[:flags].include? :write
      flags |= (1 << 2) if segment[:flags].include? :exec
      section_headers << [
        name_index,   # Section name (string tbl index), Special section indices:
                      # 0: Undefined section (SHN_UNDEF)
                      # >= 0xff00: reserved or processor-specific indices
        1,      # Section type
                # 0:  Section header table entry unused (SHT_NULL)
                # 1:  Program data (SHT_PROGBITS)
                # 2:  Symbol table (SHT_SYMTAB)
                # 3:  String table (SHT_STRTAB)
                # 4:  Relocation entries with addends (SHT_RELA)
                # 5:  Symbol hash table (SHT_HASH)
                # 6:  Dynamic linking information (SHT_DYNAMIC)
                # 7:  Notes (SHT_NOTE)
                # 8:  Program space with no data (bss) (SHT_NOBITS)
                # 9:  Relocation entries, no addends (SHT_REL)
                # 10: Reserved (SHT_SHLIB)
                # 11: Dynamic linker symbol table (SHT_DYNSYM)
                # 14: Array of constructors (SHT_INIT_ARRAY)
                # 15: Array of destructors (SHT_FINI_ARRAY)
                # 16: Array of pre-constructors (SHT_PREINIT_ARRAY)
                # 17: Section group (SHT_GROUP)
                # 18: Extended section indeces (SHT_SYMTAB_SHNDX)
        flags,  # Section flags
                # (1 << 0):  Writable (SHF_WRITE)
                # (1 << 1):  Occupies memory during execution (SHF_ALLOC)
                # (1 << 2):  Executable (SHF_EXECINSTR)
                # (1 << 4):  Might be merged (SHF_MERGE)
                # (1 << 5):  Contains nul-terminated strings (SHF_STRINGS)
                # (1 << 6):  `sh_info' contains SHT index (SHF_INFO_LINK)
                # (1 << 7):  Preserve order after combining (SHF_LINK_ORDER)
                # (1 << 8):  Non-standard OS specific handling required (SHF_OS_NONCONFORMING)
                # (1 << 9):  Section is member of a group.  (SHF_GROUP)
                # (1 << 10): Section hold thread-local data.  (SHF_TLS)
        segment[:base_addr],       # Section virtual addr at execution
        f.pos,                      # Section file offset
        segment[:data].bytesize,    # Section size in bytes
        0,                          # Link to another section
                                    # 0: SHN_UNDEF
        0,                          # Additional section information
        page_size,                  # Section alignment
        0,                          # Entry size if section holds table
      ].pack("L L Q Q Q Q L L Q Q")
      
      f.write segment[:data]
    end
    
    # Create section header for string table and write it
    f.pos += page_size - (f.pos % page_size);
    name_index = string_table.bytesize
    string_table += ".shstrtab\0"
    
    section_headers << [
      name_index,   # Section name (string tbl index), Special section indices:
                    # 0: Undefined section (SHN_UNDEF)
                    # >= 0xff00: reserved or processor-specific indices
      3,      # Section type
              # 0:  Section header table entry unused (SHT_NULL)
              # 1:  Program data (SHT_PROGBITS)
              # 2:  Symbol table (SHT_SYMTAB)
              # 3:  String table (SHT_STRTAB)
              # 4:  Relocation entries with addends (SHT_RELA)
              # 5:  Symbol hash table (SHT_HASH)
              # 6:  Dynamic linking information (SHT_DYNAMIC)
              # 7:  Notes (SHT_NOTE)
              # 8:  Program space with no data (bss) (SHT_NOBITS)
              # 9:  Relocation entries, no addends (SHT_REL)
              # 10: Reserved (SHT_SHLIB)
              # 11: Dynamic linker symbol table (SHT_DYNSYM)
              # 14: Array of constructors (SHT_INIT_ARRAY)
              # 15: Array of destructors (SHT_FINI_ARRAY)
              # 16: Array of pre-constructors (SHT_PREINIT_ARRAY)
              # 17: Section group (SHT_GROUP)
              # 18: Extended section indeces (SHT_SYMTAB_SHNDX)
      (1 << 5),   # Section flags
                  # (1 << 0):  Writable (SHF_WRITE)
                  # (1 << 1):  Occupies memory during execution (SHF_ALLOC)
                  # (1 << 2):  Executable (SHF_EXECINSTR)
                  # (1 << 4):  Might be merged (SHF_MERGE)
                  # (1 << 5):  Contains nul-terminated strings (SHF_STRINGS)
                  # (1 << 6):  `sh_info' contains SHT index (SHF_INFO_LINK)
                  # (1 << 7):  Preserve order after combining (SHF_LINK_ORDER)
                  # (1 << 8):  Non-standard OS specific handling required (SHF_OS_NONCONFORMING)
                  # (1 << 9):  Section is member of a group.  (SHF_GROUP)
                  # (1 << 10): Section hold thread-local data.  (SHF_TLS)
      0,       # Section virtual addr at execution
      f.pos,                      # Section file offset
      string_table.bytesize,      # Section size in bytes
      0,                          # Link to another section
                                  # 0: SHN_UNDEF
      0,                          # Additional section information
      page_size,                  # Section alignment
      0,                          # Entry size if section holds table
    ].pack("L L Q Q Q Q L L Q Q")
    f.write string_table
    
    # Write section headers at end of file
    section_headers_offset = f.pos
    section_headers.each do |s|
      f.write s
    end
    
    # Write ELF and program header at start of file
    elf_header = [
      # Magic number and other info (e_ident)
      0x7f, "E".ord, "L".ord, "F".ord,  # File identification
      2,  # File class
          # 0: Invalid class
          # 1: 32-bit objects
          # 2: 64-bit objects (ELFCLASS64)
      1,  # Data encoding
          # 0: Invalid data encoding
          # 1: 2's complement, little endian (ELFDATA2LSB)
          # 2: 2's complement, big endian
      1,  # File version
          # 0: Invalid ELF version
          # 1: Current version (EV_CURRENT)
      0,  # OS ABI identification (omitted some...)
          # 0: UNIX System V ABI (ELFOSABI_SYSV)
          # 3: Object uses GNU ELF extensions (ELFOSABI_GNU)
            # 64: ARM EABI (ELFOSABI_ARM_AEABI)
            # 97: ARM (ELFOSABI_ARM)
      0, 0, 0, 0, 0, 0, 0, 0,  # padding bytes
      
      2,  # Object file type
          # 0: No file type (ET_NONE)
          # 1: Relocatable file (ET_REL)
          # 2: Executable file (ET_EXEC)
          # 3: Shared object file (ET_DYN)
          # 4: Core file (ET_CORE)
      
      62, # Architecture
          # 3:   Intel 80386 (EM_386)
          # 40:  ARM (EM_ARM)
          # 62:  AMD x86-64 architecture (EM_X86_64)
          # 183: ARM AARCH64 (EM_AARCH64)
      
      1,  # Object file version
          # 0: Invalid ELF version
          # 1: Current version (EV_CURRENT)
      
      start_addr,                  # Entry point virtual address
      elf_header_size,             # Program header table file offset
      section_headers_offset,      # Section header table file offset
      0,                           # Processor-specific flags
      elf_header_size,             # ELF header size in bytes
      program_header_size,         # Program header table entry size
      program_headers.count,       # Program header table entry count
      section_header_size,         # Section header table entry size
      section_headers.count,       # Section header table entry count
      section_headers.count - 1    # Section header string table index
    ].pack("C16 S S L Q Q Q L S S S S S S")
    
    f.pos = 0
    f.write elf_header
    program_headers.each do |p|
      f.write p
    end
  end
  
end


=begin
# Blank ELF test (just exits with an exit code)

def hex(string)
  [ string.gsub(/\s+|#.*$/, "") ].pack("H*")
end

page_size = 4096
write_elf "test.raw", page_size * 16, {
  flags: [:exec, :read],
  section_name: ".text",
  base_addr: page_size * 16,
  data: hex("
    48 b8 3c 00 00 00 00 00 00 00   # mov rax, 60
    48 bf 07 00 00 00 00 00 00 00   # mov rdi, 7
    0f 05                           # syscall
  ")
}
=end

# Hello World example with assembler
require "./test.asm.rb"

r0 = rax = reg(0)
r1 = rcx = reg(1)
r2 = rdx = reg(2)
r3 = rbx = reg(3)
r4 = rsp = reg(4)
r5 = rbp = reg(5)
r6 = rsi = reg(6)
r7 = rdi = reg(7)
r8       = reg(8)
r9       = reg(9)
r10      = reg(10)
r11      = reg(11)
r12      = reg(12)
r13      = reg(13)
r14      = reg(14)
r15      = reg(15)
page_size = 4096

data = "Hello World!\n"
data_addr = page_size * 17
a = Assembler.new

a.mov rax, imm(1)  # write
a.mov rdi, imm(1)  # stdout
a.mov rsi, imm(data_addr)      # buffer ptr
a.mov rdx, imm(data.bytesize)  # buffer size
a.syscall

a.mov rax, imm(60)  # exit
a.mov rdi, mem(data_addr)  # exit code
a.add rdi, imm(2)
a.sub rdi, imm(1)
a.add rdi, imm(-2)
#a.mov rdi, imm(0)   # exit code
a.syscall

write_elf "test.raw", page_size * 16, {
  flags: [:exec, :read],
  section_name: ".text",
  base_addr: page_size * 16,
  data: a.buffer
}, {
  flags: [:read],
  section_name: ".data",
  base_addr: data_addr,
  data: data
}


=begin
section = {
  flags: [:read, :write, :exec],
  section_name: ".foo",  # name used for debugging
  base_addr: x, # vitual address this segment will be mapped to
  data: "..."
}



# ELF header
# program headers
# data
# section headers

# pack directive, C type, ELF types
# C  uint8_t
# S  uint16_t  Elf64_Half, Elf64_Section, Elf64_Versym
# L  uint32_t  Elf64_Word
# Q  uint64_t  Elf32_Xword, Elf64_Addr, Elf64_Off


elf_header = [
  # Magic number and other info (e_ident)
  0x7f, "E".ord, "L".ord, "F".ord,  # File identification
  2,  # File class
      # 0: Invalid class
      # 1: 32-bit objects
      # 2: 64-bit objects (ELFCLASS64)
  1,  # Data encoding
      # 0: Invalid data encoding
      # 1: 2's complement, little endian (ELFDATA2LSB)
      # 2: 2's complement, big endian
  1,  # File version
      # 0: Invalid ELF version
      # 1: Current version (EV_CURRENT)
  0,  # OS ABI identification (omitted some...)
      # 0: UNIX System V ABI (ELFOSABI_SYSV)
      # 3: Object uses GNU ELF extensions (ELFOSABI_GNU)
        # 64: ARM EABI (ELFOSABI_ARM_AEABI)
        # 97: ARM (ELFOSABI_ARM)
  0, 0, 0, 0, 0, 0, 0, 0,  # padding bytes
  
  2,  # Object file type
      # 0: No file type (ET_NONE)
      # 1: Relocatable file (ET_REL)
      # 2: Executable file (ET_EXEC)
      # 3: Shared object file (ET_DYN)
      # 4: Core file (ET_CORE)
  
  62, # Architecture
      # 3:   Intel 80386 (EM_386)
      # 40:  ARM (EM_ARM)
      # 62:  AMD x86-64 architecture (EM_X86_64)
      # 183: ARM AARCH64 (EM_AARCH64)
  
  1,  # Object file version
      # 0: Invalid ELF version
      # 1: Current version (EV_CURRENT)
  
  entry,      # Entry point virtual address
  phoff,      # Program header table file offset
  shoff,      # Section header table file offset
  flags,      # Processor-specific flags
  ehsize,     # ELF header size in bytes
  phentsize,  # Program header table entry size
  phnum,      # Program header table entry count
  shentsize,  # Section header table entry size
  shnum,      # Section header table entry count
  shstrndx    # Section header string table index
].pack("C16 S S L Q Q Q L S S S S S S")

prog_header = [
  type,     # Segment type
            # 0: Program header table entry unused (PT_NULL)
            # 1: Loadable program segment (PT_LOAD)
            # 2: Dynamic linking information (PT_DYNAMIC)
            # 3: Program interpreter (PT_INTERP)
            # 4: Auxiliary information (PT_NOTE)
            # 5: Reserved (PT_SHLIB)
            # 6: Entry for header table itself (PT_PHDR)
            # 7: Thread-local storage segment (PT_TLS)
            # 0x60000000: Start of OS-specific (PT_LOOS)
            # 0x6474e550: GCC .eh_frame_hdr segment (PT_GNU_EH_FRAME)
            # 0x6474e551: Indicates stack executability (PT_GNU_STACK)
            # 0x6474e552: Read-only after relocation (PT_GNU_RELRO)
            # 0x6ffffffa: ? (PT_LOSUNW)
            # 0x6ffffffa: Sun Specific segment (PT_SUNWBSS)
            # 0x6ffffffb: Stack segment (PT_SUNWSTACK)
            # 0x6fffffff: ? (PT_HISUNW)
            # 0x6fffffff: End of OS-specific (PT_HIOS)
  flags,    # Segment flags
            # (1 << 0): Segment is executable (PF_X)
            # (1 << 1): Segment is writable (PF_W)
            # (1 << 2): Segment is readable (PF_R)
  offset,   # Segment file offset
  vaddr,    # Segment virtual address
  paddr,    # Segment physical address
  filesz,   # Segment size in file
  memsz,    # Segment size in memory
  align,    # Segment alignment
].pack("L L Q Q Q Q Q Q")

section_header = [
  sh_name,    # Section name (string tbl index), Special section indices:
              # 0: Undefined section (SHN_UNDEF)
              # >= 0xff00: reserved or processor-specific indices
  sh_type,    # Section type
              # 0:  Section header table entry unused (SHT_NULL)
              # 1:  Program data (SHT_PROGBITS)
              # 2:  Symbol table (SHT_SYMTAB)
              # 3:  String table (SHT_STRTAB)
              # 4:  Relocation entries with addends (SHT_RELA)
              # 5:  Symbol hash table (SHT_HASH)
              # 6:  Dynamic linking information (SHT_DYNAMIC)
              # 7:  Notes (SHT_NOTE)
              # 8:  Program space with no data (bss) (SHT_NOBITS)
              # 9:  Relocation entries, no addends (SHT_REL)
              # 10: Reserved (SHT_SHLIB)
              # 11: Dynamic linker symbol table (SHT_DYNSYM)
              # 14: Array of constructors (SHT_INIT_ARRAY)
              # 15: Array of destructors (SHT_FINI_ARRAY)
              # 16: Array of pre-constructors (SHT_PREINIT_ARRAY)
              # 17: Section group (SHT_GROUP)
              # 18: Extended section indeces (SHT_SYMTAB_SHNDX)
  sh_flags,   # Section flags
              # (1 << 0):  Writable (SHF_WRITE)
              # (1 << 1):  Occupies memory during execution (SHF_ALLOC)
              # (1 << 2):  Executable (SHF_EXECINSTR)
              # (1 << 4):  Might be merged (SHF_MERGE)
              # (1 << 5):  Contains nul-terminated strings (SHF_STRINGS)
              # (1 << 6):  `sh_info' contains SHT index (SHF_INFO_LINK)
              # (1 << 7):  Preserve order after combining (SHF_LINK_ORDER)
              # (1 << 8):  Non-standard OS specific handling required (SHF_OS_NONCONFORMING)
              # (1 << 9):  Section is member of a group.  (SHF_GROUP)
              # (1 << 10): Section hold thread-local data.  (SHF_TLS)
  sh_addr,    # Section virtual addr at execution
  sh_offset,  # Section file offset
  sh_size,    # Section size in bytes
  sh_link,       # Link to another section
  sh_info,       # Additional section information
  sh_addralign,  # Section alignment
  sh_entsize,    # Entry size if section holds table
].pack("L L Q Q Q Q L L Q Q")

=end