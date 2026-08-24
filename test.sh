#!/usr/bin/env bash

./build.sh re


CMD="./build/release/phosphor"
ARGS="-p 1000 -w 1000 -i 8"

for model in ./models/test/*glb;
do
	echo $(basename "$model")
done
