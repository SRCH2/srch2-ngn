import httplib

conn = httplib.HTTPConnection('127.0.0.1', 8081)

conn.request("POST", "/srch2/index/save", "")
response = conn.getresponse()
print response.read()

