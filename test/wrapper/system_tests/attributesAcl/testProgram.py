# Automated test script to start engine, fire queryes and verify results
import sys, shutil, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

greenColor = "\x1B[32;40m"
redColor = "\x1B[31;40m"
endColor = "\x1B[0m"

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
                print query + redColor + ' test failed' + endColor
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record_id']+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query + redColor + ' test failed' + endColor
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
        print  query + greenColor + ' test PASS' + endColor
        return 0
    return 1

#verify facet reults by matching input facet fields in response JSON
#for this test case it is enough to check whether 'facets' is present in response JSON from engine.
def checkFacetResults(query, response_json, facetFields):

    if len(facetFields) == 0  and 'facets' not in response_json:
        print query + greenColor + ' PASS!' + endColor
        return 0

    if len(facetFields) == 0 and 'facets' in response_json:
        print query + redColor + ' FAIL! ...facet found when not expected' + endColor
        return 1

    if len(facetFields) > 0 and 'facets' not in response_json:
        print query + redColor + ' FAIL! ...no facet found' + endColor
        return 1

    facetResults = response_json['facets']

    if len(facetFields) != len(facetResults):
        print query + redColor + ' FAIL! ..facet mismtach' + endColor
        return 1

    for facetField in facetFields:
        facetMatch = False
        for facetResult in facetResults:
            if facetField == facetResult['facet_field_name']:
                facetMatch = True
                break
        if not facetMatch:
            print query + redColor + ' FAIL! ..facet field not found ' + endColor
            return 1

    print query + greenColor + ' PASS!' + endColor
    return 0

def checkFieldsInResults(query, response_results, fields, key):
    for results in  response_results:
        keyValues = results[key]
        if len(keyValues) != len(fields):
            print query + redColor + ' Fail! ...fields mimatch in ' + key + ' field of the response' + endColor
            print str(len(keyValues)) + '||' + str(len(fields))
            return 1

        for field in fields:
            if field not in keyValues:
                print query + redColor + ' Fail! ...field not found in ' + key + ' field of the response' + endColor
                return 1

    print query + greenColor + ' PASS!' + endColor
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

    serverHandle = test_lib.startServer(args, 30)
    if serverHandle == None:
        return -1

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

            test_lib.sendPutRequest(command, payload, coreName)
            time.sleep(1)
        else:
            # the line is a search query
            queryValue=value[1].split()
            resultValue=(value[2]).split()
            coreName = ''
            if len(value) > 3:
                coreName = value[3].strip('\n').strip()

            #construct the query
            response_json = test_lib.searchRequest(prepareQuery(queryValue),coreName)

            #check the result
            if value[0] == 'F': # test case for facet query
                failCount += checkFacetResults(prepareQuery(queryValue), response_json, resultValue )
            elif value[0] == 'H': # test case for checking highlighting only
                failCount += checkFieldsInResults(prepareQuery(queryValue), response_json['results'], resultValue, 'snippet')
            elif value[0] == 'R': # test case for only checking fields in response
                failCount += checkFieldsInResults(prepareQuery(queryValue), response_json['results'], resultValue, 'record' )
            else:
                failCount += checkResult(prepareQuery(queryValue), response_json['results'], resultValue )

    test_lib.kill9Server(serverHandle)
    print '=============================='
    return failCount

if __name__ == '__main__':      
    
    if(os.path.exists("./attributesAcl/SRCH2Cluster")):
        shutil.rmtree("./attributesAcl/SRCH2Cluster")
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    exitCode = 0
    binary_path = sys.argv[1]
    queriesAndResultsPath = './attributesAcl/testCases.txt'
    #exitCode = test(queriesAndResultsPath, binary_path , './attributesAcl/conf.xml')
    #time.sleep(5)
    if(os.path.exists("./attributesAcl/SRCH2Cluster")):
        shutil.rmtree("./attributesAcl/SRCH2Cluster")
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    queriesAndResultsPath = './attributesAcl/testCasesMultiCore.txt'
    #exitCode |= test(queriesAndResultsPath, binary_path, './attributesAcl/conf-multicore.xml')
    #time.sleep(5)
    if(os.path.exists("./attributesAcl/SRCH2Cluster")):
        shutil.rmtree("./attributesAcl/SRCH2Cluster")
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    queriesAndResultsPath = './attributesAcl/testCasesFilterSortFacetQuery.txt'
    exitCode |= test(queriesAndResultsPath, binary_path , './attributesAcl/conf2.xml')
    time.sleep(5)
    if(os.path.exists("./attributesAcl/SRCH2Cluster")):
        shutil.rmtree("./attributesAcl/SRCH2Cluster")
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    queriesAndResultsPath = './attributesAcl/testCasesFilterSortFacetQueryWithSwitch.txt'
    exitCode |= test(queriesAndResultsPath, binary_path , './attributesAcl/conf3.xml')
    os._exit(exitCode)

