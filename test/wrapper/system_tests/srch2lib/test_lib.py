#! /usr/bin/python

# Library of useful srch2 system test functions
# Instruction to use the python library
#
# 1. Start the engine with binary path and config files
#    Pass the binary path and config files in a list
#    For example: 
#         args = [ binary_path, './attributes/conf-A.xml', './attributes/conf-B.xml', './attributes/conf-C.xml', './attributes/conf-D.xml' ]
#         serverHandleList = test_lib.startServer(args)
#    The "startServer" function will call the "confirmAllUrlAvailable" and "pingAllServers" automatically.
#
# 2. Prepare queries and send them to the engine.
#    The queries will be sent through a random host and port within the list of config files 
#    For example : 
#        test_lib.searchRequest("testName","coreA")
#        test_lib.insertRequest({"id":"1234","name":"testName"}, "coreA")
#        test_lib.deleteRequest("id=1234", "coreB")
#        test_lib.saveRequest("coreC")
#    List of functions:
#        def searchRequest(query, coreName = '')
#        def deleteRequest(query, coreName = '')
#        def insertRequest(record, coreName = '')
#        def updateRequeset(record, coreName = '')
#        def infoRequest(query = '', coreName = '')
#        def saveRequest(coreName = '')
#        def resetLoggerRequest(coreName = '')
#        def shutdownRequest()
#        def sendGetRequest(query, coreName = '')  //Send general get request
#        def sendPutRequest(command, record = '', coreName = '')  //Send general put request
# 3. Kill the server.
#        def killServer(serverHandleList)  //Stop the server normally
#        def kill9Server(serverHandleList) //Kill the server by kill -9 

import sys, subprocess, time, signal, urllib2, json, random

hostUrlList = None

# Start the srch2 search engine server
# Non-blocking (background process)
# argList is an array - argList[0] is the path to the server
# Remaining elements are server parameters such as --config-file=
def startServer(argList):
    if len(argList) < 2:
        printLog('Missing args to start the engine!')
        return None
    populateHostUrlList(argList)
    if confirmAllUrlAvailable() == False:
        return None

    binaryPath = argList[0]
    serverHandleList = []

    for i in range(1,len(argList)):
        startArgList = [argList[0]]
        if "--config-file=" not in argList[i]: 
            startArgList.append('--config-file=' + argList[i])
        else:
            startArgList.append(argList[i])
        printLog('starting engine: {0} {1}'.format(startArgList[0], startArgList[1]))
        serverHandleList.append(subprocess.Popen(startArgList))
    
    #print 'server started, process ' + str(serverHandle.pid) + ' returned ' + str(serverHandle.returncode)
    if pingAllServers() != 0:
        return None

    print 'Wait for ' + str(len(argList)*2) + ' sec to stabilize the nodes.'
    time.sleep(len(argList)*2)

    return serverHandleList

# make sure the server is started
# curl's a request to the server until it gets a response
# blocks until server is up (!)
def pingServer(url, timeout = 15):
    info = 'curl -s \"' + str(url) + '/info\"'
    #print "Pinging with: " + info
    while timeout >= 0 and subprocess.call(info, shell=True) != 0:
        timeout -= 1
        time.sleep(1)
    #print 'server is built!'
    if timeout < 0:
        printLog("WARNING: Timed out waiting for the server(URL: " + str(url) + ") to start!")
        return 1
    return 0

#Ping all the servers to make sure all of them are started.
#Return the fail count
def pingAllServers(timeout = 15):
    global hostUrlList
    if hostUrlList == None:
        printLog('Host url list is empty! Please call \"startServer\" to populate the url list first!')
        return -1

    failCount = 0
    for hostUrl in hostUrlList:
        failCount += pingServer(hostUrl, timeout)
    return failCount    

# make sure no server is running on our port
def confirmUrlAvailable(url) :
    query = str(url) + '/info'
    opener = urllib2.build_opener(urllib2.HTTPHandler)
    request = urllib2.Request(query, '')
    try:
        response = opener.open(request).read()
    except urllib2.URLError as err:
        # err code is 111 on Ubuntu Linux and 61 on Mac (darwin)
        if hasattr(err, 'reason') and hasattr(err.reason, 'errno') and ((sys.platform == 'linux2' and err.reason.errno == 111) or (sys.platform == 'darwin' and err.reason.errno == 61)):
            return True # connection refused - port available
        return False # unexpected error response - nonetheless port must be in use
    return False # no error - port already in use

