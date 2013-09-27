#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal, argparse

port = '8081'
numberOfFacetFields= 3;

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    print info 
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

facetParamNameTuple= ('start', 'end', 'gap')

#prepare a facet parameter for the query based on list entry:
def prepareFacet(fieldTuple):
    query = 'facet.'
    query =  query + ('field' if (len(fieldTuple) == 1) else 'range')
    query = query + '=' + fieldTuple[0]
    i=0
    for value in fieldTuple:
        query = query + '&f.' + fieldTuple[0] + '.'
        query = query + facetParamNameTuple[i]
        ++i
        query = query + '=' + value
    return query

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords, facetedFields):
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
    query = query + '&facet=true'
    for field in facetedFields:
        query = query + '&' + prepareFacet(field)
    ################# rows parameter
    query = query + '&rows=1'
    print 'Query : ' + query
    ##################################
    return query

def testFacetedSearch(f_in , f_facet, binary_path, facetedFields):
    # Start the engine server
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=./faceted_search/conf.xml &'
    print 'starting engine: ' + binary 
    os.popen(binary)
    #make sure that start the engine up
    pingServer()

    #construct the query
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query='http://localhost:' + port + '/search?'
        query = query + (prepareQuery(queryValue, facetedFields))
        print query
        #print query
        

        # get facet correct result from file
        facetResultValue=[]
        for i in xrange(0, numberOfFacetFields):
            facetResultValue.append(f_facet.next().strip())
           
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
   parser= argparse.ArgumentParser()
   parser.add_argument('--srch2', required=True,
                                                dest='binary_path')
   parser.add_argument('--qryNrlst', type=file, required=True,
                                                dest='queriesAndResults')
   parser.add_argument('--frlst', type=file, required=True,
                                                dest='facetResults')
   parser.add_argument('-f',  metavar='facet', nargs='+', 
                                                    action='append')

   args= parser.parse_args()
   print args.f
   binary_path = args.binary_path
   queriesAndResultsPath = args.queriesAndResults
   facetResultsPath = args.facetResults
   testFacetedSearch(queriesAndResultsPath, facetResultsPath, binary_path, args.f)

