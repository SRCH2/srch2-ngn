#! /usr/bin/python

import sys,shutil, urllib2, json, time, subprocess, os, commands, signal, re

sys.path.insert(0, 'srch2lib')
import test_lib

greenColor = "\x1B[32;40m"
redColor = "\x1B[31;40m"
endColor = "\x1B[0m"

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
                print query + redColor + ' test failed' + endColor
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']['id']) + '||' + resultValue[i]
                break
    else:
        isPass=0
        print query + redColor + ' test failed' + endColor + ' - differing response lengths'
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
        print  query + greenColor + ' test PASS' + endColor
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
    

def startCluster(binary_path):
    #Start the engine server
    args = [ binary_path, './acl-distributed/recordAcl/conf-acl-1.xml', './acl-distributed/recordAcl/conf-acl-2.xml', './acl-distributed/recordAcl/conf-acl-3.xml' ]
    serverHandle = test_lib.startServer(args)
    return serverHandle

def loadInitialData(corename, dataFile):
    f = open(dataFile)
    records  = f.readlines()
    f.close()
    for rec in records:
        print '-------------------------------------------------'
        response = test_lib.insertRequest(rec, corename)
        print response
    return 0
    

def loadAcl(corename, aclFile):
    f = open(aclFile)
    records  = f.readlines()
    f.close()
    for rec in records:
        print '-------------------------------------------------'
        query = 'http://localhost:8087/' + corename + '/aclRecordRoleAppend'
        print query
        payload = rec.strip('\n')
        try:
            request = urllib2.Request(query, data=payload)
            request.get_method = lambda: 'PUT'
            opener = urllib2.build_opener(urllib2.HTTPHandler)
            url = opener.open(request)
        except urllib2.HTTPError as e:
            print e.read()
            test_lib.killServer(serverHandle)
            return 1
    return 0

def testMultipleCores(queriesAndResultsPath):
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
            		query = prepareQuery(queryValue[1], queryValue[2], False)
            		response_json = test_lib.searchRequest(query, queryValue[0])

            		#check the result
            		failCount += checkResult(query, response_json['results'], resultValue)

	else:
		# the line is command query (insert/delete/update/acl etc)
		coreName = value[1]
                command = value[2]
                payload = value[3]
                
                test_lib.sendPutRequest(command, payload, coreName)
                time.sleep(1)


    print '=============================='
    return failCount

if __name__ == '__main__':      
    if(os.path.exists("./acl-distributed/recordAcl/core1Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core1Data")
    if(os.path.exists("./acl-distributed/recordAcl/core2Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core2Data")
    if(os.path.exists("./acl-distributed/recordAcl/core3Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core3Data")
    if(os.path.exists("./acl-distributed/recordAcl/core4Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core4Data")
    #Path of the query file
    #each line like "core1 trust 1000||01c90b4effb2353742080000" ---- coreName query roleId||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    # start cluster
    serverHandleList = startCluster(binary_path)

    if serverHandleList == None:
       os._exit(-1)
    # wait for load balancing before sending the queries. 
    time.sleep(40)
    # load data
    loadInitialData("core1", "./acl-distributed/recordAcl/core1/movie-data.json")
    loadInitialData("core2", "./acl-distributed/recordAcl/core2/stackoverflow-data-100.json")
    loadInitialData("core3", "./acl-distributed/recordAcl/core3/business-directory-data.json")
    loadInitialData("core4", "./acl-distributed/recordAcl/core4/movie-data.json")
    time.sleep(10)

    loadAcl("core1", "./acl-distributed/recordAcl/core1/acl-data1.json")
    loadAcl("core3", "./acl-distributed/recordAcl/core3/acl-data3.json")
    time.sleep(10)

    exitCode = testMultipleCores(queriesAndResultsPath)

    if(os.path.exists("./acl-distributed/recordAcl/core1Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core1Data")
    if(os.path.exists("./acl-distributed/recordAcl/core2Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core2Data")
    if(os.path.exists("./acl-distributed/recordAcl/core3Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core3Data")
    if(os.path.exists("./acl-distributed/recordAcl/core4Data")):
        shutil.rmtree("./acl-distributed/recordAcl/core4Data")

    test_lib.kill9Server(serverHandleList)
    os._exit(exitCode)
