#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThirdScriptExample");
void addApplication(std::string dataRate, int packetSize, int nFlows, ApplicationContainer* sinkApps, ApplicationContainer* senderApps, Ipv4InterfaceContainer receiverInterfaces, Ipv4InterfaceContainer senderInterfaces, NodeContainer receiverWifiStaNodes, NodeContainer senderWifiStaNodes, uint32_t nWifiStatNodes);
Ptr<OutputStreamWrapper> stream;
int totalBytesSent = 0;
int totalBytesReceived = 0;
int nPacketsSent = 0;
int nPacketsReceived = 0;

// rx callback
static void
RxCallback(std::string fileName, Ptr<const Packet> packet, const Address &address)
{
    totalBytesReceived += packet->GetSize();
    // ignore everything but data packets
    if (packet->GetSize() > 0)
        nPacketsReceived++;
}

// tx callback
static void
TxCallback(std::string fileName, Ptr<const Packet> packet)
{
    totalBytesSent += packet->GetSize();
    // ignore everything but data packets
    if (packet->GetSize() > 0)
        nPacketsSent++;
}


int
main(int argc, char* argv[])
{
    uint32_t nNodes = 20;
    uint32_t nFlows = 10; // what is that? what is ack flow?
    uint32_t nPackets = 100;
    uint32_t coverageAreaMultiplier = 1; // for static
    uint32_t tx_range = 5; // for static
    std::string fileName = "tpvsflow";
    
    CommandLine cmd(__FILE__);
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("nPackets", "Number of packets per second", nPackets);
    cmd.AddValue("coverageAreaMultiplier", "Coverage area multiplier*Tx_range", coverageAreaMultiplier);
    cmd.AddValue("fileName", "Output file name", fileName);

    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    // LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    uint32_t nWifiStatNodes = nNodes / 2 - 1; 
    int packetSize = 1024; // bytes
    std::string dataRate = std::to_string(nPackets) + "kbps";
    uint32_t coverageArea = coverageAreaMultiplier * tx_range;



    // the bottleneck link
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("50ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    // now we create sender nodes.
    // They connect to the ap which is the first node of p2pNodes
    // via WiFi.
    NodeContainer senderWifiStaNodes;
    senderWifiStaNodes.Create(nWifiStatNodes);
    NodeContainer senderAPNode = p2pNodes.Get(0);

    // now the receiver nodes.
    // They connect to the ap which is the second node of p2pNodes
    // via WiFi.
    NodeContainer receiverWifiStaNodes;
    receiverWifiStaNodes.Create(nWifiStatNodes);
    NodeContainer receiverAPNode = p2pNodes.Get(1);

    // channel and adding channel to physical layer
    YansWifiChannelHelper channelSender = YansWifiChannelHelper::Default();
    YansWifiChannelHelper channelReceiver = YansWifiChannelHelper::Default();
    channelSender.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange",
                                   DoubleValue(coverageArea)); 
    channelReceiver.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange",
                                   DoubleValue(coverageArea)); 

    YansWifiPhyHelper phySender, phyReceiver;
    phySender.SetChannel(channelSender.Create());
    phyReceiver.SetChannel(channelReceiver.Create());

    // mac layer
    WifiMacHelper senderMac;
    WifiMacHelper receiverMac;
    Ssid sender_ssid = Ssid("sender");
    Ssid receiver_ssid = Ssid("receiver");
    senderMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(sender_ssid), "ActiveProbing", BooleanValue(false));
    receiverMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(receiver_ssid), "ActiveProbing", BooleanValue(false));

    // installing wifi with physical and mac layer
    WifiHelper wifi;
    NetDeviceContainer senderStaDevices = wifi.Install(phySender, senderMac, senderWifiStaNodes);
    NetDeviceContainer receiverStaDevices = wifi.Install(phyReceiver, receiverMac, receiverWifiStaNodes);

    // active probing for ap nodes
    senderMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(sender_ssid));
    receiverMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(receiver_ssid));
    NetDeviceContainer senderAPDevices = wifi.Install(phySender, senderMac, senderAPNode);
    NetDeviceContainer receiverAPDevices = wifi.Install(phyReceiver, receiverMac, receiverAPNode);


    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(.05),
                                  "DeltaY",
                                  DoubleValue(.05),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.Install(senderWifiStaNodes);
    mobility.Install(receiverWifiStaNodes);


    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(senderAPNode);
    mobility.Install(receiverAPNode);



    // now we install internet stack on all nodes.
    InternetStackHelper stack;
    stack.Install(p2pNodes);
    stack.Install(senderWifiStaNodes);
    stack.Install(receiverWifiStaNodes);

    // adding IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer senderInterfaces = address.Assign(senderStaDevices);
    Ipv4InterfaceContainer senderAPInterfaces = address.Assign(senderAPDevices);


    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer receiverInterfaces = address.Assign(receiverStaDevices);
    Ipv4InterfaceContainer receiverAPInterfaces = address.Assign(receiverAPDevices);
 

   

    ApplicationContainer sinkApps;
    ApplicationContainer senderApps;
    addApplication(dataRate, packetSize, nFlows, &sinkApps, &senderApps, receiverInterfaces, senderInterfaces, receiverWifiStaNodes, senderWifiStaNodes, nWifiStatNodes);

    sinkApps.Start(Seconds(1.0));
    senderApps.Start(Seconds(2.0));
    sinkApps.Stop(Seconds(10.0));
    senderApps.Stop(Seconds(9.0));
    

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(10.5));

    
    AsciiTraceHelper asciiTraceHelper;
    stream  = asciiTraceHelper.CreateFileStream("scratch/stats/" + fileName);

    // install trace here for calculating throughput
    for(uint32_t i = 0; i < nWifiStatNodes; i++)
        sinkApps.Get(i)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxCallback, "scratch/stats/throughput.txt"));
  
    for(uint32_t i = 0; i < nFlows; i++)
        senderApps.Get(i)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TxCallback, "scratch/stats/throughput.txt"));
    
    Simulator::Run();
    Simulator::Destroy();


    std::cout << nNodes  << "\t" << (double)totalBytesReceived * 8 / 9 / 1000 << "kBit/s" << std::endl;
    *stream->GetStream() << nNodes << "\t" << nFlows << "\t" << coverageAreaMultiplier << "\t" << nPackets << "\t" << 
    (double)totalBytesReceived * 8 / 9  / 1000 << "\t" << (double)nPacketsReceived / nPacketsSent << std::endl;
  
  
    std::cout << "Packet Delivery Ratio: " << (double)nPacketsReceived / nPacketsSent << std::endl;
    std::cout << "R/S byte Ratio " << (double)totalBytesReceived / totalBytesSent << std::endl;
    return 0;
}

