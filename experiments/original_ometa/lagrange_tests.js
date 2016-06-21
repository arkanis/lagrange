var test_cases = [
  { rule: 'identifier',
    code: 'f2oo_bar',
    result: 'f2oo_bar'
  },
  
  { rule: 'mod_header',
	  code: '    v0.75 meta module',
	  result: [ 'header', [ 'attr', 'meta' ], [ 'version', '0.75' ] ]
  },
  { rule: 'mod_header',
	  code: 'meta module',
	  result: [ 'header', [ 'attr', 'meta' ], [ 'version', undefined ] ]
  },
  { rule: 'mod_header',
	  code: 'module',
	  result: [ 'header', [ 'attr' ], [ 'version', undefined ] ]
  },
  
	{ rule: 'import_path',
	  code: 'std.elg',
	  result: [ ['mod', 'std'], ['mod', 'elg'] ]
  },
  { rule: 'import_path',
	  code: '^.core.net',
	  result: [ ['mod_up', '^'], ['mod', 'core'], ['mod', 'net'] ]
  },
  { rule: 'import_path',
	  code: 'std.elg().^.users',
	  result: [ ['mod', 'std'], ['mod_meta', 'elg', ['args', []]], ['mod_up', '^'], ['mod', 'users'] ]
  },
  
  { rule: 'only_modifier',
  	code: '  only ( foo as bar )',
  	result: ['only', ['sym', ['foo'], ['bar']]]
  },
  { rule: 'only_modifier',
  	code: 'only(x.y)',
  	result: ['only', ['sym', ['x', 'y'], undefined]]
  },
  { rule: 'only_modifier',
  	code: 'only(write, read)',
  	result: ['only', ['sym', ['write'], undefined], ['sym', ['read'], undefined]]
  },
  { rule: 'only_modifier',
  	code: 'only(io.write as io.w, io.read as io.r)',
  	result: ['only', ['sym', ['io', 'write'], ['io', 'w']], ['sym', ['io', 'read'], ['io', 'r']]]
  },
  
  { rule: 'rename_modifier',
  	code: '  rename ( io.write as io.w , io.read as io.r )',
  	result: ['rename', ['sym', ['io', 'write'], ['io', 'w']], ['sym', ['io', 'read'], ['io', 'r']]]
  },
  { rule: 'except_modifier',
  	code: ' except ( io.write , io.read )',
  	result: ['except', ['sym', ['io', 'write']], ['sym', ['io', 'read']]]
  },
  
  { rule: 'mod_import',
    code: 'public import std.io as local.io',
    result: [ 'import', [ 'attr', 'public' ], [ 'entries', [ 'entry', [ 'path', [ 'mod', 'std' ], [ 'mod', 'io' ] ], [ 'name', [ 'local', 'io' ] ] ] ] ]
  },
  { rule: 'mod_import',
    code: 'import fudel except(bar) rename(foo as foo_new)',
    result: [ 'import', [ 'attr', undefined ], [ 'entries', [ 'entry', [ 'path', [ 'mod', 'fudel' ] ], [ 'name', undefined ], [ 'except', [ 'sym', [ 'bar' ] ] ], [ 'rename', [ 'sym', [ 'foo' ], [ 'foo_new' ] ] ] ] ] ]
  }
]


var om = require('./ometa_util.js')
var util = require('util')

// Load the grammar
try {
	eval( om.load('lagrange.js') )
	//om.inject_tracer(SimpleLagrangeParser)
} catch(e) {
	console.error(e)
	process.exit(1)
}

// Run the test cases
var failed_tests = []

var error_handler = om.pretty_error_handler('Error while parsing code', '  ')
for (var i = 0; i < test_cases.length; i++){
	var test_case = test_cases[i]
	
	try {
		var actual_result = SimpleLagrangeParser.matchAll(test_case.code, test_case.rule, undefined, error_handler)
		
		var actual_inspect = util.inspect(actual_result, false, null)
		var expected_inspect = util.inspect(test_case.result, false, null)
	
		if (actual_inspect == expected_inspect) {
			process.stdout.write('\033[32m' + '.' + '\033[0m')
		} else {
			process.stdout.write('\033[31m' + 'F' + '\033[0m')
			failed_tests.push({test: test_case, actual: actual_inspect, expected: expected_inspect})
		}
	} catch(e) {
		process.stdout.write('\033[31m' + 'E' + '\033[0m')
		failed_tests.push({test: test_case, exception: e})
	}
}
process.stdout.write('\n')

if (failed_tests.length == 0) {
	console.log("All %d tests passed, good job :)", test_cases.length)
} else {
	console.log('%d FAILED tests:', failed_tests.length)
	for(var i = 0; i < failed_tests.length; i++){
		var failed = failed_tests[i]
		
		console.log('rule %s:', failed.test.rule)
		if (failed.exception)
			console.log(failed.exception)
		else
			console.log('  expected: %s\n  got: %s', failed.expected, failed.actual)
	}
}
