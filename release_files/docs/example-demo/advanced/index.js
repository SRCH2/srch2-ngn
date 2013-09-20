client = {
    init : function() {
        _client_this = this;
        _client_this.bindSortEvents();
        _client_this.bindFacetCat();
        _client_this.bindFacetFilterRemove();

    },
    bindSortEvents : function() {
        _client_this.log("binding sort event", "debug");
        var elements = document.getElementsByName('sort_filter_order');
        for (var j = 0; j < elements.length; j++) {
            _client_this.log("button clicked", "debug");
            _client_this.bindEvent(elements[j], 'click', function(event) {
                _client_this.log("clicked", "debug");
                var query = _client_this.getQueryString({});
                srch2service.jsonpCall(query);
            });
        }
    },
    log : function(msg, debug) {
        /*
         handy debug function. use this instead of using alers or console.log
         */
        if (debug == "debug") {
            console.log(msg);
        }
    },
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
                var query = _client_this.getQueryString(params);
                srch2service.jsonpCall(query);
            });
        }
    },
    bindConfigEvents : function() {
        var element = document.getElementById("toggleDebug");
        _client_this.bindEvent(element, 'click', function(event) {
            _client_this.config.debug = !_client_this.config.debug;
        });

        var element = document.getElementById("toggleLogOutput");
        _client_this.bindEvent(element, 'click', function(event) {
            if (_client_this.config.logType == "alert") {
                _client_this.config.logType = "console";
            } else {
                _client_this.config.logType = "alert";
            }
        });

        var element = document.getElementById("toggleEnableLogs");
        _client_this.bindEvent(element, 'click', function(event) {
            _client_this.config.enableLog = !_client_this.config.enableLog;
        });

    },
    bindFacetFilterRemove : function() {
        var element = document.getElementById("facet_remove_filter");
        _client_this.bindEvent(element, 'click', function(event) {
            element.innerHTML = "";
            localStorage.setItem('facetFilter', "NULL");
            var query = _client_this.getQueryString({});
            srch2service.jsonpCall(query);
        });
    },
    bindEvent : function(element, type, handler) {
        if (element.addEventListener) {
            element.addEventListener(type, handler, false);
        } else {
            element.attachEvent('on' + type, handler);
        }
    },
    responseHandler : function(responseText) {
        if (responseText) {
            var output = "";
            var results = responseText.results;
            output += "<table width='450px'>";
            output += "<tr>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>  </td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Title' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Genre' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Director' + "</td>";
            output += "<td style='border-bottom:thick;font-weight:bold;'>" + 'Year' + "</td>";
            output += "</tr>";
            _client_this.queryKeywords = responseText.query_keywords;
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted;width:9%'>" + "<img style='width:100%; height:auto' src='" + record.banner_url + "'></td>";
                output += "<td style='border-bottom:thin dotted'>" + _client_this.addHighliting(prefix, record.title) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighliting(prefix, record.genre) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighliting(prefix, record.director) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _client_this.addHighliting(prefix, record.year) + "</td>";
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
            _client_this.bindFacetCat();
        } else {
            var element = document.getElementById("resultContainer");
            element.innerHTML = "No results mate!";
            _client_this.log("empty response", "debug");
        }
    },
    findPrefix : function(keywords, prefix) {
        prefix = prefix.toLocaleLowerCase();
        for (var index in keywords) {
            if (keywords[index].localeCompare(prefix) == 0) {
                return true
            }
        }
        return false;
    },
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
    addHighliting : function(prefix, input) {
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
    getQueryString : function(params) {

        // 1. generate the query
        var query = '';
        // 1.1 . main query
        query += 'searchType=getAll&q={defaultPrefixComplete=COMPLETE}';
        var queryBoxValue = document.getElementById('query_box').value;
        queryBoxValue = queryBoxValue.trim();
        //if(queryBoxValue == "") return;
        if (queryBoxValue == "") {
            // no keyword, return empty
            return "";
        } else {
            var keywords = queryBoxValue.split(" ");
            for (var i = 0; i < keywords.length; i++) {
                if (i == keywords.length - 1) {
                    query += keywords[i] + "*" + "~";
                } else {
                    query += keywords[i] + "~" + " AND ";
                }
            }
        }

        query += '&fuzzy=true';
        // 1.2 . facets
        query += '&facet=true';
        var attributes = ['year', 'genre'];
        for (var i = 0; i < attributes.length; i++) {
            if (attributes[i] == 'genre') {
                query += '&facet.field=genre';
            } else {
                query += '&facet.range=' + attributes[i];
                var start = document.getElementById('facet_' + attributes[i] + '_start').value;
                var end = document.getElementById('facet_' + attributes[i] + '_end').value;
                var gap = document.getElementById('facet_' + attributes[i] + '_gap').value;
                if (start != "") {
                    query += '&f.' + attributes[i] + '.facet.start=' + start;
                }
                if (end != "") {
                    query += '&f.' + attributes[i] + '.facet.end=' + end;
                }
                if (gap != "") {
                    query += '&f.' + attributes[i] + '.facet.gap=' + gap;
                }
            }

        }
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
            query += '&sort=year&orderby=' + value;
        }

        // 1.4 . filter query
        var filterQueryIsthere = 0;
        var fqAssField = document.getElementById('filter_assignment_field').value;
        var fqAssValue = document.getElementById('filter_assignment_value').value;
        var fqAss = "";
        if (!(fqAssField == "" || fqAssValue == "")) {
            filterQueryIsthere += 1;
            fqAss = fqAssField + ':' + fqAssValue;
        }

        var fqRngField = document.getElementById('filter_range_field').value;
        var fqRngStart = document.getElementById('filter_range_start').value;
        var fqRngEnd = document.getElementById('filter_range_end').value;
        var fqRng = "";
        if (!(fqRngField == "" || fqRngStart == "" || fqRngEnd == "")) {
            filterQueryIsthere += 1;
            fqRng = fqRngField + ':[ ' + fqRngStart + ' TO ' + fqRngEnd + ' ]';
        }

        var fqComplex = document.getElementById('filter_complex').value;
        var fqCmp = "";
        if (!(fqComplex == "" )) {
            filterQueryIsthere += 1;
            fqCmp = "boolexp$" + fqComplex + "$";
        }
        var op = "";
        if (filterQueryIsthere == 1) {
            query += '&fq=' + fqAss + fqRng + fqCmp;
        } else if (filterQueryIsthere > 1) {

            var opRadios = document.getElementsByName('filter_op');
            for (var i = 0; opRadios.length; i++) {
                if (opRadios[i].checked) {
                    op = opRadios[i].value;
                    break;
                }
            }
            if (fqAss == "") {
                query += '&fq=' + fqRng + " " + op + " " + fqCmp;
            } else if (fqRng == "") {
                query += '&fq=' + fqAss + " " + op + " " + fqCmp;
            } else if (fqCmp == "") {
                query += '&fq=' + fqAss + " " + op + " " + fqRng;
            } else {
                query += '&fq=' + fqAss + " " + op + " " + fqRng + " " + op + " " + fqCmp;
            }
        }

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
                query += " " + op + " " + fqStr;
            } else {
                query += '&fq=' + fqStr;
                isFqSet = true;
            }
            localStorage.setItem('facetFilter', fqStr);
        } else if (localStorage.getItem('facetFilter') != "NULL") {
            var fqStr = localStorage.getItem('facetFilter');
            if (filterQueryIsthere > 0) {
                query += " " + op + " " + fqStr;
            } else {
                query += '&fq=' + fqStr;
                isFqSet = true;
            }
        }
        return query;
    }
};

