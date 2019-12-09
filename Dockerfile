# Development/testing environment for Linux
FROM ubuntu:19.04

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    git make curl wget ca-certificates less vim \
    clang uuid-dev libcurl-openssl-dev zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

#RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-3.9 370 \
#  && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.9 370

# Install premake
RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-linux.tar.gz \
  && tar xvf ./premake-5.0.0-alpha14-linux.tar.gz \
  && rm premake-5.0.0-alpha14-linux.tar.gz \
  && mv premake5 /usr/local/bin

WORKDIR /work
