FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        make \
        file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN make fclean && make CROSS=aarch64-linux-gnu- static

CMD file sensor_reader && file logger_app
