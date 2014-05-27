
Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration 

shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-queryA.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-deleteB.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-R1-B-R2-deleteA-R1-B-R2.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-R1-B-R2-deleteB-R1-A-R1.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-R1-B-R2-deleteB-R1-queryA-R1.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertAB-queryAB.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertAABB.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/testUpdate.txt ./2n-testCaseFramework/list-of-2-nodes.txt
shell> python ./2n-testCaseFramework/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-testCaseFramework/2n-insertA-queryB.txt ./2n-testCaseFramework/list-of-2-nodes.txt
