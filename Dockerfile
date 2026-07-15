# Stage 1 - Build
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    git \
    gcc \
    make \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Clone YOUR fork of AquapureD
ARG REPO_URL=https://github.com/Kranzy/AquapureD.git
RUN git clone ${REPO_URL} .

# Remove any pre-compiled ARM object files so they get recompiled for the host arch
RUN rm -f *.o GPIO_Pi/*.o minIni/*.o

# Build the main binary
RUN make

# Stage 2 - Runtime
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Create required directories
RUN mkdir -p /app/web \
             /app/release \
             /var/log \
             /var/lib/aquapured \
             /config

WORKDIR /app

# Copy binary and web files from builder
COPY --from=builder /build/release/aquapured ./release/aquapured
COPY --from=builder /build/web ./web

# Copy config template from your repo (contains placeholders, not real values)
COPY --from=builder /build/release/aquapured.conf ./release/aquapured.conf

# Copy entrypoint script from repo (cloned in builder stage)
COPY --from=builder /build/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Expose web port
EXPOSE 8080

ENTRYPOINT ["/entrypoint.sh"]
