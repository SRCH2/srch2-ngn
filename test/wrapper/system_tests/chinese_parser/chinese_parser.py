#coding=gbk
#This test tests the chinese query parser
#If query keyword contains more words like "曼联皇马", the engine should 
#split the query keyword by using the chinese_dictionary into "曼联","皇马".
#The test sends query to the server and checks the response "prefix" part to see if the 
#keyword is split by the engine correctly.
 
import sys, json, os, urllib2, shutil
sys.path.insert(0, 'srch2lib')
import test_lib

reload(sys) #reload default decoder   
sys.setdefaultencoding('utf-8') 

port = '8087'
totalFailCount = 0

#Start the SRCH2 engine.
def startSrch2Engine():
	global serverHandle
	#Start the engine server
        args = [binary_path , '--config-file=chinese_parser/conf.xml']

        if test_lib.confirmPortAvailable(port) == False:
                print 'Port' + str(port) + ' already in use -aborting '
                return -1

        print 'starting engine: ' + args[0] + ' ' + args[1]
        serverHandle = test_lib.startServer(args)
        test_lib.pingServer(port)

#Shut down the srch2 engine
def shutdownSrch2Engine():
	global serverHandle
	#Shutdown the engine server
	test_lib.killServer(serverHandle)

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords):
        query = 'http://localhost:' + port + '/search?'
        # prepare the main query part
        query = query + 'q='
        # keywords section
        for i in range(0, len(queryKeywords)):
                if i == (len(queryKeywords)-1):
                        query=query+urllib2.quote(queryKeywords[i]) # last keyword prefix
                else:
                        query=query+urllib2.quote(queryKeywords[i])+'%20AND%20'

        return query

#Compare the results with the expected outputs.
def compareResults(testQueriesPath):
	f_test = open(testQueriesPath,'r')
	failCount = 0
	global totalFailCount

        for line in f_test:
                #Get the query keyword and result from the input file
                value = line.split('||')
		queryValue = value[0].rstrip('\n').split()
		resultValue = []
		if(len(value) != 1):
			resultValue = value[1].rstrip('\n').split('\n')
                
                #Construct the query
                query = prepareQuery(queryValue)

                #Execute the query
		#print query
                response = urllib2.urlopen(query).read()
                response_json = json.loads(response)

                #Check the result
                failCount += checkResult(query, response_json['results'],resultValue)

        totalFailCount += failCount

#Function of checking the results
#Compare the return value 'prefix' part with the result value
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    print responseJson
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            prefixResult = responseJson[i]['matching_prefix'][0]
            for j in range(1, len(responseJson[i]['matching_prefix'])):
                prefixResult = prefixResult + ',' + responseJson[i]['matching_prefix'][j]
            #print prefixResult + unicode(type(prefixResult))
            #print resultValue[i] + str(type(resultValue[i]))
            if prefixResult !=  resultValue[i]:
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print prefixResult+'||'+resultValue[i]
                break
    else:
        isPass=0
        print query+' test failed'
        print 'query results||given results'
        print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
        maxLen = max(len(responseJson),len(resultValue))
        for i in range(0, maxLen):
            prefixResult = responseJson[i]['matching_prefix'][0]
            for j in range(1, len(responseJson[i]['matching_prefix'])):
                prefixResult = prefixResult + ',' + responseJson[i]['matching_prefix'][j]

            if i >= len(resultValue):
             print prefixResult+'||'
            elif i >= len(responseJson):
             print '  '+'||'+resultValue[i]
            else:
             print prefixResult+'||'+resultValue[i]

    if isPass == 1:
        print  query+' test pass'
        return 0
    return 1

if __name__ == '__main__':
	if(os.path.exists("data")):
		shutil.rmtree("data")
	#Start the test cases
	binary_path = sys.argv[1]
	startSrch2Engine()
	compareResults(sys.argv[2])
	shutdownSrch2Engine()
	if(os.path.exists("data")):
		shutil.rmtree("data")
	os._exit(totalFailCount)
