ometa Parser {
	
	start = module,
	
	char_range char:x char:y = char:c ?(x <= c && c <= y)                         -> c,
	
	nl = '\n' | seq('\r\n'),
	ws = ' ' | '\t' | '\u000B' | '\u000C',
	
	token :k = ws* seq(k),
	
	identifier = < letter (letterOrDigit | '_')* >,
	eos        = nl+ | ';'? nl*,
	
	type = "string" | "word",
	
	
	module = mod_header?:h mod_import*:i func_def*:f end                                              -> [#mod, h, [#imports].concat(i), [#defs].concat(f)],
	
	
	// Module header
	mod_header   = ws* lang_version?:v mod_attr*:a "module" eos                                       -> [#header, [#attr].concat(a), [#version, v]],
	lang_version = 'v' < char_range('0', '9') ( char_range('0', '9') | '.' )+ >,
	mod_attr     = "meta",
	
	
	// Import statement
	mod_import      = "public"?:a "import" listOf(#import_entry, ','):e eos                           -> [#import, [#attr].concat(a ? [a] : []), [#entries].concat(e)],
	import_entry    = ws* import_path:p ( "as" ws* nested_id )?:n import_modifier:m                   -> [#entry, [#path].concat(p)].concat(n ? [[#sym].concat(n)] : [], m),
	import_modifier = only_modifier:om                                                                -> [om]
	                | rename_modifier:rm                                                              -> [rm]
	                | except_modifier:ex rename_modifier?:rm                                          -> [ex, rm]
	                | empty                                                                           -> [],
	
	only_modifier   = "only" "(" listOf(#only_entry, ','):e ")"                                       -> [#only].concat(e),
	only_entry      = ws* nested_id:s ( "as" ws* nested_id:n )?                                       -> [#entry, [#sym].concat(s)].concat(n ? [[#alias].concat(n)] : []),
	
	rename_modifier = "rename" "(" listOf(#rename_entry, ','):e ")"                                   -> [#rename].concat(e),
	rename_entry    = ws* nested_id:s "as" ws* nested_id:n                                            -> [#sym, s, n],
	
	except_modifier = "except" "(" listOf(#except_entry, ','):e ")"                                   -> [#except].concat(e),
	except_entry    = ws* nested_id:s                                                                 -> [#sym, s],
	
	import_path       = listOf(#import_path_entry, '.'),
	import_path_entry = '^':i                                                                         -> [#mod_up, i]
	                  | identifier:i meta_mod_args:a                                                  -> [#mod_meta, i, [#args].concat(a)]
	                  | identifier:i                                                                  -> [#mod, i],
	meta_mod_args     = '(' func_call_args:a ')'                                                      -> a,
	
	nested_id = identifier:first ( '.' identifier )*:rest                                             -> [first].concat(rest),
	
	
	// Function definitions
	func_decl    = func_head,
	func_def     = func_attr*:a ws* identifier:i "("
	               listOf(#func_in_arg, ','):ia ("," "...")?:va
					       ( "|" listOf(#func_out_arg, ','):oa )?
					       ")" func_body?:b eos                                                               -> [#func, i, [#attr].concat(a), [#args_in].concat(ia, va ? [#var_args] : []), [#args_out].concat(oa) ].concat( b ? [[#body].concat(b)] : [] ),
	func_attr    = "meta" | "extern.c",
	func_in_arg  = type:t ws* identifier:i                                                            -> [#arg, i, t],
	func_out_arg = type,
	func_body    = "{" statement*:s "}"                                                               -> s,
	
	
	statement = (nl | ws)* ( func_call ):s eos                                                        -> s,
	
	expr = string_literal | func_call,
	
	string_literal = '"' < ( ~'"' char )* >:str '"'                                                   -> [#string, str],
	func_call      = identifier:i '(' func_call_args:a ')'                                            -> [#call, i, [#args].concat(a)],
	
	func_call_args = listOf(#expr, ',')
}
