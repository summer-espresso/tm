#!/bin/bash

echo "Example long task running from 1 to 10"

for f in $(seq 1 10)
do
    echo "$f ..."
    sleep 1
done

echo "Example task done"

exit 0
