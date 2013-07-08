1. Simplify State Media's data to the format: primary_key^searchable_field
    > python simplifyData.py

2. Group the simplified data by the length of the searchable field
    > python group_by_len.py {GROUP_SIZE}

3. Run test/unit/ForwardIndex_Performance_Test for each group
    > bash run_test_for_groups.sh {MAX_LENGTH}
