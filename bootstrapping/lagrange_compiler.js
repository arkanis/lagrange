var fs = require('fs')
var path = require('path')
var om = require('./ometa_util.js')
var util = require('util')

//
// Process command line arguments
//
if (process.argv.length < 3){
	console.log('Missing command line arguments')
	console.log('Usage: ' + process.argv[0] + ' ' + path.basename(process.argv[1]) + ' <source file>')
	process.exit(1)
}

var source_file = process.argv[2]

// Load the grammar
try {
	eval( om.load('lagrange.js') )
	eval( om.load('lagrange_translator.js') )
	om.inject_tracer(LagrangePreparator)
} catch(e) {
	console.error(e)
	process.exit(1)
}

// Load source code
console.log('Parsing ' + source_file + '…')
var source_code = fs.readFileSync(source_file, 'utf8')
//try {
	var ast = SimpleLagrangeParser.matchAll(source_code, 'module', undefined, om.pretty_error_handler("Error while parsing " + source_file))
	var backend_code = LagrangePreparator.match(ast, 'start', undefined, om.pretty_error_handler("Error while generating code for " + source_file))
//} catch(e) {
//	console.error(e)
//}

console.log(backend_code)

// Write the trace HTML file
console.log('Writing HTML trace file…')
var trace_data = LagrangePreparator.trace()
var trace_file = fs.createWriteStream('translator_trace.html')

trace_file.write(['<!DOCTYPE html><html><head>',
	'<meta charset="utf-8" />',
	'<title>OMeta Tracer Tree</title>',
	'<link rel="stylesheet" href="trace.css" />',
	'<script src="jquery-1.6.4.min.js"></script>',
	'<script src="trace.js"></script>',
	'</head><body>\n'].join(''))

function recursive_writer(data){
	trace_file.write(['<div><p>', '<code class="rule">', data.rule, '</code>'].join(''))
	
	if (data.args && data.args.length > 0)
		trace_file.write(['( <code class="args">', util.inspect(data.args, false, null), '</code> )'].join(''))
	
	if (data.consumed)
		trace_file.write(['<code class="consumed">', util.inspect(data.consumed), '</code>'].join(''))
	
	trace_file.write(['→ <code class="matched">', util.inspect(data.matched, false, null), '</code>', '</p>\n'].join(''))
	
	if (data.children.length > 0){
		trace_file.write('<ul>')
		
		for(var i = 0; i < data.children.length; i++){
			trace_file.write('<li>\n')
			recursive_writer(data.children[i])
			trace_file.write('</li>')
		}
		
		trace_file.write('</ul>')
	}
	
	trace_file.write('</div>\n')
}

recursive_writer(trace_data)
trace_file.write('</body></html>')
trace_file.destroySoon()

// Write the DOT graph file for the AST
console.log('Generating AST graph…')
if (backend_code){
	var dot_node_index = 0
	var dot = require('child_process').spawn('dot', ['-Tsvg'])
	var svg_file = fs.createWriteStream('trace.svg')
	
	dot.stdout.on('data', function(data){
		svg_file.write(data)
	})
	
	dot.stderr.on('data', function(data){
		console.error('dot error: %s', data)
	})
	
	dot.stdin.write('digraph AST {\n')
	
	function recursive_node_writer(node){
		var this_index = dot_node_index
		
		if (node instanceof Array) {
			dot.stdin.write('n' + this_index + ' [label="' + node[0] + '", shape=box];\n')
			
			var children = node.slice(1)
			for (var i = 0; i < children.length; i++){
				var child_index = ++dot_node_index
				recursive_node_writer(children[i])
				dot.stdin.write('n' + this_index + ' -> n' + child_index + ';\n')
			}
		} else {
			dot.stdin.write('n' + this_index + ' [label="' + node + '"];\n')
		}
	}
	
	recursive_node_writer(backend_code)
	
	dot.stdin.write('}')
	dot.stdin.end()
}
