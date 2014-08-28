#this test is used for Heartbeat

import sys, urllib2, json, time, subprocess, os, commands, signal

sys.path.insert(0, 'srch2lib')
import test_lib

def detectHeartBeatTimer(configPath):
    wholecontent = open(configPath).read()
    keywords = ['<heartbeattimer>', '</heartbeattimer>']
    start = wholecontent.find(keywords[0]) + len(keywords[0]) 
    end = wholecontent.find(keywords[1], start)
    try:
        return int(wholecontent[start:end])
    except Exception, err:
        print "Detect port Exception: " + str(err)

def testShouldShutDownWhenTimerDown(binary_path, conf_file, port, timer):
    print 'testShouldShutDownWhenTimerDown'
    print 'starting engine: ' + binary_path + ' ' + conf_file

    serverHandle = test_lib.startServer([binary_path, '--config=' + conf_file])
    import time
    time.sleep(timer+1) 
    try:
        urllib2.urlopen('http://127.0.0.1:'+ str(port) + '/info')
        test_lib.killServer(serverHandle)
        print 'should be closed by now, but it is still alive'
        return -1
    except urllib2.URLError, e:
        print 'successfully closed'
        return 0

def testShouldNotShutDownIfHasAnyPing(binary_path, conf_file, port, timer):
    if timer < 3:
        print 'timer:' + str(timer) + 's is too short'
        return -1
    
    pingtimer = timer - 1 
    serverHandle = test_lib.startServer([binary_path, '--config=' + conf_file])
    import time
    for x in xrange(0,4):
        time.sleep(pingtimer) 
        try:
            urllib2.urlopen('http://127.0.0.1:'+ str(port) + '/info')
        except urllib2.URLError, e:
            print e
            print 'Error: server does not response'
            return -1
    test_lib.killServer(serverHandle)
    print 'info responsed successfully'
    return 0

if __name__ == '__main__':      
    binary_path = sys.argv[1]
    conf_file = './heartbeat/conf.xml'
    port = test_lib.detectPort(conf_file)
    timer = detectHeartBeatTimer(conf_file)
    print 'config:' , conf_file, 'port:', port, 'timer:', timer

    if test_lib.confirmPortAvailable(port) == False:
        print 'Port ' + str(port) + ' already in use - aborting'
        ox._exit(-1)

    exitCode =  testShouldShutDownWhenTimerDown(binary_path, conf_file, port, timer)
    exitCode += testShouldNotShutDownIfHasAnyPing(binary_path, conf_file, port, timer)

    print '===================='
    os._exit(exitCode)