void addApplication(std::string dataRate, int packetSize, int nFlows, ApplicationContainer* sinkApps, ApplicationContainer* senderApps, Ipv4InterfaceContainer receiverInterfaces, Ipv4InterfaceContainer senderInterfaces, NodeContainer receiverWifiStaNodes, NodeContainer senderWifiStaNodes, uint32_t nWifiStatNodes){
    // changing segment size to packetSize
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));
    /* Install TCP Receiver on the access point */
    for(uint32_t i = 0; i < nWifiStatNodes; i++){
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), 9));
        ApplicationContainer sinkApp = sinkHelper.Install(receiverWifiStaNodes.Get(i));
        sinkApps->Add(sinkApp);
    }

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper sender_helper("ns3::TcpSocketFactory", Address ());
    sender_helper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    sender_helper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    sender_helper.SetAttribute("PacketSize", UintegerValue(packetSize));    
    sender_helper.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    int cnt = 0;
    for(uint32_t i = 0; i < nWifiStatNodes; i++){
        for(uint32_t j = 0; j < nWifiStatNodes; j++){
            sender_helper.SetAttribute("Remote", AddressValue(InetSocketAddress(receiverInterfaces.GetAddress(i), 9)));
           
            ApplicationContainer senderApp = sender_helper.Install(senderWifiStaNodes.Get(j));
            senderApps->Add(senderApp);
            if(++cnt >= nFlows) break;
        }
        if(cnt >= nFlows) break;
    }
}
