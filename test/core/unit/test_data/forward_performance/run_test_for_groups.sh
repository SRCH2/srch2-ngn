MAX_LEN=$1

count=1

while [ $count -le $MAX_LEN ]; do
    echo Testing records with $count keywords
    echo -----------------------------------------------
    cp record_with_${count}_keywords current_group
    data_path=current_group ../../../../build/test/unit/ForwardIndex_Performance_Test
    count=$[ $count + 1 ]
    echo -----------------------------------------------
done
