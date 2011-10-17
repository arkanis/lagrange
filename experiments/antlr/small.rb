require 'generated/SmallLexer'
require 'generated/SmallParser'

lexer = Small::Lexer.new <<EOD
int blub
uword fudel
say(int what | word, uword)
put(int what | byte) { pass }
EOD

parser = Small::Parser.new lexer

x = parser.module
puts x.tree.inspect
File.open('ast.dot', 'wb') do |f|
	f.write ANTLR3::DOT::TreeGenerator.generate(x.tree)
end

system "dot -Tsvg ast.dot > ast.svg"


require 'generated/SmallTreeWalker'

nodes = ANTLR3::AST::CommonTreeNodeStream.new(x.tree)
tree_grammar = SmallTreeWalker::TreeParser.new nodes
puts tree_grammar
