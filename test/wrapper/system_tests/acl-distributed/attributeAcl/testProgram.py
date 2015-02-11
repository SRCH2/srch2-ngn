# Automated test script to start engine, fire queryes and verify results
import sys, shutil, urllib2, json, time, subprocess, os, commands, signal, traceback

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
        returnedRecIds = []
        for i in range(0, len(resultValue)):
            returnedRecIds.append(responseJson[i]['record_id'])
        if (sorted(returnedRecIds) != sorted(resultValue)):
            #print response_json['results'][i]['record']['id']
            isPass=0
            print query + redColor + ' test failed' + endColor
            print 'query results||expected results'
            print 'number of results:'+str(len(responseJson))+'||'+str(len(resultValue))
            for i in range(0, len(responseJson)):
                print responseJson[i]['record_id']+'||'+resultValue[i]
            
    else:
        isPass=0
        print query + redColor + ' test failed' + endColor
        print 'query results||expected results'
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
    
def test(queriesAndResultsPath):

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

    print '=============================='
    return failCount

def startCluster(binary_path, configs, nullFd = None):
    #Start the engine server
    args = [binary_path] + configs
    serverHandle = test_lib.startServer(args, redirectFd=nullFd)
    return serverHandle

def loadInitialData(dataFile, corename = '') :
    test_lib.bulkLoadRequest(dataFile, 'D',  corename)

def loadInitialData_API(dataFile, corename = ''):
    f = open(dataFile)
    records  = f.readlines()
    f.close()
    print '-------------------------------------------------'
    print 'loading data'
    for rec in records: 
        if len(rec) == 0 or rec[0] == '#' or rec[0] == '[' or rec[0] == ']':
            continue
        if corename == '':
            query = 'http://localhost:8087/docs'
        else:
            query = 'http://localhost:8087/' + corename + '/docs'

        payload = rec.strip('\n')
        try:
            request = urllib2.Request(query, data=payload)
            request.get_method = lambda: 'PUT'
            opener = urllib2.build_opener(urllib2.HTTPHandler)
            url = opener.open(request)
            #print url.read()
        except urllib2.HTTPError as e:
            print e.read()
        #response = test_lib.insertRequest(rec, corename)
        #print response
    return 0


def loadAcl(aclFile, corename = ''):
    test_lib.bulkLoadRequest(aclFile, 'A',  corename)

def loadAcl_Api(aclFile, corename = ''):
    f = open(aclFile)
    records  = f.readlines()
    f.close()
    for rec in records:
        if len(rec) == 0:
            continue
        if rec[0] == '#' or rec[0] == '[' or rec[0] == ']':
            continue
     
        print '-------------------------------------------------'
        if corename == '':
            query = 'http://localhost:8087/aclAttributeRoleAppend'
        else:
            query = 'http://localhost:8087/' + corename + '/aclAttributeRoleAppend'

        payload = rec.strip('\n')
        print query + "-X PUT -d '" + payload + "'"
        try:
            request = urllib2.Request(query, data=payload)
            request.get_method = lambda: 'PUT'
            opener = urllib2.build_opener(urllib2.HTTPHandler)
            url = opener.open(request)
        except urllib2.HTTPError as e:
            print e.read()
            return 1
    return 0


def checkIfNodeReady(nodeCount):
    url = 'http://localhost:8087/info'
    try:
        rs = urllib2.urlopen(url).read();
        response = json.loads(rs)
        if response["nodes"]["count"] == nodeCount:
            print rs
            return True
        else:
            return False
    except urllib2.HTTPError as e:
        return False

