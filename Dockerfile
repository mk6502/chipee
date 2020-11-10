FROM ubuntu:20.04

WORKDIR /app
RUN apt-get update && \
    apt-get install -y build-essential clang libsdl2-dev
