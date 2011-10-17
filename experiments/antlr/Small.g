grammar Small;

options {
	output = AST;
	backtrack = false;
}

tokens {
	MODULE;
	VAR_DEF;
	FUNC_DEF;
	IN_ARGS;
	OUT_ARGS;
	ARG;
}

fragment NEWLINE_CHARACTERS: '\n' | '\r\n';

// '\u000B' = vertical tab, '\u000C' = form feed
WS: ( ' ' | '\t' | '\u000B' | '\u000C' )+ { $channel=HIDDEN; } ;
NL: NEWLINE_CHARACTERS+ ;

TYPE_FOO: 'u'? ('byte' | 'short' | 'int' | 'long' | 'word') ;
IDENTIFIER: ( 'a' .. 'z' | 'A' .. 'Z' | '_' ) ('a' .. 'z' | 'A' .. 'Z' | '_' | '0' .. '9')+ ;

fragment DIGIT: '0' .. '9';
INT: '-'? '1' .. '9' DIGIT*;

LINE_COMMENT: '//' ~NEWLINE_CHARACTERS* { $channel=HIDDEN; } ;
BLOCK_COMMENT: '/*' .* '*/' { $channel=HIDDEN; } ;
NESTED_COMMENT: '/+' ( options {greedy=false;}: (NESTED_COMMENT | .) )* '+/' { $channel=HIDDEN; } ;


module: definition+  ->  ^( MODULE definition+ );
definition: ( variable_def | function_def ) eos! ;
eos: NL | EOF | ';' ;

variable_def: TYPE_FOO IDENTIFIER ( '=' value )?  ->  ^(VAR_DEF IDENTIFIER TYPE_FOO value? );
value: INT ;

function_def:  IDENTIFIER '(' func_in_args? ( '|' func_out_args )? ')' ( '{' func_body '}' )?
               -> ^( FUNC_DEF IDENTIFIER func_in_args? func_out_args? func_body? );
func_in_args:  func_in_arg ( ',' func_in_arg )*
	-> ^(IN_ARGS func_in_arg+);
func_in_arg:   TYPE_FOO IDENTIFIER  ->  ^(ARG IDENTIFIER TYPE_FOO);
func_out_args: func_out_arg ( ',' func_out_arg )*  ->  ^(OUT_ARGS func_out_arg+);
func_out_arg: TYPE_FOO ;
func_body:     'pass' ;
