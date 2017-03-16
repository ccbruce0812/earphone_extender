var g_fadeWD=null;

function getParam(qstr, field) {
	var regex=new RegExp("[?&]"+field.replace(/[\[\]]/g, "\\$&")+"(=([^&#]*)|&|#|$)"),
		res=regex.exec(qstr);

	if(!res)
		return null;

	if(!res[2])
		return '';

	return decodeURIComponent(res[2].replace(/\+/g, " "));
}

function initFace() {
	$('body').append('<div id="divFade" class="fade" style="visibility: visible"></div>');
}

function fade(timeout) {
	if(g_fadeWD)
		return;

	$('#divFade').css("visibility", "visible");
	g_fadeWD=setTimeout(function(){
		location.reload();
		console.log("Fade watchdog is triggered.");
	}, timeout);
}

function noFade() {
	clearTimeout(g_fadeWD);
	g_fadeWD=null;
	$('#divFade').css("visibility", "hidden");
}

function Init(handler, limit, action) {
	this.m_count=limit;
	this.m_callback=function(evt) {
		handler(evt);

		if(!--this.m_count)
			action();
	}.bind(this);
}
