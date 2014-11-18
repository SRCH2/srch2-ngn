#curl "http://localhost:7049/docs" -i -X PUT -d '{"category": "Insurance Services Insurance Agent", "name": "Prescription Service Of California fifteen", "relevance": 3.0147461915737148, "lat": 61.171669000000001, "lng": -149.881021, "id": "115"}'

#!/usr/bin/python

import sys
import json

CULR_MAX_ARG_CHAR_SIZE=10000
port = "7049"
hostname = "localhost"
corename = "stackoverflow"

if len(sys.argv) < 2:
   print "Source file not given."
   sys.exit(0);

sourceName = sys.argv[1]
sourceFile = open(sourceName, 'r')
lineNum = 0
recordBatch = "["
for line in sourceFile:
   line = line.replace("'" , "")
   if len(line.strip()) > CULR_MAX_ARG_CHAR_SIZE:
      continue 
   if recordBatch == "[":
      newRecordBatch = recordBatch +  line.strip()
   else:
      newRecordBatch = recordBatch + "," +  line.strip()
   if len(newRecordBatch) > CULR_MAX_ARG_CHAR_SIZE:
      recordBatch = recordBatch + "]"
      print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'"
      recordBatch = "[" + line.strip()
   else:      
      recordBatch = newRecordBatch
if recordBatch[-1] != ']':
   recordBatch = recordBatch + "]"   
print "curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'"
