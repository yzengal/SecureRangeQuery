# SecureRangeQuery

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
