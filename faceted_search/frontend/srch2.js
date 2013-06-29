
var pending = false;
var old_q = "";
var busy=false;
var data;
var response;
var busy = false;
var pending = false;
var page = 0;
var m = 20;
var filter = 1;
var inf = 0;

/*
 * the information of  search url
 */
function InformationUrl(isFuzzy,testAppName)
{
    this.isFuzzy=isFuzzy;
    this.testAppName=testAppName;// search engine application name
}

/*
 * the information of showing
 */
function InformationToShow(isDropDownList,attibuteArray,resultDivId)
{
    this.isDropDownList=isDropDownList;
    this.attibuteArray=attibuteArray;
    this.resultDivId=resultDivId;
}



/*
   When someone types in the input box, it calls a JavaScript function called 
 *  check(), 
 *    | if the keyword is ok, do the query,
 *  query() 
 *    | contrust the query and do the Ajax to get the json result
 *  display()
 *    | show the results
 *  prepareRegex()
 *   | prepare the Regex with edit-distance ed for matching prefix keys
 *  highlight()
 *   | highlight the matching prefix keys and show the results  
 */

//check whether query value is null or  its length >= 3
function check(value,infoToShow,infoUrl) {
    var regex=/\+|\s/gi;
   
    if(value.replace(regex, "").length >= 3) {
        if(busy)pending = true;
        else query(value,infoToShow,infoUrl);
    }
    else
    {
        busy = false;
        document.getElementById(infoToShow.resultDivId).innerHTML = "";
    }
}


