#!/usr/bin/env bash

# -- sphinx-include-start --
gcc -o hello_mongoc hello_mongoc.c $(pkg-config --libs --cflags mongoc$major-static)
