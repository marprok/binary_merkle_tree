# binary_merkle_tree
Compute the root of the binary merkle tree for a given file.

# Prerequisites
Makes use of `libssl-dev`

# Compile
`g++ -Wall -Wextra -std=c++20 main.cc -o main -lcrypto`

# Usage
`./main file_name block_size`
