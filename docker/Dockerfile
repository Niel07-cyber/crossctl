FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        python3 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
COPY . /work

RUN make clean && make

CMD ["make", "test"]
