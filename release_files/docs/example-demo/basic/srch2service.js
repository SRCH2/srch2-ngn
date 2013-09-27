srch2service = {
    init : function(config) {
        /*
         function to do the init stuff like binding the events and
         reading the config object.
         */
        _srch2_this = this;
        defaultConf = _srch2_this.getDefaultConfig();
        $.extend(defaultConf, config);
        _srch2_this.config = defaultConf;
        _srch2_this.bind();
        _srch2_this.addScriptTag("");
    },
    clean : function() {
        document.getElementById(_srch2_this.config.resultContainer).innerHTML = "";
    },
    getDefaultConfig : function() {
        /*
         returns the default config object
         */
        var config = {
            enableLog : true, // true / false
            logType : "console", // console or alert. Note: console.log might not work on IE.
            inputElementId : "input",
            getQueryString : _srch2_this.getQueryString,
            debug : true, // true /false
            evenNameToBindInputElement : "keyup",
            searchType : "getAll",
            isFuzzy : true,
            resultContainer : 'results',
            responseHandler : 'srch2service.responseHandlerDefault', // this is a string. just give the name of your function as a string.
            cleanup : _srch2_this.clean // this is a function reference. Do not specify a string.
        };
        return config;
    },
    addScriptTag : function(src) {
        /*
         dynamically adds the script tag to the head of the document.
         This is done to make a jsonp call to our server
         */
        var script = document.createElement('script');
        script.id = "srch2ResponseHandlerScriptTagId";
        script.src = src;
        document.getElementsByTagName('head')[0].appendChild(script);
        return false;
    },
    removeScriptTag : function() {
        /*
         removes the script tag added by addScriptTag.
         this is done to remove the unwanted or stale script tags from the document.
         */
        var script = document.getElementById("srch2ResponseHandlerScriptTagId");
        script.parentNode.removeChild(script);
        return false;
    },
    log : function(msg, debug) {
        /*
         handy debug function. use this instead of using alers or console.log
         it is easy to switch off all the logging from the config object.
         */
        if (_srch2_this.config.enableLog == true) {
            if (debug == "debug") {
                if (!_srch2_this.config.debug) {
                    return;
                }
            }
            if (_srch2_this.config.logType == "alert") {
                alert(msg);
            } else {
                console.log(msg);
            }
        }
    },
    bind : function() {
        /*
         controller function to call the bindEvent function
         it reads the input field element id from the configurations and then binds the event as
         specified in the configurations.
         */
        _srch2_this.log("binding events", "debug");
        var element = document.getElementById(_srch2_this.config.inputElementId);
        _srch2_this.bindEvent(element, _srch2_this.config.evenNameToBindInputElement, function(event) {
            if (event.keyCode == 13) {
                return;
            }
            _srch2_this.log("pressed", "debug");
            var query = _srch2_this.config.getQueryString({});
            _srch2_this.jsonpCall(query);
        });
    },
    jsonpCall : function(query) {
        /*
         * appends the query to the server host and port string and calls the addScriptTag function to execute the jsonP call.
         */
        if (query != "") {
            _srch2_this.log("inside jsonPcall, query is " + query, "debug");
            _srch2_this.removeScriptTag();
            var src = encodeURI(_srch2_this.config.jsonPsrc + query + "&callback=" + _srch2_this.config.responseHandler);
            _srch2_this.addScriptTag(src);
            _srch2_this.log("jsonpCall over", "debug");
        } else {
            _srch2_this.log("empty query", "debug");
            // call cleanup
            _srch2_this.config.cleanup();
        }
    },
    bindEvent : function(element, type, handler) {
        /*
         * binds the specified handler function to the specified event type on the element
         */
        if (element.addEventListener) {
            element.addEventListener(type, handler, false);
        } else {
            element.attachEvent('on' + type, handler);
        }
    },
    responseHandler : function(responseText) {
        /*
         * the jsonp response handler function.
         * populates the various sections of the html page.
         * users will have to write their own response handler based on their html page.
         */
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
            _srch2_this.queryKeywords = responseText.query_keywords;
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted;width:9%'>" + "<img style='width:100%; height:auto' src='" + record.banner_url + "'></td>";
                output += "<td style='border-bottom:thin dotted'>" + _srch2_this.addHighliting(prefix, record.title) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix, record.genre) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix, record.director) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + _srch2_this.addHighliting(prefix, record.year) + "</td>";
                output += "</tr>";
            }
            output += "</table>";
            _srch2_this.log("got it", "debug");
            _srch2_this.log(JSON.stringify(responseText), "debug");
            var element = document.getElementById(_srch2_this.config.resultContainer);
            if (output == "") {
                element.innerHTML = "No results mate!";
            } else {
                element.innerHTML = output;
            }
        } else {
            var element = document.getElementById(_srch2_this.config.resultContainer);
            element.innerHTML = "No results mate!";
            _srch2_this.log("empty response", "debug");
        }
    },
    responseHandlerDefault : function(responseText) {
        /*
         * the jsonp response handler function.
         * populates the various sections of the html page.
         * users will have to write their own response handler based on their html page.
         */
        if (responseText) {
            var output = "";
            var results = responseText.results;
            output += "<table width='450px'>";
            _srch2_this.queryKeywords = responseText.query_keywords;
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted;width:9%'>" + "<img style='width:100%; height:auto' src='" + record.banner_url + "'></td>";
                output += "<td style='border-bottom:thin dotted;font-weight:bold'>" + _srch2_this.addHighliting(prefix, JSON.stringify(record)) + "</td>";
                output += "</tr>";
            }
            output += "</table>";
            _srch2_this.log("got it", "debug");
            _srch2_this.log(JSON.stringify(responseText), "debug");
            var element = document.getElementById(_srch2_this.config.resultContainer);
            if (output == "") {
                element.innerHTML = "No results mate!";
            } else {
                element.innerHTML = output;
            }
        } else {
            var element = document.getElementById(_srch2_this.config.resultContainer);
            element.innerHTML = "No results mate!";
            _srch2_this.log("empty response", "debug");
        }
    },
    findPrefix : function(keywords, prefix) {
        /*
         * helper function that finds the prefix in the given keywords.
         */
        prefix = prefix.toLocaleLowerCase();
        for (var index in keywords) {
            if (keywords[index].localeCompare(prefix) == 0) {
                return true;
            }
        }
        return false;
    },
    regexMatch : function(prefixArray, word) {
        /*
         * helper function that matches the prefixes in the input word as received in prefixArray.
         */
        var prefixStr = prefixArray.join("|^");
        var re = new RegExp('^' + prefixStr, 'i');
        var m = re.exec(word);
        var returnObj = {};
        if (m == null) {
            _srch2_this.log("no match", "debug");
            returnObj.success = false;
            return returnObj;
        } else {
            var first_part = word.substring(0, m[0].length);
            var second_part = word.substring(m[0].length);
            returnObj.success = true;
            returnObj.first = first_part;
            returnObj.second = second_part;
            returnObj.matched = m[0];

            if (_srch2_this.findPrefix(_srch2_this.queryKeywords, returnObj.matched)) {
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
        /*
         * highlights the prefix found in input string by adding span elements around the prefixes in the input string.
         * uses a helper function : regexMatch to match the prefix in the giving input.
         */
        if (input) {
            input = input.toString();
            var words = input.split(/[ ]+/);
            // split by space(s)
            var output = "";
            for (var wordIndex in words) {
                var word = words[wordIndex];
                var tempWord = word.toLocaleLowerCase();
                var match = _srch2_this.regexMatch(prefix, word);
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
    getQueryString : function() {
        // 1. generate the query
        var query = '';
        if (_srch2_this.config.searchType.length > 0) {
            query += 'searchType=' + _srch2_this.config.searchType + '&';
        }
        // 1.1 . main query
        query += 'q={defaultPrefixComplete=COMPLETE}';
        var queryBoxValue = document.getElementById(_srch2_this.config.inputElementId).value;
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
        if (_srch2_this.config.isFuzzy) {
            query += '&fuzzy=true';
        }
        return query;
    }
};
