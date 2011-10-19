ometa SimpleLagrangeParser {
	
	char_range char:x char:y = char:c ?(x <= c && c <= y) -> c,
	
	nl = '\n' | '\r\n',
	ws = ' ' | '\t' | '\u000B' | '\u000C',
	
	token :k = ws* seq(k),
	
	identifier = letter letterOrDigit*,
	eos        = end | nl | ';',
	
	module = mod_header?:h mod_import*:i eos -> {header: h, imports: i},
	
	mod_header   = lang_version?:v mod_attr*:a "module" -> {type: 'mod', version: v, attr: a},
	lang_version = 'v' ( char_range('0', '9') | '.' | '_' | '-' )+:n -> n.join(''),
	mod_attr     = "meta",
	
	mod_import      = "public"? "import" import_entry ( ',' import_entry )*,
	import_entry    = dot_path ( "as" nested_id )? import_modifier*,
	import_modifier = identifier '(' nested_id ( "as" nested_id )? ')',
	
	dot_path       = dot_path_entry ( '.' dot_path_entry )*,
	dot_path_entry = '^' | identifier,
	
	nested_id = identifier ( '.' identifier )*
	
}
