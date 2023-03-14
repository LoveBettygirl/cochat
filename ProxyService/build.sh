#!/bin/bash
mkdir -p `pwd`/build
cd `pwd`/build/ && cmake .. && make

cd ../bin
openssl genrsa -out private.pem 1024
openssl rsa -in private.pem -out public.pem -outform PEM -pubout
cd ..
