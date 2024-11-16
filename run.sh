#!/bin/bash

# Use 'set -e' to exit the script if any command fails
set -e

# Function to handle cleanup
cleanup() {
    echo "Cleaning up..."
    make clean
}

# Trap INT (CTRL-C) and TERM signals to call the cleanup function
trap cleanup INT TERM

# Move to the src directory and execute the commands
cd src
make format ..

# Execute make run
make

./kvs $1 $2 $3

cd ..
