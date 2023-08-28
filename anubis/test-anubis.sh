#! /bin/bash

if ! [[ -x anubis ]]; then
    echo "anubis executable does not exist"
    exit 1
fi

../tester/run-tests.sh $*


