#!/bin/sh

for f in test/*/*.cpp; do 
    sed -n "s/BOOST_AUTO_TEST_CASE[ \t]*([ \t]*\(.*\)[ \t]*)/\1/p" "$f"
done

