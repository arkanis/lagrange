var fs = require('fs')
var path = require('path')
var ometa = require('./ometajs.js')

//
// Process command line arguments
//
if (process.argv.length < 6){
	console.log('Missing command line arguments')
	console.log('Usage: ' + process.argv[0] + ' ' + path.basename(process.argv[1]) + ' <grammar file> <grammar name> <grammar start rule> <sample file>')
	process.exit(1)
}

var grammar_file = process.argv[2]
var grammar_name = process.argv[3]
var grammar_start_rule = process.argv[4]
var sample_file = process.argv[5]


//
// Utility functions to support OMeta
//

// Just a stupid string repeater function
var repeat_str = function(str, count){
	var list = []
	for (var i = 0; i < count; i++)
		list.push(str)
	return list.join('')
}

// Shows the exact error location and code where the OMeta parser failed to match
var pretty_error = function(error_description){
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
		
		var line_pos = index - line_start
		var error_msg = error_description + '\n'
		error_msg += 'line ' + line_no + ' at pos ' + line_pos + ':\n'
		error_msg += characters.slice(line_start, line_end) + '\n'
		error_msg += repeat_str(' ', line_pos) + '^'
		
		throw error_msg
	}
}

// Load and use an OMeta grammar. The grammar objects are created in the module
// namespace.
var load_grammar = function(grammar_file){
	var grammar = fs.readFileSync(grammar_file, 'utf-8')
	var tree = ometa.BSOMetaJSParser.matchAll(grammar, 'topLevel', undefined, pretty_error("Error while parsing OMeta grammar"));
	var parserString = ometa.BSOMetaJSTranslator.match(tree, 'trans', undefined, pretty_error("Error while translating grammar"));
	
	// The evaled code defines a global object for each grammar
	eval(parserString)
}

// Injects a trace into the specified OMeta object. Every applied rule will be
// shown on the console.
var inject_tracer = function(grammar_object){
  
  var tracer_level = 0
  var tracer_last_idx = 0
  
  var level_prefix = function(level){
  	return repeat_str('  ', level)
  }
  
  var tracer = function(original_func, rule_prefix){
		return function(rule){
			if (rule != 'anything'){
				var arg_list = Array.prototype.slice.call(arguments, 1);
				if (arg_list.length > 0)
					console.error( level_prefix(tracer_level) + rule_prefix + rule + '(' + arg_list.join(', ') + ')' );
				else
					console.error( level_prefix(tracer_level) + rule_prefix + rule );
			}
		  
		  tracer_level++;
		  try {
		    var result = original_applyWithArgs.apply(this, arguments);
		  } finally {
		    tracer_level--;
		    
		    /*
		    var consumed = this.input.idx > tracer_last_idx, matched = !!result
		    if (consumed || matched){
		    	var report_msg = [ level_prefix(tracer_level), '→' ]
		    	if (consumed)
		    		report_msg.push( ' consumed "' + this.input.lst.slice(tracer_last_idx, this.input.idx) + '"' )
		    	if (matched)
		    		report_msg.push( ' matched: ' + result )
		    	console.error(report_msg.join(''))
		    	
		    	tracer_last_idx = this.input.idx
		    }
		    */
		  }
		  
		  return result;
		}
	}
	
	var original_apply = grammar_object._apply
	grammar_object._apply = tracer(original_apply, '')
	
	var original_applyWithArgs = grammar_object._applyWithArgs
	grammar_object._applyWithArgs = tracer(original_applyWithArgs, '')
	
	var original_superApplyWithArgs = grammar_object._superApplyWithArgs
	grammar_object._superApplyWithArgs = tracer(original_superApplyWithArgs, '^')
	
	// We inject code to reset the tracer level and index before each match
	var original_genericMatch = global[grammar_name]._genericMatch;
	global[grammar_name]._genericMatch = function() {
		tracer_level = 0;
		tracer_last_idx = 0;
		return original_genericMatch.apply(this, arguments);
	}
	
}


try {
	load_grammar(grammar_file)
	inject_tracer(global[grammar_name])
} catch(e) {
	console.error(e)
	process.exit(1)
}

// Load sample code
console.log('Matching grammar ' + grammar_name + ' against ' + sample_file)
var sample_code = fs.readFileSync(sample_file, 'utf8')
var result = global[grammar_name].matchAll(sample_code, grammar_start_rule, undefined, pretty_error("Error while parsing sameple code"))
console.log(result)
