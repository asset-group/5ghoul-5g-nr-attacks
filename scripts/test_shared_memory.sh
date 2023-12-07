#!/bin/bash
PWD=$(pwd)
# delete shared memory
# rm 


konsole --hold -e "sudo ./shm_server" &
pid1=$!
sleep 0.1
konsole --hold -e "sudo ./shm_client" 


sudo kill $pid1
