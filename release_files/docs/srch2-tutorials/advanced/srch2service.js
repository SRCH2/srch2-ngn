srch2service = {
    init: function(config){
        _srch2_this = this;
        _srch2_this.config = _srch2_this.getDefaultConfig();
        _srch2_this.bind();
        _srch2_this.bindSortEvents();
        _srch2_this.addScriptTag("");
        _srch2_this.bindFacetCat();
	_srch2_this.bindFacetFilterRemove();
        
    },
    getDefaultConfig: function(){
        var config = {
            enableLog:true, // true / false
            logType: "console", // console or alert. Note: console.log might not work on IE.
            jsonPsrc:"http://localhost:8081/search?", //http://calvin.calit2.uci.edu/srch2demo/search?
            inputElementId: "query_box",
            debug:true, // true /false
            evenNameToBindInputElement:"keyup"
        };
        return config;
    },
    addScriptTag: function(src){
        var script = document.createElement('script');
        script.id= "srch2ResponseHandlerScriptTagId";
        script.src = src;
        document.getElementsByTagName('head')[0].appendChild(script);
        return false;
    },
    removeScriptTag: function(){
        var script = document.getElementById("srch2ResponseHandlerScriptTagId");
        script.parentNode.removeChild(script);
        return false;
    },
    log: function(msg,debug){
        if(_srch2_this.config.enableLog==true){
            if(debug=="debug"){
                if(!_srch2_this.config.debug){
                    return;
                }
            }
            if(_srch2_this.config.logType=="alert"){
                alert(msg);
            }else{
                console.log(msg);
            }
        }
    },
    bind: function(){
        _srch2_this.log("binding events","debug");
        var element = document.getElementById(_srch2_this.config.inputElementId);
        _srch2_this.bindEvent(element, _srch2_this.config.evenNameToBindInputElement, function(event){
            if(event.keyCode == 13){
                return;
            }
            _srch2_this.log("pressed","debug");
            var query = populateResults({});
            _srch2_this.jsonpCall(query);
        });
    },
    bindSortEvents: function(){
        _srch2_this.log("binding sort event","debug");
        var elements = document.getElementsByName('sort_filter_order');
        for(var j=0;j<elements.length ; j++){
            _srch2_this.log("button clicked","debug");
            _srch2_this.bindEvent(elements[j], 'click', function(event){
                _srch2_this.log("clicked","debug");
                var query = populateResults({});
                _srch2_this.jsonpCall(query);
            });
        }
    },
    bindFacetCat:function(){
        _srch2_this.log("binding facet_cat click event","debug");
        var elements = document.getElementsByClassName('facet_cat');
        for(var j=0;j<elements.length ; j++){
            _srch2_this.log("binding facet_cat_index, j is "+ j,"debug");
            _srch2_this.bindEvent(elements[j], 'click', function(event){
                _srch2_this.log("clicked","debug");
                var fqObj = {};
                var element= event.srcElement
                var dataset = element.dataset;
                if(Object.keys(dataset).length === 0){
                    dataset = element.parentElement.dataset;
                }
                fqObj.field=dataset.field;
		if(fqObj.field == 'year'){
		        value = element.innerHTML;
			fqObj.valueLeft = value.split(",")[0];
			fqObj.valueRight = value.split(" ")[1];
			fqObj.valueLeft = fqObj.valueLeft.substring(1);
			if(fqObj.valueLeft.indexOf('b>') != -1){
				fqObj.valueLeft = fqObj.valueLeft.substring(2);
				//alert(fqObj.valueLeft + " TO "+fqObj.valueRight);
			}
			fqObj.valueRight = fqObj.valueRight.substring(0,fqObj.valueRight.length-1);
			if(fqObj.valueLeft.charAt(0) == '-'){
				fqObj.valueLeft = "*";
			}
			if(fqObj.valueRight.charAt(0) == '+'){
				fqObj.valueRight = "*";
			}
		}else{
			value = element.innerHTML;
			if(value.indexOf('<b>') != -1){
				value=value.substring(3,value.length - 4);
				//alert(value);
			}
			fqObj.valueLeft = value;
			fqObj.valueRight = value;
			
		}
		document.getElementById('facet_remove_filter').innerHTML = 
				"<button >Remove Facet Filtering</button>";
                var params = {};
                params['fq'] = fqObj;
                var query = populateResults(params);
                _srch2_this.jsonpCall(query);
            });
        }
    },
    bindConfigEvents: function(){
        var element = document.getElementById("toggleDebug");
        _srch2_this.bindEvent(element, 'click', function(event){
            _srch2_this.config.debug = !_srch2_this.config.debug;
        });

        var element = document.getElementById("toggleLogOutput");
        _srch2_this.bindEvent(element, 'click', function(event){
            if(_srch2_this.config.logType=="alert"){
                _srch2_this.config.logType="console";
            }else{
                _srch2_this.config.logType="alert";
            }
        });
        
        var element = document.getElementById("toggleEnableLogs");
        _srch2_this.bindEvent(element, 'click', function(event){
            _srch2_this.config.enableLog = !_srch2_this.config.enableLog;
        });
        

    },
    bindFacetFilterRemove: function(){
	var  element = document.getElementById("facet_remove_filter");
        _srch2_this.bindEvent(element, 'click', function(event){
            element.innerHTML = "";                
	    localStorage.setItem('facetFilter', "NULL");
            var query = populateResults({});
	    _srch2_this.jsonpCall(query);
        });
    },
    jsonpCall: function (query){
        if(query!=""){
            _srch2_this.log("inside jsonPcall, query is "+ query,"debug");
            _srch2_this.removeScriptTag();
            var src = encodeURI(_srch2_this.config.jsonPsrc +query+"&callback=srch2service.responseHandler");
            console.log(src)
            _srch2_this.addScriptTag(src);
            _srch2_this.log("jsonpCall over","debug");
        }else{
            _srch2_this.log("empty query","debug");
            // clear facets.
            // hard code alert
            document.getElementById("facet_year").innerHTML="";
            document.getElementById("facet_genre").innerHTML="";
	    document.getElementById("resultContainer").innerHTML="";
        }
    },
    bindEvent: function (element, type, handler) {
        if(element.addEventListener) {
            element.addEventListener(type, handler, false);
        } else {
            element.attachEvent('on'+type, handler);
        }
    },
    responseHandler: function(responseText){
        if(responseText){
            var output = "";
            var results = responseText.results;
	        output += "<table width='450px'>";
            output += "<tr>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>  </td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Title'+ "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>"+ 'Genre' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>"+ 'Director' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Year' + "</td>";
            output += "</tr>";
            _srch2_this.queryKeywords = responseText.query_keywords;
            for (var i=0;i<results.length;i++){
		        output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted;width:9%'>" +  "<img style='width:100%; height:auto' src='"+record.banner_url+"'></td>";
                output += "<td style='border-bottom:thin dotted'>" + _srch2_this.addHighliting(prefix,record.title)+ "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix,record.genre)+ "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix,record.director)+ "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix,record.year)+ "</td>";
		        output += "</tr>";
            }
	        output += "</table>";
            _srch2_this.log("got it","debug");
            _srch2_this.log(JSON.stringify(responseText),"debug");
            var element = document.getElementById("resultContainer");
            if(output ==""){
                element.innerHTML = "No results mate!";
            }else{
                element.innerHTML = output;
            }
            // populate facets
            var facets =  responseText.facets;
            for (var i=0;i<facets.length;i++){
                var row = facets[i];
                var fieldName = row.facet_field_name;
		if(fieldName == 'year'){
		        var info = row.facet_info;
		        var htmlVal = "";
			htmlVal += "<table>";
		        for (var j=0;j<info.length;j++){
				if(info.length > 1){
					if(j ==0){
						htmlVal += '<tr><td class="facet_cat" data-field='+fieldName+' style="width:100px"><b>'+
							'(-&#8734, '+info[j+1].category_name+') </b></td> <td align=right>'+info[j].category_value+'</td></tr>';
					}else if (j == info.length-1){
						htmlVal += '<tr><td class="facet_cat" data-field='+fieldName+' style="width:100px"><b>'+ 
							'['+info[j].category_name+', +&#8734) </b></td> <td align=right>'+info[j].category_value+'</td></tr>';
					}else{
						htmlVal += '<tr><td class="facet_cat" data-field='+fieldName+' style="width:100px"><b>'+
							'['+info[j].category_name+', ' + info[j+1].category_name+') ' +' </b></td> <td align=right>'+info[j].category_value+'</td></tr>';
					}
				}else{
				}
		        }
			htmlVal += "</table>";
		        var fcontainer = document.getElementById("facet_"+fieldName);
		        fcontainer.innerHTML = htmlVal;
		}else{
		        var info = row.facet_info;
		        var htmlVal = "";
			htmlVal += "<table>";
		        for (var j=0;j<info.length;j++){
		            htmlVal += '<tr><td class="facet_cat" data-field='+fieldName+'><b>'+info[j].category_name+'</b></td> <td align=right>'+info[j].category_value+'</td></tr>';
		        }
			htmlVal += "</table>"
		        var fcontainer = document.getElementById("facet_"+fieldName);
		        fcontainer.innerHTML = htmlVal;
		}
            }
            _srch2_this.bindFacetCat();
        }else{
            var element = document.getElementById("resultContainer");
            element.innerHTML = "No results mate!";
            _srch2_this.log("empty response","debug");
        }
    },
    getKeywordString : function(){
        var queryBoxValue = document.getElementById('query_box').value;
        queryBoxValue = queryBoxValue.trim();
        return queryBoxValue;
    },
    findPrefix : function(keywords,prefix){
        prefix = prefix.toLocaleLowerCase();
        for(var index in keywords){
            if(keywords[index].localeCompare(prefix)==0){
                return true
            }
        }
        return false;
    },
    regexMatch : function(prefixArray, word){
        var prefixStr = prefixArray.join("|^");
        var re = new RegExp('^'+prefixStr,'i');
        var m = re.exec(word);
        var returnObj = {};
        if(m==null){
            _srch2_this.log("no match","debug");
            returnObj.success = false;
            return returnObj;
        }else{
            var first_part = word.substring(0,m[0].length);
            var second_part = word.substring(m[0].length); 
            returnObj.success=true;
            returnObj.first = first_part;
            returnObj.second = second_part;
            returnObj.matched = m[0];

            if(_srch2_this.findPrefix(_srch2_this.queryKeywords,returnObj.matched)){
                // exact
                if(word.length==m[0].length){
                    // exact complete
                    returnObj.class = 'exact_complete';
                }else{
                    // exact prefix
                    returnObj.class = 'exact_prefix';
                }
            }else{
                // fuzzy
                if(word.length==m[0].length){
                    // fuzzy complete
                    returnObj.class = 'fuzzy_complete';
                }else{
                    // fuzzy prefix
                    returnObj.class = 'fuzzy_prefix';
                }
            }
            return returnObj;facet_remove_filter
        }
    },
    addHighliting : function(prefix, input){
        if(input){
            input = input.toString();
            var words = input.split(/[ ]+/);// split by space(s)
            var output = "";
            for (var wordIndex in words){
                var word = words[wordIndex];
                var tempWord = word.toLocaleLowerCase();
                var match = _srch2_this.regexMatch(prefix,word);
                var space = "";
                if(wordIndex>0){
                    space = " ";
                }
                if(match.success){
                    var first_part = match.first;
                    var second_part = match.second;
                    var classStr = match.class;
                    output += space+"<span class='"+classStr+"'>"+first_part+"</span>"+second_part;
                }else{
                        output += space+word;
                }
            }
            return output;
        }
        return input;
    }
};



