var page = 0;
var fuzzy = 1;
var type = 0;
var m = 20;
var busy = false;
var pending = false;
var old_q = "";
var old_fuzzy = 1;
var old_page = 0;
var old_type = 0;
var opt1V = "0";
var opt2V = "1";
var simboost = "0.9";
var old_opt1V = "0";
var old_opt2V = "1";
var old_simboost = "0.9";

function check(value) {

	page=0;
	hideError();
	var regex=/\+|\s/gi;
	if (value.length==0)
	{
		busy = false;
		document.getElementById('results').innerHTML="";
		document.getElementById('pagination').innerHTML="";
	}
	else if(value.replace(regex,"").length >= 3){
		if(busy)pending = true;
		else query(value);
	}
}

function query(value) {
	var regex=/\+/gi;
	value = value.replace(regex,"");
	value = value.replace(/ +(?= )/g,'');
	var valuestmp = value.split(' ');
	var values = valuestmp;

	for (var i = 0; i < valuestmp.length; i++ )
	{
		if (valuestmp[i]=="")
		values.splice(i,i+1);
	}
	
	var url = "/srch2/search?q=";
	var first = 1;
	var q = "";
	for(var i = 0; i < values.length; i++) {
		if(values[i]!=''){
    		if(!first)q += "+";
    		q += values[i].toLowerCase();
			first = 0;
		}
	}
	url += q; 
	// if any option's value changes
	if(q != old_q || fuzzy != old_fuzzy || page != old_page || type != old_type || opt1V != old_opt1V || opt2V != old_opt2V || simboost != old_simboost){
		if(fuzzy == 1)
			url += "&fuzzy=1";
		else{
			url += "&fuzzy=0";
		}
		if(type == 1 && opt1V != ""){
			url += "&type=1";
			if(opt2V == "")
				opt2V = "0";
			url += "&sortby=" + opt1V + "&order=" + opt2V ;
		}
		else
			url += "&type=0";
			
		url += "&start=" + page*m + "&limit=" + (page+1)*m;
		url += "&simboost=" + simboost;
		for(var i=1 ; i<values.length; i++)
		{
			if(values[i]!="")
				url+= "+" + simboost;
		}
		url += "&termtypes=";
		for ( var i = 0 ; i < values.length - 1; i++ )
                {
                        if(values[i]!="")
                                url += "1+";
                }
		url += "0";
		
		//alert(old_page +" "+m+" "+page);
		busy = true;
		self.scrollTo(0, 0);
		sendAjaxRequest(display, url);
		old_q = q;
		old_fuzzy = fuzzy;
		old_page = page;
		old_type = type;
		old_opt1V = opt1V;
		old_opt2V = opt2V;
		old_simboost = simboost;
	}

}

function sendAjaxRequest(callback, url) {
	// Provide the XMLHttpRequest class for stupid IE 5.x-6.x:
	if( typeof XMLHttpRequest == "undefined" ) XMLHttpRequest = function() {
	try { return new ActiveXObject("Msxml2.XMLHTTP.6.0") } catch(e) {}
	try { return new ActiveXObject("Msxml2.XMLHTTP.3.0") } catch(e) {}
	try { return new ActiveXObject("Msxml2.XMLHTTP") } catch(e) {}
	try { return new ActiveXObject("Microsoft.XMLHTTP") } catch(e) {}
	throw new Error( "This browser does not support XMLHttpRequest." )
	};

	var request =  new XMLHttpRequest();

	request.onreadystatechange = function() {
		if (request.readyState == 4 && request.status == 200) {
			if (request.responseText) {
				callback(request.responseText);
			}
		}
		else if (request.readyState == 4 && request.status == 400) {
			busy = false;
			displayError(request.status+" "+request.statusText);
		}
	};

	request.open("get", url);
	request.send(null);
}

function display(responseText) {
	busy = false;
	var response = eval("(" + responseText + ")");
	var time = response.searcher_time;
	var data = response.results;

	html = "Searcher Time: "+time+" ms<br>";
	html+= "Payload access Time: "+response.payload_access_time+" ms"
	html += "<hr>";
	for(var i = 0; i < data.length; i++) {
		var regex = prepareRegex(data[i].matching_prefix, data[i].edit_dist);
		if(data[i].record){
			//for(var j = 0; j < data[i].record.length; j++) {
			html += highlight("\""+data[i].record.title+"\"", regex, "exact", "fuzzy")+"<br>";
			html += JSON.stringify(data[i].record)+"<br>";
			//}
			html += "<i>Overall Score:"+data[i].score+"</i><hr>";
		}
		else{
			html += "Record Id: "
			html += highlight(data[i].record_id.toString(), regex, "exact", "fuzzy")+"<br>";
			html += "Score: "
			html += highlight(data[i].score.toString(), regex, "exact", "fuzzy")+"<br>";
			html += "Edit Distances: "
			html += highlight(data[i].edit_dist.toString(), regex, "exact", "fuzzy")+"<br>";
			html += "Matching Prefixes: "
			html += highlight(data[i].matching_prefix.toString(), regex, "exact", "fuzzy")+"<br>";
		}
		html += "<br>";
	}

	document.getElementById('results').innerHTML=html;

	html = "";

	if(page) 
		html += "<a style='text-decoration: none;' href='#' onclick='goToFirstPage()'>&#171;&#160;First</a> "
		+ "&nbsp;&nbsp;<a style='text-decoration: none;' href='#' onclick='goToPreviousPage()'>Previous</a> "
		+ "&nbsp;&nbsp;<a href='#' onclick='goToPreviousPage()'>" + page + "</a> &nbsp;|&nbsp;";

	if(page || data.length >= m)
		html += (page+1);

	if(data.length >= m)
		html += "&nbsp;|&nbsp;<a href='#' onclick='goToNextPage()'>" + (page+2) + "</a>&nbsp;&nbsp;"
		+ "<a style='text-decoration: none;' href='#' onclick='goToNextPage()'>Next</a>";
       
	document.getElementById('pagination').innerHTML = html;

	if(pending) {
		pending = false;
		query(document.getElementById('input').value);
	}

}

