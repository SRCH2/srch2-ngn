#! /usr/bin/python

# Library of useful srch2 system test functions

import sys, subprocess, time, signal, urllib2, json

# Start the srch2 search engine server
# Non-blocking (background process)
# argList is an array - argList[0] is the path to the server
# Remaining elements are server parameters such as --config-file=
def startServer(argList):
    #print 'starting engine: {0} {1}'.format(argList[0], argList[1])
    serverHandle = subprocess.Popen(argList)
    #print 'server started, process ' + str(serverHandle.pid) + ' returned ' + str(serverHandle.returncode)
    return serverHandle

# make sure the server is started
# curl's a request to the server until it gets a response
# blocks until server is up (!)
def pingServer(port, query = 'q=march', timeout = 15):
    info = 'curl -s \"http://127.0.0.1:' + str(port) + '/search?' + query + '\" | grep -q results'
    #print "Pinging with: " + info
    while timeout >= 0 and subprocess.call(info, shell=True) != 0:
        timeout -= 1
        time.sleep(1)
    #print 'server is built!'
    if timeout < 0:
        print "WARNING: Timed out waiting for the server to start!"
        return -1
    return 0

# make sure no server is running on our port
def confirmPortAvailable(port) :
    query = 'http://localhost:' + str(port) + '/info'
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

# Tell the server to shutdown by sending it a signal
def killServer(serverHandle):
    """
    kills the server
    """
    try:
        #print ("killing srch2 server")
        serverHandle.send_signal(signal.SIGINT)
        #print ("server killed!")
    except Exception, err:
        print "Kill server exception: " + str(err)
        try:
            serverHandle.kill()
        except:
            print "no running instance found to kill, moving ahead."


def detectPort(configPath):
    wholecontent = open(configPath).read()
    keywords = ['<listeningPort>', '</listeningPort>']
    start = wholecontent.find(keywords[0]) + len(keywords[0]) 
    end = wholecontent.find(keywords[1], start)
    try:
        return int(wholecontent[start:end].strip())
    except Exception, err:
        print "Detect port Exception: " + str(err)

def open_url_get(url):
    try:
        return json.loads(urllib2.urlopen(url).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())
    
def open_url_put(url, record):
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, record)
        request.get_method = lambda: 'PUT'
        return json.loads( opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())

def open_url_delete(url):
    try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, '')
        request.get_method = lambda: 'DELETE'
        return json.loads(opener.open(request).read())
    except urllib2.HTTPError as e:
        return json.loads(e.read())


