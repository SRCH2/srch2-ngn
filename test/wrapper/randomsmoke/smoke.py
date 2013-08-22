import commands
import httplib
import json
import os
import subprocess
import signal
import sys
import time
import urllib
import urllib2

CONST_FAIL_STRING = "FAIL"
CONST_PASS_STRING = "PASS"
class SmokeTest():

    def __init__(self, config):
        """
        @config , config dict. 
        example:  
        config = { 'server_binaryPath': '../../../build/src/server/',
                'queriesPath': './queries.txt',
                'server_port':8081,
                'server_host': 'http://localhost',
                'server_confg_file_path':'./conf.ini',
                'debug' : True,
                'server_binary_name':'srch2-search-server'
        }
        """
        self.binaryPath = config['server_binaryPath']
        self.binaryName = config['server_binary_name']
        self.queriesPath = config['queriesPath']
        self.config_file_path =config['server_confg_file_path']
        self.port = config['server_port']
        self.host = config['server_host']
        self.debug = config['debug']
        self.shouldPass = True
        self.blockCommentOn = False

    def startServer(self):
        """
        start the server. 
        """
        binary = self.binaryPath + self.binaryName
        binary = binary + ' --config-file={0} &'.format(self.config_file_path)
        self.debugToConsole('starting engine: {0}'.format(binary))
        os.popen(binary)

    def pingServer(self):
        """
        ping the server
        """
        self.debugToConsole("pinging server")
        info = 'curl -s {0}:{1}/search?q=Garden | grep -q results'.format(self.host, self.port)
        while os.system(info) != 0:
            time.sleep(1)
            info = 'curl -s {0}:{1}/search?q=Garden | grep -q results'.format(self.host, self.port)
        self.debugToConsole("pinging done returning")

    def fireQuery(self, query):
        """
        @query : query string as read from the queries.txt file
        calls the prepareQuery to get a uriencoded query and then sends the request to server with urlEncoded  prepared query
        """
        self.debugToConsole("inside fireQuery")
        preparedQuery = self.prepareQuery(query)
        self.debugToConsole("prepared query is: {0}".format(preparedQuery))
        try:
            connection = urllib2.urlopen(preparedQuery)
            code = connection.getcode()
            self.debugToConsole("response code received is: {0}".format(code))
            if self.shouldPass:
                if code == 200:
                    return CONST_PASS_STRING, "{0}: ,{1}".format(code, preparedQuery)
                else:
                    return CONST_FAIL_STRING, "{0}: ,{1}".format(code, preparedQuery)
            else:
                if code == 200:
                    return CONST_FAIL_STRING, "{0}: ,{1}".format(code, preparedQuery)
                else:
                    return CONST_PASS_STRING, "{0}: ,{1}".format(code, preparedQuery)
        except urllib2.HTTPError, error:
            self.debugToConsole("urllib2.HTTPError cought")
            if self.shouldPass:
                return CONST_FAIL_STRING, "{0}: ,{1}".format(error.code, preparedQuery)
            else:
                return CONST_PASS_STRING, "{0}: ,{1}".format(error.code, preparedQuery)
        except urllib2.URLError, error:
            self.debugToConsole("urllib2.URLError cought")
            return CONST_FAIL_STRING, preparedQuery
        except httplib.BadStatusLine, error:
            self.debugToConsole("httplib.BadStatusLine error cought")
            self.restartServer();
            return CONST_FAIL_STRING, "{0}: {1}".format("server crashed for query", preparedQuery)

    def prepareQuery(self, query):
        """
        @query : the query string as read from the queries.txt file
        url encodes the query
        """
        query = self.quote_url(query, "~*&$=,") # encode the uri
        preparedQuery = '{0}:{1}/search?{2}'.format(self.host, self.port, query)
        return preparedQuery

    def doTest(self):
        """
        reads the file containing query string and for each query string call the fireQuery method
        """
        self.debugToConsole("inside doTest")
        f = open(self.queriesPath, 'r')
        queries = f.readlines()
        failed = []
        passed = []
        results = {}
        lineNum = 0
        try:
            for query in queries:
                lineNum += 1
                query = query.strip()
                # '@@' is for block comment. all lines after @@ will be ignored till another line with '@@' is found
                if query == "@@":
                     # it's a block comment, toggle block
                     self.blockCommentOn = not self.blockCommentOn
                     self.debugToConsole("blockCommentOn toggled to {0}".format(self.blockCommentOn))
                     continue
                if not self.blockCommentOn:
                    # '#' is a line comment. line starting with '#' will be ignored
                    if len(query) > 0 and query[0] != "#":
                        # '@pass' , all queries below @pass should receive response code 200 from the server
                        if query == "@pass":
                            self.shouldPass = True
                            self.debugToConsole("shouldPass toggled to {0}".format(self.shouldPass))
                            continue
                        # '@fail' all lines, all queries below '@fail' should not recevie response code 200 from server 
                        elif query == "@fail":
                            self.shouldPass = False
                            self.debugToConsole("shouldPass toggled to {0}".format(self.shouldPass))
                            continue
                        status, msg = self.fireQuery(query)
                        if status == CONST_FAIL_STRING:
                            failed.append("{0}. {1}".format(lineNum, msg))
                        elif status == CONST_PASS_STRING:
                            passed.append("{0}. {1}".format(lineNum, msg))
                        else:
                            print "UNKNOWN STATUS", status
            results['failed'] = failed
            results['passed'] = passed
            self.debugToConsole("size of failed cases: {0}".format(len(failed)))
            self.debugToConsole("size of passed cases: {0}".format(len(passed)))
            self.debugToConsole("returning from doTest")
            return results
        except:
            print "Unexpected error:", sys.exc_info()[0]
    def quote_url(self, url, safe):
        """URL-encodes a string (either str (i.e. ASCII) or unicode);
        uses de-facto UTF-8 encoding to handle Unicode codepoints in given string.
        """
        return urllib.quote(unicode(url).encode('utf-8'), safe)

    def killServer(self):
        """
        kills the server
        """
        try:
            self.debugToConsole ("killing srch2 server")
            s = commands.getoutput('ps aux | grep srch2-search-server')
            stat = s.split()
            os.kill(int(stat[1]), signal.SIGUSR1)
            self.debugToConsole("server killed!")
        except:
            try:
                s = commands.getoutput("ps -A | grep -m1 srch2-search-server | awk '{print $1}'")
                a = s.split()
                cmd = "kill -9 {0}".format(a[-1])
                os.system(cmd)
                self.debugToConsole("server killed")
            except:
                print "no running instance found to kill, moving ahead."
    def restartServer(self):
        """
            restart the server, first kills the existing running server, then starts server
        """
        self.debugToConsole("********restarting the server**********")
        #self.killServer()  
        self.startServer()
        self.pingServer()
        self.debugToConsole("********server up**********")

    def resultHandler(self, results):
        """
        receives the dict results. results={'failed':failed, 'passed':passed}.
        failed: list of failed queries
        passed: list of passed queries
        This function calls the printResults for each list
        """
        self.debugToConsole("inside resultHandler")
        for key in results:
            self.debugToConsole("checking for {0} cases".format(key))
            self.debugToConsole("size of {0} cases: {1}".format(key, len(results[key])))
            if results[key]:
                print "following queries {0}".format(key)
                self.printResults(results[key], key)
        self.debugToConsole("retunring from resultHandler")
    
    def printResults(self, results, status):
        """
        prints the msgs in the result vector.
        @status is either 'passed' or 'failed' strings
        """
        for result in results:
            print "{0}: {1}".format(status, result)

    def debugToConsole(self, msg):
        if self.debug:
            print msg

    def rebootServer(self):
        self.killServer()
        self.startServer()
        time.sleep(3) # let the server restart
