#curl "http://localhost:7049/docs" -i -X PUT -d '{"category": "Insurance Services Insurance Agent", "name": "Prescription Service Of California fifteen", "relevance": 3.0147461915737148, "lat": 61.171669000000001, "lng": -149.881021, "id": "115"}'

#!/usr/bin/python

import sys
import json


port = "7049"
hostname = "localhost"
corename = "core1"

if len(sys.argv) < 2:
   print "Source file not given."
   sys.exit(0);

sourceName = sys.argv[1]
sourceFile = open(sourceName, 'r')
lineNum = 0
recordBatch = "["
for line in sourceFile:
   lineNum = lineNum + 1
   if lineNum % 1000 == 0:
      #print "Line number " + str(lineNum)
      recordBatch = recordBatch + "]"
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'"
      recordBatch = "["
   line = line.replace("'" , "")
   if recordBatch == "[":
      recordBatch = recordBatch +  line.strip()
   else:
      recordBatch = recordBatch + "," +  line.strip()
if recordBatch[-1] != ']':
   recordBatch = recordBatch + "]"   
print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'"
