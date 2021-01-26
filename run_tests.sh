#!/bin/bash

TESTS=$(find build/tests -executable -type f)
for test in $TESTS
do
    echo "running: "$test
    $test
done
