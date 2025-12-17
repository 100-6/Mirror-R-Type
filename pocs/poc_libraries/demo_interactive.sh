#!/bin/bash

cd build

echo ""
echo "=========================================="
echo "Benchmark: Execution time"
echo "=========================================="
echo ""

echo -n "Static (.a):     "
(time ./poc_demo > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}'

echo -n "Dynamic (.so):   "
(time ./benchmark > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}'

echo ""
echo "File sizes:"
ls -lh poc_demo libbasic_enemy.so libboss_enemy.so 2>/dev/null | awk 'NR>1{printf "%-30s %10s\n", $9, $5}'

echo ""

cd ..
