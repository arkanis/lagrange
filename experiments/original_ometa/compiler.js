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

//
// Utility functions
//

// Converts the `ast` into the DOT graph format and directly feeds it to the
// `dot` command line program. `dot` renders the graph into SVG code which is
// stored in `output_file`. The `graphviz` package must be installed for this to
// work.
// 
// The unmodified input AST itself is returned (useful for chaining).
var visualize_ast = function(ast, output_file){
	console.log('Generating AST graph in ' + output_file + '…')
	
	var dot_node_index = 0
	var dot = require('child_process').spawn('dot', ['-Tsvg'])
	var svg_file = fs.createWriteStream(output_file)
	
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
	
	recursive_node_writer(ast)
	
	dot.stdin.write('}')
	dot.stdin.end()
	
	return ast
}

// Creates an HTML report out of the trace data of the `traced_grammar` object.
// The report is written to `output_file`.
var write_trace_report = function(traced_grammar, output_file){
	if (traced_grammar.trace === undefined){
		console.warning('Could not create trace report from grammar. Grammar has no trace data.')
		return
	}
	
	console.log('Writing trace report to ' + output_file + '…')
	var trace_data = traced_grammar.trace()
	var trace_file = fs.createWriteStream(output_file)

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
}





//
// Compile
//

// Load source code
var source_file = process.argv[2]
var source_code = fs.readFileSync(source_file, 'utf8')

// Parser pass
console.log('Parsing ' + source_file + '…')
eval( om.load('passes/parser.js') )
var ast = Parser.matchAll(source_code, 'start', undefined, om.pretty_error_handler('Error while parsing ' + source_file))

// Visualize AST
visualize_ast(ast, 'debug/ast.svg')

// Convert AST to XML
console.log('Converting AST to XML…')
eval( om.load('passes/ast_to_xml.js') )
var xml_ast = AstToXml.match(ast, 'start', undefined, om.pretty_error_handler('Error while converting AST to XML'))
fs.writeFileSync('debug/ast.xml', xml_ast)

var jQuery = require('jquery')
var jast = jQuery(xml_ast)

// Run jQuery passes
var prepared_jast = require('./passes/llvm_prepare').transform(jast)

// Convert jQuery DOM to AST
console.log('Converting jQuery AST to AST…')
function recursive_converter(node){
	if (node.nodeType == 3) {
		return node.nodeValue
	} if(node._nodeName == 'undefined') {
		return undefined
	} else {
		var ast_node = [ node._nodeName ]
		$(node).contents().each(function(){
			ast_node.push( recursive_converter(this) )
		})
		return ast_node
	}
}
prepared_ast = recursive_converter(prepared_jast.get(0))
visualize_ast(prepared_ast, 'debug/jast.svg')
console.log(prepared_ast)

// Backend code generation
console.log('Generating code for ' + source_file + '…')
eval( om.load('passes/llvm_translator.js') )
om.inject_tracer(LagrangePreparator)
try {
	var llvm_code = LagrangePreparator.match(ast, 'start', undefined, om.pretty_error_handler('Error while generating LLVM code for ' + source_file))
	write_trace_report(LagrangePreparator, 'debug/llvm_translator_trace.html')
	console.log(llvm_code)
} catch(e) {
	console.error(e)
}
