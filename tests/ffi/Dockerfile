# Cb FFI Test Environment
# Multi-language support: C, C++, Rust, Go, Zig
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/root/.cargo/bin:/usr/local/go/bin:/usr/local/zig:${PATH}"

# 基本ツールのインストール
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    curl \
    wget \
    git \
    pkg-config \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Rustのインストール
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y \
    && . $HOME/.cargo/env \
    && rustc --version

# Goのインストール
RUN ARCH=$(dpkg --print-architecture) && \
    wget https://go.dev/dl/go1.21.5.linux-${ARCH}.tar.gz && \
    tar -C /usr/local -xzf go1.21.5.linux-${ARCH}.tar.gz && \
    rm go1.21.5.linux-${ARCH}.tar.gz && \
    go version

# Zigのインストール
RUN ARCH=$(dpkg --print-architecture) && \
    if [ "$ARCH" = "amd64" ]; then \
        ZIG_ARCH="x86_64"; \
    elif [ "$ARCH" = "arm64" ]; then \
        ZIG_ARCH="aarch64"; \
    else \
        ZIG_ARCH="x86_64"; \
    fi && \
    wget https://ziglang.org/download/0.11.0/zig-linux-${ZIG_ARCH}-0.11.0.tar.xz && \
    tar -C /usr/local -xf zig-linux-${ZIG_ARCH}-0.11.0.tar.xz && \
    mv /usr/local/zig-linux-${ZIG_ARCH}-0.11.0 /usr/local/zig && \
    rm zig-linux-${ZIG_ARCH}-0.11.0.tar.xz && \
    /usr/local/zig/zig version

# 作業ディレクトリ
WORKDIR /cb

# エントリーポイント
CMD ["/bin/bash"]
