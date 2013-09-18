#this test is used for exact A1
#using: python exact_A1.py queriesAndResults.txt

import sys, urllib2, json, time, subprocess, os, commands, signal

port = '8081'

#make sure that start the engine up
def pingServer():
    info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
    while os.system(info) != 0:
        time.sleep(1)
        info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'


def testEmptyIndex(binary_path):
    #Start the engine server
    binary= binary_path + '/srch2-search-server'
    binary= binary+' --config-file=./empty_index/conf.xml &'
    print 'starting engine: ' + binary
    os.popen(binary)

    pingServer()

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
    print 'test pass'
    print '=============================='

if __name__ == '__main__':      
    #Path of the query file
    #each line like "trust||01c90b4effb2353742080000" ---- query||record_ids(results)
    binary_path = sys.argv[1]
    testEmptyIndex(binary_path)

