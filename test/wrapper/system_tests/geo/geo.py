#using python geo.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

sys.path.insert(0, 'srch2lib')
import test_lib

#the function of checking the results
def checkResult(query, responseJson, resultValue):
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
def prepareQuery(ct_lat,ct_long,ct_radius):
    query = ''
    ################# GEO parameters
    query = query + 'radius=' + ct_radius
    query = query + '&clat=' + ct_lat
    query = query + '&clong=' + ct_long
    #print 'Query : ' + query
    ##################################
    return query


def testGeo(queriesAndResultsPath, binary_path):
    # Start the engine server
    args = [ binary_path, './geo/conf.xml', './geo/conf-A.xml', './geo/conf-B.xml' ]

    serverHandle = test_lib.startServer(args)
    if serverHandle == None:
        return -1

    #construct the query
    failCount = 0
    radius = 0.25
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value = line.split('||')
        queryGeo = value[0].split('+')
        resultValue=(value[1]).split()
        #construct the query
        query = prepareQuery(queryGeo[1],queryGeo[0],str(radius))
        response_json = test_lib.searchRequest(query)
      
        #check the result
        failCount += checkResult(query, response_json['results'], resultValue )

    test_lib.killServer(serverHandle)
    return failCount

if __name__ == '__main__':   
    #Path of the query file
    #  each line like "-149.880918+61.155358||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCount = testGeo(queriesAndResultsPath, binary_path)
    os._exit(exitCount)
