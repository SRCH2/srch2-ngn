function populateResults(params)
{

// 1. generate the query
var query = '';
// 1.1 . main query
query += 'searchType=getAll&q={defaultPrefixComplete=COMPLETE}';
var queryBoxValue = document.getElementById('query_box').value;
queryBoxValue = queryBoxValue.trim();
//if(queryBoxValue == "") return;
if(queryBoxValue==""){
	// no keyword, return empty
	return "";
}else{
	var keywords = queryBoxValue.split(" ");
	for(var i=0;i<keywords.length;i++){
		if(i == keywords.length-1){
			query += keywords[i] + "*" + "~"
        }else{
			query += keywords[i] + "~" + " AND "
        }
	}
}

query += '&fuzzy=true'
// 1.2 . facets
query += '&facet=true'
var attributes = ['year' , 'genre'];
for(var i = 0 ; i<attributes.length ; i++){
	if(attributes[i] == 'genre'){
		query += '&facet.field=genre';
	}else{
		query += '&facet.range='+attributes[i];
		var start = document.getElementById('facet_'+attributes[i]+'_start').value;
		var end = document.getElementById('facet_'+attributes[i]+'_end').value;
		var gap = document.getElementById('facet_'+attributes[i]+'_gap').value;
		if(start != ""){
			query += '&f.'+attributes[i]+'.facet.start=' + start;
		}
		if(end != ""){
			query += '&f.'+attributes[i]+'.facet.end=' + end;
		}
		if(gap != ""){
			query += '&f.'+attributes[i]+'.facet.gap=' + gap;
		}
	}

}

// 1.3 . sort filter
/*var first = "";
var second = "";
var third = "";
for(var i = 0 ; i<attributes.length ; i++){
	var sortRadios = document.getElementsByName('sort_'+ attributes[i] +'_priority');
	for(var j=0;j<sortRadios.length ; j++){
		if(sortRadios[j].checked){
			var value = sortRadios[j].value;
			if(value == 'first'){
				first = attributes[i];
			}else if(value == 'second'){
				second = attributes[i];
			}else if(value == 'third'){
				third = attributes[i];
			}
		}
	}
}
if(!( first == "" && second == "" && third == "") ){
	query += '&sort=';
	if(first == ""){
		if(second != ""){
			first = second;
			second = "";
		}else{
			first = third;
			third = "";
		}
	}
	query += first;
	if(second != ""){
		query += ","+second;
	}
	if(third != ""){
		query += ","+third;
	}
	var sortOrderRadios = document.getElementsByName('sort_filter_order');
	for(var i=0; sortOrderRadios.length ; i++){
		if(sortOrderRadios[i].checked){
			query += '&orderby=' + sortOrderRadios[i].value;
			break;
		}
	}
}*/
var orderRadios = document.getElementsByName('sort_filter_order');
var setSort=false;
for(var j=0;j<orderRadios.length ; j++){
	if(orderRadios[j].checked){
		var value = orderRadios[j].value;
		if(value == 'asc'){
			setSort=true;
		}else if(value == 'desc'){
			setSort=true;
		}
	}
}
if(setSort){
	query += '&sort=year&orderby='+value;
}

// 1.4 . filter query
var filterQueryIsthere = 0;
var fqAssField = document.getElementById('filter_assignment_field').value;
var fqAssValue =  document.getElementById('filter_assignment_value').value;
var fqAss = "";
if(! (fqAssField == "" || fqAssValue == "") ){
	filterQueryIsthere += 1;
	fqAss = fqAssField + ':' + fqAssValue;
}

var fqRngField = document.getElementById('filter_range_field').value;
var fqRngStart = document.getElementById('filter_range_start').value;
var fqRngEnd = document.getElementById('filter_range_end').value;
var fqRng = "";
if(! (fqRngField == "" || fqRngStart == "" || fqRngEnd == "") ){
	filterQueryIsthere += 1;
	fqRng = fqRngField + ':[ ' + fqRngStart + ' TO ' + fqRngEnd + ' ]';
}

var fqComplex = document.getElementById('filter_complex').value;
var fqCmp = "";
if(! ( fqComplex == "" ) ){
	filterQueryIsthere += 1;
	fqCmp = "CMPLX$" + fqComplex + "$";
}
var op = "";
if(filterQueryIsthere == 1){
	query += '&fq=' + fqAss + fqRng + fqCmp;
}else if(filterQueryIsthere > 1){
	
	var opRadios = document.getElementsByName('filter_op');
	for(var i=0; opRadios.length ; i++){
		if(opRadios[i].checked){
			op = opRadios[i].value;
			break;
		}
	}
	if(fqAss == ""){
		query += '&fq=' + fqRng + " " + op + " " + fqCmp;
	}else if (fqRng == "" ){
		query += '&fq=' + fqAss + " " + op + " " + fqCmp;
	}else if(fqCmp == ""){
		query += '&fq=' + fqAss + " " + op + " " + fqRng;
	}else{
		query += '&fq=' + fqAss + " " + op + " " + fqRng + " " + op + " " + fqCmp ;
	}
}

if("fq" in params){
	var fqObj = params['fq'];
	var fqStr = fqObj['field']+":[ "+fqObj['valueLeft'] + " TO " + fqObj['valueRight'] + " ]";
	if(fqObj['valueLeft'] != fqObj['valueRight']){
		fqStr += " AND -"+fqObj['field']+":"+fqObj['valueRight']
	}
	if(localStorage.getItem('facetFilter') != "NULL"){
		fqStr += " AND " + localStorage.getItem('facetFilter');
	}
	if(filterQueryIsthere>0){
		query += " "+ op + " " + fqStr;
	}else{
		query += '&fq=' + fqStr;
		isFqSet=true;
	}
	localStorage.setItem('facetFilter' , fqStr);
}else if(localStorage.getItem('facetFilter') != "NULL"){
	var fqStr = localStorage.getItem('facetFilter');
	if(filterQueryIsthere>0){
	query += " "+ op + " " + fqStr;
	}else{
		query += '&fq=' + fqStr;
		isFqSet=true;
	}
}

return query
//alert(query);
// 2. Use AJAX to send the query and get the results

// 3. Populate screen


}
