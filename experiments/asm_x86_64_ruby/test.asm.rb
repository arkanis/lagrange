

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


class Assembler
	@@instructions = []
	
	def self.instr(mnemonic, operand1, operand2, encode_bits)
		@@instructions << [mnemonic.to_sym, operand1, operand2, encode_bits]
	end
	
	
	attr_reader :buffer
	
	def initialize
		@buffer = ""
	end
	
	def method_missing(mnemonic, *operands)
		op1_type, op1_value = operands.count >= 1 ? operands[0] : [nil, nil]
		op2_type, op2_value = operands.count >= 2 ? operands[1] : [nil, nil]
		
		# Search matching instruction definition
		instructions = @@instructions.select{|instr| instr[0] == mnemonic and instr[1] == op1_type and instr[2] == op2_type }
		
		# Raise error if we found none or to many definitions
		if instructions.empty?
			raise "No instruction defined for #{mnemonic} #{operands.collect{|o| o[0]}.join(", ")}"
		elsif instructions.count > 1
			raise "Multiple candidates for instruction #{mnemonic} #{operands.collect{|o| o[0]}.join(", ")}:\n#{instructions.collect{|i| i.inspect}.join("\n")}"
		end
		
		# Define variables that can be used in encoding string.
		# Convert register indices to binary strings and reverse them. Then we can use the string
		# indices to get individual bits: op1_bits[0..2] will return bis 0 to 2 (first 3 bits).
		# But after we fetched them with the index operator we have to reverse the bits again.
		# Otherwise we would insert them into the string in the wrong order.
		types = [op1_type, op2_type]
		
		vars = {}
		if types == [:reg, :reg]
			op1_bits = ("%04b" % op1_value).reverse
			op2_bits = ("%04b" % op2_value).reverse
			vars = {
				"R"   => op1_bits[3].reverse,
				"rrr" => op1_bits[0..2].reverse,
				"B"   => op2_bits[3].reverse,
				"bbb" => op2_bits[0..2].reverse
			}
		elsif types == [:reg, :imm]
			# Immediate to register, register encoded in B and bbb bits of REX prefix and ModRM byte
			op1_bits = ("%04b" % op1_value).reverse
			vars = {
				"B"   => op1_bits[3].reverse,
				"bbb" => op1_bits[0..2].reverse,
				# first encode immediate value as native endian binary value, then convert it to a binary string
				"i64"  => [op2_value].pack("Q").unpack("B*").first,
				"i32"  => [op2_value].pack("L").unpack("B*").first
			}
		elsif types == [:reg, :mem] or types == [:reg, :rel]
			# Memory to register, register encoded in R and rrr bits of REX prefix and ModRM byte
			op1_bits = ("%04b" % op1_value).reverse
			vars = {
				"R"   => op1_bits[3].reverse,
				"rrr" => op1_bits[0..2].reverse,
				# first encode displacement value as native endian binary value, then convert it to a binary string
				"d64"  => [op2_value].pack("Q").unpack("B*").first,
				"d32"  => [op2_value].pack("L").unpack("B*").first
			}
		elsif types == [:mem, :reg]
			raise "TODO"
		end
		
		# Replace variables in encoding, pack it and add the encoded instruction to the buffer
		bits = instructions[0][3].dup
		vars.each do |name, value|
			bits.gsub! name, value
		end
		@buffer += [bits.gsub(/\s|:/, "")].pack("B*")
	end
end

Assembler.instr "mov", :reg, :reg, "0100 1R0B : 1000 1011 : 11 rrr bbb"
Assembler.instr "mov", :reg, :imm, "0100 100B : 1011 1bbb : i64"
Assembler.instr "syscall", nil, nil, "0000 1111 0000 0101"

Assembler.instr "add", :reg, :reg, "0100 1R0B : 0000 0011 : 11 rrr bbb"
Assembler.instr "add", :reg, :imm, "0100 100B : 1000 0001 : 11 000 bbb : i32"  # silently truncates immediate value to 32 bits
Assembler.instr "sub", :reg, :reg, "0100 1R0B : 0010 1011 : 11 rrr bbb"
Assembler.instr "sub", :reg, :imm, "0100 100B : 1000 0001 : 11 101 bbb : i32"  # silently truncates immediate value to 32 bits

#Assembler.instr "mov", :reg, :mem, "0100 1RXB : 1000 1011 : mod qwordreg r/m"

