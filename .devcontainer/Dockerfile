FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format clang-tidy git python3-pip \
                       libssl-dev ninja-build curl && \
    pip3 install conan && \
    curl -L https://ziglang.org/builds/zig-linux-x86_64-0.14.1.tar.xz -o /tmp/zig.tar.xz && \
    tar -C /opt -xf /tmp/zig.tar.xz && \
    mv /opt/zig-linux-x86_64-0.14.1 /opt/zig && \
    ln -s /opt/zig/zig /usr/local/bin/zig

WORKDIR /workspace
