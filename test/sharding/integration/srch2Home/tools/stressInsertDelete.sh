#!/bin/bash
sh ./7049-bulk-insert-stackoverflow-1000-8.sh &
sleep(1);
sh ./7050-bulk-delete-stackoverflow-1000-8.sh &
sleep(1);
sh ./7051-bulk-insert-stackoverflow-1000-8.sh &
sleep(1)
sh ./7051-bulk-delete-stackoverflow-1000-8.sh &
