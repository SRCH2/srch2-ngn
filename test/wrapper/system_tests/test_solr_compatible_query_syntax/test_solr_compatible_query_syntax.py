#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

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
                    print str(responseJson[i]['record']['id']) + '||' + resultValue[i]
                break
    else:
        isPass=0
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                 print str(responseJson[i]['record']['id']) + '||'
            elif i >= len(responseJson):
                 print '  '+'||'+resultValue[i]
            else:
                 print str(responseJson[i]['record']['id']) + '||' + resultValue[i]
    if isPass == 1:
        isPass = checkFacetResults(query , responseJsonAll['facets'] , facetResultValue)

    if isPass == 1:
        print  query+' test pass'
        return 0
    else:
        print  query+' test failed'
        return 1

def checkFacetResults(query, responseJson, facetResultValue):
   for i in range(0,len(responseJson)):
      facet_line = ''
      facet_field_name = responseJson[i]['facet_field_name']
      facet_line = facet_line + facet_field_name + '||'
      facet_info = responseJson[i]['facet_info']
      for j in range(0,len(facet_info)):
         facet_line = facet_line + facet_info[j]['category_name'] + ',' + str(facet_info[j]['category_value']) + '|'
      if facetResultValue != facet_line:
         print facetResultValue
         print 'vs.'
         print facet_line
         return False
   return True


#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
    query = ''
    #################  prepare main query part
    #query = query + ''
    # local parameters
    query = query + queryKeywords

    #print 'Query : ' + query
    ##################################
    return query

def testNewFeatures(queriesAndResultsPath,facetResultsPath, binary_path):
    # Start the engine server
    args = [ binary_path, '--config-file=./test_solr_compatible_query_syntax/conf.xml' ]
    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)
    #make sure that start the engine up
    test_lib.pingServer(port)

    # get facet correct result from file
    f_facet = open(facetResultsPath , 'r')
    facetResultValue = []
    for facet_line in f_facet:
        facetResultValue.append(facet_line.strip())

    #construct the query
    failCount = 0
    j=0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0]
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryValue)
        #print query
        
        # do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
        #check the result
        failCount += checkResult(query, response_json, resultValue, facetResultValue[j])
        j=j+1
        #print j
        #print '------------------------------------------------------------------'
    print '=============================='
    test_lib.killServer(serverHandle)
    return failCount

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   binary_path = sys.argv[1]
   queriesAndResultsPath = sys.argv[2]
   facetResultsPath=sys.argv[3]
   exitCode = testNewFeatures(queriesAndResultsPath, facetResultsPath, binary_path)
   os._exit(exitCode)

