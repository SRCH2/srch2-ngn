
// Check http://srch2.com/releases/4.4.2/docs/ for more information
// about the API of the SRCH server.

srch2 = {
    init : function(serverSetting){
        this.jsonpScriptId = "srch2ResponseHandlerScriptTagId";
        this.addScriptTag("");
        this.debugMode = false;
        this.queryWaitingForResponse = 0;
        this.serverUrlStr = null;
        this.searchFieldsStr = null;
        this.isEnablePrefixSearch = null;
        this.isEnableFuzzySearch = null;
        this.fuzzySimilarityThreshold = null;
        this.filterQueryStr = null;
        this.fieldListStr = null;
        this.facetTypeStr = null;
        this.facetFieldStr = null;
        this.facetCategoryRows = null;
        this.facetRangeCategoryStr = null;
        this.facetRangeStartStr = null;
        this.facetRangeEndStr = null;
        this.facetRangeGapStr = null;
        this.searchTypeStr = null;
        this.sortStr = null;
        this.orderByStr = null;
        this.startStr = null;
        this.rowsStr = null;
        this.coreNameStr = null;
        this.roleIdStr = null;

        this.jsonpCallbacks = {counter : 0};
        
        if(serverSetting != null){
            this.setServerSetting(serverSetting);
        }        
    },
    
    setServerSetting : function(serverSetting){
        if(serverSetting.defaultResultContainer != null && serverSetting.defaultResultContainer != ""){
            this.defaultResultContainer = serverSetting.defaultResultContainer;
        }

        if(serverSetting.serverUrl != null && serverSetting.serverUrl != ""){
            this.setServerUrl(serverSetting.serverUrl);
        }

        if(serverSetting.debug != null){
            if(serverSetting.debug == true || serverSetting.debug == "true"){
                this.debugMode = true;
            }
            if(serverSetting.debug == false || serverSetting.debug == "false"){
                this.debugMode = false;
            }
        }
    },
    
    setEnableDebugMode : function(enableDebugMode){
        if(enableDebugMode == "true" || enableDebugMode == true){
            this.debugMode = true;
        }else if(enableDebugMode == "false" || enableDebugMode == false){
            this.debugMode = false;
        }
        
        return this;
    },
    
    /********************************************************/
    // It sends a query using the pre-defined parameters in the
    // ```init()``` function and specifies a callback function
    // "responseHandler" for handling the response.
    sendQuery : function(keyword, responseHandler){
        // deal with the query
        if (this.queryWaitingForResponse == 0) {
            // first request without facets for speed                                                                                  
            var query = this.getQueryString(keyword);
            if (query && query.length > 0) {
                if(this.jsonpCallWithCallbackFuntion(query, responseHandler)){
                    //this.queryWaitingForResponse++;   //400 bad request for input value: ^ 
                    //this.log("Query waiting for response : " + this.queryWaitingForResponse);
                }            
            }
            return true;
        } else {
            return false;
        }
    },
    
    //Do not use the params setting. Directly use the keyword for advanced search 
    //TODO : no multi core
    sendRawQuery : function(keyword, responseHandler){
        if (this.queryWaitingForResponse == 0) {
            if (keyword && keyword.length > 0) {
                if(this.jsonpCall(this.serverUrlStr + "search?" + encodeURIComponent(keyword), responseHandler)){
                    //this.queryWaitingForResponse++;
                    //this.log("Query waiting for response : " + this.queryWaitingForResponse);
                }            
            }
            return true;
        } else {
            return false;
        }
    },
    
    /*
     * Set Server Url. For example : http://localhost:8087/
     */
    setServerUrl : function(serverUrl){
        this.serverUrlStr = serverUrl;

        if(serverUrl != null && serverUrl != "" && serverUrl.charAt(serverUrl.length -1) != '/'){
            this.serverUrlStr += "/";
        }    
        return this;
    },
    
    // It sets the fields in which the keywords have to appear.
    setSearchFields : function(searchFields){
        this.searchFieldsStr = encodeURIComponent(searchFields);
        return this;
    },


    // It turns on/off prefix search for each keyword.  If enabled,
    // each keyword by default is treated as a prefix.
    setEnablePrefixSearch : function(isEnablePrefixSearch){
        this.isEnablePrefixSearch = isEnablePrefixSearch;
        return this;
    },

    // It turns on/off fuzzy search for each keyword. If it is on, the
    // fuzzy similarity can  be set in the second parameter.
    setEnableFuzzySearch : function(isEnableFuzzySearch, fuzzySimilarityThreshold){
        this.isEnableFuzzySearch = isEnableFuzzySearch;
        if(typeof(fuzzySimilarityThreshold) != undefined){
            this.fuzzySimilarityThreshold = fuzzySimilarityThreshold;
        }
        return this;
    },

    // This parameter is used to specify a filter restricting the set
    // of records to be returned.
    setFilterQueryParam : function(filterQuery){
        if(filterQuery == null || filterQuery == ""){
            this.filterQueryStr = null;
            return this;
        }
        this.filterQueryStr = "fq=" + encodeURIComponent(filterQuery);
        return this;
    },

    // This query parameter is used to specify fields the server
    // should return for each result.
    setFieldList : function(fieldList){ 
        this.fieldListStr = '';

        // Clean field list string
        if(fieldList == null || fieldList == ""){
            this.fieldListStr = null;
            return this;
        }

        for(var i = 0 ; i < fieldList.length ; i++){
            if(i == 0){
                this.fieldListStr += "fl=" + encodeURIComponent(fieldList[i]);
            }else{
                this.fieldListStr += "," + encodeURIComponent(fieldList[i]);
            }
        }
        return this;
    },

    /*
     * This value is "true", "false" or "only", indicating whether we want to enable faceting. 
     */
    enableFacetParam : function(facetType){
        if(facetType == "true" || facetType == true){
            this.facetTypeStr = "facet=true";
        }else if(facetType == "false" || facetType == false){
            this.facetTypeStr = "facet=false";
        }else if(facetType == "only"){
            this.facetTypeStr = "facet=only";
        }else{
            this.facetTypeStr = null;
        }
        return this;
    },

    // This parameter specifies a field to be treated as a categorical
    // facet.  The server finds the number of distinct values in the
    // specified field and returns the number of records for each
    // value. This parameter can be specified with multiple values in
    // a list to indicate multiple facet fields. 
    setFacetFieldList : function(facetFieldList){
        this.facetFieldStr = "";

        // Clean facet field list string
        if(facetFieldList == null || facetFieldList == ""){
            this.facetFieldStr = null;
            return this;
        }

        for(var i = 0; i < facetFieldList.length ; i++){
            if(i == 0){
                this.facetFieldStr += "facet.field=" + encodeURIComponent(facetFieldList[i]);
            }else{
                this.facetFieldStr += "&facet.field=" + encodeURIComponent(facetFieldList[i]);
            }
        }
        return this;
    },

    // This function sets the maximum number of categories with
    // maximal frequencies to be returned for a given field.  All
    // categories are returned by default. For example, adding the
    // following condition ```f.genre.rows=10``` to the query will
    // tell the engine to return the top 10 most popular genres.
    setFacetCategoryRows : function(category, rows){
        this.facetCategoryRows = "";

        if(typeof(category) == undefined || typeof(rows) == undefined
            || category == null || rows == null || category == "" || rows == ""){
            this.facetCategoryRows = null;
            return this;
        }

        this.facetCategoryRows = "f." + encodeURIComponent(category) + ".rows=" + encodeURIComponent(rows);
        return this;
    },

    // These parameters can be used to specify a field that should be
    // treated as a range facet.
    setFacetRange : function(category, start, end, gap){
        if(category == null || category == ""){
            this.facetRangeCategoryStr = null;
            return this;
        }

        this.facetRangeCategoryStr = "facet.range=" + encodeURIComponent(category);

        if(typeof(start) != undefined && start != ""){
            this.facetRangeStartStr = "f." + encodeURIComponent(category) + ".facet.start=" + encodeURIComponent(start);
        } else {
            this.facetRangeStartStr = null;
        }

        if(typeof(end) != undefined && end != ""){
            this.facetRangeEndStr = "f." + encodeURIComponent(category) + ".facet.end=" + encodeURIComponent(end);
        } else {
            this.facetRangeEndStr = null;
        }


        if(typeof(gap) != undefined && gap != ""){
            this.facetRangeGapStr = "f." + encodeURIComponent(category) + ".facet.gap=" + encodeURIComponent(gap);
        } else {
            this.facetRangeGapStr= null;
        }
        return this;
    },

    // The engine supports two different strategies to do search: 
    // (1) "topK": The results will be sorted in the descending order
    // by their score.  This approach has a high performance, but does
    // not support facet and sort operations.  (2) "getAll": Use this
    // strategy if facets and sort query parameters are needed. 
    setSearchType : function(searchType){
        if(searchType == null || searchType == ""){
            this.searchTypeStr = null;
            return this;
        }

        if(searchType == "topK"){
            this.searchTypeStr = "searchType=topK";
        }

        if(searchType == "getAll"){
            this.searchTypeStr = "searchType=getAll";
        }
        return this;
    },

    // The engine's default behavior is to sort the results using a 
    // descending order by the overall score of each record.  
    // The user can specify sorting by other fields, e.g.,
    // ```sort=director,year,title```. 
    setSortList : function(categoryList){
        this.sortStr = '';

        // Clean field list string
        if(categoryList == null || categoryList == ""){
            this.sortStr = null;
            return this;
        }

        for(var i = 0 ; i < categoryList.length ; i++){
            if(i == 0){
                this.sortStr += "sort=" + encodeURIComponent(categoryList[i]);
            }else{
                this.sortStr += "," + encodeURIComponent(categoryList[i]);
            }
        }
        return this;
    },

    // It specifies the order in which the result set should be
    // sorted.  Its default value is "desc" (descending). This order
    // is valid for all the fields specified in the "sort" parameter.
    setOrderBy : function(orderBy){
        if(orderBy == null || orderBy == ""){
            this.orderByStr = null;
            return this;
        }

        if(orderBy == "asc"){
            this.orderByStr = "orderby=asc";
        }

        if(orderBy == "desc"){
            this.orderByStr = "orderby=desc";
        }
        return this;
    },

    // It specifies the offset in the complete result set of the
    // query,  where the set of returned records should begin.
    setStart : function(start){
        if(start == null || start == ""){
            this.startStr = null;
            return this;
        }
        this.startStr = "start=" + encodeURIComponent(start);
        return this;
    },

    // The parameter indicates the number of records to return from
    // the complete result set.
    setRows : function(rows){
        if(rows == null || rows == ""){
            this.rowsStr = null;
            return this;
        }
        this.rowsStr = "rows=" + encodeURIComponent(rows);
        return this;
    },

    // The "Cores" tag set in the server configuration file allows the
    // user to search on multiple "cores" within the same server. A
    // query can specify a particular core. Here is an example query:
    // ```http://localhost:8081/example-core/search?q=term```.
    // If a user wants to get results from all the cores, the query 
    // should add a prefix "/_all/search" to the request, e.g., 
    // ```http://localhost:8081/_all/search?q=martn~```.
    setSearchCore : function(coreName){
        if(coreName == null || coreName == ""){
            this.coreNameStr = null;
            return this;
        }
        this.coreNameStr = encodeURIComponent(coreName) + "/";
        return this;
    },

    // The function specifies the "role" of the user who issues the
    // query. This parameter is used for record-based access control or
    // attribute-based access control. 
    setRoleId : function(roleId){
        if(roleId == null || roleId == ""){
            this.roleIdStr = null;
            return this;
        }
        this.roleIdStr = "roleId=" + encodeURIComponent(roleId);
        return this;
    },

    // This function sets all the parameters to null.
    clearAllParams : function(){
        this.serverUrlStr = null;
        this.searchFieldsStr = null;
        this.isEnablePrefixSearch = null;
        this.isEnableFuzzySearch = null;
        this.fuzzySimilarityThreshold = null;
        this.filterQueryStr = null;
        this.fieldListStr = null;
        this.facetTypeStr = null;
        this.facetFieldStr = null;
        this.facetCategoryRows = null;
        this.facetRangeCategoryStr = null;
        this.facetRangeStartStr = null;
        this.facetRangeEndStr = null;
        this.facetRangeGapStr = null;
        this.searchTypeStr = null;
        this.sortStr = null;
        this.orderByStr = null;
        this.startStr = null;
        this.rowsStr = null;
        this.coreNameStr = null;
        this.roleIdStr = null;
        return this;
    },

    /********************************************************/

    /*
     * Dynamically adds the script tag to the head of the document.
     * This triggers a jsonp call to our server.
     */    
    addScriptTag : function(url){
        if (url) {
            var script = document.createElement('script');
            script.id = this.jsonpScriptId;
            script.src = url;
            script.value = 'alert(' + script.toString() + ');';
            var head = document.getElementsByTagName('head')[0];

            if (head) {
                head.appendChild(script);
            } else {
                this.log("Cannot get head!!");
            }
        }
            return false;
    },

    /*
     * Removes the script tag added by addScriptTag.
     * This triggers the removal of the unwanted or stale script tags
     * from the document.
     */
    removeScriptTag : function() {
        var script = document.getElementById(this.jsonpScriptId);
        if (script) {
            script.parentNode.removeChild(script);
        }
        return false;
    },

    /*
     * Build srch2 query string from input element
     */   
    getQueryString : function(keyword, params){
        // Generate the query
        var query = "";

        // Append Server URL
        if(this.serverUrlStr == null || this.serverUrlStr == ""){
            this.log("Server Url is empty!");
            return "";
        }

        query += this.serverUrlStr;

        // Check and append if multi core
        if(this.coreNameStr != null && this.coreNameStr != ""){
            query += this.coreNameStr;
        }

        // Append search header
        query += "search?";

        // Append keyword
        query += "q=";

        // Append search field 
        if(this.searchFieldsStr != null && this.searchFieldsStr != ""){
            query += encodeURIComponent(this.searchFieldsStr) + ":"
        }

        // grab search terms from input control on web page
        var words = [];

        // massage (clean up) user input before building query string
        if (keyword && keyword.length > 0 ) {
            keyword = keyword.trim();
            //TODO : Do not support phrase search 
            keyword = keyword.replace(/\s+/g, " ");
            words = keyword.split(" ");
        } else {
            this.log("No input from which to build search query!");
            return '';
        }

        // append search terms to query, with query syntax
        for (var i = 0; i < words.length; i++) {
            //Skip boolean search operator
            //if (words[i] == "AND" || words[i] == "OR" || words[i] == "NOT"){
            //    query += " " + encodeURIComponent(words[i]) + " ";
            //    continue;
            //}

            if (i == words.length - 1) {
                words[i] = encodeURIComponent(words[i]);
                // last word in search terms
                query += words[i].replace(/[+]/g, '%2B'); // encode here to handle terms like C++
                if (keyword[keyword.length - 1] != ' ') {
                    // prefix search unless input ends in a space
                    if(this.isEnablePrefixSearch == true){
                        query += '*';
                    }                
                }

                if(this.isEnableFuzzySearch == true){
                    if(this.fuzzySimilarityThreshold != null && this.fuzzySimilarityThreshold != ""){
                        query += '~' + this.fuzzySimilarityThreshold;
                    }else {
                        query += '~';
                    }

                }            
            } else {
                // not the last word in search terms
                words[i] = encodeURIComponent(words[i]);
                if(this.isEnableFuzzySearch == true){
                    if(this.fuzzySimilarityThreshold != null && this.fuzzySimilarityThreshold != ""){
                        query += words[i].replace(/[+]/g, '%2B') + '~' + this.fuzzySimilarityThreshold + ' AND ';
                    }else{
                        query += words[i].replace(/[+]/g, '%2B') + '~' + ' AND ';
                    }

                } else {
                    query += words[i].replace(/[+]/g, '%2B') + ' AND ';
                }            
            }
        }

        // Append search type
        if(this.searchTypeStr != null){
            query += "&" + this.searchTypeStr;
        }

        // Append filter query parameter
        if(this.filterQueryStr != null && this.filterQueryStr != ""){
            query += "&" + this.filterQueryStr;
        }

        // Append filter field list 
        if(this.fieldListStr != null && this.fieldListStr != ""){
            query += "&" + this.fieldListStr;
        }

        //Append facet 
        if(this.facetTypeStr != null && this.facetTypeStr != ""){
            query += "&" + this.facetTypeStr;
        }

        //Append facet field
        if(this.facetFieldStr != null && this.facetFieldStr != ""){
            query += "&" + this.facetFieldStr;
        }

        //Append facet rows
        if(this.facetCategoryRows != null && this.facetCategoryRows != ""){
            query += "&" + this.facetCategoryRows;
        }

        //Append facet range
        if(this.facetRangeCategoryStr != null && this.facetRangeCategoryStr != ""){
            query += "&" + this.facetRangeCategoryStr;
        }

        if(this.facetRangeStartStr != null && this.facetRangeStartStr != ""){
            query += "&" + this.facetRangeStartStr;
        }

        if(this.facetRangeEndStr != null && this.facetRangeEndStr != ""){
            query += "&" + this.facetRangeEndStr;
        }

        if(this.facetRangeGapStr != null && this.facetRangeGapStr != ""){
            query += "&" + this.facetRangeGapStr;
        } 

        //Append sort
        if(this.sortStr != null && this.sortStr != ""){
            query += "&" + this.sortStr;
        }

        //Append orderby
        if(this.orderByStr != null && this.orderByStr != ""){
            query += "&" + this.orderByStr;
        }

        //Append start
        if(this.startStr != null && this.startStr != ""){
            query += "&" + this.startStr;
        }

        //Append rows
        if(this.rowsStr != null && this.rowsStr != ""){
            query += "&" + this.rowsStr;
        }

        //Append roleId
        if(this.roleIdStr != null && this.roleIdStr != ""){
            query += "&" + this.roleIdStr;
        }

        this.log(query);
        return query;
    },

    /* JSONP call with callback function */
    jsonpCallWithCallbackFuntion : function(query, responseHandler){
        var name = "srch2_" + this.jsonpCallbacks.counter++;

        this.jsonpCallbacks[name] = function(response){
            //srch2.queryWaitingForResponse--;
            responseHandler(response);
            delete srch2.jsonpCallbacks[name];
            //srch2.log("Query waiting for response : " + srch2.queryWaitingForResponse);
        }

        return this.jsonpCall(query, "srch2.jsonpCallbacks." + name + "");
    },

    /* make query to srch2 server */
    jsonpCall : function(query, responseHandlerName) {
        if (query && query != "") {
            this.removeScriptTag();
            // query must be already URI encoded!!!!
            var url = query + encodeURI("&callback=" + responseHandlerName);
            this.addScriptTag(url);
            return true;
        } else {
            this.log("empty query");
            return false;
        }
    },

    /* got a server response, check if we have backlogged requests to resume sending */
    resumeServerRequests : function(inputElement, response) {

    },

    log : function(s) {
        if (this.debugMode == true) {
            console.log("srch2 log : " + s);
        }
    },

    responseHandlerDefault : function(responseText) {
        /*
         * the jsonp response handler function.
         * populates the various sections of the html page.
         * users will have to write their own response handler based on their html page.
         */
        if(this.defaultResultContainer == null){
            this.log("defaultResultContainer is not set.");
            return;
        }
        if (responseText) {
            var output = "";
            var results = responseText.results;
            output += "<table width='450px'>";
            this.queryKeywords = responseText.query_keywords;
            for (var i = 0; i < results.length; i++) {
                output += "<tr class='result_row'>";
                var record = results[i].record;
                var prefix = results[i].matching_prefix;
                output += "<td style='border-bottom:thin dotted;width:9%'>" + "<img style='width:100%; height:auto' src='" + record.banner_url + "'></td>";
                output += "<td style='border-bottom:thin dotted'>" + this.addHighlighting(prefix, JSON.stringify(record)) + "</td>";
                output += "</tr>";
            }
            output += "</table>";
            var element = document.getElementById(this.defaultResultContainer);
            if (output == "") {
                element.innerHTML = "No results mate!";
            } else {
                element.innerHTML = output;
            }
        } else {
            var element = document.getElementById(this.defaultResultContainer);
            element.innerHTML = "No results mate!";
            this.log("empty response");
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
            this.log("no match");
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

    addHighlighting : function(prefix, input) {
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
}









