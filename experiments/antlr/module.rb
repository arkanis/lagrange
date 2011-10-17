require 'generated/ModuleLexer'
require 'generated/ModuleParser'

lexer = Module::Lexer.new <<EOD
meta module

import std.io, std.ebnf

// test
EOD

parser = Module::Parser.new lexer

p parser.module
