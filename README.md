# SecureRangeQuery

[![License](https://img.shields.io/badge/License-Apache--3.0-purple.svg)](https://www.apache.org/licenses/LICENSE-2.0.html)
![](https://img.shields.io/badge/Platform-linux-yellow.svg)
![](https://img.shields.io/badge/Language-C++-red.svg)
[![Total Lines](https://tokei.rs/b1/github/yzengal/SecureRangeQuery?category=lines)](https://github.com/yzengal/SecureRangeQuery)

This depository provides a secure and extension implementation of range query processings.

## Environment

Linux: Ubuntu 18.04.4 LTS   
GCC/G++: 8.4.0   
CMake: 3.22.1   
gRPC: 1.64.0   
SEAL: 4.1.1    

## Required Third-Party Library

### gRPC: the RPC framework developed by Google
#### Install gRPC

Execute the following commands:
```
git clone https://github.com/grpc/grpc.git
export MY_INSTALL_DIR=$HOME/.local
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
make -j 4
make install
```

### SEAL: the homomorphic encryption library developed by Microsoft
#### Install SEAL

Execute the following commands:
```
git clone https://github.com/microsoft/SEAL.git
export MY_INSTALL_DIR=$HOME/.local
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"
cd SEAL
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR
cmake --build build
cmake --install build
```

### Data: generate the synthetic dataset

#### Generate data

Execute the following commands to generate the synthetic data
```
cd generator
python generate_data.py <n> <data_filename>
```
n: the number of points   
data_filename: the file name of the generated 2D points  

#### Generate query

Execute the following commands to generate the query workload
```
cd generator
python generate_query.py <n> <query_filename>
```
n: the number of (circular) range queries
query_filename: the file name of the generated query workload

#### Generate ground truth

Execute the following commands to generate the query workload
```
cd generator
python generate_truth.py <data_filename> <query_filename> <truth_filename>
```
n: the number of queries
data_filename: the file name of the generated 2D points     
query_filename: the file name of the generated query workload    
truth_filename: the ground truth of the generated queries   

### Run the program

#### Compile the program

Execute the following commands to compile the server and silo programs:
```
mkdir -p ./cmake/build/
cd bash
chmod +x ./compile.sh
./compile.sh
```
After executing these commands, there will be executable programs **server** and **silo** in the directory **./cmake/build**

#### Run the silo program
Execute the following commands to run the silo program:
```
cd bash
chmod +x ./silo.sh
./silo.sh
```
When terminating the silo program, enter ```ctrl+c```

#### Run the server program

After running the silo program, execute the following commands to run the server program
```
cd bash
chmod +x ./server.sh
./server.sh >../data/result.txt
```
After executing these commands, the query result will be dumped into the file **../data/result.txt**

#### Compute the recall and accuracy

Execute the following commands to compute the recall and accuracy:
```
cd generator
python compute_query_accuracy.py <truth_filename> <result_filename> <output_filename(optional)>
```
After executing these commands, the recall and accuracy will be dumped into the file **\<output_filename\>**.


### Note

1. Notice the above algorithm cannot be used to answer range counting query.
2. The current code hasn't been tested with rectangular range query.

### Reference

1. Cetin Sahin, Tristan Allard, Reza Akbarinia, Amr El Abbadi, Esther Pacitti. **A Differentially Private Index for Range Query Processing in Clouds**. ICDE 2018. [PDF](https://doi.org/10.1109/ICDE.2018.00082)
   
2. Hoang Van Tran, Tristan Allard, Laurent d'Orazio, Amr El Abbadi. **FRESQUE: A Scalable Ingestion Framework for Secure Range Query Processing on Clouds**. EDBT 2021. [PDF](https://doi.org/10.5441/002/edbt.2021.19)

### Contact

yxzeng@buaa.edu.cn
