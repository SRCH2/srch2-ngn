#! /usr/bin/python

# Library of useful srch2 system test functions

import subprocess, time, signal

def startServer(argList):
    #print 'starting engine: {0} {1}'.format(argList[0], argList[1])
    serverHandle = subprocess.Popen(argList)
    #print 'server started, process ' + str(serverHandle.pid) + ' returned ' + str(serverHandle.returncode)
    return serverHandle

#make sure the server is started
def pingServer(port, query = 'q=march'):
    info = 'curl -s \"http://localhost:' + str(port) + '/search?' + query + '\" | grep -q results'
    #print "Pinging with: " + info
    while subprocess.call(info, shell=True) != 0:
        time.sleep(1)
    #print 'server is built!'

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

