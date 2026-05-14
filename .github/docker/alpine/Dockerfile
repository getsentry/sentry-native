# docker build --build-arg BASE=alpine:3.21
ARG BASE=alpine:latest
FROM ${BASE}

RUN apk add --no-cache \
    bash \
    build-base \
    cargo \
    clang \
    cmake \
    curl \
    file \
    git \
    icu \
    linux-headers \
    lsb-release-minimal \
    llvm \
    mitmproxy \
    moreutils \
    nodejs \
    npm \
    openssl-dev \
    perl \
    powershell \
    python3 \
    python3-dev \
    py3-pip \
    rustup \
    sudo \
    tar \
    tree \
    wget

# 3.23+
RUN cargo install --locked bpf-linker --root /usr/local || true
RUN RUSTUP_INIT_SKIP_PATH_CHECK=yes rustup-init -y --profile minimal --default-toolchain none \
    && . /root/.cargo/env \
    && rustup toolchain install nightly --profile minimal --component rust-src \
    || true
ENV PATH="/root/.cargo/bin:$PATH"

RUN curl -sSL --retry 5 https://dot.net/v1/dotnet-install.sh | bash -eo pipefail /dev/stdin --channel 10.0 --install-dir /usr/share/dotnet
RUN ln -s /usr/share/dotnet/dotnet /usr/local/bin/dotnet

# https://github.com/actions/runner-images/blob/main/images/ubuntu/scripts/build/install-python.sh
COPY pip.conf /etc/pip.conf
ENV PIPX_BIN_DIR=/opt/pipx_bin
ENV PIPX_HOME=/opt/pipx
RUN python3 -m pip install pipx
RUN python3 -m pipx ensurepath
ENV PATH="PIPX_BIN_DIR:$PATH"
