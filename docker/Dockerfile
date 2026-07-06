FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libhiredis-dev \
    libssl-dev \
    gcc \
    g++ \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY cmake/ cmake/
COPY shared/ shared/
COPY sandbox/ sandbox/
COPY api/ api/
COPY worker/ worker/

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# --- API image ---
FROM ubuntu:22.04 AS api

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libhiredis0.14 \
    libssl3 \
    gcc \
    g++ \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/api/ocpe_api /app/ocpe_api
RUN mkdir -p /app/data/submissions /app/problems

EXPOSE 8080
CMD ["/app/ocpe_api"]

# --- Worker image ---
FROM ubuntu:22.04 AS worker

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libhiredis0.14 \
    libssl3 \
    gcc \
    g++ \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/worker/ocpe_worker /app/ocpe_worker
RUN mkdir -p /app/data/submissions /app/problems

CMD ["/app/ocpe_worker"]

# --- Web image ---
FROM nginx:alpine AS web
COPY web/ /usr/share/nginx/html/
COPY docker/nginx.conf /etc/nginx/conf.d/default.conf
