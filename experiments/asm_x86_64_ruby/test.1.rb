class Node
	attr_reader :type
	attr_accessor :attr, :nodes
	
	def initialize(type)
		@type = type
		@attr = {}
		@nodes = []
	end
	
	def <<(child)
		nodes << child
	end
	
	def [](type)
		nodes.find{|n| n.type == type}
	end
	
	def []=(type, pseudo_node)
		raise "tried to add normal node #{type} as pseudo-node!" unless type.to_s.start_with? "::"
		raise "duplicated pseudo-node of type #{type}!" if self[type]
		nodes << pseudo_node
	end
	
	def dump(level = 0)
		prefix = "  " * level
		text = "#{prefix}#{self.type} #{self.attr.collect{|k, v| k.to_s + ': ' + v.inspect}.join(", ")}\n"
		nodes.sort{|a, b| a.type.to_s.start_with?("::") <=> b.type.to_s.start_with?("::")}.each do |n|
			if n.kind_of? Node
				text += n.dump(level + 1)
			else
				text += "  " * (leve + 1) + n.inspect + "\n"
			end
		end
	end
	
=begin
	def to_s(level = 0)
		prefix = "  " * level
		node_attr, primitive_attr = self.attr.partition{|k, v| v.kind_of? Node}
		
		text = "#{prefix}#{self.type} #{primitive_attr.collect{|k, v| k.to_s + ': ' + v.inspect}.join(", ")}\n"
		
		node_attr.each do |k, n|
			text += "#{prefix}  ::#{k}\n"
			text += n.to_s(level + 2)
		end
		
		children.each do |n|
			if n.kind_of? Node
				text += n.to_s(level + 1)
			else
				text += n.inspect + "\n"
			end
		end
		
		text
	end
=end
end


code = <<EOF
function main in(word argc, word argv)  out(word, word) {
	word x = 17 + 3 * 9;
	word y = "Hello World!";
	syscall 17, y;
}

function calc in(word x) out(word) {
	return x + 1;
}
EOF

require "pp"
require "strscan"

s = StringScanner.new(code)

def token(s)
	keywords = %w( function in out word syscall return )
	separators = [';', '.', ',', '(', ')', '{', '}', '[', ']', '=', '+', '-', '*', '/']
	#operators = %w( + - * / )
	if s.scan /\d+/
		[:int, s.matched.to_i]
	elsif s.scan /\"/
		[:str, s.scan_until(/\"/).chop]
	elsif s.scan /\w+/
		if keywords.include? s.matched
			[s.matched.to_sym, s.matched]
		else
			[:id, s.matched]
		end
	#elsif s.scan Regexp.new( operators.collect{|s| Regexp.escape(s) }.join('|')  )
	#	[:op, s.matched]
	elsif s.scan Regexp.new( separators.collect{|s| Regexp.escape(s) }.join('|')  )
		[s.matched, s.matched]
	elsif s.scan /\s+/
		#[:ws, s.matched]
		token(s)
	elsif s.eos?
		[:eof, "EOF"]
	end
end

test_stream = s.dup
while (t = token(test_stream)) and t[0] != :eof
	printf t[0].inspect + " "
end
puts "\ntokenizer finished!"


$s = s

def next_token()
	token($s.dup)[0]
end

def match(token_type)
	t = token($s)
	raise "Expected #{token_type}, got #{t[1]}" unless t[0] == token_type
	t[1]
end


def source_module
	n = Node.new :module
	until next_token == :eof
		n << function
	end
	n
end

def function
	n = Node.new :function
	
	match :function
	n.attr[:name] = match :id
	while [:in, :out].include? next_token
		type, args = modifier
		pp type, args
		n[type] = args
	end
	match '{'
	#n.children = statement_list
	match(next_token) while next_token != '}'
	match '}'
	
	n
	#{}"FUNCTION #{name} MODS #{mods.inspect} STMTS #{stmts.inspect}"
end

def modifier
	if next_token == :in
		match :in
		match '('
		args = arg_list()
		match ')'
		n = Node.new :":in"
		n.nodes.concat args
		[:"::in", n]
	elsif next_token == :out
		match :out
		match '('
		types = type_list()
		match ')'
		[:"::out", types]
	else
		raise 'SyntaxError'
	end
end

def type_list
	list = [ type ]
	while next_token == ','
		match ','
		list << type
	end
	list
end



def arg_list
	list = [ arg ]
	while next_token == ','
		match ','
		list << arg
	end
	list
end

def arg
	n = Node.new :arg
	n << type
	if next_token == :id
		id = Node.new :id
		id.attr[:name] = match(:id)
		n << id
	end
	n
end

def type
	n = Node.new :id
	n.attr[:name] = match :word
	n
end

def statement_list
	list = []
	until next_token == '}'
		list << statement
	end
	list
end

def statement
	if next_token == :syscall
		match :syscall
		list = expr_list
		match ';'
		{ :type => :syscall, :args => list }
	elsif next_token == :return
		match :return
		list = expr_list
		match ';'
		{ :type => :return, :args => list }
	elsif next_token == :word
		t = type
		name = match :id
		value = nil
		if next_token == '='
			match '='
			value = expr
		end
		match ';'
		{ :type => :var_decl, :type_name => t, :name => name, :value => value }
	else
		raise "Syntax error, got #{next_token.inspect} expected :syscall or :word"
	end
end

def expr_list
	list = [ expr ]
	while next_token == ','
		match ','
		list << expr
	end
	list
end

=begin
expr_list = expr { , expr }
expr = factor { add_op factor }
factor = primary { mul_op primary }
primary = ( expr ) | id | int | str
add_op = + -
mul_op = * /
=end

def expr
	a = factor
	if next_token == '+' or next_token == '-'
		op = match(next_token)
		b = factor
		[op, a, b]
	else
		[a]
	end
end

def factor
	a = primary
	if next_token == '*' or next_token == '/'
		op = match(next_token)
		b = primary
		[op, a, b]
	else
		[a]
	end
end

def primary
	if next_token == '('
		match '('
		a = expr
		match ')'
		[a]
	elsif next_token == :int
		[match(:int)]
	elsif next_token == :str
		[match(:str)]
	elsif next_token == :id
		[match(:id)]
	end 
end

#function()
tree = source_module
pp tree
puts tree.dump
