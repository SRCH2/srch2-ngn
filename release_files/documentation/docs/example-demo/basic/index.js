/*
 * This is the basic demo for srch2 javascript library.
 * To use the srch2lib, please visit:
 *  http://srch2.com/releases/4.4.2/docs/library/
 *
 * For more information about Search API, please visit: 
 *  http://srch2.com/releases/4.4.2/docs/restful-search/
 */
client = {
    init : function() {
        /*
         * Initialize the srch2lib, serverUrl must be set.
         */ 
        var config = {
            serverUrl : "http://127.0.0.1:8081/",
            debug : true, // enable the debug mode which will display the debug msg to the console. 
                            //IE may not have "console.log" function. If you are using IE, please set it to false.
        };
        srch2lib.init(config);  //Initialize the srch2lib with config.
        srch2lib.setSearchType("getAll");   //Set the search type to "getAll"
        srch2lib.setEnablePrefixSearch(true);   //Enable the prefix search 
        srch2lib.setEnableFuzzySearch(true);    //Enable the fuzzy search
        this.bindInput();
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
     * Bind the input box "query_box" key up event with 
     * a function which sending the query to the server. 
     */
    bindInput : function() {
        var element = document.getElementById("query_box");
        this.bindEvent(element, "keyup", function(event) {
            if (event.keyCode == 13) {
                return;
            }
            var query = client.sendQuery();
        });
    },
    /*
     * "sendQuery" is called by "bindInput".
     * This function gets the keyword from the "query_box"
     * and send it to the server. The response results 
     * will trigger the function "this.responseHandler".
     */
    sendQuery : function() {
        var keyword = document.getElementById('query_box').value;
        srch2lib.sendQuery(keyword, this.responseHandler);
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
            output += "</tr>";
            client.queryKeywords = responseText.query_keywords;
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted'>" + client.addHighlighting(prefix, record.title) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + client.addHighlighting(prefix, record.genre) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + client.addHighlighting(prefix, record.director) + "</td>";
                output += "<td style='border-bottom:thin dotted '>" + client.addHighlighting(prefix, record.year) + "</td>";
                output += "</tr>";
            }
            output += "</table>";
            client.log("got it", "debug");
            client.log(JSON.stringify(responseText), "debug");
            var element = document.getElementById("resultContainer");
            if (output == "") {
                element.innerHTML = "No results mate!";
            } else {
                element.innerHTML = output;
            }
        } else {
            var element = document.getElementById("resultContainer");
            element.innerHTML = "No results mate!";
            client.log("empty response", "debug");
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
                var match = this.regexMatch(prefix, word);
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
    regexMatch : function(prefixArray, word) {
        var prefixStr = prefixArray.join("|^");
        var re = new RegExp('^' + prefixStr, 'i');
        var m = re.exec(word);
        var returnObj = {};
        if (m == null) {
            this.log("no match", "debug");
            returnObj.success = false;
            return returnObj;
        } else {
            var first_part = word.substring(0, m[0].length);
            var second_part = word.substring(m[0].length);
            returnObj.success = true;
            returnObj.first = first_part;
            returnObj.second = second_part;
            returnObj.matched = m[0];

            if (this.findPrefix(this.queryKeywords, returnObj.matched)) {
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
    findPrefix : function(keywords, prefix) {
        prefix = prefix.toLocaleLowerCase();
        for (var index in keywords) {
            if (keywords[index].localeCompare(prefix) == 0) {
                return true
            }
        }
        return false;
    },
};

