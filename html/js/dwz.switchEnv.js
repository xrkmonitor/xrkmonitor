/**
 * @author zhanghuihua@msn.com
 */
(function($){
	$.fn.navMenu = function(){
		return this.each(function(){
			var $box = $(this);
			$box.find("li>a").click(function(){
				var $a = $(this);

				$.post($a.attr("href"), {}, function(html){
					$("#sidebar").find(".accordion").remove().end().append(html).initUI();
					$box.find("li").removeClass("selected");
					$a.parent().addClass("selected");

					// ysy -- modify
					if(!$('#navDocMenu').hasClass('selected') && DWZ.ui.sbar == false) {
						$("#sidebar_s .toggleCollapse div").trigger("click");
					}
					navTab.closeAllTab();

					// ysy -- modify, 打开首个菜单链接
					var afirst = $("#sidebar").find("ul li:first").find("a");
					if(afirst != null) {
						afirst.click();
						$("#sidebar").find("ul li:first").find("div:first").addClass('selected');
					}
				});
				return false;
			});
		});
	}
	
	// ysy -- modify
	/*
	$.fn.switchEnv = function(){
		var op = {cities$:">ul>li", boxTitle$:">a>span"};
		return this.each(function(){
			var $this = $(this);
			$this.click(function(){
				if ($this.hasClass("selected")){
					_hide($this);
				} else {
					_show($this);
				}
				return false;
			});
			
			$this.find(op.cities$).click(function(){
				var $li = $(this);

				$.post($li.find(">a").attr("href"), {}, function(html){
					_hide($this);
					$this.find(op.boxTitle$).html($li.find(">a").html());
					navTab.closeAllTab();
					$("#sidebar").find(".accordion").remove().end().append(html).initUI();
				});
				return false;
			});
		});
	}
	*/
	
	function _show($box){
		$box.addClass("selected");
		$(document).bind("click",{box:$box}, _handler);
	}
	function _hide($box){
		$box.removeClass("selected");
		$(document).unbind("click", _handler);
	}
	
	function _handler(event){
		_hide(event.data.box);
	}
})(jQuery);


