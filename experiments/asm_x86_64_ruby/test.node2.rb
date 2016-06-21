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
	
	def dump(level = 0)
		prefix = "  " * level
		text = "#{prefix}#{type} #{attrs.collect{|k, v| k.to_s + ': ' + v.inspect}.join(", ")}\n"
		
		props.each do |k, n|
			text += "#{prefix}  ::#{k}\n"
			text += n.dump(level + 2)
		end
		
		nodes.each do |n|
			text += n.dump(level + 1)
		end
		
		text
	end
end

require "pp"

node = Node.new :function
id_node = Node.new :id, attrs: {name: "foo"}

node.attr(:name, "main")
pp node.attr(:name)

node.prop(:in, id_node)
pp node.prop(:in)

pp node.nodes

puts node.dump