# make sure all servers are available
def confirmAllUrlAvailable():
    global hostUrlList
    if hostUrlList == None:
        printLog('Host url list is empty! Please call \"startServer\" to populate the url list first!')
        return False

    for hostUrl in hostUrlList:
        if confirmUrlAvailable(hostUrl) == False:
            printLog('url : ' + str(hostUrl) + ' is not available!!')
            return False
    return True

# Tell the server to shutdown by sending it a signal
def killServer(serverHandleList):
    """
    kills the server
    """
    for serverHandle in serverHandleList:
        try:
            serverHandle.send_signal(signal.SIGINT)
        except Exception, err:
            printLog("Kill server exception: " + str(err))
            try:
                serverHandle.kill()
            except:
                printLog("no running instance found to kill, moving ahead.")


#Kill the server with kill -9
def kill9Server(serverHandleList):
    """
    kills the server by SIGKILL like kill -9
    """
    for serverHandle in serverHandleList:
        try:
            serverHandle.send_signal(signal.SIGKILL)
            printLog("server killed with kill -9!")
        except Exception, err:
            printLog("Kill server exception: " + str(err))
            try:
                serverHandle.kill()
            except:
                printLog("no running instance found to kill, moving ahead.")

#Detect port from the config file
def detectPort(configPath):
    wholecontent = open(configPath).read()
    keywords = ['<listeningPort>', '</listeningPort>']
    start = wholecontent.find(keywords[0]) + len(keywords[0]) 
    end = wholecontent.find(keywords[1], start)
    try:
        return int(wholecontent[start:end].strip())
    except Exception, err:
        printLog("Detect port Exception: " + str(err))

#Detect hostname from the config file
def detectHostname(configPath):
    wholecontent = open(configPath).read()
    keywords = ['<listeningHostname>', '</listeningHostname>']
    start = wholecontent.find(keywords[0]) + len(keywords[0]) 
    end = wholecontent.find(keywords[1], start)
    try:
        return wholecontent[start:end].strip()
    except Exception, err:
        printLog("Detect hostname Exception: " + str(err))

#Populate the host url from the config files in the argsList.
def populateHostUrlList(argList):
    global hostUrlList
    hostUrlList = []
  
    for i in range(1,len(argList)):
        configPath = argList[i]
        if "--config-file=" in argList[i]:
            configPath = configPath[configPath.find("--config-file=") + len("--config-file="):]
        port = detectPort(configPath)
        hostname = detectHostname(configPath)
        hostUrlList.append('http://'+ hostname + ':' + str(port))
  
#Get a random url with port from the hostUrlList
def getHostUrl(coreName = ''):
    global hostUrlList 
    if hostUrlList == None:
        printLog('Host url list is empty! Please call \"startServer\" to populate the url list first!')
        return ''
    hostUrl = random.choice(hostUrlList)
    if coreName == '':
        return hostUrl
    else:
        return hostUrl + '/' + coreName

#Send Get request to the engine 
#return the response in json format
def searchRequest(query, coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/search?' + query
    printLog('query : ' + url)
    try:
        return json.loads(urllib2.urlopen(url).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send delete request to the engine
#return the response in json format
def deleteRequest(query, coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/docs?' + query
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'DELETE'
        printLog('query : ' + str(url) + ' delete record ')
        return json.loads(opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send insert request to the engine
#return the response in json format
def insertRequest(record, coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/docs'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, record)
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url) + ' record : ' + str(record))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send update request to the engine
#return the response in json format
def updateRequeset(record, coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/update'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, record)
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url) + ' record : ' + str(record))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send get info request to the engine
#return the response in json format
def infoRequest(query = '', coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/info'
    if query != '':
        url += '?' + query
    printLog('query : ' + url)
    return json.loads(urllib2.urlopen(url).read())

#Send save request to the engine
#return the response in json format
def saveRequest(coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/save'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send reset logger request to the engine
#return the response in json format
def resetLoggerRequest(coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/resetLogger'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send shutdown request to the engine
#return the response in json format
def shutdownRequest():
    hostUrl = getHostUrl()
    url = hostUrl + '/_all/shutdown'
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send raw request to the engine
#return the response in json format
def sendGetRequest(query, coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/' + query
    printLog('query : ' + url)
    try:
        return json.loads(urllib2.urlopen(url).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

#Send update request to the engine
#return the response in json format
def sendPutRequest(command, record = '', coreName = ''):
    hostUrl = getHostUrl(coreName)
    url = hostUrl + '/' + command
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, record)
        request.get_method = lambda: 'PUT'
        printLog('query : ' + str(url) + ' record : ' + str(record))
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

def printLog(log):
    print 'Python SRCH2 lib => ' + log
