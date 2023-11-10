#!/bin/bash

file1="tpByPktLossRate"
file2="tpByBottleneckDataRate"

# first arg is foldername
# if folder name not empty
if [ -n "$1" ]; then
    rm -rf "scratch/$1"
    mkdir "scratch/$1"
fi

# bottle data rate experiment ( 1, 50, 100, 150, 200, 250, 300 Mbps)
for i in 1 50 100 150 200 250 300; do
    echo "Running experiment with bottleneck data rate = $i Mbps"
    ./ns3 run "offline1 --totalPackets=10000000 --bottleNeckDataRate=$i --outputFolder=scratch/$1 --errorRate=0.000001 --outputFile=$file2 --verbose=false --tcp2=$2"
done

# packet loss rate experiment (0.000001, 0.00001, 0.0001, 0.001, 0.01)
for i in 0.000001 0.00001 0.0001 0.001 0.01; do
    echo "Running experiment with packet loss rate = $i"
    ./ns3 run "offline1 --totalPackets=10000000 --bottleNeckDataRate=50 --outputFolder=scratch/$1 --errorRate=$i --outputFile=$file1 --verbose=false --tcp2=$2"
done

gnuplot -persist <<EOFMarker
    set terminal png size 640,480;
    set output "scratch/$1/TPvsBottleneckDataRate.png";
    set title "Throughput VS Bottleneck Data Rate";
    plot "scratch/$1/$file2.txt" using 1:3 title "TCP New Reno" with linespoints,"scratch/$1/$file2.txt" using 1:4 title "$2" with linespoints;
EOFMarker

gnuplot -persist <<EOFMarker
    set terminal png size 640,480;
    set output "scratch/$1/JIvsBottleneckDataRate.png";
    set title "Jain's Fairness Index VS Bottleneck Data Rate";
    plot "scratch/$1/$file2.txt" using 1:5 title "JI" with linespoints;
EOFMarker

gnuplot -persist <<EOFMarker
    set terminal png size 640,480;
    set output "scratch/$1/TPvsPacketLossRate.png";
    set title "Throughput VS Packet Loss Rate";
    plot "scratch/$1/$file1.txt" using 2:3 title "TCP New Reno" with linespoints, "scratch/$1/$file1.txt" using 2:4 title "$2" with linespoints;
EOFMarker

gnuplot -persist <<EOFMarker
    set terminal png size 640,480;
    set output "scratch/$1/JIvsPacketLossRate.png";
    set title "Jain's Fairness Index VS Packet Loss Rate";
    plot "scratch/$1/$file1.txt" using 2:5 title "JI" with linespoints;
EOFMarker

./ns3 run "offline1 --totalPackets=10000000 --bottleNeckDataRate=150 --outputFolder=scratch/$1 --errorRate=0.001 --outputFile=temp --verbose=false --tcp2=$2"

gnuplot -persist <<EOFMarker
    set terminal png size 640,480;
    set output "scratch/$1/CongVsTime.png";
    set title " VS Congestion Window Over Time";
    plot "scratch/$1/flow1.tr" using 1:2 title "TCP New Reno" with linespoints,"scratch/$1/flow2.tr" using 1:2 title "$2" with linespoints;
EOFMarker