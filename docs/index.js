var pending = false;
var old_q = "";
var old_page = 0;
var busy=false;

function check(value) {
        var regex=/\+|\s/gi;
        if (value.length==0)
        {
                busy = false;
                document.getElementById('results').innerHTML="";
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
        var url = "http://simpson.calit2.uci.edu/exampleapp?q=";
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
        if( q != old_q  ){
                url += "&fuzzy=1";
                busy = true;
                self.scrollTo(0, 0);
                url += "&callback=?";
                $.getJSON(url,
                  function (data) {
                    display(data);
                  });
                old_q = q;
        }

}

function display(response) {
        busy = false;
        var time = response.searcher_time;
        var data = response.results;
        html = "Searcher Time: "+time+" ms<br>";
        html += "<hr>";
        for(var i = 0; i < data.length; i++) {
                var regex = prepareRegex(data[i].matching_prefix, data[i].edit_dist);
                if(data[i].record){
                        html += "<b>Name:</b> " + highlight(data[i].record.name, regex, "exact", "fuzzy")+"<br>";
                        html += "<b>Category:</b> " + highlight(data[i].record.category, regex, "exact", "fuzzy"); 
			html += "<br>";
                        html += "<hr>";
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
        if(pending) {
                pending = false;
                query(document.getElementById('input').value);
        }

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
        return str.replace(/^\s+/, '');
}
