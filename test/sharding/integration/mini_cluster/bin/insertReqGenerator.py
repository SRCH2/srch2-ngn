#curl "http://localhost:7049/docs" -i -X PUT -d '{"category": "Insurance Services Insurance Agent", "name": "Prescription Service Of California fifteen", "relevance": 3.0147461915737148, "lat": 61.171669000000001, "lng": -149.881021, "id": "115"}'

#!/usr/bin/python

import sys
import json


port = "8002"
hostname = "128.195.185.107"
corename = "stackoverflow"
unit = 20
operation = "insert"
if len(sys.argv) < 5:
   print "Usage 1: " + sys.argv[0] + " sourceName hostname portNumber corename insert [num_rec_per_curl_req] [startIndex endIndex]" 
   print "Usage 2: " + sys.argv[0] + " sourceName hostname portNumber corename update [num_rec_per_curl_req] [startIndex endIndex]" 
   print "Usage 3: " + sys.argv[0] + " sourceName hostname portNumber corename delete [primary_key_attr_name] [startIndex endIndex]" 
   sys.exit(0)

sourceName = sys.argv[1]
hostname = sys.argv[2]
port = sys.argv[3]
corename = sys.argv[4]
operation = sys.argv[5]
pkName = "pk_attribute_name"
startIndex=0
endIndex=1000000000
if len(sys.argv) >= 7:
   if operation == "insert" or operation == "update":
      unit = int(sys.argv[6])
   elif operation == "delete":
      pkName = sys.argv[6]
   else:
      print "Operation not recognized."
      sys.exit(0)
elif operation == "delete":
   print "Primary key attribute name must be given after operation name if it's delete."
   sys.exit(0)
if len(sys.argv) == 9:
   startIndex=int(sys.argv[7])
   endIndex=int(sys.argv[8])

def printRequest(hostname, port, corename, operation, data, pkName = "pk_attribute_name"):
   if operation == "insert":
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + data + "'"
   elif operation == "delete":
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs?"+pkName+"="+data+"\" -i -X DELETE"
   elif operation == "update":
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/update\" -i -X PUT -d '" + data + "'"
   else:
      sys.exit(0)

def prepareRecord(line):
   # Preparing the line to only contain the json object of the record
   line = line.replace("'" , "")
   line = line.strip()
   if line[-1] == ',':
      line = line[:-1]
   return line

sourceFile = open(sourceName, 'r')

if operation == "insert" or operation == "update":
   lineNum = 0
   recordList=[]
   for line in sourceFile:
      lineNum = lineNum + 1
      if lineNum < startIndex or lineNum >= endIndex:
         continue
      if unit == 1:
         line = prepareRecord(line)
         printRequest(hostname, port, corename, operation, line.strip())
         continue
      
      # prepare the json object
      line = prepareRecord(line).strip() 
      # line now contains one record
      # add record to recordList
      recordList.append(line)

      if lineNum % unit == 0 :
         #print recordList + "\n\n\n\n\n\n"
         printRequest(hostname, port, corename, operation, "["+','.join(recordList)+"]")
         recordList = []
   if recordList != []:
      printRequest(hostname, port, corename, operation, "["+','.join(recordList)+"]")
elif operation == "delete":
   lineNum = 0
   for line in sourceFile:
      lineNum = lineNum + 1
      if lineNum < startIndex or lineNum >= endIndex:
         continue
      # Preparing the line to only contain the json object of the record
      line = line.replace("'" , "")
      line = line.strip()
      if line[-1] == ',':
         line = line[:-1]
      # line now contains one record
      ##################################################################
      # now extract the primary key value
      lineJson = json.loads(line)
      printRequest(hostname, port, corename, operation, lineJson[pkName], pkName)
