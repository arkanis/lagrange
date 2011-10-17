var fs = require('fs')
var $ = require('jquery')

var code = fs.readFileSync('index.html', 'utf8')
console.log( $(code).find('nav').html() )