FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format clang-tidy git python3-pip \
                       libssl-dev ninja-build && \
    pip3 install conan

WORKDIR /workspace
