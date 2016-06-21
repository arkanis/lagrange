$(document).ready(function(){
	$('div').each(function(){
		var child_count = $(this).find('div').size();
		if (child_count > 0)
			$(this).find('> p').append('<small class="child_count">' + child_count + '</small>')
		
		var consumed_by_children = [];
		$(this).find('> ul code.consumed').each(function(){
			consumed_by_children.push( $(this).text() );
		});
		
		if (consumed_by_children.length > 0)
			$(this).find('> p').append('<small class="consumed">' + consumed_by_children.join('') + '</small>')
	});
	
	$('code.rule').click(function(e){
		var container = $(this).parent().parent();
		var elems = container;
		if (e.ctrlKey)
			elems = elems.find('div').andSelf();
		
		if (container.hasClass('opened'))
			elems.removeClass('opened')
		else
			elems.addClass('opened')
		
		return false;
	});
});