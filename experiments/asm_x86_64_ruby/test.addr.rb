=begin

operand size:
	encoded via two bits and one prefix:
	
	w      operand size bit, part of instruction
	       w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
	W      64 Bit Operand Size bit
	       W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
	66H    prefixing instruction with this byte changes default size to 16 bit
	       set = 16 bit op size, not set = 32 bit (ignored when w or W bit is set)
	
	operand size   w bit           REX.W bit          66H prefix
	8 bit          0 (byte size)   ignored            ignored
	16 bit         1 (full size)   0 (default size)   yes (16 bit size)
	32 bit         1 (full size)   0 (default size)   no (default size)
	64 bit         1 (full size)   1 (64 bit size)    ignored
	
	See Volume 2A - Instruction Set Reference, A-L - 2.2.1.2 More on REX Prefix Fields - page 35


[:reg, :reg]
	rrr    bits[0..2] of register 1 (reg1)
	R      bit[3] of register 1 in REX prefix
	bbb    bits[0..2] of register 2 (reg2)
	B      bit[3] of register 2 in REX prefix
	d      direction, d = 0 reg1 to reg2, d = 1 reg2 to reg1
	       best always set d = 1, matches intel syntax (dest = src)
	       not marked in instructions, bit 1 in primary opcode byte (the bit left to the w bit)
	
	w      operand size bit
	       w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
	W      64 Bit Operand Size bit
	       W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
	
	66H    set to 66H prefix op size should be 16 bit
	       set = 16 bit op size, empty = other op sizes
	

[:reg]
	bbb    bits[0..2] of register _1_ (reg)
	B      bit[3] of register _1_ in REX prefix
	w      operand size bit
	       w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
	W      64 Bit Operand Size bit
	       W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
	66H    set to 66H prefix op size should be 16 bit

[:reg, :mem]
	# memory to register 0100 0RXB : 0001 001w : mod reg r/m
	rrr    bits[0..2] of register (reg)
	R      bit[3] of register
	X      ?
	B      ?
	w      operand size bit
	
	mod    address mode
	r/m    address mode
	SIB    scale-index-base byte
	
	[:reg, :mem (:reg) ]               # register indirect
	[:reg, :mem (:reg, :imm) ]         # register indirect + displacement
	[:reg, :mem (:reg, :reg, :imm) ]   # register indirect (base) + register indirect (index) + displacement
	  
[:mem, :reg]
	# same as [:reg, :mem], but different d (direction) bit, see Volume 2C - Instruction Set Reference,  B.1.4.8 Direction (d) Bit, page 78

[:reg, :imm]
[:mem, :imm]
	# both have s bit (sign extend immediate data) in instruction

[:mem]

:reg
:imm
:mem
	:reg → number
	:rel → offset (from RIP)
	:reg, :reg, scale, offset → base + index*scale + offset

[:reg]
[:reg, :reg]
[:reg, :imm]
[:reg, :mem, :reg]  # [reg]
[:reg, :mem, :rel]  # [RIP + rel]
[:reg, :mem, :reg, :imm]  # [reg + imm]
[:reg, :mem, :reg, :reg, :imm]  # [reg + reg + imm]
[:reg, :mem, :reg, :reg, :imm, :imm]  # [reg + reg*imm + imm]

def addr_reg(reg, op_size)
end

def addr_reg_from_reg(reg, reg, op_size)
end

def addr_reg_from_imm(reg, imm, op_size)
end


=end


#
# Helper functions
#

# Returns bits of a 4 bit register index. Indices count bits right to left as in x86 manuals.
# 
#   bits(11, 0..2)  # => "011"
#   bits(11, 3)     # => "1"
#   bits(11, 0..4)  # => "1011"
# 
# Inner workings:
# 
# - converts number to binary string (read bits right to left):              11  => "1011"
# - reverses bit string (to left to right):                               "1011" => "1101"
#   now string slicing can be used to get individual chars (bits) either by index or range (but the content of the range will still be reversed)
# - extracts the bits of interest from string ([] operator):        "1011"[0..2] => "110"
# - reverses the bit order of the extracted slice back to right to left:   "110" => "011"
def bits(number, bit_range_or_index)
	("%04b" % number).reverse[bit_range_or_index].reverse