if __name__ == '__main__':
    config = { 'server_binaryPath': '../../../build/src/server/',
                'queriesPath': './queries.txt',
                'server_port':8081,
                'server_host': 'http://localhost',
                'server_confg_file_path':'./conf.ini',
                'debug' : False,
                'server_binary_name':'srch2-search-server',
                'geoqueriesPath':'./geoqueries.txt',
                'server_geoconfig_file_path': './geoconf.ini'
    }
    smoke = SmokeTest(config)
    try:
        #kill any existing instance of server
        smoke.killServer();
        #start the server
        smoke.startServer();
        #ping the server
        smoke.pingServer();
        decoration = 40
        print "*"*decoration, "TESTING BEGINS", "*"*decoration
        #start testing
        results = smoke.doTest();
        smoke.queriesPath=config['geoqueriesPath']
        smoke.server_confg_file_path= config['server_geoconfig_file_path']
        smoke.rebootServer()
        geoResults = smoke.doTest();
        #print the test results
        smoke.resultHandler(results)
        print "*"*decoration, "TESTING GEO QUERIES", "*"*decoration
        smoke.resultHandler(geoResults)
        print "*"*decoration, "TESTING ENDS", "*"*decoration
        #smoke.killServer();
    except:
        #exception caught, kill the server
        print "sys.exc_info:", sys.exc_info()[0]
        smoke.killServer();
