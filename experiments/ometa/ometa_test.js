var path = require('path')
var fs = require('fs')
var ometa = require('./ometajs.js')

// Process command line arguments
var input_file = process.argv[2]
var output_file = path.basename(input_file, path.extname(input_file))

if (input_file == output_file)
	throw 'input file same as output file (' + input_file + ')'

// Load source code
var code = fs.readFileSync(input_file, 'utf8')

// Load and use an OMeta grammar
var create_grammar = function(grammar_file){
	var grammar = fs.readFileSync(grammar_file, 'utf-8')
	
	var tree = ometa.BSOMetaJSParser.matchAll(grammar, 'topLevel', undefined, function( m, i ) {
		throw 'Error while Parsing Grammar at position ' + i;
	});
	
	var parserString = ometa.BSOMetaJSTranslator.match(tree, 'trans', undefined, function( m, i ) {
		throw 'Error while Translating Grammar at position ' + i;
	});
	
	// after evaling, we got our parser-reference at __ometajs_parser__
	eval('var __ometajs_parser__='+parserString);
	return __ometajs_parser__;
}

var num_grammar = create_grammar('grammar.js')
console.log( num_grammar.matchAll(code, 'num') )