var Context = {
	string_table: {},
	string_table_size: 0,
	
	register_string: function(str){
		return this.string_table[str] || (this.string_table[str] = '@s' + this.string_table_size++);
	},
	
	string_table_nodes: function(){
		var nodes = [];
		for(var str in this.string_table){
			if ( this.string_table.hasOwnProperty(str) )
				nodes.push([ this.string_table[str], str ])
		}
		return nodes;
	}
	
	decl_nodes: function(){
	}
}

ometa LagrangePreparator {
	start  = walker:c !( c.concat([
		[#constants].concat(Context.string_table_nodes()),
		[#decl].concat(Context.decl_nodes())
	]) ),
	
	walker = :c ?(c == undefined)                                -> undefined
	       | :c ?(typeof c === 'string')                         -> c
	       | [:name ?(this[name] !== undefined) apply(name):c]   -> c
				 | [:name walker*:c]                                   -> [name].concat(c)
				 | anything,
	
	header = [#attr :name] [#version :v] -> ['header', name, v].join(', '),
	
	string :value -> Context.register_string(value)
}

ometa LagrangeTranslator {
	trans  = [:t apply(t):ans]     -> ans,
	
	mod       trans*:c             -> ['// Module\n'].concat(c).join(''),
	header    anything*            -> '// Header ignored\n',
	imports   anything*            -> '// Imports ignored\n',
	defs      trans*:c             -> ('// Declarations and definitions\n' + c),
	
	func      :name anything*      -> 'func'
}
