import httplib
import sys

conn = httplib.HTTPConnection('127.0.0.1', 8081)

primary_key = sys.argv[1]
delete_payload = "{\"_id\":\"" + primary_key + "\"}"

print "delete", delete_payload


conn.request("POST", "/srch2/index/delete", delete_payload)
response = conn.getresponse()
print response.read()

