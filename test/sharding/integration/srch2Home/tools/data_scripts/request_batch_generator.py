#curl "http://localhost:7049/docs" -i -X PUT -d '{"category": "Insurance Services Insurance Agent", "name": "Prescription Service Of California fifteen", "relevance": 3.0147461915737148, "lat": 61.171669000000001, "lng": -149.881021, "id": "115"}'

#!/usr/bin/python

import sys
import json

CULR_MAX_ARG_CHAR_SIZE=10000
port = "7049"
hostname = "localhost"
corename = "stackoverflow"
primary_key_name = "id"
if len(sys.argv) < 4:
   print "Usage : " + sys.argv[0] + " sourceFileName.json port corename"
   sys.exit(0);

sourceName = sys.argv[1]
port = sys.argv[2]
corename = sys.argv[3]
if corename == "statemedia":
   primary_key_name = "index"
insertOutputFileName = "./"+corename+"/"+ port + "-insert-" + sourceName[sourceName.rfind("/")+1:sourceName.rfind(".")] + ".sh";
deleteOutputFileName = "./"+corename+"/"+ port + "-delete-" + sourceName[sourceName.rfind("/")+1:sourceName.rfind(".")] + ".sh";
sourceFile = open(sourceName, 'r')
insertOutputFile = open(insertOutputFileName, 'w')
deleteOutputFile = open(deleteOutputFileName, 'w')
lineNum = 0
recordBatch = "["
for line in sourceFile:
   line = line.replace("'" , "")
   jsonRecord = json.loads(line)
   #print json.dumps(jsonRecord)
   #print "--------------------------------------"
   deleteOutputFile.write("curl \"http://"+hostname+":"+port+"/"+corename+"/docs?"+primary_key_name+"=" + jsonRecord[primary_key_name] + "\" -i -X DELETE" + "\n")
   if len(line.strip()) > CULR_MAX_ARG_CHAR_SIZE:
      continue 
   if recordBatch == "[":
      newRecordBatch = recordBatch +  line.strip()
   else:
      newRecordBatch = recordBatch + "," +  line.strip()
   if len(newRecordBatch) > CULR_MAX_ARG_CHAR_SIZE:
      recordBatch = recordBatch + "]"
      insertOutputFile.write("curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'" + "\n")      
      recordBatch = "[" + line.strip()
   else:      
      recordBatch = newRecordBatch
if recordBatch[-1] != ']':
   recordBatch = recordBatch + "]"   
insertOutputFile.write("curl \"http://"+hostname+":"+port+"/"+corename+"/docs\" -i -X PUT -d '" + recordBatch + "'" + "\n")

insertOutputFile.close()
deleteOutputFile.close()
sourceFile.close()
