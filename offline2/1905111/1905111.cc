#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/flow-monitor-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FifthScriptExample");

class TutorialApp : public Application
{
  public:
    TutorialApp();
    ~TutorialApp() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Setup the socket.
     * \param socket The socket.
     * \param address The destination address.
     * \param packetSize The packet size to transmit.
     * \param nPackets The number of packets to transmit.
     * \param dataRate the data rate to use.
     */
    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               uint32_t nPackets,
               DataRate dataRate);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /// Schedule a new transmission.
    void ScheduleTx();
    /// Send a packet.
    void SendPacket();

    Ptr<Socket> m_socket;   //!< The transmission socket.
    Address m_peer;         //!< The destination address.
    uint32_t m_packetSize;  //!< The packet size.
    uint32_t m_nPackets;    //!< The number of packets to send.
    DataRate m_dataRate;    //!< The data rate to use.
    EventId m_sendEvent;    //!< Send event.
    bool m_running;         //!< True if the application is running.
    uint32_t m_packetsSent; //!< The number of packets sent.
};

TutorialApp::TutorialApp()
    : m_socket(nullptr),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

TutorialApp::~TutorialApp()
{
    m_socket = nullptr;
}

/* static */
TypeId
TutorialApp::GetTypeId()
{
    static TypeId tid = TypeId("TutorialApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<TutorialApp>();
    return tid;
}

void
TutorialApp::Setup(Ptr<Socket> socket,
                   Address address,
                   uint32_t packetSize,
                   uint32_t nPackets,
                   DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void
TutorialApp::StartApplication()
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void
TutorialApp::StopApplication()
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
TutorialApp::SendPacket()
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void
TutorialApp::ScheduleTx()
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &TutorialApp::SendPacket, this);
    }
}

/**
 * Congestion window change callback
 *
 * \param oldCwnd Old congestion window.
 * \param newCwnd New congestion window.
 */
static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{  
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

int
main(int argc, char* argv[])
{
    uint32_t nLeaf = 2;
    int bndr = 1;
    uint32_t nPackets = 100;
    uint32_t nFlows = 2;
    std::string tcp1 = "ns3::TcpNewReno";
    std::string tcp2 = "ns3::TcpHighSpeed"; // ns3::TcpNewReno ns3::TcpAdaptiveReno ns3::TcpHighSpeed
    std::string senderDataRate = "1Gbps";
    std::string bottleNeckDelay = "100ms";
    std::string senderDelay = "1ms";
    double simulationTime = 60.0;
    double errorRate = 0.00001;
    std::string outputFolder = "scratch/outputs";
    std::string outputFile = "tpByPktLossRate"; // tpByBottleneckDataRate
    bool verbose = true;
    int totalPackets = 1000;

    CommandLine cmd(__FILE__);
    cmd.AddValue("totalPackets", "Number of packets to send", totalPackets);
    cmd.AddValue("tcp2", "2nd tcp variant", tcp2);
    cmd.AddValue("bottleNeckDataRate", "Bottleneck Data Rate", bndr);
    cmd.AddValue("nPackets", "Number of packets per second", nPackets);
    cmd.AddValue("errorRate", "Error Rate", errorRate);
    cmd.AddValue("outputFolder", "Output folder", outputFolder);
    cmd.AddValue("outputFile", "Output file", outputFile);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);

    cmd.Parse(argc, argv);

    std::string bottleNeckDataRate = std::to_string(bndr) + "Mbps";
    int packetSize = 1024; // bytes
    std::string dataRate = std::to_string(nPackets) + "kbps";
    std::string filePath = outputFolder + "/" + outputFile + ".txt";
    std::ofstream out(filePath, std::ios::app);


    Time::SetResolution(Time::NS);

    
    PointToPointHelper bottleNeckLink;
    bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckDataRate));
    bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckDelay));
    
    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue(senderDataRate));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue(senderDelay));
    uint32_t bandwidth_delay_product = bndr * 1000 / 1024;
    pointToPointLeaf.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (std::to_string (bandwidth_delay_product) + "p"));
    PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
    d.m_routerDevices.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d.m_routerDevices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    // TCP 1
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp1));
    InternetStackHelper stack1;
    stack1.Install(d.GetLeft(0));
    stack1.Install(d.GetRight(0));
    stack1.Install(d.GetLeft());      
    stack1.Install(d.GetRight());  

    // TCP 2
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp2));
    InternetStackHelper stack2;
    stack2.Install(d.GetLeft(1));
    stack2.Install(d.GetRight(1));

    // IPs
    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                            Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                            Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

      // flow monitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    uint16_t sinkPort = 8080;
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    for(uint32_t i = 0; i < nFlows; i++){
        Address sinkAddress(InetSocketAddress(d.GetRightIpv4Address(i), sinkPort));
        ApplicationContainer sinkApps = packetSinkHelper.Install(d.GetRight(i));
        sinkApps.Start(Seconds(0.));
        sinkApps.Stop(Seconds(simulationTime));

        Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(d.GetLeft(i), TcpSocketFactory::GetTypeId());
        // ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));

        Ptr<TutorialApp> app = CreateObject<TutorialApp>();
        app->Setup(ns3TcpSocket, sinkAddress, packetSize, totalPackets, DataRate(senderDataRate));
        d.GetLeft(i)->AddApplication(app);
        app->SetStartTime(Seconds(1.));
        app->SetStopTime(Seconds(simulationTime));

        std::ostringstream oss;
        oss << outputFolder << "/flow" << i + 1 << ".tr";
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(oss.str());
        ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&CwndChange, stream)); 
        // d.GetLeft(i)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));
    }

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    float thoughputs[2] = {0, 0};
    double jainDenominator = 0;
    double jainNumerator = 0;

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
    if(verbose) std::cout << "Flow\tSrc\t\tDst\t\tPackets\tBytes\tThroughput\tDelay\t\tJitter" << std::endl;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        if(verbose) std::cout << i->first << "\t" << t.sourceAddress << "\t" << t.destinationAddress << "\t" << i->second.txPackets << "\t" << i->second.txBytes << "\t" << i->second.rxBytes * 8.0 / (simulationTime * 1000000.0) << " Mbps"
                  << "\t" << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\t" << i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1) << std::endl;
        double throughput = i->second.rxBytes * 8.0 / (simulationTime * 1000000.0);
        thoughputs[(i->first - 1) % 2] += throughput;
        jainDenominator += pow(throughput, 2);
        jainNumerator += throughput;
    }
    if(verbose) std::cout << "BottleNeckDataRate: " << bottleNeckDataRate << " | ErrorRate: " << errorRate << " | Thoughput 1 : " << thoughputs[0] << " | Thoughput 2 : " << thoughputs[1] << std::endl;
    // take the log10 of the error rate
    double logErrorRate = log10(errorRate);
    double jainIndex = pow(jainNumerator, 2) / (nFlows * jainDenominator);
    out << bndr << "\t" << logErrorRate << "\t" << thoughputs[0] << "\t" << thoughputs[1] << "\t" << jainIndex << std::endl;
    out.close();

    Simulator::Destroy();

    return 0;
}
