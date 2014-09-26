ln -s ~chrysler/comparison-data/stackoverflow/server/data/sharding-test/node-intensive-novalidation/* ./nodeIntensive-noValidation/
ln -s ~chrysler/comparison-data/stackoverflow/server/data/sharding-test/node-intensive/* ./nodeIntensive/
ln -s ~chrysler/comparison-data/stackoverflow/server/data/sharding-test/data-intensive/* ./dataIntensive/

ln -s ~chrysler/comparison-data/stackoverflow/server/queries/sharding-test/node-intensive-novalidation/* nodeIntensive-noValidation/
ln -s ~chrysler/comparison-data/stackoverflow/server/queries/sharding-test/node-intensive/* nodeIntensive/
ln -s ~chrysler/comparison-data/stackoverflow/server/queries/sharding-test/data-intensive/* ./dataIntensive/

mkdir -p dashboardFiles/nodeIntensive-noValidation/
mkdir -p dashboardFiles/test_dynamicShards_noValidation/

ln -s ~chrysler/comparison-data/stackoverflow/server/data/sharding-test/node-intensive-novalidation/* ./test_dynamicShards_noValidation/
ln -s ~chrysler/comparison-data/stackoverflow/server/queries/sharding-test/node-intensive-novalidation/* ./test_dynamicShards_noValidation/

