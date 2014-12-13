core=$1;
count=$2;
for i in `seq 1 $count`
do 
	python ./request_batch_generator.py ../../$core/$core-1000-$i.json 7049 $core
	python ./request_batch_generator.py ../../$core/$core-1000-$i.json 7050 $core
	python ./request_batch_generator.py ../../$core/$core-1000-$i.json 7051 $core
	python ./request_batch_generator.py ../../$core/$core-1000-$i.json 7052 $core
done
