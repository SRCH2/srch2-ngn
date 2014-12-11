#This case tests  whether the last character is a whitespace (which is transformed from "+")
#For example, for a query "q=trus+", we will take "trus" as a complete term.

#using: python term_type.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

port = '8087'

#Function of checking the results
def checkResult(query, responseJson, resultValue):
    isPass = 1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if (resultValue.count(responseJson[i]['record']['id']) != 1):
                isPass = 0
                print query + ' test failed'
                print 'query results||given results'
                print 'number of results:' + str(len(responseJson)) + '||' + str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print responseJson[i]['record']['id'] + '||' + resultValue[i]
                break
    else:
        isPass = 0
        print query + ' test failed'
        print 'query results||given results'
        print 'number of results:' + str(len(responseJson)) + '||' + str(len(resultValue))
        maxLen = max(len(responseJson), len(resultValue))
        for i in range(0, maxLen):
            if i >= len(resultValue):
                 print responseJson[i]['record']['id'] + '||'
            elif i >= len(responseJson):
                 print '  ' + '||' + resultValue[i]
            else:
                 print responseJson[i]['record']['id'] + '||' + resultValue[i]

    if isPass == 1:
        print  query + ' test pass'
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
        # first extract the filters
        queryTermParts = queryKeywords[i].split(':')
        fieldFilter = ''
        if len(queryTermParts) == 2:
            fieldFilter = queryTermParts[1] + '%3A'
        keyword = queryTermParts[0]
        # now add them to the query
        if i == (len(queryKeywords) - 1):
            if keyword != '+':
                query = query + fieldFilter + keyword + '*' # last keyword prefix
            else:
                query = query[:-9] # remove the last inserted %20AND%20
        else:
            query = query + fieldFilter + keyword + '%20AND%20'

    #print 'Query : ' + query
    ##################################
    return query

def testTermType(queriesAndResultsPath, conf, binary_path):
	#Start the engine server
	args = [ binary_path, '--config-file=' + conf ]
        if test_lib.confirmPortAvailable(port) == False:
            print 'Port ' + str(port) + ' already in use - aborting'
            return -1

	serverHandle = test_lib.startServer(args)
	test_lib.pingServer(port)

	#construct the query
	failCount = 0
	f_in = open(queriesAndResultsPath, 'r')
	for line in f_in:
	    #get the query keyword and results
	    value = line.split('||')
	    queryValue = value[0].split()
	    resultValue = (value[1]).split()
	    #construct the query
            query = 'http://localhost:' + port + '/search?'
	    query = query + prepareQuery(queryValue)
	    #do the query
	    response = urllib2.urlopen(query).read()
	    response_json = json.loads(response)

	    #check the result
	    failCount += checkResult(query, response_json['results'], resultValue)

	f_in.close();
	print '=============================='
	test_lib.killServer(serverHandle)
	return failCount

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]    
    queriesAndResultsPath = sys.argv[2]  
  
    exitCode = testTermType(queriesAndResultsPath, './term_type/conf.xml', binary_path)
    time.sleep(5)
    print '--------Term type test  for attribute_based_search--------------'  
    exitCode += testTermType(queriesAndResultsPath, './term_type/conf_for_attribute_based_search.xml', binary_path)
    os._exit(exitCode)

