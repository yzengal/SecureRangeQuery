#!/bin/sh

../cmake/build/silo --ip=172.21.0.11:8881 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_0 --silo_id=1
../cmake/build/silo --ip=172.21.0.8:8882 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_1 --silo_id=2
../cmake/build/silo --ip=172.21.0.8:8883 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_2 --silo_id=3
../cmake/build/silo --ip=172.21.0.2:8884 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_3 --silo_id=4
../cmake/build/silo --ip=172.21.0.15:8885 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_4 --silo_id=5
../cmake/build/silo --ip=172.21.0.15:8886 --data_path=../../traffic_dataset/traffic_data/traffic_6/traffic_5 --silo_id=6