end

def immediate_as_le_bitstring(immediate_value, bits)
	directive = case bits
	when 8 then "C"
	when 16 then "S"
	when 32 then "L"
	when 64 then "Q"
	else raise("invalid immediate size #{bits}")
	end
	
	directive.downcase! if immediate_value < 0
	return [immediate_value].pack(directive).unpack("B*").first
end

# Returns [w, REX.W, 66H prefix] for the specified operand size
def operand_size(size)
	case size
	#              w    REX.W    66H prefix
	when 8  then ["0",  "0",     ""         ]
	when 16 then ["1",  "0",     "0110 0110"]
	when 32 then ["1",  "0",     ""         ]
	when 64 then ["1",  "1",     ""         ]
	else raise "unknown op size: #{size}!"
	end
end


def reg(index); [:reg, index]; end
def imm(value); [:imm, value]; end
def mem(addr);  [:mem, addr];  end
def rel(addr);  [:rel, addr];  end

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


# op_code e.g. "0001 00dw"
# op_size: 8, 16, 32, 64
def encode(op_code, dest, src, op_size)
	vars = {}
	
	# Take care of direction
	# Direction bit d, 0 = reg field to ModR/M, 1 = ModR/M to reg field
	dest_type, src_type = dest[0], src[0]
	if dest_type == :mem and src_type == :reg
		# Register to memory operation. Memory destination can only be encoded encoded via ModR/M.
		vars["d"] = "0"
		reg, modrm = src, dest
	elsif dest_type == :reg
		# Register destination can be directly encoded in reg field. Source can be anything this way.
		vars["d"] = "1"
		reg, modrm = dest, src
	else
		raise "Invalid or unimplemented operator combination: #{src_type} to #{dest_type}"
	end
	
	# Take care of operand size
	bit_w, bit_W, prefix_66h = operand_size(op_size)
	vars.merge! "w" => bit_w, "W" => bit_W, "66H" => prefix_66h
	
	# Take care of register operand
	reg_index = reg[1]
	vars.merge!(
		"reg" => bits(reg_index, 0..2), "R"   => bits(reg_index, 3)
	)
	
	
	# Take care of ModR/M operand
	modrm_type, modrm_value = modrm
	if modrm_type == :reg
		rm_reg_index = modrm_value
		vars.merge!(
			"mod"  => "11",  # register-direct addressing mode
			"r/m"  => bits(rm_reg_index, 0..2),
			"B"    => bits(rm_reg_index, 3),
			"SIB"  => "",
			"disp" => "",
			"imm"  => ""
		)
	elsif modrm_type == :imm
		raise "TODO"
	elsif modrm_type == :mem and modrm_value[0] == :reg
		mem_reg_index = modrm_value[1]
		vars.merge!(
			"mod"  => "00",  # register-indirect addressing mode: deref(reg)
			                 # see Volume 2A - Instruction Set Reference, A-L
			"r/m"  => bits(mem_reg_index, 0..2),
			"B"    => bits(mem_reg_index, 3),
			"SIB"  => "",
			"disp" => "",
			"imm"  => ""
		)
		
		if vars["r/m"] == "100"
			# "100": address register-indirect of SP (stack pointer). special case that signals SIB byte follows. so we have to encode it in some other way.
			# Encode this case using an SIB byte that dereferences only the base register and that register set to SP.
			vars["SIB"] = "00 100 " + bits(mem_reg_index, 0..2)  # 00 (scale = 1), 100 (index = SP, with mod = 00 this is a special case that signals [base] mode), base = index of SP register
		elsif vars["r/m"] == "101"
			# "101": address register-indirect of BP (base pointer). special case that signals instruction pointer relative addressing followed by 32 bit displacement. so we have to encode it in some other way.
			# Encode this by using a reg + disp8 mode (mod = 01, works for everything except [SP + disp8]) and an displacement of 0.
			# The r/m part of the ModRM byte specifies the register in that mode. So we can leave the variable r/m as it is.
			vars["mm"]  = "01"          # signals :reg + disp8 addressing mode, r/m is used as register
			vars["disp"] = "0000 0000"  # add displacement byte the addressing mode requires
		end
	elsif modrm_type == :mem and modrm_value[0] == :rel
		# RIP relative addressing. Encoded as a special case of register indirect addressing (mod = 00)
		# of RBP (base pointer of stack frame, r/m = 101). Doesn't make much sense to load the byte at
		# the stack frame pointer without any offset or something.
		# But means RBP (R5) can't be dereferenced that way.
		# See AMD64 Architecture Programmer's Manual Volume 3 page 61 - 1.7.1 RIP-Relative Addressing - Encoding
		raise "RIP relative addressing only works with 32 bit displacement" if op_size != 32
		rel_offset = modrm_value[1]
		vars.merge!(
			"mod"  => "00", "r/m"  => "101",  # special case that signals instruction point relative addressing: deref(RIP + disp32)
			"B"    => "0",
			
			"SIB"  => "",
			"disp" => immediate_as_le_bitstring(rel_offset, 32),
			"imm"  => ""
		)
	end
	
	# Take care of REX preifx
	# REX overview: AMD64 Architecture Programmer’s Manual Volume 3: General Purpose and System Instructions, Figure 1-6. Encoding Examples Using REX R, X, and B Bits, page 64
	# REX variable is empty if W, R and B are 0.
	# W, R and B can be used by instruction but when REX is used it automatically is optimized away when instruction doesn't need it
	if vars["W"] == "0" and vars["R"] == "0" and vars["B"] == "0"
		vars["REX"] = ""
	else
		vars["REX"] = "0100 #{vars["W"]}#{vars["R"]}0#{vars["B"]}"
	end
	
	puts vars.inspect
	
	# Encode instruction
	instruction = "66H : REX : #{op_code} : mod reg r/m : SIB : disp : imm"
	vars.sort_by{|k, v| k.length}.reverse.each do |name, value|
		instruction.gsub! name, value
	end
	puts instruction
	
	[instruction.gsub(/\s+|:/, "")].pack("B*")
