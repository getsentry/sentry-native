# docker build --build-arg BASE=alpine:3.24
ARG BASE=alpine:latest
FROM ${BASE}

ARG BPF_LINKER_VERSION=v0.10.4
COPY requirements.txt /tmp/requirements.txt

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
    wget \
    zstd

# mitmproxy-linux builds eBPF code for bpfel-unknown-none and needs bpf-linker plus nightly rust-src.
RUN curl -sSL --retry 5 "https://github.com/aya-rs/bpf-linker/releases/download/${BPF_LINKER_VERSION}/bpf-linker-$(uname -m)-unknown-linux-musl.tar.zst" | \
    tar --zstd -xpf - -C /usr/local/bin && \
    RUSTUP_INIT_SKIP_PATH_CHECK=yes rustup-init -y --profile minimal --default-toolchain none && \
    . /root/.cargo/env && \
    rustup toolchain install nightly --profile minimal --component rust-src && \
    ln -s /root/.cargo/bin/rustup /usr/local/bin/rustup && \
    python3 -m pip install --break-system-packages --upgrade --constraint /tmp/requirements.txt mitmproxy
ENV CARGO_HOME=/root/.cargo
ENV RUSTUP_HOME=/root/.rustup

RUN curl -sSL --retry 5 https://dot.net/v1/dotnet-install.sh | bash -eo pipefail /dev/stdin --channel 10.0 --install-dir /usr/share/dotnet
RUN ln -s /usr/share/dotnet/dotnet /usr/local/bin/dotnet

# https://github.com/actions/runner-images/blob/main/images/ubuntu/scripts/build/install-python.sh
COPY pip.conf /etc/pip.conf
ENV PIPX_BIN_DIR=/opt/pipx_bin
ENV PIPX_HOME=/opt/pipx
RUN python3 -m pip install pipx
RUN python3 -m pipx ensurepath
ENV PATH="${PIPX_BIN_DIR}:$PATH"
