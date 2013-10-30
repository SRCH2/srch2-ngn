#this test is used for phrase search  

# In this test case we read the queries from queries2.txt and match the response with the list of results 
# given in the same file.
# The last 4 queries are to test phrase search with multivalued attributes. The idea of this test is the following:
# Suppose we have one attribute "authors" which is multivalued and its value for record R1 is 
# "authors":["John Smith" , "George Orwell"]. Now if the phrase is "John Smith", R1 must be found but if 
# the phrase is "Smith George", it should not be found since "Smith" and "George" are not in the same value in R1.
# According to this explanation, ps-test2.json is tailored so that record 2 is a results of "Smith Ronald" while record 1 is not.


import sys, urllib2, urllib, json, time, subprocess, os, commands, signal

port = '8081'

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    while os.system(info) != 0:
        time.sleep(1)
        info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

#Function of checking the results
def checkResult(query, responseJson,resultValue):
#    for key, value in responseJson:
#        print key, value
    isPass=1
    if  len(responseJson) == len(resultValue):
        for i in range(0, len(resultValue)):
            if str(responseJson[i]['record']['id']).strip() !=  str(resultValue[i]).strip():
                isPass=0
                print query+' test failed'
                print 'query results||given results'
                print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
                for i in range(0, len(responseJson)):
                    print str(responseJson[i]['record']['id'])+'||'+str(resultValue[i])
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
                print str(responseJson[i]['record']['id'])+'||'+str(resultValue[i])

    if isPass == 1:
        print  query+' test pass'



def testPhraseSearch(queriesAndResultsPath, binary_path):
    #Start the engine server
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=./phraseSearch/ps2.xml &'
    print 'starting engine: ' + binary
    os.popen(binary)

    pingServer()

    #construct the query
    #format : phrase,proximity||rid1 rid2 rid3 ...ridn
    f_in = open(queriesAndResultsPath, 'r')
    for line in f_in:
        value=line.split('||')
        phrase=value[0]
        expectedRecordIds=(value[1]).split()
        query='http://localhost:' + port + '/search?q='+ urllib.quote(phrase) + '&sort=ranking&orderby=desc'
        print query
        response = urllib2.urlopen(query).read()
        response_json = json.loads(response)
        #print response_json['results']
        #check the result
        checkResult(query, response_json['results'], expectedRecordIds)

    #get pid of srch2-search-server and kill the process
    try:
        s = commands.getoutput('ps aux | grep srch2-search-server')
        stat = s.split()
        os.kill(int(stat[1]), signal.SIGUSR1)
    except:
        s = commands.getoutput("ps -A | grep -m1 srch2-search-server | awk '{print $1}'")
        a = s.split()
        cmd = "kill -9 {0}".format(a[-1])
        os.system(cmd)
    print '=============================='

if __name__ == '__main__':      
    #Path of the query file
    binary_path = sys.argv[1]
    queriesAndResultsPath = sys.argv[2]
    testPhraseSearch(queriesAndResultsPath, binary_path)

