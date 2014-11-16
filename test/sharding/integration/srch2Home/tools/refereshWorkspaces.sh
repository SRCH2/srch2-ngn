currentDir=`pwd`
cd /home/jamshid/workspace-srch2-v4/srch2-ngn/test/sharding/integration/srch2Home
rm -r ./TEST_CLUSTER/node-A/
truncate --size 0k ./logs/node-A-log.txt

cd /home/jamshid/workspace-srch2-1/srch2-ngn/test/sharding/integration/srch2Home
rm -r ./TEST_CLUSTER/node-B/
truncate --size 0k ./logs/node-B-log.txt

cd /home/jamshid/workspace-srch2-2/srch2-ngn/test/sharding/integration/srch2Home
rm -r ./TEST_CLUSTER/node-C/
truncate --size 0k ./logs/node-C-log.txt

cd /home/jamshid/workspace-srch2-3/srch2-ngn/test/sharding/integration/srch2Home
rm -r ./TEST_CLUSTER/node-D/
truncate --size 0k ./logs/node-D-log.txt

cd $currentDir