if __name__ == '__main__':      
    
    currDir = os.path.abspath(".")
    
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")

    exitCode = 0
    binary_path = sys.argv[1]
    
    #NullDeviceFd = open(os.devnull, 'wb')
    NullDeviceFd = None 

    confFiles = [ './acl-distributed/attributeAcl/config/conf-test1-nodeA.xml', './acl-distributed/attributeAcl/config/conf-test1-nodeB.xml', './acl-distributed/attributeAcl/config/conf-test1-nodeC.xml' ]
    serverHandle = startCluster(binary_path, confFiles,  NullDeviceFd);
    while checkIfNodeReady(3) == False:
        print ' cluster is not ready ...waiting'
        time.sleep(10)
    print 'wait for load balancing'
    time.sleep(60)  # time for loadbalancing
    loadInitialData(currDir + '/acl-distributed/attributeAcl/test1-data.json')
    loadAcl(currDir + '/acl-distributed/attributeAcl/test1-acl.json')
    
    time.sleep(30)  # let the merge happen
    queriesAndResultsPath = './acl-distributed/attributeAcl/testCases.txt'
    try:
        #print ''
        exitCode = test(queriesAndResultsPath)
    except Exception as e:
        print("-"*60)
        traceback.print_exc(file=sys.stdout)
        print("-"*60)
        exitCode = 1
    test_lib.kill9Server(serverHandle)
    time.sleep(5)
    
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    
    confFiles = ['./acl-distributed/attributeAcl/config/conf-test2-nodeA.xml', './acl-distributed/attributeAcl/config/conf-test2-nodeB.xml', './acl-distributed/attributeAcl/config/conf-test2-nodeC.xml']
    serverHandle = startCluster(binary_path, confFiles, NullDeviceFd);
    while checkIfNodeReady(3) == False:
        print ' cluster is not ready ...waiting'
        time.sleep(10)
    print 'load balancing  ...waiting'
    time.sleep(80)
    loadInitialData(currDir + '/acl-distributed/attributeAcl/stackoverflow/stackoverflow-data-100.json', 'stackoverflow')
    loadInitialData(currDir + '/acl-distributed/attributeAcl/worldbank/world_bank.json', 'worldbank')
    loadAcl(currDir + '/acl-distributed/attributeAcl/stackoverflow/acl-stackoverflow.json', 'stackoverflow')
    loadAcl(currDir + '/acl-distributed/attributeAcl/worldbank/acl-worldbank.json', 'worldbank')
    time.sleep(60)  # let the merge happen    
    print '-------------------------------------------------'
    queriesAndResultsPath = './attributesAcl/testCasesMultiCore.txt'
    try:
        exitCode |= test(queriesAndResultsPath)
    except :
        print("-"*60)
        traceback.print_exc(file=sys.stdout)
        print("-"*60)
        exitCode = 1
    test_lib.kill9Server(serverHandle)
    time.sleep(5)
    
    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")

	confFiles = [ './acl-distributed/attributeAcl/config/conf-test3-nodeA.xml', './acl-distributed/attributeAcl/config/conf-test3-nodeB.xml', './acl-distributed/attributeAcl/config/conf-test3-nodeC.xml' ]    
    serverHandle = startCluster(binary_path, confFiles , NullDeviceFd);
    while checkIfNodeReady(3) == False:
        print ' cluster is not ready ...waiting'
        time.sleep(10)
    print 'load balancing  ...waiting'
    time.sleep(70)
    loadInitialData(currDir + '/acl-distributed/attributeAcl/worldbank/world_bank.json')
    loadAcl(currDir + '/acl-distributed/attributeAcl/test3-acl.json')
    #os._exit(exitCode)
    print '-------------------------------------------------'
    time.sleep(20)  # let the merge happen    
    queriesAndResultsPath = './attributesAcl/testCasesFilterSortFacetQuery.txt'
    try:
        exitCode |= test(queriesAndResultsPath)
    except Exception as e:
        print("-"*60)
        traceback.print_exc(file=sys.stdout)
        print("-"*60)
        exitCode = 1
    test_lib.kill9Server(serverHandle)
    time.sleep(5)

    if(os.path.exists("./SRCH2Cluster")):
        shutil.rmtree("./SRCH2Cluster")
    
    confFiles = ['./acl-distributed/attributeAcl/config/conf-test4-nodeA.xml', './acl-distributed/attributeAcl/config/conf-test4-nodeB.xml', './acl-distributed/attributeAcl/config/conf-test4-nodeC.xml' ]
    serverHandle = startCluster(binary_path, confFiles, NullDeviceFd);
    while checkIfNodeReady(3) == False:
        print ' cluster is not ready ...waiting'
        time.sleep(10)
    print 'load balancing  ...waiting'
    time.sleep(70)
    loadInitialData(currDir + '/acl-distributed/attributeAcl//test4-data.json')
    loadAcl(currDir + '/acl-distributed/attributeAcl/test4-acl.json')
    print '-------------------------------------------------'
    time.sleep(20)  # let the merge happen    
    queriesAndResultsPath = './attributesAcl/testCasesFilterSortFacetQueryWithSwitch.txt'
    try:
        exitCode |= test(queriesAndResultsPath)
    except Exception as e:
        print("-"*60)
        traceback.print_exc(file=sys.stdout)
        print("-"*60)
        exitCode = 1
    test_lib.kill9Server(serverHandle)
    if NullDeviceFd:
        NullDeviceFd.close()
    os._exit(exitCode)

