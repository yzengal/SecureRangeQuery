#!/bin/sh

#../cmake/build/server --query_path=../../traffic_dataset/traffic_query/silo_number_range_traffic --ip_path=../data/ip_cloud.txt


../cmake/build/server --query_path=../../traffic_dataset/traffic_query/query_area_e_3_range_traffic --ip_path=../data/ip_cloud.txt >query_area_e_3_range_traffic.log
../cmake/build/server --query_path=../../traffic_dataset/traffic_query/query_area_e_4_range_traffic --ip_path=../data/ip_cloud.txt >query_area_e_4_range_traffic.log
../cmake/build/server --query_path=../../traffic_dataset/traffic_query/query_area_e_5_range_traffic --ip_path=../data/ip_cloud.txt >query_area_e_5_range_traffic.log
../cmake/build/server --query_path=../../traffic_dataset/traffic_query/query_area_e_6_range_traffic --ip_path=../data/ip_cloud.txt >query_area_e_6_range_traffic.log
../cmake/build/server --query_path=../../traffic_dataset/traffic_query/query_area_e_7_range_traffic --ip_path=../data/ip_cloud.txt >query_area_e_7_range_traffic.log


