# f1985gfw
High performance game server framework and tool chain.

- building and installing dependent
    ```bash
    mkdir ${PWD}/dependent

    wget https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz
    tar -zxvf boost_1_78_0.tar.gz
    cd boost_1_78_0
    ./bootstrap.sh --prefix=${PWD}/dependent/boost
    ./b2
    ./b2 headers
    ./b2 install

    wget https://github.com/protocolbuffers/protobuf/releases/download/v3.20.0/protobuf-cpp-3.20.0.tar.gz
    tar -zxvf protobuf-cpp-3.20.0.tar.gz
    cd protobuf-3.20.0/
    ./configure --prefix=${PWD}/dependent/protobuf
    make
    make install
    ```
