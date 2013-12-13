#using python fuzzy_M1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

port = '8081'

def pingServer():
    info = 'curl -s "http://localhost:' + port + '/search?q=goods&clat=61.18&clong=-149.1&radius=0.5" | grep -q results '
    while os.system(info) != 0:
         time.sleep(1)
         info = 'curl -s "http://localhost:' + port + '/search?q=goods&clat=61.18&clong=-149.1&radius=0.5" | grep -q results '

#the function of checking the results
def checkResult(query, responseJson, resultValue):
    isPass = 1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
                #print response_json['results'][i]['record']['id']
            if responseJson[i]['record']['id'] != resultValue[i]:
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

#prepare the query based on the valid syntax
def prepareQuery(queryKeywords, ct_lat, ct_long, ct_radius):
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
            query = query + fieldFilter + keyword + '*' + '~' # last keyword prefix
        else:
            query = query + fieldFilter + keyword + '~' + '%20AND%20'

    ################# fuzzy parameter
    query = query + '&fuzzy=true'
    ################# GEO parameters
    query = query + '&radius=' + ct_radius
    query = query + '&clat=' + ct_lat
    query = query + '&clong=' + ct_long
    #print 'Query : ' + query
    ##################################
    return query


def testFuzzyM1(queriesAndResultsPath, binary_path):
    # Start the engine server
    binary = binary_path + '/srch2-search-server'
    binary = binary + ' --config-file=./fuzzy_m1/conf.xml &'
    print 'starting server: ' + binary
    os.popen(binary)

    #make sure that start the engine up
    pingServer()

    #construct the query
    print "queriesAndResultsPath:  ",queriesAndResultsPath
    radius = 0.5
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        #print"line is", line
        #get the query keyword and results
        value = line.split('||')
        queryValue = value[0].split('^')
        queryKeyword = queryValue[0].split()
        queryGeo = queryValue[1].split('+')
        resultValue = (value[1]).split()
        #construct the query
        query = 'http://localhost:' + port + '/search?'
        query = query + prepareQuery(queryKeyword, queryGeo[1], queryGeo[0], str(radius))
        #print query
        
        # do the query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
      
        #check the result
        checkResult(query, response_json['results'], resultValue)
    
    #get pid of srch2-search-server and kill the process
    print '=============================='
    try:
        s = commands.getoutput('ps aux | grep srch2-search-server')
        stat = s.split()
        os.kill(int(stat[1]), signal.SIGUSR1)
    except: 
        s = commands.getoutput("ps -A | grep -m1 srch2-search-server | awk '{print $1}'")
        a = s.split()
        cmd = "kill -9 {0}".format(a[-1])
        os.system(cmd)
if __name__ == '__main__':   
    #Path of the query file
    #  each line like "trust^-149.880918+61.155358||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    testFuzzyM1(queriesAndResultsPath, binary_path)
