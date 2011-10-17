lexer grammar Something;

options {
	language = Ruby;
}


fragment NEWLINE_CHARACTERS: '\n' | '\r\n';
fragment DIGIT: '0' .. '9';
fragment BIN_DIGIT: '0' | '1';
fragment OCT_DIGIT: '0' .. '7';
fragment HEX_DIGIT: '0' .. '9' | 'a' .. 'f' | 'A' .. 'F';
fragment IDENTIFIER_START: 'a' .. 'z' | 'A' .. 'Z' | '_';
fragment IDENTIFIER_CHARACTER: 'a' .. 'z' | 'A' .. 'Z' | '_' | '0' .. '9';

MODULE: 'module';
ATTRIBUTE: 'wildcard';

IMPORT: 'import';

IDENTIFIER: IDENTIFIER_START IDENTIFIER_CHARACTER*;

// '\u000B' = vertical tab, '\u000C' = form feed
WS: ( ' ' | '\t' | '\u000B' | '\u000C' )+;
NL: NEWLINE_CHARACTERS+;


INT: '-'? '1' .. '9' DIGIT*;
BIN_INT: '0b' BIN_DIGIT+;
OCT_INT: '0o' OCT_DIGIT+;
HEX_INT: '0x' HEX_DIGIT+;


LINE_COMMENT: '//' ~NEWLINE_CHARACTERS*;
BLOCK_COMMENT: '/*' .* '*/';
NESTED_COMMENT: '/+' ( options {greedy=false;}: (NESTED_COMMENT | .) )* '+/' {$channel=HIDDEN;} ;
 
BLOCK_START:   '{';
BLOCK_END:     '}';
BRACKET_START: '[';
BRACKET_END:   ']';
BRACES_START:  '(';
BRACES_END:    ')';

OPERATOR: '<=>' | '='
				| ('==' | '+' | '-' | '*' | '/' | '~' | '·' | '×' | 
				   'and' | 'or' | 'not' | '&&' | '||' | '!' | 
				   '&' | '|' | '^' | '<<<' | '>>>' | '<<' | '>>' | 
				   '<' | '>' ) '='?;
