/*
 * This is the advanced demo for srch2 javascript library.
 * To use the srch2lib, please visit:
 *  http://srch2.com/releases/4.4.2/docs/library/
 *
 * For more information about Search API, please visit: 
 *  http://srch2.com/releases/4.4.2/docs/restful-search/
 */
client = {
    init : function() {
        _client_this = this;
        
        /*
         * Initialize the srch2lib, serverUrl must be set.
         */ 
        this.srch2 = srch2lib;
        var config = {
            serverUrl : "http://127.0.0.1:8081/",
            debug : true, // true /false
        };        
        this.srch2.init(config);  //Initialize the srch2lib with config.
        _client_this.srch2.setServerUrl("http://localhost:8081/");  
        //_client_this.srch2.setServerUrl("http://simpson.calit2.uci.edu/srch2_movies_engine/");  //Server URL also can be set by setServerUrl
        _client_this.srch2.setSearchType("getAll"); //Set the search type to "getAll"
        _client_this.srch2.setEnablePrefixSearch(true); //Enable the prefix search 
        _client_this.srch2.setEnableFuzzySearch(true);  //Enable the fuzzy search
        
        /*
         * Set facet parameters, for more information, please visit : 
         * http://srch2.com/releases/4.4.2/docs/restful-search/#7-facet-parameters
         */
        _client_this.srch2.enableFacetParam(true);//Enable Facet
        _client_this.srch2.setFacetFieldList(['genre']);    //Set the facet field list
        
        _client_this.bindInput();
        _client_this.bindSortEvents();
        _client_this.bindFacetCat();
        _client_this.bindFacetFilterRemove();


    },
    log : function(msg, debug) {
        /*
         handy debug function. use this instead of using alers or console.log
         */
        if (debug == "debug") {
            console.log(msg);
        }
    },
    /*
     * Bind the "Ascending", "Descending" buttons click event with 
     * a function which sending the query with order parameters to the server. 
     */    
    bindSortEvents : function() {
        _client_this.log("binding sort event", "debug");
        var elements = document.getElementsByName('sort_filter_order');
        for (var j = 0; j < elements.length; j++) {
            _client_this.log("button clicked", "debug");
            _client_this.bindEvent(elements[j], 'click', function(event) {
                _client_this.log("clicked", "debug");
                _client_this.sendQuery({});
            });
        }
    },
    /*
     * Bind the facet category buttons click event with 
     * a function which sending the query with facet parameters to the server. 
     */    
    bindFacetCat : function() {
        _client_this.log("binding facet_cat click event", "debug");
        var elements = document.getElementsByClassName('facet_cat');
        for (var j = 0; j < elements.length; j++) {
            _client_this.log("binding facet_cat_index, j is " + j, "debug");
            _client_this.bindEvent(elements[j], 'click', function(event) {
                _client_this.log("clicked", "debug");
                var fqObj = {};
                var element = event.srcElement
                var dataset = element.dataset;
                if (Object.keys(dataset).length === 0) {
                    dataset = element.parentElement.dataset;
                }
                fqObj.field = dataset.field;
                if (fqObj.field == 'year') {
                    value = element.innerHTML;
                    fqObj.valueLeft = value.split(",")[0];
                    fqObj.valueRight = value.split(" ")[1];
                    fqObj.valueLeft = fqObj.valueLeft.substring(1);
                    if (fqObj.valueLeft.indexOf('b>') != -1) {
                        fqObj.valueLeft = fqObj.valueLeft.substring(2);
                        //alert(fqObj.valueLeft + " TO "+fqObj.valueRight);
                    }
                    fqObj.valueRight = fqObj.valueRight.substring(0, fqObj.valueRight.length - 1);
                    if (fqObj.valueLeft.charAt(0) == '-') {
                        fqObj.valueLeft = "*";
                    }
                    if (fqObj.valueRight.charAt(0) == '+') {
                        fqObj.valueRight = "*";
                    }
                } else {
                    value = element.innerHTML;
                    if (value.indexOf('<b>') != -1) {
                        value = value.substring(3, value.length - 4);
                        //alert(value);
                    }
                    fqObj.valueLeft = value;
                    fqObj.valueRight = value;

                }
                document.getElementById('facet_remove_filter').innerHTML = "<button >Remove Facet Filtering</button>";
                var params = {};
                params['fq'] = fqObj;
                _client_this.sendQuery(params);
            });
        }
    },
    /*
     * Bind the button "facet_remove_filter" click event with 
     * a function which sending the query to the server. 
     */    
    bindFacetFilterRemove : function() {
        var element = document.getElementById("facet_remove_filter");
        _client_this.bindEvent(element, 'click', function(event) {
            element.innerHTML = "";
            localStorage.setItem('facetFilter', "NULL");
            _client_this.sendQuery({});
        });
    },
    /*
     * Bind the input box "query_box" key up event with 
     * a function which sending the query to the server. 
     */
    bindInput : function() {
        var element = document.getElementById("query_box");
        _client_this.bindEvent(element, "keyup", function(event) {
            if (event.keyCode == 13) {
                return;
            }
            _client_this.sendQuery({});
        });
    },
    /*
     * Bind the event "type" to the HTTP element "element",
     * if the "type" event is triggered, call the function "handler".
     */
    bindEvent : function(element, type, handler) {
        if (element.addEventListener) {
            element.addEventListener(type, handler, false);
        } else {
            element.attachEvent('on' + type, handler);
        }
    },
    /*  
     * "responseHandler" is called by the server response.
     * The "responseText" contains all the results in JSON format. 
     */
    responseHandler : function(responseText) {
        if (responseText) {
            var output = "";
            var results = responseText.results;
            output += "<table width='450px'>";
            output += "<tr>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Title' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Genre' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Director' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Year' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + ' ' + "</td>";
            output += "</tr>";
            _client_this.queryKeywords = responseText.query_keywords;

            
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var recordPrimaryKey = results[i].record_id;
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted'>" + _client_this.addHighlighting(prefix, record.title) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighlighting(prefix, record.genre) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighlighting(prefix, record.director) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighlighting(prefix, record.year) + "</td>";
                output += "<td style='border-bottom:thin dotted '><input type='submit' value='Like' onclick='_client_this.sendFeedback(\""+ recordPrimaryKey + "\")'></td>";
                output += "</tr>";
            }
            output += "</table>";
            _client_this.log("got it", "debug");
            _client_this.log(JSON.stringify(responseText), "debug");
            var element = document.getElementById("resultContainer");
            if (output == "") {
                element.innerHTML = "No results mate!";
            } else {
                element.innerHTML = output;
            }
            // populate facets
            var facets = responseText.facets;
            if( facets == null ){
                return;
            }
            for (var i = 0; i < facets.length; i++) {
                var row = facets[i];
                var fieldName = row.facet_field_name;
                if (fieldName == 'year') {
                    var info = row.facet_info;
                    var htmlVal = "";
                    htmlVal += "<table>";
                    for (var j = 0; j < info.length; j++) {
                        if (info.length > 1) {
                            if (j == 0) {
                                htmlVal += '<tr><td class="facet_cat" data-field=' + fieldName + ' style="width:100px"><b>' + '(-&#8734, ' + info[j + 1].category_name + ') </b></td> <td align=right>' + info[j].category_value + '</td></tr>';
                            } else if (j == info.length - 1) {
                                htmlVal += '<tr><td class="facet_cat" data-field=' + fieldName + ' style="width:100px"><b>' + '[' + info[j].category_name + ', +&#8734) </b></td> <td align=right>' + info[j].category_value + '</td></tr>';
                            } else {
                                htmlVal += '<tr><td class="facet_cat" data-field=' + fieldName + ' style="width:100px"><b>' + '[' + info[j].category_name + ', ' + info[j + 1].category_name + ') ' + ' </b></td> <td align=right>' + info[j].category_value + '</td></tr>';
                            }
                        } else {
                        }
                    }
                    htmlVal += "</table>";
                    var fcontainer = document.getElementById("facet_" + fieldName);
                    fcontainer.innerHTML = htmlVal;
                } else {
                    var info = row.facet_info;
                    var htmlVal = "";
                    htmlVal += "<table>";
                    for (var j = 0; j < info.length; j++) {
                        htmlVal += '<tr><td class="facet_cat" data-field=' + fieldName + '><b>' + info[j].category_name + '</b></td> <td align=right>' + info[j].category_value + '</td></tr>';
                    }
                    htmlVal += "</table>";
                    var fcontainer = document.getElementById("facet_" + fieldName);
                    fcontainer.innerHTML = htmlVal;
                }
            }
            _client_this.bindFacetCat();    //Bind the facet category buttons with click event.
        } else {
            var element = document.getElementById("resultContainer");
            element.innerHTML = "No results mate!";
            _client_this.log("empty response", "debug");
        }
    },
    /*
     * Helper function to highlight the prefix keywords  
     */
    findPrefix : function(keywords, prefix) {
        prefix = prefix.toLocaleLowerCase();
        for (var index in keywords) {
            if (keywords[index].localeCompare(prefix) == 0) {
                return true
            }
        }
        return false;
    },
    /*
     * Helper function to highlight the prefix keywords  
     */
    regexMatch : function(prefixArray, word) {
        var prefixStr = prefixArray.join("|^");
        var re = new RegExp('^' + prefixStr, 'i');
        var m = re.exec(word);
        var returnObj = {};
        if (m == null) {
            _client_this.log("no match", "debug");
            returnObj.success = false;
            return returnObj;
        } else {
            var first_part = word.substring(0, m[0].length);
            var second_part = word.substring(m[0].length);
            returnObj.success = true;
            returnObj.first = first_part;
            returnObj.second = second_part;
            returnObj.matched = m[0];

            if (_client_this.findPrefix(_client_this.queryKeywords, returnObj.matched)) {
                // exact
                if (word.length == m[0].length) {
                    // exact complete
                    returnObj.class = 'exact_complete';
                } else {
                    // exact prefix
                    returnObj.class = 'exact_prefix';
                }
            } else {
                // fuzzy
                if (word.length == m[0].length) {
                    // fuzzy complete
                    returnObj.class = 'fuzzy_complete';
                } else {
                    // fuzzy prefix
                    returnObj.class = 'fuzzy_prefix';
                }
            }
            return returnObj; facet_remove_filter
        }
    },
    /*
     * Highlight the prefix in the search results.
     */
    addHighlighting : function(prefix, input) {
        if (input) {
            input = input.toString();
            var words = input.split(/[ ]+/);
            // split by space(s)
            var output = "";
            for (var wordIndex in words) {
                var word = words[wordIndex];
                var tempWord = word.toLocaleLowerCase();
                var match = _client_this.regexMatch(prefix, word);
                var space = "";
                if (wordIndex > 0) {
                    space = " ";
                }
                if (match.success) {
                    var first_part = match.first;
                    var second_part = match.second;
                    var classStr = match.class;
                    output += space + "<span class='" + classStr + "'>" + first_part + "</span>" + second_part;
                } else {
                    output += space + word;
                }
            }
            return output;
        }
        return input;
    },
    /*
     * "sendQuery" is called when the event is triggered.
     * This function gets the keyword, facet, filter and sort 
     * info from the page, and send them to the server.
     * The response results will trigger the function 
     * "this.responseHandler".
     */
    sendQuery : function(params) {
       
        // Get the facet range values from the page and pass it to srch2lib.
        var start = document.getElementById('facet_year_start').value;
        var end = document.getElementById('facet_year_end').value;
        var gap = document.getElementById('facet_year_gap').value;
        _client_this.srch2.setFacetRange("year", start, end, gap);
        
        // Get the order setting from the page and pass it to srch2lib.
        var orderRadios = document.getElementsByName('sort_filter_order');
        var setSort = false;
        for (var j = 0; j < orderRadios.length; j++) {
            if (orderRadios[j].checked) {
                var value = orderRadios[j].value;
                if (value == 'asc') {
                    setSort = true;
                } else if (value == 'desc') {
                    setSort = true;
                }
            }
        }
        
        if (setSort) {
            _client_this.srch2.setSortList(['year']);
            _client_this.srch2.setOrderBy(value);
        }
        
        /*
         * Get the filter info from the page, generate a query, and pass it to the srch2lib.
         * For the syntax of filter query, please refer to : 
         * http://srch2.com/releases/4.4.2/docs/restful-search/#62-fq-filter-query-parameter
         */
         
         // Get and set the filter value
        var filterQuery = "";
        var filterQueryIsthere = 0;
        var fqAssField = document.getElementById('filter_assignment_field').value;
        

        var fqAssValue = document.getElementById('filter_assignment_value').value;
        var fqAss = "";
        if (!(fqAssField == "" || fqAssValue == "")) {
            filterQueryIsthere += 1;
            fqAss = fqAssField + ':' + fqAssValue;
        }

        // Get and set the filter range field
        var fqRngField = document.getElementById('filter_range_field').value;
        var fqRngStart = document.getElementById('filter_range_start').value;
        var fqRngEnd = document.getElementById('filter_range_end').value;
        var fqRng = "";
        if (!(fqRngField == "" || fqRngStart == "" || fqRngEnd == "")) {
            filterQueryIsthere += 1;
            fqRng = fqRngField + ':[ ' + fqRngStart + ' TO ' + fqRngEnd + ' ]';
        }


         // Get and set the boolean expressions
        var fqComplex = document.getElementById('filter_complex').value;
        var fqCmp = "";
        if (fqComplex != null && fqComplex != "" ) {
            filterQueryIsthere += 1;
            fqCmp = "boolexp$" + fqComplex + "$";
        }
        var op = "";
        if (filterQueryIsthere == 1) {
            filterQuery += fqAss + fqRng + fqCmp;
        } else if (filterQueryIsthere > 1) {

            var opRadios = document.getElementsByName('filter_op');
            for (var i = 0; opRadios.length; i++) {
                if (opRadios[i].checked) {
                    op = opRadios[i].value;
                    break;
                }
            }
            if (fqAss == "") {
                filterQuery += fqRng + " " + op + " " + fqCmp;
            } else if (fqRng == "") {
                filterQuery += fqAss + " " + op + " " + fqCmp;
            } else if (fqCmp == "") {
                filterQuery += fqAss + " " + op + " " + fqRng;
            } else {
                filterQuery += fqAss + " " + op + " " + fqRng + " " + op + " " + fqCmp;
            }
        }

        // Append more filters if a function is called when the user
        // clicks the facet category.
        if ("fq" in params) {
            var fqObj = params['fq'];
            var fqStr = fqObj['field'] + ":[ " + fqObj['valueLeft'] + " TO " + fqObj['valueRight'] + " ]";
            if (fqObj['valueLeft'] != fqObj['valueRight']) {
                fqStr += " AND -" + fqObj['field'] + ":" + fqObj['valueRight'];
            }
            if (localStorage.getItem('facetFilter') != "NULL") {
                fqStr += " AND " + localStorage.getItem('facetFilter');
            }
            if (filterQueryIsthere > 0) {
                filterQuery += " " + op + " " + fqStr;
            } else {
                filterQuery += fqStr;
                isFqSet = true;
            }
            localStorage.setItem('facetFilter', fqStr);
        } else if (localStorage.getItem('facetFilter') != "NULL") {
            var fqStr = localStorage.getItem('facetFilter');
            if (filterQueryIsthere > 0) {
                filterQuery += " " + op + " " + fqStr;
            } else {
                filterQuery += fqStr;
                isFqSet = true;
            }
        }
        
        // Pass the filterQuery to srch2lib
        _client_this.srch2.setFilterQueryParam(filterQuery);
        
        // Get the query from the "query_box" and send it to the server.
        var query = document.getElementById('query_box').value;
        _client_this.srch2.sendQuery(query, _client_this.responseHandler);
       
    },

    sendFeedback : function(recordPrimaryKey) {        
        _client_this.log( "sending feedback = { " +  _client_this.srch2.feedbackQueryStr  + "," + recordPrimaryKey + "}", "debug");        
       _client_this.srch2.sendFeedback(_client_this.srch2.feedbackQueryStr, recordPrimaryKey);
    }

};

