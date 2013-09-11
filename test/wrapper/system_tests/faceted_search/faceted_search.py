#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

port = '8081'

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    while os.system(info) != 0:
        time.sleep(1)
        info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

#the function of checking the results
def checkResult(query, responseJsonAll,resultValue, facetResultValue):
    responseJson = responseJsonAll['results']
    isPass=1
    if  len(responseJson) == len(resultValue):
         for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['id'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                 print responseJson[i]['record']['id']+'||'
            elif i >= len(responseJson):
                 print '  '+'||'+resultValue[i]
            else:
                 print responseJson[i]['record']['id']+'||'+resultValue[i]

    isPass = checkFacetResults(query , responseJsonAll['facets'] , facetResultValue)

    if isPass == 1:
        print  query+' test pass'

def checkFacetResults(query, responseJson, resultValue):
   if len(responseJson) != len(resultValue):
      return False
   for i in range(0,len(responseJson)):
      facet_line = ''
      facet_field_name = responseJson[i]['facet_field_name']
      facet_line = facet_line + facet_field_name + '||'
      facet_info = responseJson[i]['facet_info']
      for j in range(0,len(facet_info)):
         facet_line = facet_line + facet_info[j]['category_name'] + ',' + str(facet_info[j]['category_value']) + '|'
      if resultValue[i] != facet_line:
         print resultValue[i]
         print 'vs.'
         print facet_line
         return False
   return True


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    for i in range(0, len(queryKeywords)):
        if i == (len(queryKeywords)-1):
            query=query+queryKeywords[i]+'*' # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'

    ################# fuzzy parameter
    query = query + '&fuzzy=true'
    ################# facet parameters
    query = query + '&facet=true&facet.field=model&facet.range=price&facet.range=likes'
    ################# rows parameter
    query = query + '&rows=1'
    print 'Query : ' + query
    ##################################
    return query

def testFacetedSearch(queriesAndResultsPath , facetResultsPath, binary_path):
    # Start the engine server
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=/home/jamshid/workspace-srch2-v4/srch2-ngn/test/wrapper/system_tests/faceted_search/conf.ini &'
    print 'starting engine: ' + binary 
    os.popen(binary)
    #make sure that start the engine up
    pingServer()

    #construct the query

    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryValue)
        #print query
        

        # get facet correct result from file
        f_facet = open(facetResultsPath , 'r')
        facetResultValue = []
        for facet_line in f_facet:
            facetResultValue.append(facet_line.strip())
            
        # do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
      
        #check the result
        checkResult(query, response_json, resultValue , facetResultValue )

    #get pid of srch2-search-server and kill the process
    s = commands.getoutput('ps aux | grep srch2-search-server')
    stat = s.split() 
    os.kill(int(stat[1]), signal.SIGUSR1)
    print '=============================='

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   binary_path = sys.argv[1]
   queriesAndResultsPath = sys.argv[2]
   facetResultsPath = sys.argv[3]
   testFacetedSearch(queriesAndResultsPath, facetResultsPath, binary_path)

