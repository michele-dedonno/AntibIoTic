#!/bin/bash
export GOPATH=$(pwd)

# Create bin folder
mkdir -p bin

# Install dependencies
go get golang.org/x/crypto/ssh
go get golang.org/x/sys/unix

# Compile the code
go build -o bin/gateway gateway
