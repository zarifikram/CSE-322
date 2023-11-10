#include "tcp-adaptive-reno.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("TcpAdaptiveReno");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpAdaptiveReno);

TypeId
TcpAdaptiveReno::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpAdaptiveReno")
            .SetParent<TcpNewReno>()
            .SetGroupName("Internet")
            .AddConstructor<TcpAdaptiveReno>()
            .AddAttribute(
                "FilterType",
                "Use this to choose no filter or Tustin's approximation filter",
                EnumValue(TcpAdaptiveReno::TUSTIN),
                MakeEnumAccessor(&TcpAdaptiveReno::m_fType),
                MakeEnumChecker(TcpAdaptiveReno::NONE, "None", TcpAdaptiveReno::TUSTIN, "Tustin"))
            .AddTraceSource("EstimatedBW",
                            "The estimated bandwidth",
                            MakeTraceSourceAccessor(&TcpAdaptiveReno::m_currentBW),
                            "ns3::TracedValueCallback::DataRate");
    return tid;
}

TcpAdaptiveReno::TcpAdaptiveReno()
    : TcpWestwoodPlus(),
        m_minRtt(Time(0)),
        m_currentRtt(Time(0)),
        m_congRtt(Time(0)),
        m_congRttEst(Time(0)),
        m_packetLossRtt(Time(0)),
        m_baseWnd(0),
        m_probWnd(0),
        m_incWnd(0)
{
    NS_LOG_FUNCTION(this);
}

TcpAdaptiveReno::TcpAdaptiveReno(const TcpAdaptiveReno& sock)
    : TcpWestwoodPlus(sock),
      m_minRtt(Time(0)),
        m_currentRtt(Time(0)),
        m_congRtt(Time(0)),
        m_congRttEst(Time(0)),
        m_packetLossRtt(Time(0)),
        m_baseWnd(0),
        m_probWnd(0),
        m_incWnd(0)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Invoked the copy constructor");
}

TcpAdaptiveReno::~TcpAdaptiveReno()
{
}

void
TcpAdaptiveReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

    if (rtt.IsZero())
    {
        NS_LOG_WARN("RTT measured is zero!");
        return;
    }

    m_ackedSegments += packetsAcked;
    if(m_minRtt.IsZero()) m_minRtt = rtt;
    m_minRtt = Seconds(std::min(m_minRtt.GetSeconds(), rtt.GetSeconds()));
    m_currentRtt = rtt;

    if (!(rtt.IsZero() || m_IsCount))
    {
        m_IsCount = true;
        m_bwEstimateEvent.Cancel();
        m_bwEstimateEvent = Simulator::Schedule(rtt, &TcpAdaptiveReno::EstimateBW, this, rtt, tcb);
    }
}


uint32_t
TcpAdaptiveReno::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight [[maybe_unused]])
{
    m_congRtt = m_congRttEst;
    m_packetLossRtt = m_currentRtt;

    double c = EstimateCongestionLevel();

    // TODO
    uint32_t ssThresh = static_cast<uint32_t>(tcb->m_cWnd / (1.0 + c));
    ssThresh = std::max(ssThresh, 2 * tcb->m_segmentSize);

    NS_LOG_LOGIC("CurrentBW: " << m_currentBW << " minRtt: " << tcb->m_minRtt
                               << " ssThresh: " << ssThresh);

    m_baseWnd = ssThresh;
    m_probWnd = 0; // cuz just lost packet!

    return ssThresh;
}

Ptr<TcpCongestionOps>
TcpAdaptiveReno::Fork()
{
    return CreateObject<TcpAdaptiveReno>(*this);
}

void
TcpAdaptiveReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    EstimateIncWnd(tcb);
    double MSS = tcb->m_segmentSize * tcb->m_segmentSize;
    m_baseWnd += MSS / tcb->m_cWnd;
    m_probWnd = std::max((double)m_probWnd + m_incWnd / tcb->m_cWnd.Get(), 0.0);
    tcb->m_cWnd = m_baseWnd + m_probWnd;
}

void
TcpAdaptiveReno::EstimateIncWnd(Ptr<TcpSocketState> tcb)
{
    int m = 1000;
    double c = EstimateCongestionLevel();
    double MSS = tcb->m_segmentSize * tcb->m_segmentSize;
    double maxWnd = m_currentBW.Get().GetBitRate() / m * MSS;
    double alpha = 10;
    double beta = 2 * maxWnd * (1 / alpha - (1/alpha + 1) / std::exp(alpha));
    double gamma = 1 - 2 * maxWnd * (1 / alpha - (1 / alpha + 0.5) / std::exp(alpha));
    m_incWnd = maxWnd / std::exp(c*alpha) + c * beta + gamma;
}

double
TcpAdaptiveReno::EstimateCongestionLevel()
{
    float alpha = 0.85;
    if(m_congRtt <= m_minRtt)
    {
        m_congRttEst = m_packetLossRtt;
    }
    else
    {
        m_congRttEst = (alpha * m_congRtt) + ((1 - alpha) * m_packetLossRtt);
    }

    return std::min(
        (m_currentRtt.GetSeconds() - m_minRtt.GetSeconds()) / (m_congRttEst.GetSeconds() - m_minRtt.GetSeconds()), 1.0
    );
}
} // namespace ns3