function query(value,infoToShow,infoUrl) {
	
	if(value.length >= 2){
		var regex=/\+/gi;
		value = value.replace(regex,"");
		value = value.replace(/ +(?= )/g,'');
		var valuestmp = value.split(' ');
		var values = valuestmp;
		for (var i = 0; i < valuestmp.length; i++ )
		{
		    if (valuestmp[i] == "")
		    values.splice(i,i+1);
		}
		//set the url for query
		var url = "http://localhost/";
		url+=infoUrl.testAppName;
		url+="?q=";
		var first = 1;
		var q = "";
		for(var i = 0; i < values.length; i++) {
		    if(values[i]!=''){
		    if(!first)q += "+";//use "+" to  the keywords
		    q += values[i].toLowerCase();//transform value int lowercase
		            first = 0;
		    }
		}
		url += q;//create the url
		
		//  so you can set the parameters of the query
		if(infoUrl.isFuzzy)
		{
		 url += "&fuzzy=1";
		}
		else
		{
		 url += "&fuzzy=0";
		}
		busy = true;
		self.scrollTo(0, 0);
		url += "&callback=?";
		$.getJSON(url, function(data) {
			if(!infoToShow.isDropDownList)
		      display(data,infoToShow);//show the results directly
		    else
		     display_dropdown(data,infoToShow);//show the results using dropdownlist
		   
		    });
   }

}
//display json data
function display(response,infoToShow) {
        busy = false;
        var time = response.searcher_time;
        var data = response.results;
        html = "Searcher Time: "+time+" ms<br>";
        html += "<hr>";
        for(var i = 0; i < data.length; i++) {
                var regex = prepareRegex(data[i].matching_prefix, data[i].edit_dist);
                if(data[i].record){
                	    //so in here you can set different attributes in the record
                	    //display the arrtributes that you give
            	    	for(var j=0;j<infoToShow.attibuteArray.length;j++)
						{
							var attribute=infoToShow.attibuteArray[j];
							if(data[i].record[attribute] != null)
							{
								html +=attribute;
					   	      	html += highlight(": \""+data[i].record[attribute]+"\"", regex, "exact", "fuzzy")+"<br>"
					   	    }
						}
	                	 //in here you can set css style:exact or fuzzy and
	         		    // set the color and size font in css file like
	         		    //<style type="text/css">
						//	span.fuzzy {
						//	        color: #ff00ff;
						//	        font-weight: bold;
						//	}
						//	span.exact {
						//	        color: red;
						//	        font-weight: bold;
						//	}</style>
                        html += highlight(JSON.stringify(data[i].record), regex, "exact", "fuzzy");
                        html += "<br>";
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

        document.getElementById(infoToShow.resultDivId).innerHTML=html;
        if(pending) {//for the next query,set the pending=false
                pending = false;
                query(document.getElementById('input').value);
        }

}


function display_dropdown(responseJSON,infoToShow) {
 	busy = false;
						  
    data = responseJSON.results;
    time = responseJSON.payload_access_time+ responseJSON.searcher_time;
    
    var html = "<div id='tbody' align='left'>";
    for(var i = 0; i < data.length; i++) {
    	
		    var regex = prepareRegex(data[i].matching_prefix, data[i].edit_dist);
			html += "<div "+(i%2?"class=item1":"class='item2'")+">";
			for(var j=0;j<infoToShow.attibuteArray.length;j++)
			{
				var attribute=infoToShow.attibuteArray[j];
				html += "<div "+(j%2?"class=entry0":"class='entry1'")+">&nbsp;&nbsp;&nbsp;&nbsp;";
				if(data[i].record[attribute] != null)
		   	      	html += highlight(data[i].record[attribute].replace(/\uFFFD/gi," "), regex, "highlight1", "highlight2");
		   	    html += "</div>";
			}
		    html += "</div>";
	    }
	html+="<div class='entry0'><Font color='blue'> Full Text search for \""+document.getElementById("input").value+"\"</font></div>";
    html += "</div><div class='static'>"		
 
    html += "<div style='width:60%;float:left;text-align:left;'><font color='#333'>(<b>"+time+"</b> milliseconds) Results <b>"
        +(page*m+1)+"</b> - <b>"+(page*m+data.length)+"</b></font></div>";
       
	html+="<div style='width:40%;text-align:right;float:right;'><a href = \"http://www.srch2.com\" style=\"text-decoration:none\"> <font color='#333'>&nbsp;&nbsp;powered&nbsp;by&nbsp;</font><font face=\"Times New Roman\" color=\"#ff0000\">SRCH2</font></a></div>";
    html += "</div>";

    ac.clearItems();
   	    ac.list.innerHTML = html;
    ac.addItems(ac.list.childNodes[0]);
    ac.showList();
    if(pending) {
        pending = false;
        query();
    }
						   }
//prepare the Regex with edit-distance ed for matching prefix keys
function prepareRegex(keys, ed) {
    var regex1 = "(";
    var regex2 = "(";
    var j1 = 0;
    var j2 = 0;
    for(var k = 0; k < keys.length; k++){
        if(keys[k].length){
            if(ed[k] == 0) { //if the edit-distance is 0
                if(j1)regex1 += "|";
                regex1 += keys[k].replace(/\W/g, 
                    function(sub) {return "\\" + sub;});
                j1++; 
            }
            else {
                if(j2)regex2 += "|";
                regex2 += keys[k].replace(/\W/g, 
                    function(sub) {return "\\" + sub;});
                j2++;
            }
        }
    }

    regex1 += ")";
    regex2 += ")";
    return [ (j1)?new RegExp(regex1, "gi"):null, (j2)?new RegExp(regex2, "gi"):null ];
}

//highlight the matching characters in string using css style: stylec1 or stylec2.
function highlight(string, regexp, stylec1, stylec2) {
    var str = " " + string.replace(/</g, "&lt;"); 
    if(regexp == null)
        return str;
    if(regexp[0] != null)//for the edit-distance is 0
        str = str.replace(regexp[0], function(sub, m1, m2) {
        	//m1  is the matching keyword, m2 is the positon of starting matching character
        	var previousChar = str.charAt(m2-1);//get the previous char of starting matching char
        	//check the previous char is null or is not English letter or number
        	if(previousChar == null || previousChar.match(/^[a-zA-Z0-9_&]+$/) == null)
               return "<span class='" + stylec1 + "'>" + m1 + "</span>";
            else return m1;
            });
    if(regexp[1] != null)//for the edit-distance is not 0
        str = str.replace(regexp[1], function(sub, m1, m2) {
        	var previousChar = str.charAt(m2-1);
            if(previousChar == null || previousChar.match(/^[a-zA-Z0-9_&]+$/) == null)
               return "<span class='" + stylec2 + "'>" + m1 + "</span>";
            else return m1;
        });
    return str.replace(/^\s+/, '');;
}