end


open("test.raw", "w") do |f|
	# ADC - ADD with Carry
	#f.write encode("0001 00dw", reg(0), reg(1), 64)
	#f.write encode("0001 00dw", reg(0), mem(reg(1)), 64)
	#f.write encode("0001 00dw", reg(0), mem(rel(-8)), 32)
	#f.write encode("0001 00dw", mem(rel(-8)), reg(0), 32)
	#16.times do |i|
	#	f.write adc(rax, mem(reg(i)), 64)
	#end
end

=begin
dest: reg, mem
src:  reg, mem, imm

dest src
reg  reg   reg_modrm
     mem   reg_modrm
     imm   reg_imm
mem  reg   reg_modrm
     imm   mem_imm
reg        reg

reg  reg
     mem
mem  reg

patterns:
	
	[:reg, :modrm]
	[:reg, :imm]
	[:reg]
	[]

Assembler.pattern "66H : REX : op : mod reg r/m : SIB : disp : imm" do |a|
	a.adc "0001 00dw", :reg, :modrm
	a.adc "1000 00sw", :reg, :imm
	
	register1 to register2 0100 0R0B : 0001 000w : 11 reg1 reg2
	immediate to register    0100 000B : 1000 00sw : 11  010 reg : immediate
	immediate to memory      0100 00XB : 1000 00sw : mod 010 r/m : immediate
	immediate32 to memory64  0100 10XB : 1000 0001 : mod 010 r/m : imm32
	immediate8 to memory64   0100 10XB : 1000 0031 : mod 010 r/m : imm8
	a.adc "0001 00dw", :imm, :reg
