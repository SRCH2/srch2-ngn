import httplib
import sys

import urllib2

jsonfile = open(sys.argv[1], 'r')
conn = httplib.HTTPConnection('127.0.0.1', 8081)

for postData in jsonfile:
	opener = urllib2.build_opener(urllib2.HTTPHandler)
	request = urllib2.Request('http://localhost:8081/docs_v0', postData)
	request.add_header('Content-Type', 'application/json')
	request.get_method = lambda: 'PUT'
	url = opener.open(request)
jsonfile.close()
