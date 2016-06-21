$ = require('jquery')

exports.transform = function(jast){
	var string_table = {}, id = 0
	var const_table = $('<constants>').appendTo(jast)
	
	jast.find('string').replaceWith(function(){
		var str = $(this).text(), id = '_s' + const_table.children().size()
		$('<' + id +'>').text(str).appendTo(const_table)
		return string_table[str] || (string_table[str] = id);
	})
	
	return jast
}
