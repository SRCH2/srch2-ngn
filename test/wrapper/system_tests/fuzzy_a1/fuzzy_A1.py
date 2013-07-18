#using python fuzzy_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands,signal

port = '8081'

#make sure that start the engine up
def pingServer():
	info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
	while os.system(info) != 0:
		time.sleep(1)
		info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

#the function of checking the results
def checkResult(query, responseJson,resultValue):
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

    if isPass == 1:
        print  query+' test pass'

def testFuzzyA1(queriesAndResultsPath):
	# Start the engine server
	binary='../../../../build/src/server/srch2-search-server'
	binary=binary+' --config-file=conf.ini &'
	os.popen(binary)
	#make sure that start the engine up
	pingServer()

	#construct the query

	f_in = open(queriesAndResultsPath, 'r')
	for line in f_in:
		#get the query keyword and results
		value=line.split('||')
		queryValue=value[0].split()
		resultValue=(value[1]).split()
		#construct the query
		query='http://localhost:' + port + '/search?q='
		for i in range(0, len(queryValue)):
		    if i == (len(queryValue)-1):
		        query=query+queryValue[i]
		    else:
		        query=query+queryValue[i]+'+'
		query=query+'&fuzzy=1'
		#print query
		
		# do the query
		response = urllib2.urlopen(query).read()
		response_json = json.loads(response)
	  
		#check the result
		checkResult(query, response_json['results'], resultValue )
    

	#get pid of srch2-search-server and kill the process
	s = commands.getoutput('ps aux | grep srch2-search-server')
	stat = s.split() 
	os.kill(int(stat[1]), signal.SIGUSR1)
	os.popen('rm *.idx')
	os.popen('rm log.txt')
	print '=============================='

if __name__ == '__main__':    
   #Path of the query file
   #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
   queriesAndResultsPath = sys.argv[1]
   testFuzzyA1(queriesAndResultsPath)

