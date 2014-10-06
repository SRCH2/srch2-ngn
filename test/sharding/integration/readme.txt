To run the sharding test framework on multiple machines, use the script given below:

shell> python nodeIntensive/distributedTestFramework.py config.json nodeIntensive/transaction-trial.txt
shell> ../../../build/src/server/srch2-search-server --config=./nodeIntensive/conf-A.xml

PS: ignore the below command, they are for sharding v0
shell> python ./distributedCheck.py ../../../build/src/server/srch2-search-server ./2nodeInCalvin-2nodeInDilbert/4n-insertA-queryB-infoCD.txt ./2nodeInCalvin-2nodeInDilbert/input.txt
