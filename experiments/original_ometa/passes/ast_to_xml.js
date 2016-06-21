ometa AstToXml {
	start  = walker,
	
	walker = :c ?(c == undefined)                                -> '<undefined />'
	       | :c ?(typeof c === 'string')                         -> c
	       | [:name ?(this[name] !== undefined) apply(name):c]   -> c
				 | [:name walker*:c]                                   -> ['<', name, '>'].concat(c, ['</', name, '>']).join('')
				 | anything:c                                          -> c.toString(),
	
	string = :c -> ['<string>', c, '</string>'].join('')
}
