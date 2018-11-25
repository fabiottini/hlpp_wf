#!/bin/bash

###### BW #####
#NON CE LA FANNO
./scripts/mixedGraph2.sh "agnostic_1-13_r100000_i0_w5000_s3000_sbt_50_BW.txt" "agnostic_1-13_r100000_i0_w5000_s5000_sbt_50_BW.txt" 1 "BW" "w: 5000ms s: 3000tuples" "w: 5000ms s: 5000tuples" 0 25
#CE LA FANNO
./scripts/mixedGraph2.sh "agnostic_1-13_r100000_i0_w5000_s7500_sbt_50_BW.txt" "agnostic_1-13_r100000_i0_w5000_s10000_sbt_50_BW.txt" 1 "BW" "w: 5000ms s: 7500tuples" "w: 5000ms s: 10000tuples" 0 25


###### SCALABILITY #####
#NON CE LA FANNO
./scripts/mixedGraph2.sh "agnostic_1-13_r100000_i0_w5000_s3000_sbt_50_SCALABILITY.txt" "agnostic_1-13_r100000_i0_w5000_s5000_sbt_50_SCALABILITY.txt" 1 "SCALABILITY" "w: 5000ms s: 3000tuples" "w: 5000ms s: 5000tuples" 
#CE LA FANNO
./scripts/mixedGraph2.sh "agnostic_1-13_r100000_i0_w5000_s7500_sbt_50_SCALABILITY.txt" "agnostic_1-13_r100000_i0_w5000_s10000_sbt_50_SCALABILITY.txt" 1 "SCALABILITY" "w: 5000ms s: 7500tuples" "w: 5000ms s: 10000tuples"
