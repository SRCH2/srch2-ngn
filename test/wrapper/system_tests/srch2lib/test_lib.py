#! /usr/bin/python

# Library of useful srch2 system test functions

import subprocess, time, signal

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
    info = 'curl -s \"http://localhost:' + str(port) + '/search?' + query + '\" | grep -q results'
    #print "Pinging with: " + info
    while timeout >= 0 and subprocess.call(info, shell=True) != 0:
        timeout -= 1
        time.sleep(1)
    #print 'server is built!'
    if timeout < 0:
        print "WARNING: Timed out waiting for the server to start!"

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

