#!/bin/bash

# Update, install build tools, and set up Rust
sudo apt-get update
sudo apt-get install -y gcc make cmake libssl-dev libpcre2-dev libxml2-dev libjson-c-dev zlib1g-dev libbz2-dev libcurl4-openssl-dev
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
