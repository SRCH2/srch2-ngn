#!/usr/bin/env python

# File: test_srch2_top_k_pagination.py
# Author: contact@srch2.com
# Retrieves JSON response using
# Srch2-base-search-url for a given query.
# 
# 1) Test order. For example:
# JSON A:
# 1 - a - 0.9 
# 2 - b - 0.8
# 3 - c - 0.8
# 4 - d - 0.8
# 5 - e - 0.5
#
# JSON B:
# 1 - a - 0.9 
# 2 - c - 0.8
# 3 - d - 0.8
# 4 - b - 0.8
# 5 - e - 0.5
#
# JSON C:
# 1 - a - 0.9 
# 2 - b - 0.8
# 3 - c - 0.8
# 4 - d - 0.8
# 5 - f - 0.5
# 6 - e - 0.5
# 7 - g - 0.3
# 
# JSON A, B, C are all valid
#
import sys, urllib2, json, time, subprocess, os, commands, signal

port = '8081'

#make sure that start the engine up
def pingServer():
	info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'
	while os.system(info) != 0:
		time.sleep(1)
		info = 'curl -s http://localhost:' + port + '/search?q=Garden | grep -q results'

def resultsScoreToRecordMap(json_response):
     decoded_json = json.loads(json_response)
     results = decoded_json["results"]
     resultsScoreToRecordMap = {}
     for item in results:
         try:
             #print item["score"], item["record"]["_id"]
             resultsScoreToRecordMap[item["score"]].add(str(item["record"]["id"]))
         except KeyError:
             resultsScoreToRecordMap[item["score"]] = set()
             resultsScoreToRecordMap[item["score"]].add(str(item["record"]["id"]))
                                                             
     return resultsScoreToRecordMap
 
def verify(jsonA, topk_A, jsonB, topk_B):
    resultsA = resultsScoreToRecordMap(jsonA)
    resultsB = resultsScoreToRecordMap(jsonB)

    counter = 0
    items_A = sorted(resultsA.iteritems())
    items_A.reverse()
    for key,value in items_A: # iter from top score to least score items
        if (resultsB[key] != value):
            if ((counter + len(value)) == int(topk_A) ) and (resultsB[key].issuperset(value)): # corner case where top k element has same score as top k+1 element
                continue
            else:
                return False
        counter = counter + len(value)
    return True
   
if __name__ == "__main__":
   	#Start the engine server
	binary='../../../../build/src/server/srch2-search-server'
	binary=binary+' --config-file=conf.ini &'
	os.popen(binary)

	pingServer()

	base = 'http://localhost:' + port
    #base = "http://shrek.calit2.uci.edu:8081"
    
	query = "obam"
	topk_A = str(10)
	topk_B = str(20)
	if len(sys.argv) == 4:
		query = sys.argv[1]
		if (sys.argv[2] < sys.argv[3]):
			topk_A = str(sys.argv[2])
			topk_B = str(sys.argv[3])
		else:
			topk_B = str(sys.argv[2])
			topk_A = str(sys.argv[3])
                base_url = base + "/search?fuzzy=1&type=0&start=0&q="+query
                url_A = base_url + "&limit="+topk_A
                url_B = base_url + "&limit="+topk_B
    
                jsonTop10 = urllib2.urlopen(url_A).read()
                jsonTop20 = urllib2.urlopen(url_B).read()

                if ( verify(jsonTop10, topk_A,  jsonTop20, topk_B) ):
		    print "Test passed."
                else:
                    print "Test failed."
        else:
		print "Usage:"
		print "python test_srch2_top_k.py test+obam 10 20"
		print "python test_srch2_top_k.py ${query} ${topk_A} ${topk_B}"

        #get pid of srch2-search-server and kill the process
	s = commands.getoutput('ps aux | grep srch2-search-server')
	stat = s.split() 
	os.kill(int(stat[1]), signal.SIGUSR1)
	os.popen('rm *.idx')
	os.popen('rm log.txt')
    
