grammar Module;

options {
	language = Ruby;
	output = AST;
}

//
// Some basic lexer tokens
//

fragment NEWLINE_CHARACTERS: '\n' | '\r\n';

// '\u000B' = vertical tab, '\u000C' = form feed
WS: ( ' ' | '\t' | '\u000B' | '\u000C' )+ { $channel=HIDDEN; };
NL: NEWLINE_CHARACTERS+;

LINE_COMMENT: '//' ~NEWLINE_CHARACTERS* { $channel=HIDDEN; };
BLOCK_COMMENT: '/*' .* '*/' { $channel=HIDDEN; };
NESTED_COMMENT: '/+' ( options {greedy=false;}: (NESTED_COMMENT | .) )* '+/' { $channel=HIDDEN; };

IDENTIFIER: ('a' .. 'z' | 'A' .. 'Z' | '_') ('0' .. '9' | 'a' .. 'z' | 'A' .. 'Z' | '_')*;


//
// Parser rules
//

eos: NL | EOF;

module: module_header? module_import* ;

module_header:    /* language_version? */ module_attribute* 'module'^ eos ;
// Problems with ANTLR ("v0" at the beginning is tokenized as an identifier)
// Therefore disabled for bootstrapping
//language_version: 'v' ( '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9' | '.' | '-' | '_' )+ ;
module_attribute: 'meta' ;


module_import:   'public'? 'import'^ import_entry ( ',' import_entry )* eos ;
import_entry:    mod_path ( 'as' nested_id )? import_modifier* ;
import_modifier: IDENTIFIER '(' nested_id ( 'as' nested_id )? ')' ;

mod_path:       mod_path_entry ( '.' mod_path_entry )* ;
mod_path_entry: '^' | ( IDENTIFIER ( '(' func_arg_list ')' )? ) ;
nested_id:      IDENTIFIER ( '.' IDENTIFIER )* ;

func_arg_list: ( func_arg ( ',' func_arg )* )? ;
func_arg:      'a' ;


/*
module: module_header? module_import* ( definition | meta_statement )*;

module_header: 'wildcard'? 'module'^ eos;

module_import: 'import' IDENTIFIER ( '.' IDENTIFIER )* eos;

definition: 'int' IDENTIFIER eos;

meta_statement: IDENTIFIER '()' eos;
*/
