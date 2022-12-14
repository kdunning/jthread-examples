#!/usr/bin/bash

echo "Building Example 1"
g++ -std=c++20 -o ex1 jthread-ex1-basic/jthread-ex1-basic.cpp

echo "Building Example 2"
g++ -std=c++20 -o ex2 jthread-ex2-stopping/jthread-ex2-stopping.cpp

echo "Building Example 3"
g++ -std=c++20 -o ex3 jthread-ex3-more-stopping/jthread-ex3-more-stopping.cpp

echo "Building Example 4"
g++ -std=c++20 -o ex4 jthread-ex4-adv-stopping/jthread-ex4-adv-stopping.cpp

echo "Building Example 5"
g++ -std=c++20 -o ex5 jthread-ex5-class-basic/jthread-ex5-class-basic.cpp

echo "Building Example 6"
g++ -std=c++20 -o ex6 jthread-ex6-class-adv/jthread-ex6-class-adv.cpp