end
=end

class Assembler
	@@instructions = Hash.new{|hash, key| hash[key] = {}}
	
	def self.instructions(template, mnemonic_opcode_pattern_list)
		mnemonic_opcode_pattern_list.each do |mnemonic, opcode, pattern|
			pattern ||= :noops
			@@instructions[mnemonic][pattern] = template.gsub("op", opcode)
		end
	end
	
	def self.method_missing(mnemonic, *operands)
		pattern = case operands.collect{|op| op[0]}
		when [:reg, :mem], [:mem, :reg], [:reg, :reg] then :reg_modrm
		when [:reg, :imm]                             then :reg_imm
		when [:reg]                                   then :reg
		when []                                       then :noops
		else raise "Unknown operand combination: #{operands.collect{|op| op[0]}}"
		end
		
		raise "Undefined instruction #{mnemonic}" unless @@instructions.has_key? mnemonic
		template = @@instructions[mnemonic][pattern]
		raise "Undefined operand pattern #{pattern} for instruction #{mnemonic}" unless template
		
		# Create variables for operand combination
		vars = {}
		%w( 66H REX mod reg r/m SIB disp imm ).each{|name| vars[name] = ""}
		%w( W R X B ).each{|name| vars[name] = "0"}
		vars.merge! self.send("vars_for_#{pattern}", 64, operands)
		
		# Take care of REX preifx
		# REX overview: AMD64 Architecture Programmer’s Manual Volume 3: General Purpose and System Instructions, Figure 1-6. Encoding Examples Using REX R, X, and B Bits, page 64
		# REX variable is empty if W, R and B are 0.
		# W, R and B can be used by instruction but when REX is used it automatically is optimized away when instruction doesn't need it
		if vars["W"] == "0" and vars["R"] == "0" and vars["X"] == "0" and vars["B"] == "0"
			vars["REX"] = ""
		else
			vars["REX"] = "0100 #{vars["W"]}#{vars["R"]}#{vars["X"]}#{vars["B"]}"
		end
		
		puts vars.inspect
		instruction = template.dup
		vars.sort_by{|k, v| k.length}.reverse.each do |name, value|
			instruction.gsub! name, value
		end
		puts instruction
		
		[instruction.gsub(/\s+|:/, "")].pack("B*")
	end
	
	def self.vars_for_reg_modrm(op_size, operands)
		raise "TODO"
	end
	
	def self.vars_for_reg_imm(op_size, operands)
		raise "TODO"
	end
	
	def self.vars_for_reg(op_size, operands)
		reg_index = operands.first[1]
		bit_w, bit_W, prefix_66h = operand_size(op_size)
		
		{
			"w" => bit_w, "W" => bit_W, "66H" => prefix_66h,
			"reg" => bits(reg_index, 0..2), "B"   => bits(reg_index, 3)
		}
	end
	
	def self.vars_for_noops(op_size, operands)
		{}
	end
end

Assembler.instructions "op", [
	[ :syscall, "0000 1111 0000 0101" ],
]


Assembler.instructions "66H : REX : op : mod reg r/m : SIB : disp : imm", [
	[ :adc, "0001 00dw", :reg_modrm ],
	[ :adc, "0001 00dw" ],
]

Assembler.instructions "66H : REX : op", [
	[ :not, "1111 011w : 11 010 reg", :reg ],
	#       "0100 000B : 1111 0111 : 11 010 qwordreg"
]


open("test.raw", "w") do |f|
	#f.write  Assembler.adc rax, mem(rdx)
	#f.write  Assembler.xor rax, rax
	f.write  Assembler.syscall
	f.write  Assembler.not reg(13)
end