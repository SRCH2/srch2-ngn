#curl "http://localhost:7049/docs" -i -X PUT -d '{"category": "Insurance Services Insurance Agent", "name": "Prescription Service Of California fifteen", "relevance": 3.0147461915737148, "lat": 61.171669000000001, "lng": -149.881021, "id": "115"}'

#!/usr/bin/python

import sys
import json


port = "8002"
hostname = "128.195.185.107"
corename = "stackoverflow"
unit = 20
if len(sys.argv) < 5:
   print "Usage: " + sys.argv[0] + " sourceName hostname portNumber corename [num_rec_per_curl_req]" 
   sys.exit(0);

sourceName = sys.argv[1]
hostname = sys.argv[2]
port = sys.argv[3]
corename = sys.argv[4]
if len(sys.argv) == 6:
   unit = int(sys.argv[5])


sourceFile = open(sourceName, 'r')
lineNum = 0
recordList=""
for line in sourceFile:
   lineNum = lineNum + 1
   if lineNum % unit == 0 :
      #print recordList + "\n\n\n\n\n\n"
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordList.strip() + "'"
      recordList = ""
   # Preparing the line to only contain the json object of the record
   line = line.replace("'" , "")
   line = line.strip()
   if line[-1] == ',':
      line = line[:-1]
   # line now contains one record
   ##################################################################
   # add record to recordList
   if lineNum % unit != 1:
      recordList = recordList + ','
   recordList = recordList + line
if recordList != "":
   print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordList.strip() + "'"
