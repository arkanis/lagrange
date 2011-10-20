var fs = require('fs')
var path = require('path')
var ometa = require('./ometajs.js')

// Process command line arguments
if (process.argv.length < 6){
	console.log('Missing command line arguments')
	console.log('Usage: ' + process.argv[0] + ' ' + path.basename(process.argv[1]) + ' <grammar file> <grammar name> <grammar start rule> <sample file>')
	process.exit(1)
}

var grammar_file = process.argv[2]
var grammar_name = process.argv[3]
var grammar_start_rule = process.argv[4]
var sample_file = process.argv[5]

var error_handler = function(msg){
	return function(state, index){
		var characters = state.input.lst, line_no = 0, line_start = 0, line_end = 0
		
		// Calculate the line number of the error and remember the last line start
		for (var i = 0; i < index && i < characters.length; i++){
			if (characters[i] == '\n'){
				line_no++
				line_start = i+1
			}
		}
		
		// Search for the end of line
		for (var i = line_start; i < characters.length; i++){
			if (characters[i] == '\n'){
				line_end = i
				break
			}
		}
		
		var col_no = index - line_start
		var pre_error = characters.slice(line_start, index)
		var post_error = characters.slice(index, line_end)
		
		throw msg + '\n' + pre_error + ' PARSER ERROR --> ' + post_error
	}
}

// Load and use an OMeta grammar. The grammar objects are created in the module
// namespace.
var load_grammar = function(grammar_file, callback){
	var grammar = fs.readFileSync(grammar_file, 'utf-8')
	var tree = ometa.BSOMetaJSParser.matchAll(grammar, 'topLevel', undefined, error_handler("Error while parsing OMeta grammar"));
	var parserString = ometa.BSOMetaJSTranslator.match(tree, 'trans', undefined, error_handler("Error while translating grammar"));
	
	// The evaled code defines a global object for each grammar
	eval(parserString)
}

try {
	load_grammar(grammar_file)
} catch(e) {
	console.error(e)
	process.exit(1)
}

// Load sample code
console.log('Matching grammar ' + grammar_name + ' against ' + sample_file)
var sample_code = fs.readFileSync(sample_file, 'utf8')
var result = global[grammar_name].matchAll(sample_code, grammar_start_rule, undefined, error_handler("Error while parsing sameple code"))
console.log(result)
