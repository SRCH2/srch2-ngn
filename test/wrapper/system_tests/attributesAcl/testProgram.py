# Automated test script to start engine, fire queryes and verify results
import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if responseJson[i]['record_id'] !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record_id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
             print responseJson[i]['record_id']+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print responseJson[i]['record_id']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1

#verify facet reults by matching input facet fields in response JSON
#for this test case it is enough to check whether 'facets' is present in response JSON from engine.
def checkFacetResults(query, response_json, facetFields):

    if len(facetFields) == 0  and 'facets' not in response_json:
        print query + ' PASS!'
        return 0

    if len(facetFields) == 0 and 'facets' in response_json:
        print query + ' FAIL! ...facet found when not expected'
        return 1

    if len(facetFields) > 0 and 'facets' not in response_json:
        print query + ' FAIL! ...no facet found'
        return 1

    facetResults = response_json['facets']

    if len(facetFields) != len(facetResults):
        print query + ' FAIL! ..facet mismtach'
        return 1

    for facetField in facetFields:
        facetMatch = False
        for facetResult in facetResults:
            if facetField == facetResult['facet_field_name']:
                facetMatch = True
                break
        if not facetMatch:
            print query + ' FAIL! ..facet field not found '
            return 1

    print query + ' PASS!'
    return 0

def checkFieldsInResults(query, response_results, fields, key):
    for results in  response_results:
        keyValues = results[key]
        if len(keyValues) != len(fields):
            print query + ' Fail! ...fields mimatch in ' + key + ' field of the response'
            print str(len(keyValues)) + '||' + str(len(fields))
            return 1

        for field in fields:
            if field not in keyValues:
                print query + ' Fail! ...field not found in ' + key + ' field of the response'
                return 1

    print query + ' PASS!'
    return 0

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
            query=query+queryKeywords[i] # last keyword prefix
        else:
            query=query+queryKeywords[i]+'%20AND%20'
    return query
    
def test(queriesAndResultsPath, binary_path, configFilePath):
    #Start the engine server
    args = [ binary_path, '--config-file=' + configFilePath]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port, 'q=garbage', 30)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #if line is empty ignore
        if len(line) == 0:
            continue
        #ignore comments
        if line[0] == '#':
            continue
        #get the query keyword and results
        value=line.split('||')
        if len(value) < 2:
            continue # ignore bad line
        if value[0] == 'C':
            # the line is command query (insert/delete/update/acl etc)
            command = value[1]
            payload = value[2]
            coreName = ''
            if len(value) > 3:
                coreName = value[3].strip('\n').strip()

            if coreName == "":
                query='http://localhost:' + port + '/' + command
            else:
                query='http://localhost:' + port + '/' + coreName + '/' + command
            print query
            request = urllib2.Request(query, data=payload)
            request.get_method = lambda: 'PUT'
            opener = urllib2.build_opener(urllib2.HTTPHandler)
            url = opener.open(request)
            time.sleep(1)
        else:
            # the line is a search query
            queryValue=value[1].split()
            resultValue=(value[2]).split()
            coreName = ''
            if len(value) > 3:
                coreName = value[3].strip('\n').strip()

            #construct the query
            if coreName == '':
                query='http://localhost:' + port + '/search?'
            else:
                query='http://localhost:' + port + '/' + coreName  + '/' + 'search?'
            query = query + prepareQuery(queryValue) 
            print query
            #do the query
            response = urllib2.urlopen(query).read()
            response_json = json.loads(response)

            #check the result
            if value[0] == 'F': # test case for facet query
                failCount += checkFacetResults(query, response_json, resultValue )
            elif value[0] == 'H': # test case for checking highlighting only
                failCount += checkFieldsInResults(query, response_json['results'], resultValue, 'snippet')
            elif value[0] == 'R': # test case for only checking fields in response
                failCount += checkFieldsInResults(query, response_json['results'], resultValue, 'record' )
            else:
                failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    binary_path = sys.argv[1]
    queriesAndResultsPath = './attributesACL/testCases.txt'
    exitCode = test(queriesAndResultsPath, binary_path , './attributesAcl/conf.xml')
    time.sleep(2)
    queriesAndResultsPath = './attributesACL/testCasesMultiCore.txt'
    exitCode |= test(queriesAndResultsPath, binary_path, './attributesAcl/conf-multicore.xml')
    time.sleep(2)
    queriesAndResultsPath = './attributesACL/testCasesFilterSortFacetQuery.txt'
    exitCode |= test(queriesAndResultsPath, binary_path , './attributesAcl/conf2.xml')
    os._exit(exitCode)

