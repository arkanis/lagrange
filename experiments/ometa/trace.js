$(document).ready(function(){
	$('div').each(function(){
		var child_count = $(this).find('div').size();
		if (child_count > 0)
			$(this).find('> p').append('<small>' + child_count + '</small>')
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