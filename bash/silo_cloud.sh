#!/bin/sh

../cmake/build/silo --ip=101.42.136.249:50051 --data_path=../data/data.txt --silo_id=1 &
../cmake/build/silo --ip=81.70.135.109:50052 --data_path=../data/data_1.txt --silo_id=2
#../cmake/build/silo --ip=43.140.219.171:50053 --data_path=../data/data_03.txt --silo_id=3 &
