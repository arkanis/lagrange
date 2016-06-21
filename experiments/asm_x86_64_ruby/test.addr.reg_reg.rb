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

# dest, src: reg(x)
# op_size: 8, 16, 32, 64
def adc(dest, src, op_size)
	# ADC – ADD with Carry
	#   register1 to register2   0001 000w : 11 reg1 reg2
	
	# operand size   w bit   REX.W bit   66H prefix
	# 8 bit          0       ignored     ignored
	# 16 bit         1       0           yes
	# 32 bit         1       0           no
	# 64 bit         1       1           ignored
	case op_size
	when 8  then bit_w = 0; bit_W = 0; prefix = ""
	when 16 then bit_w = 1; bit_W = 0; prefix = "0110 0110"
	when 32 then bit_w = 1; bit_W = 0; prefix = ""
	when 64 then bit_w = 1; bit_W = 1; prefix = ""
	else raise "unknown op size: #{op_size}!"
	end
	
	r1_bits = ("%04b" % dest[1]).reverse
	r2_bits = ("%04b" % src[1]).reverse
	
	vars = {
		"rrr" => r1_bits[0..2].reverse,  # bits[0..2] of register 1 (reg1)
		"R"   => r1_bits[3].reverse,     # bit[3] of register 1 in REX prefix
		"bbb" => r2_bits[0..2].reverse,  # bits[0..2] of register 2 (reg2)
		"B"   => r2_bits[3].reverse,     # bit[3] of register 2 in REX prefix
		"d"   => "1",            # direction, d = 0 reg1 to reg2, d = 1 reg2 to reg1
		                         # best always set d = 1, matches intel syntax (dest = src)
		                         # not marked in instructions, bit 1 in primary opcode byte (the bit left to the w bit)
		"w"   => bit_w.to_s,     # operand size bit
	                             # w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
		"W"   => bit_W.to_s,     # 64 Bit Operand Size bit
	                             # W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
		"66H" => prefix          # set to 66H prefix op size should be 16 bit
		                         # set = 16 bit op size, empty = other op sizes
	}
	
	# use REX as variable, instead of using W, R and B bits individually.
	# REX is empty if W, R and B are 0
	if vars["W"] == "0" and vars["R"] == "0" and vars["B"] == "0"
		vars["REX"] = ""
	else
		vars["REX"] = "0100 #{vars["W"]}#{vars["R"]}0#{vars["B"]}"
	end
	
	puts vars.inspect
	
	#instr = "66H : 0100 WR0B : 0001 00dw : 11 rrr bbb"
	instr = "66H : REX : 0001 00dw : 11 rrr bbb"
	vars.sort_by{|k, v| k.length}.reverse.each do |name, value|
		instr.gsub! name, value
	end
	puts instr
	
	[instr.gsub(/\s+|:/, "")].pack("B*")
end

open("test.raw", "w") do |f|
	f.write adc(rax, r9, 64)
	f.write adc(rax, r9, 32)
	f.write adc(rax, r9, 16)
	f.write adc(rax, r9, 8)
	
	f.write adc(r0, r1, 64)
	f.write adc(r0, r1, 32)
	#f.write ["0000 1111 0000 0101".gsub(" ", "")].pack("B*")
end