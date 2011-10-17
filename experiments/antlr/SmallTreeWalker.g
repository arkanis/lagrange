tree grammar SmallTreeWalker;
options {
	tokenVocab = Small;
	ASTLabelType = CommonTree;
	language = Ruby;
	//backtrack = false;
}

var_def: ^( VAR_DEF TYPE_FOO IDENTIFIER ) { puts "bla" } ;
topdown: var_def ;
