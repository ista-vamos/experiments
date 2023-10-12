FROM ubuntu:20.04 AS base

RUN set -e

ENV DEBIAN_FRONTEND=noninteractive

# Install packages
RUN apt-get -y update
RUN apt-get install -y --no-install-recommends\
        python3\
        python3-pip\
        g++\
        gcc\
        make\
        time

FROM base AS base-git
RUN apt-get install -y --no-install-recommends\
        cmake\
        git

FROM base-git AS dynamorio
WORKDIR /opt
RUN git clone --depth=1 https://github.com/DynamoRIO/dynamorio &&\
    mkdir -p dynamorio/build &&\
    cd dynamorio/build &&\
    cmake .. &&\
    make -j4

FROM base-git AS llvm

# Install LLVM 14
RUN echo 'deb [trusted=yes] http://apt.llvm.org/focal/  llvm-toolchain-focal-14 main' >> /etc/apt/sources.list &&\
    apt-get update
#RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh &&\
#    sed -i 's@PKG="clang-$LLVM_VERSION.*@PKG="clang-$LLVM_VERSION"@' ./llvm.sh &&\
#    ./llvm.sh 14 &&\
RUN apt-get install -y --no-install-recommends clang-14 llvm-14-dev &&\
    ln -s /usr/bin/clang-14 /usr/bin/clang &&\
    ln -s /usr/bin/clang++-14 /usr/bin/clang++ &&\
    ln -s /usr/bin/llvm-link-14 /usr/bin/llvm-link &&\
    ln -s /usr/bin/llvm-config-14 /usr/bin/llvm-config &&\
    ln -s /usr/bin/opt-14 /usr/bin/opt

# clone VAMOS
FROM llvm AS vamos
WORKDIR /opt
#RUN git clone --depth=1 https://github.com/ista-vamos/vamos -b sttt
RUN pip install lark
RUN git clone --depth=2 https://github.com/ista-vamos/vamos -b dev
#compile VAMOS
WORKDIR /opt/vamos
COPY --from=dynamorio /opt/dynamorio/build /opt/dynamorio/build
RUN make -j2 CC=clang CXX=clang++ BUILD_TYPE=Release DynamoRIO_DIR=/opt/dynamorio/build/cmake
# copy and setup the experiments
COPY . /opt/vamos/fase23-experiments
RUN make fase23-experiments

FROM base as prepare-artifact
RUN pip install matplotlib==3.6.0 pandas==1.5.0 seaborn==0.12.0 && mkdir /opt/results
# again install LLVM -- if we try to copy it, it gets broken
RUN echo 'deb [trusted=yes] http://apt.llvm.org/focal/  llvm-toolchain-focal-14 main' >> /etc/apt/sources.list &&\
    apt-get update
RUN apt-get install -y --no-install-recommends\
        cargo\
	clang-14 \
	llvm-14\
        openjdk-17-jdk\
        valgrind && \
        ln -s /usr/bin/clang-14 /usr/bin/clang &&\
        ln -s /usr/bin/clang++-14 /usr/bin/clang++ &&\
        ln -s /usr/bin/llvm-link-14 /usr/bin/llvm-link &&\
        ln -s /usr/bin/llvm-config-14 /usr/bin/llvm-config &&\
        ln -s /usr/bin/opt-14 /usr/bin/opt


FROM prepare-artifact as artifact
COPY --from=vamos /opt/vamos /opt/vamos
COPY --from=dynamorio /opt/dynamorio/build /opt/dynamorio/build
WORKDIR /opt/vamos/fase23-experiments

# "install" TSan for dataraces experiments
# for some reason we cannot install TSan normally
# Install packages for generating plots and prepare the dir for results
COPY ./dataraces/libclang_rt.tsan-x86_64.a /usr/lib/llvm-14/lib/clang/14.0.6/lib/linux/libclang_rt.tsan-x86_64.a