function goToFirstPage(){
	page=0;
	query(document.getElementById('input').value);
	return false;
}

function goToPreviousPage(){
	page--;
	query(document.getElementById('input').value);
	return false;
}

function goToNextPage(){
	page++;
	query(document.getElementById('input').value);
	return false;
}

function prepareRegex(keys, ed) {
	var regex1 = "([^A-Za-z0-9_&])(";
	var regex2 = "([^A-Za-z0-9_&])(";
	var j1 = 0;
	var j2 = 0;
	for(var k = 0; k < keys.length; k++)
	if(keys[k].length)
		if(ed[k] == 0) {
		if(j1)regex1 += "|";
		regex1 += keys[k].replace(/\W/g, 
		    function(sub){return "\\" + sub;});
		j1++;
		}
		else {
		if(j2)regex2 += "|";
		regex2 += keys[k].replace(/\W/g, 
		    function(sub){return "\\" + sub;});
		j2++;
		}

	regex1 += ")";
	regex2 += ")";
	return [ (j1)?new RegExp(regex1, "gi"):null, (j2)?new RegExp(regex2, "gi"):null ];
}

function highlight(string, regexp, stylec1, stylec2) {
	var str = " " + string.replace(/</g, "&lt;");
	if(regexp == null)
		return str;
	if(regexp[0] != null)
		str = str.replace(regexp[0], function(sub, m1, m2) {
			return m1 + "<span class='" + stylec1 + "'>" + m2 + "</span>";
		    });
	if(regexp[1] != null)
	    str = str.replace(regexp[1], function(sub, m1, m2) {
	    return m1 + "<span class='" + stylec2 + "'>" + m2 + "</span>";
	});
	return str.replace(/^\s+/, '');;
}

function enableLink(linkid,textid) {
	var link=document.getElementById(linkid);
	var text=document.getElementById(textid);
	text.style.display="none";
	link.style.display="";
}
function disableLink(linkid, textid) {
	var link=document.getElementById(linkid);
	var text=document.getElementById(textid);
	link.style.display="none";
	text.style.display="";

}

function hideOptions(){
	var label1=document.getElementById('opt1Label');
	label1.style.display="none";
	var label2=document.getElementById('opt2Label');
	label2.style.display="none";
	var label3=document.getElementById('opt3Label');
        label3.style.display="none";

	var text1=document.getElementById('opt1Text');
	text1.style.display="none";
	var text2=document.getElementById('opt2Text');
	text2.style.display="none";
	var text3=document.getElementById('similarity-boost');
        text3.style.display="none";
}

function displayOptions(){
	var label1=document.getElementById('opt1Label');
	label1.style.display="";
	var label2=document.getElementById('opt2Label');
	label2.style.display="";
	var label3=document.getElementById('opt3Label');
        label3.style.display="";

	var text1=document.getElementById('opt1Text');
	text1.style.display="";
	var text2=document.getElementById('opt2Text');
	text2.style.display="";
	var text3=document.getElementById('similarity-boost');
        text3.style.display="";

}

function displayError(txt){
	var err = document.getElementById( 'errorMsg' );
	err.style.display="";
	err.innerHTML = txt;
}

function hideError(){
	var err = document.getElementById( 'errorMsg' );
	err.style.display="none";
}

function clickTopK(){
	type=0;
	check(document.getElementById('input').value);
	disableLink('TopK','topkText');
	enableLink('Advanced','advancedText');
	
	hideOptions();
}

function clickAdvanced(){
	type=1;
	check(document.getElementById('input').value);
	disableLink('Advanced','advancedText');
	enableLink('TopK','topkText');
	
	displayOptions();
}

function clickOn(){
	fuzzy=1;
	check(document.getElementById('input').value);
	disableLink('on','onText');
	enableLink('off','offText');
}

function clickOff(){
	fuzzy=0;
	check(document.getElementById('input').value);
	disableLink('off','offText');
	enableLink('on','onText');
}

function opt1Changed(){
	opt1V = document.getElementById('opt1Text').value;
	check(document.getElementById('input').value);
}

function opt2Changed(){
	opt2V = document.getElementById('opt2Text').value;
	check(document.getElementById('input').value);
}

function simboostChanged(){
	simboost = document.getElementById('similarity-boost').value;
	check(document.getElementById('input').value);
}

