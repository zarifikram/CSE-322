
rm -rf scratch/stats
mkdir -p scratch/stats

rm -rf scratch/plots
mkdir -p scratch/plots

for n in 20 40 60 80 100
do
    ./ns3 run "offline1 --nNodes=$n --fileName=Node$n.dat"
    cat scratch/stats/Node$n.dat >> scratch/stats/Nodes.dat
    rm -rf scratch/stats/Node$n.dat
done

for n in 10 20 30 40 50
do
    ./ns3 run "offline1 --nFlows=$n --fileName=Flow$n.dat" 
    cat scratch/stats/Flow$n.dat >> scratch/stats/Flows.dat
    rm -rf scratch/stats/Flow$n.dat
done


for n in 1 2 3 4 5
do
    ./ns3 run "offline1 --coverageAreaMultiplier=$n --fileName=Area$n.dat"
    cat scratch/stats/Area$n.dat >> scratch/stats/Area.dat
    rm -rf scratch/stats/Area$n.dat
done

for n in 100 200 300 400 500
do
    ./ns3 run "offline1 --nPackets=$n --fileName=Packet$n.dat"
    cat scratch/stats/Packet$n.dat >> scratch/stats/Packets.dat
    rm -rf scratch/stats/Packet$n.dat
done

echo 'set terminal png size 640,480;
set output "scratch/plots/TPvsNodes.png";
plot "scratch/stats/Nodes.dat" using 1:5 title "Nodes VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/DRvsNodes.png";
plot "scratch/stats/Nodes.dat" using 1:6 title "Nodes VS Delivery Ratio" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/TPvsFlows.png";
plot "scratch/stats/Flows.dat" using 2:5 title "Flows VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/DRvsFlows.png";
plot "scratch/stats/Flows.dat" using 2:6 title "Flows VS Delivery Ratio" with linespoints' | gnuplot


echo 'set terminal png size 640,480;
set output "scratch/plots/TPvsArea.png";
plot "scratch/stats/Area.dat" using 3:5 title "Area VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/DRvsArea.png";
plot "scratch/stats/Area.dat" using 3:6 title "Area VS Delivery Ratio" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/TPvsPackets.png";
plot "scratch/stats/Packets.dat" using 4:5 title "Packets VS Throughput" with linespoints' | gnuplot

echo 'set terminal png size 640,480;
set output "scratch/plots/DRvsPackets.png";
plot "scratch/stats/Packets.dat" using 4:6 title "Packets VS Delivery Ratio" with linespoints' | gnuplot