# RIP relative addressing. Encoded as a special case of register indirect addressing (mod = 00)
# of RBP (base pointer of stack frame, r/m = 101). Doesn't make much sense to load the byte at
# the stack frame pointer without any offset or something.
# But means RBP (R5) can't be dereferenced that way.
# See AMD64 Architecture Programmer's Manual Volume 3 page 61 - 1.7.1 RIP-Relative Addressing - Encoding
Assembler.instr "mov", :reg, :rel, "0100 1R00 : 1000 1011 : 00 rrr 101 : d32"

# Direct addressing with a displacement only.
# Encoded as special case of register indirect addressing (mod = 00) of RSP (stack pointer, r/m = 100).
# That special case signals that an scale-index-base (SIB) follows and defines the details.
# If that register indirect SIB byte has an index = 100 (RSP) and base = 101 (RBP) it's a special case
# that signals a 32 bit displacement after the SIB byte. Only the displacement is used as address.
# The scale bits of the SIB byte doesn't seem to matter.
# See AMD64 Architecture Programmer's Manual Volume 3 page 61 - 1.7.1 RIP-Relative Addressing - Encoding
# SEE (Intel) Volume 2A - Instruction Set Reference, A-L page 33 - Table 2-2. 32-Bit Addressing forms with the ModR/M Byte
Assembler.instr "mov", :reg, :mem, "0100 0R00 : 1000 1010 : 00 rrr 100 : 00 100 101 : d32"

#Assembler.instr "mov", :reg, :mem, "0100 0R00 : 1000 1010 : 00 rrr 100 : 00 100 101 : d32"
# mov memory to reg  => 0100 WRXB : 1000 101w : mod reg r/m  (RAX.W marked by me, not in the docs...)
# REX.W = 1  w = 1  =>  r64 <- QWORD PTR
# REX.W = 0  w = 1  =>  r32 <- DWORD PTR
# 0x66 REX.W = 0 w = 1  =>  r16 <- WORD PTR
# REX.W = 0 w = 0  =>  r8 <- BYTE PTR  (0x66 doesn't seem to matter, REX.W = 1 unclear, best left at 0)
# 
# Operand size semantics as understood so far:
# w = 0 forces 8 bit size (overrides everything, maybe because part of instruction op code?)
# w = 1 default size (32 bit)
# 0x66 prefix make default size 16 bit
# REX.W bit makes default size 64 bit (overrides 0x66 prefix)


=begin
# RET "1100 0011"

a = Assembler.new
a.mov rax, rcx
a.mov rdx, imm(7)
a.add rbx, rsp
a.add rbp, imm(9)
a.sub rsi, rdi
a.sub r8,  imm(10)
a.mov r9,  rel(-254)
a.mov r10, mem(3)
a.syscall

File.write "test.raw", a.buffer
=end

=begin

Useful commands:

ruby2.0 test.asm.rb && objdump -D -b binary -m i386:x86-64 -M intel test.raw
xxd -b test.raw


rax = reg(0)
rcx = reg(1)
rdx = reg(2)
rbx = reg(3)
rsp = reg(4)
rbp = reg(5)
rsi = reg(6)
rdi = reg(7)
r8  = reg(8)
r9  = reg(9)
r10 = reg(10)
r11 = reg(11)
r12 = reg(12)
r13 = reg(13)
r14 = reg(14)
r15 = reg(15)

a.mov reg(0), imm(60)
a.mov reg(1), imm(7)


mov reg reg
mov reg imm(8, 16, 32, 64 bit)
mov reg mem(8, 16, 32, 64 bit)
mov mem(8, 16, 32, 64 bit) reg
syscall
add reg reg
sub reg reg


INSTR    DEST  SRC  ENCODING

# Volume 2C - Instruction Set Reference, page 97
mov      reg   reg  "0100 1R0B 1000 1011 : 11 rrr bbb", dest.value >> 3, src.value >> 3, dest.value, src.value
mov      reg   imm  "0100 100B 1011 1bbb : %l", dest.value >> 3, dest.value, src.value
# Volume 2C - Instruction Set Reference, page 107
syscall  _     _    "0000 1111 0000 0101"
# Volume 2C - Instruction Set Reference, page 90
add      reg   reg  "0100 WR0B : 0000 001w : 11 rrr bbb", 1, dest.value >> 3, src.value >> 3, 1, dest.value, src.value
add      reg   imm  "0100 W00B : 1000 000w : 11 000 bbb : %i", 1, dest.value >> 3, 1, dest.value, (int32_t)src.value
# Volume 2C - Instruction Set Reference, page 106
sub      reg   reg  "0100 WR0B : 0010 101w : 11 rrr bbb", 1, dest.value >> 3, src.value >> 3, 1, dest.value, src.value
sub      reg   imm  "0100 W00B : 1000 000w : 11 101 bbb : %i", 1, dest.value >> 3, 1, dest.value, (int32_t)src.value

=end