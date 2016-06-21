class Node
	attr_reader :type, :attrs, :props
	attr_accessor :nodes
	
	def initialize(type, attrs: {}, props: {}, nodes: [])
		raise "invalid args!" unless attrs.kind_of?(Hash) and props.kind_of?(Hash) and nodes.kind_of?(Array)
		@type = type.to_sym
		@attrs = attrs
		@props = props
		@nodes = nodes
	end
	
	def attr(name, value = nil)
		if value == nil
			@attrs[name.to_sym]
		else
			@attrs[name.to_sym] = value
		end
	end
	
	def prop(name, property_node = nil)
		if property_node == nil
			@props[name.to_sym]
		else
			@props[name.to_sym] = property_node
		end
	end
	
	def <<(node)
		nodes << node
	end
	
	def dump(level = 0, prop_node = false)
		prefix = "  " * level
		text = prop_node ? "" : prefix
		text += "#{type.upcase} #{attrs.collect{|k, v| k.to_s + ': ' + v.inspect}.join(", ")}\n"
		
		props.each do |k, n|
			text += "#{prefix}  ::#{k} "
			text += n.dump(level + 1, true)
		end
		
		nodes.each do |n|
			text += n.dump(level + 1)
		end
		
		text
	end
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
	n.attr :name, match(:id)
	while [:in, :out].include? next_token
		mod = modifier
		pp mod
		n.prop(mod.type, mod)
	end
	match '{'
	n.nodes.concat statement_list
	match(next_token) while next_token != '}'
	match '}'
	
	n
end

def modifier
	if next_token == :in or next_token == :out
		type = match next_token
		match '('
		args = arg_list()
		match ')'
		Node.new type, nodes: args
	else
		raise "SyntaxError expected :in or :out, got #{next_token}"
	end
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
	n.prop(:type, type)
	if next_token == :id
		id = match(:id)
		n << Node.new(:id, attrs: {name: id})
	end
	n
end

def type
	name = match :word
	Node.new :id, attrs: {name: name}
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
		Node.new :syscall, nodes: list
	elsif next_token == :return
		match :return
		list = expr_list
		match ';'
		Node.new :return, nodes: list
	elsif next_token == :word
		var_type = type
		var_name = match :id
		var_value = []
		if next_token == '='
			match '='
			var_value << expr
		end
		match ';'
		Node.new :var, attrs: {name: var_name}, props: {type: var_type}, nodes: var_value
	else
		raise "Syntax error, got #{next_token.inspect} expected :syscall, :word, :return"
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

def expr
	a = factor
	if next_token == '+' or next_token == '-'
		op = match(next_token)
		b = factor
		Node.new :op, attrs: {name: op}, nodes: [a, b]
	else
		a
	end
end

def factor
	a = primary
	if next_token == '*' or next_token == '/'
		op = match(next_token)
		b = primary
		Node.new :op, attrs: {name: op}, nodes: [a, b]
	else
		a
	end
end

def primary
	if next_token == '('
		match '('
		a = expr
		match ')'
		a
	elsif next_token == :int
		Node.new :int, attrs: {value: match(:int)}
	elsif next_token == :str
		Node.new :str, attrs: {value: match(:str)}
	elsif next_token == :id
		Node.new :id, attrs: {value: match(:id)}
	end 
end

#function()
tree = source_module
pp tree
puts tree.dump
