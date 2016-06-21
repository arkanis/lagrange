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

def adc(dest, src, op_size)
	# dest: reg(x)
	# src: mem(reg(x))
	# op_size: 8, 16, 32, 64
	
	reg_index = dest[1]
	mem_reg_index = src[1][1]
	
	bit_w, bit_W, prefix_66h = operand_size(op_size)
	vars = {
		# memory to register 0100 0RXB : 0001 001w : mod reg r/m
		"d"    => "1",
		
		"rrr"  => bits(reg_index, 0..2),
		"R"    => bits(reg_index, 3),
		
		"mm"   => "00",
		"bbb"  => bits(mem_reg_index, 0..2),
		"B"    => bits(mem_reg_index, 3),
		
		"w"    => bit_w,
		"W"    => bit_W,
		"66H"  => prefix_66h,
		
		"SIB"  => "",
		"disp" => ""
	}
	
	if vars["bbb"] == "100"
		# "100": address register indirect of SP (stack pointer). special case that signals SIB byte follows. so we have to encode it in some other way.
		# Encode this case using an SIB byte that dereferences only the base register and that register set to SP.
		vars["SIB"] = "00 100 " + bits(mem_reg_index, 0..2)  # 00 (scale = 1), 100 (index = SP, with mod = 00 this is a special case that signals [base] mode), base = index of SP register
	elsif vars["bbb"] == "101"
		# "101": address register indirect of BP (base pointer). special case that signals instruction pointer relative addressing followed by 32 bit displacement. so we have to encode it in some other way.
		# Encode this by using a reg + disp8 mode (mod = 01, works for everything except [SP + disp8]) and an displacement of 0.
		# The r/m part of the ModRM byte specifies the register in that mode. So we can leave the variable bbb as it is.
		vars["mm"]  = "01"          # signals :reg + disp8 addressing mode, bbb is used as register
		vars["disp"] = "0000 0000"  # add displacement byte the addressing mode requires
	end
	
	# use REX as variable, instead of using W, R and B bits individually.
	# REX is empty if W, R and B are 0
	if vars["W"] == "0" and vars["R"] == "0" and vars["B"] == "0"
		vars["REX"] = ""
	else
		vars["REX"] = "0100 #{vars["W"]}#{vars["R"]}0#{vars["B"]}"
	end
	
	puts vars.inspect
	
	#instr = "66H : 0100 WR0B : 0001 00dw : 11 rrr bbb"
	instr = "66H : REX : 0001 00dw : mm rrr bbb : SIB : disp"
	vars.sort_by{|k, v| k.length}.reverse.each do |name, value|
		instr.gsub! name, value
	end
	puts instr
	
	[instr.gsub(/\s+|:/, "")].pack("B*")
end


open("test.raw", "w") do |f|
	16.times do |i|
		f.write adc(rax, mem(reg(i)), 64)
	end
end