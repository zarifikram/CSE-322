#ifndef TCP_ADAPTIVE_RENO_H
#define TCP_ADAPTIVE_RENO_H

#include "tcp-congestion-ops.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/traced-value.h"
#include "tcp-westwood-plus.h"

namespace ns3
{

class Time;

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP Westwood+.
 *
 * Westwood+ employ the AIAD (Additive Increase/Adaptive Decrease)
 * congestion control paradigm. When a congestion episode happens,
 * instead of halving the cwnd, these protocols try to estimate the network's
 * bandwidth and use the estimated value to adjust the cwnd.
 * While Westwood performs the bandwidth sampling every ACK reception,
 * Westwood+ samples the bandwidth every RTT.
 *
 * The two main methods in the implementation are the CountAck (const TCPHeader&)
 * and the EstimateBW (int, const, Time). The CountAck method calculates
 * the number of acknowledged segments on the receipt of an ACK.
 * The EstimateBW estimates the bandwidth based on the value returned by CountAck
 * and the sampling interval (last RTT).
 *
 * WARNING: this TCP model lacks validation and regression tests; use with caution.
 */
class TcpAdaptiveReno : public TcpWestwoodPlus
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TcpAdaptiveReno();
    /**
     * \brief Copy constructor
     * \param sock the object to copy
     */
    TcpAdaptiveReno(const TcpAdaptiveReno& sock);
    ~TcpAdaptiveReno() override;

    /**
     * \brief Filter type (None or Tustin)
     */
    enum FilterType
    {
        NONE,
        TUSTIN
    };

    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt) override;

    Ptr<TcpCongestionOps> Fork() override;

  private:
    /**
     * \brief Increase the count of acked segments and update the estimated bandwidth
     */
    void PktsAcked();

    /**
     * \brief estimates the congestion level of the network using RTT
    */
    double EstimateCongestionLevel();
    /**
     * \brief Calculates W_max and update the value of W_inc
    */
    void EstimateIncWnd(Ptr<TcpSocketState> tcb);
    // void EstimateBW (const Time& rtt, Ptr<TcpSocketState> tcb);
    

  protected:
    virtual void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

    Time m_minRtt; //!< Minimum RTT seen so far
    Time m_currentRtt; //!< Current RTT
    Time m_congRtt; //!< RTT when congestion was detected
    Time m_congRttEst; //!< Estimated RTT when congestion was detected
    Time m_packetLossRtt; //!< RTT when packet loss was detected

    uint32_t m_baseWnd;
    uint32_t m_probWnd;
    uint32_t m_incWnd;
};

} // namespace ns3

#endif /* TCP_ADAPTIVE_RENO_H */
