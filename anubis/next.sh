#!/bin/bash

echo "This is a script"

for i in {0..10..2}; do
	echo "$(echo "$(($i * 7))$(($i * 4))" | wc -c)"
done
