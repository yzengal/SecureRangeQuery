#!/bin/sh

../cmake/build/silo --ip=0.0.0.0:50051 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_0 --silo_id=1 &
../cmake/build/silo --ip=0.0.0.0:50052 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_1 --silo_id=2
#../cmake/build/silo --ip=0.0.0.0:50053 --data_path=../data/data_03.txt --silo_id=3 &
