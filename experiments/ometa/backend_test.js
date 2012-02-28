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

// Spawn and setup the LLVM and GCC pipe
var spawn = require('child_process').spawn
var llvm = spawn('llc-2.9')
var gcc = spawn('gcc', ['-x', 'assembler', '-', '-o', output_file])

llvm.stdout.on('data', function(data){
	gcc.stdin.write(data)
})

llvm.stderr.on('data', function(data){
	console.log('llvm (stderr): ' + data)
})

llvm.on('exit', function(code){
	gcc.stdin.end()
	console.log('llvm exited with code ' + code)
})


gcc.stdout.on('data', function(data){
	console.log('gcc: ' + data)
})

gcc.stderr.on('data', function(data){
	console.log('gcc (stderr): ' + data)
})

gcc.on('exit', function(code){
	console.log('gcc finished with code ' + code)
})

// Send code to the pipe
llvm.stdin.write(code)
llvm.stdin.end()
console.log('finished')