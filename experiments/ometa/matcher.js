var fs = require('fs')
var path = require('path')
var om = require('./ometa_util.js')

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

// Load the grammar
try {
	eval( om.load(grammar_file) )
	om.inject_tracer(global[grammar_name])
} catch(e) {
	console.error(e)
	process.exit(1)
}

// Load sample code
console.log('Matching grammar ' + grammar_name + ' against ' + sample_file)
var sample_code = fs.readFileSync(sample_file, 'utf8')
var result = global[grammar_name].matchAll(sample_code, grammar_start_rule, undefined, om.pretty_error_handler("Error while parsing sameple code"))
console.log(result)
