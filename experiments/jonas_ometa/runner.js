// A small runner that loads a file, pushes the source code into the parser
// and the AST into the generator.
var om = require('./ometa'),
    parser = require('./parser.ojs'),
    generator = require('./generator.ojs'),
    fs = require('fs'),
    util = require('util')


var sample_file = process.argv[2]
var sample_code = fs.readFileSync(sample_file, 'utf8')

var ast = parser.parse(sample_code)
console.log( 'Parsed AST:', util.inspect(ast, false, null, true) )

code = generator.generate(ast)
console.log( 'Generated:\n', code )

fs.writeFileSync('out.ll', code)
