
rm -rf scratch/statsM
mkdir -p scratch/statsM

rm -rf scratch/plotsM
mkdir -p scratch/plotsM

for n in 20 40 60 80 100
do
    ./ns3 run "offline2 --nNodes=$n --fileName=Node$n.dat"
    cat scratch/statsM/Node$n.dat >> scratch/statsM/Nodes.dat
    rm -rf scratch/statsM/Node$n.dat
done

for n in 10 20 30 40 50
do
    ./ns3 run "offline2 --nFlows=$n --fileName=Flow$n.dat" 
    cat scratch/statsM/Flow$n.dat >> scratch/statsM/Flows.dat
    rm -rf scratch/statsM/Flow$n.dat
done


for n in 5 10 15 20 25
do
    ./ns3 run "offline2 --speed=$n --fileName=Speed$n.dat"
    cat scratch/statsM/Speed$n.dat >> scratch/statsM/Speeds.dat
    rm -rf scratch/statsM/Speed$n.dat
done

for n in 100 200 300 400 500
do
    ./ns3 run "offline2 --nPackets=$n --fileName=Packet$n.dat"
    cat scratch/statsM/Packet$n.dat >> scratch/statsM/Packets.dat
    rm -rf scratch/statsM/Packet$n.dat
done

echo 'set terminal png size 640,480;
set output "scratch/plotsM/TPvsNodes.png";
plot "scratch/statsM/Nodes.dat" using 1:5 title "Nodes VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/DRvsNodes.png";
plot "scratch/statsM/Nodes.dat" using 1:6 title "Nodes VS Delivery Ratio" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/TPvsFlows.png";
plot "scratch/statsM/Flows.dat" using 2:5 title "Flows VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/DRvsFlows.png";
plot "scratch/statsM/Flows.dat" using 2:6 title "Flows VS Delivery Ratio" with linespoints' | gnuplot


echo 'set terminal png size 640,480;
set output "scratch/plotsM/TPvsSpeed.png";
plot "scratch/statsM/Speeds.dat" using 3:5 title "Speed VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/DRvsSpeed.png";
plot "scratch/statsM/Speeds.dat" using 3:6 title "Speed VS Delivery Ratio" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/TPvsPackets.png";
plot "scratch/statsM/Packets.dat" using 4:5 title "Packets VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plotsM/DRvsPackets.png";
plot "scratch/statsM/Packets.dat" using 4:6 title "Packets VS Delivery Ratio" with linespoints' | gnuplot

