#! /usr/bin/python

import sys,shutil, urllib2, json, time, subprocess, os, commands, signal, re

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

# This test case reads data from the json files
# Then it reads all the access control data from json files too
# Then it does some search and it uses roleId in the query
# And all the results should have this roleId in their access list
# it reads the keywords and role ids from queriesAndResults.txt file
# the format of each line in this file is like:
#       coreName keyword roleid || results
# example :  core1 hello 103 || 12 14 18

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
                    print str(responseJson[i]['record']['id']) + '||' + resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed - differing response lengths'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                print str(responseJson[i]['record']['id'])+'||'
            elif i >= len(responseJson):
                print '  '+'||'+resultValue[i]
            else:
                print responseJson[i]['record']['id']+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1


#prepare the query based on the valid syntax
def prepareQuery(queryKeyword, roleId, fuzzy):
    query = ''
    #################  prepare main query part
    query = query + 'q='
    # local parameters
#    query = query + '%7BdefaultPrefixComplete=COMPLETE%7D'
    # keywords section
    if fuzzy:
            keyword = queryKeyword + '~'
    else:
            keyword = queryKeyword

    query=query+keyword+'&'
    
#    print 'Query : ' + query
    ##################################
    query = query + 'roleId=' + roleId
    return query
    


def testMultipleCores(queriesAndResultsPath, binary_path):
    #Start the engine server
    args = [ binary_path, '--config-file=./access_control/conf-acl.xml' ]

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        return -1

    print 'starting engine: ' + args[0] + ' ' + args[1]
    serverHandle = test_lib.startServer(args)

    test_lib.pingServer(port)
    failCount = 0

    print "Test core1 - access control"
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #get the query keyword and results
        value=line.split('||')
	if(value[0] == 'S'):
        	queryValue=value[1].split(' ')
        	allResults=value[2].split('@')
        	for coreResult in allResults:
            		resultValue=coreResult.split()
            		#construct the query
            		query='http://localhost:' + port + '/' + queryValue[0] + '/search?'
            		query = query + prepareQuery(queryValue[1], queryValue[2], False)

            		#do the query
            		response = urllib2.urlopen(query).read()

            		response_json = json.loads(response)

            		#check the result
            		failCount += checkResult(query, response_json['results'], resultValue)

	else:
		# the line is command query (insert/delete/update/acl etc)
		coreName = value[1]
                command = value[2]
                payload = value[3]
                
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


    time.sleep(5)
    test_lib.killServer(serverHandle)

    print '=============================='
    return failCount

if __name__ == '__main__':      
    if(os.path.exists("./access-control/core1Data")):
        shutil.rmtree("./access-control/core1Data")
    if(os.path.exists("./access-control/core2Data")):
        shutil.rmtree("./access-control/core2Data")
    if(os.path.exists("./access-control/core3Data")):
        shutil.rmtree("./access-control/core3Data")
    if(os.path.exists("./access-control/core4Data")):
        shutil.rmtree("./access-control/core4Data")
    #Path of the query file
    #each line like "core1 trust 1000||01c90b4effb2353742080000" ---- coreName query roleId||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    exitCode = testMultipleCores(queriesAndResultsPath, binary_path)
    if(os.path.exists("./access-control/core1Data")):
        shutil.rmtree("./access-control/core1Data")
    if(os.path.exists("./access-control/core2Data")):
        shutil.rmtree("./access-control/core2Data")
    if(os.path.exists("./access-control/core3Data")):
        shutil.rmtree("./access-control/core3Data")
    if(os.path.exists("./access-control/core4Data")):
        shutil.rmtree("./access-control/core4Data")
    os._exit(exitCode)
