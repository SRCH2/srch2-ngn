#This test is used for checking the case where there is only 1 searchable attribute or only 1 refining attribute in the schema, set as primary key field.
#Two config files are used here, conf.xml uses only 1 refining field which is set to the primary key
# conf1.xml uses only 1 searchable field set to the primary key

#To run individual test case
#python ./refining_field_primary_key/testPrimaryKey.py $SRCH2_ENGINE ./refining_field_primary_key/queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            #print response_json['results'][i]['record']['id']
            if (resultValue.count(responseJson[i]['record']['id']) != 1):
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

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


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
    query = query + '&fuzzy=false'
    
#    print 'Query : ' + query
    ##################################
    return query
    


def testRefiningPrimaryKey(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, './refining_field_primary_key/conf.xml','./refining_field_primary_key/conf-A.xml' ,'./refining_field_primary_key/conf-B.xml'  ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query = prepareQuery(queryValue) 
        response_json = test_lib.searchRequest(query)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount


def testSearchablePrimaryKey(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, './refining_field_primary_key/conf1.xml','./refining_field_primary_key/conf1-A.xml','./refining_field_primary_key/conf1-B.xml' ]

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    #construct the query
    failCount = 0
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
        queryValue=value[0].split()
        resultValue=(value[1]).split()
        #construct the query
        query = prepareQuery(queryValue)
        response_json = test_lib.searchRequest(query)

        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    print '=============================='
    return failCount



if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testRefiningPrimaryKey(queriesAndResultsPath, binary_path)
    import time
    time.sleep(5)
    exitCode = testSearchablePrimaryKey(queriesAndResultsPath, binary_path)
    os._exit(exitCode)
