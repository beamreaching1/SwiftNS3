/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Shravya K.S. <shravya.ks0@gmail.com>
 *
 */

#include "swift.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/simulator.h"
#include <stdio.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Swift");

NS_OBJECT_ENSURE_REGISTERED (Swift);

TypeId Swift::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::Swift")
    .SetParent<TcpLinuxReno> ()
    .AddConstructor<Swift> ()
    .SetGroupName ("Scratch")
    .AddAttribute ("SwiftShiftG",
                   "Parameter G for updating Swift_alpha",
                   DoubleValue (0.0625),
                   MakeDoubleAccessor (&Swift::m_g),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("SwiftAlphaOnInit",
                   "Initial alpha value",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&Swift::InitializeSwiftAlpha),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("UseEct0",
                   "Use ECT(0) for ECN codepoint, if false use ECT(1)",
                   BooleanValue (true),
                   MakeBooleanAccessor (&Swift::m_useEct0),
                   MakeBooleanChecker ())
    .AddTraceSource ("CongestionEstimate",
                     "Update sender-side congestion estimate state",
                     MakeTraceSourceAccessor (&Swift::m_traceCongestionEstimate),
                     "ns3::Swift::CongestionEstimateTracedCallback")
  ;
  return tid;
}

std::string Swift::GetName () const {
  return "Swift";
}

Swift::Swift ()
  : TcpLinuxReno (), m_ackedBytesEcn (0), m_ackedBytesTotal (0),
    m_priorRcvNxt (SequenceNumber32 (0)), m_priorRcvNxtFlag (false), 
    m_nextSeq (SequenceNumber32 (0)), m_nextSeqFlag (false), m_ceState (false),
    m_delayedAckReserved (false), m_initialized (false)
{
  printf("Hello from SWIFT-----------------------------------------Default\n");
  NS_LOG_FUNCTION (this);
  m_maxCwnd = 100000;
  m_minCwnd = 1;
  m_canDecrease = true;
  m_fSrange = 2;
  m_hops = 3;
  m_hopScale = 1;
  m_baseDelay = 1;
  m_addIncrease = 10;
  m_retransmitCount = 0;
  m_cWndPrev = 1.0;
  m_targetDelay = 1.0;
  m_maxDecrease = 10;
  m_maxRetries = 10;
  m_lastDecrease = Simulator::Now();
  m_alpha = m_fSrange / ((1/sqrt(m_minCwnd))-(1/sqrt(m_maxCwnd)));
  m_beta = -1 * m_alpha / sqrt(m_maxCwnd);
}

Swift::Swift (const Swift& sock)
  : TcpLinuxReno (sock),
    m_ackedBytesEcn (sock.m_ackedBytesEcn), m_ackedBytesTotal (sock.m_ackedBytesTotal),
    m_priorRcvNxt (sock.m_priorRcvNxt), m_priorRcvNxtFlag (sock.m_priorRcvNxtFlag),
    m_alpha (sock.m_alpha), m_nextSeq (sock.m_nextSeq),
    m_nextSeqFlag (sock.m_nextSeqFlag), m_ceState (sock.m_ceState),
    m_delayedAckReserved (sock.m_delayedAckReserved), m_g (sock.m_g),
    m_useEct0 (sock.m_useEct0), m_initialized (sock.m_initialized)
{
  printf("Hello from SWIFT--------------------------------------------Copy\n");
  NS_LOG_FUNCTION (this);
  m_maxCwnd = 100000;
  m_minCwnd = 1;
  m_canDecrease = true;
  m_fSrange = 2;
  m_hops = 3;
  m_hopScale = 1;
  m_baseDelay = 1;
  m_addIncrease = 10;
  m_retransmitCount = 0;
  m_cWndPrev = 1.0;
  m_targetDelay = 1.0;
  m_maxDecrease = 10;
  m_lastDecrease = Simulator::Now();
  m_alpha = m_fSrange / ((1/sqrt(m_minCwnd))-(1/sqrt(m_maxCwnd)));
  m_beta = -1 * m_alpha / sqrt(m_maxCwnd);
  m_maxRetries = 10;
}

Swift::~Swift (void) {
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps> Swift::Fork (void) {
  NS_LOG_FUNCTION (this);
  return CopyObject<Swift> (this);
}

uint32_t Swift::clamp(uint32_t a, uint32_t b, uint32_t c) {
  return std::max(a, std::min(b, c));
}

void Swift::Init (Ptr<TcpSocketState> tcb) {
  NS_LOG_FUNCTION (this << tcb);
  NS_LOG_INFO (this << "Enabling SwiftEcn for SWIFT");
  tcb->m_useEcn = TcpSocketState::On;
  tcb->m_ecnMode = TcpSocketState::SwiftEcn;
  tcb->m_ectCodePoint = m_useEct0 ? TcpSocketState::Ect0 : TcpSocketState::Ect1;
  m_initialized = true;
}

/*// Step 9, Section 3.3 of RFC 8257.  GetSsThresh() is called upon
// entering the CWR state, and then later, when CWR is exited,
// cwnd is set to ssthresh (this value).  bytesInFlight is ignored.
uint32_t Swift::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) {
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  return static_cast<uint32_t> ((1 - m_alpha / 2.0) * tcb->m_cWnd);
}*/

void Swift::UpdateTargetDelay(Ptr<TcpSocketState> tcb) {
  m_targetDelay = m_baseDelay + (m_hops * m_hopScale);
  m_targetDelay += std::max((double)0.0, std::min(1/sqrt((double)tcb->m_cWnd)+m_beta, (double)m_fSrange));
}

void Swift::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) {

  //Add a section here for packets acked?
  //RTT info may not be easily accessible from this method
  m_retransmitCount = 0;


  //If an ACK is recieved normally
  Swift::UpdateTargetDelay(tcb);
  if(tcb->m_lastRtt.Get().GetMilliSeconds() < m_targetDelay) {
    if(tcb->m_cWnd.Get() > 1) {
      //Does num_acked in the paper refer to the total acked?
      //Or does it mean the number acked this particular frame?
      tcb->m_cWnd = tcb->m_cWnd + (m_addIncrease / tcb->m_cWnd) * segmentsAcked;
    }
    else {
      tcb->m_cWnd = tcb->m_cWnd + (m_addIncrease * segmentsAcked);
    }
  }
  else {
    if(m_canDecrease) {
      tcb->m_cWnd = std::max(1 - (m_beta * (tcb->m_lastRtt.Get().GetMilliSeconds() - m_targetDelay)),
      1 - m_maxDecrease) * tcb->m_cWnd;
    }
  }


  //Retransmission Timeout
  if(tcb->m_congState == TcpSocketState::CA_LOSS) {
    m_retransmitCount++;
    if(m_retransmitCount >= m_maxRetries) {
      tcb->m_cWnd = m_minCwnd;
    }
    else {
      if(m_canDecrease) {
        tcb->m_cWnd = (1 - m_maxCwnd) * tcb->m_cWnd;
      }
    }
  }

  //Fast Recovery State
  if(tcb->m_congState == TcpSocketState::CA_RECOVERY) {
    if(m_canDecrease) {
      tcb->m_cWnd = (1 - m_maxCwnd) * tcb->m_cWnd;
    }
  }

  //Ensure things stay in bounds
  //There should also be a pacing delay here,
  //But who knows where it is.
  
  tcb->m_cWnd = clamp(tcb->m_cWnd.Get(), m_minCwnd, m_maxCwnd);
  if(tcb->m_cWnd.Get() <= m_cWndPrev) {
    m_lastDecrease = Simulator::Now();
  }
  m_cWndPrev = tcb->m_cWnd.Get();

}


void Swift::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt) {
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
  m_ackedBytesTotal += segmentsAcked * tcb->m_segmentSize;
  
/*
  m_retransmitCount = 0;

  double delay = rtt.GetNanoSeconds();
  UpdateTargetDelay();
  if(delay < m_targetDelay){
    if(tcb->m_cWnd > 1){
      //Does num_acked in the paper refer to the total acked?
      //Or does it mean the number acked this particular frame?
      tcb->m_cWnd = tcb->m_cWnd + (m_addIncrease / tcb->m_cWnd) * segmentsAcked;
    }
    else{
      tcb->m_cWnd = tcb->m_cWnd + (m_addIncrease * segmentsAcked);
    }
  }
  else{
    if(m_canDecrease){
      tcb->m_cWnd = std::max(1 - (m_beta * (delay - m_targetDelay)),
      1 - m_maxDecrease) * tcb->m_cWnd;
    }
  }


  //This redundancy is because I'm not
  //sure whether this, or IncreaseWindow is called
  //last, but it doesn't hurt to ensure cWnd is
  //within bounds an extra time
  tcb->m_cWnd = std::clamp(tcb->m_cWnd, m_minCwnd, m_maxCwnd);
  if(tcb->m_cWnd <= m_cWndPrev){
    m_lastDecrease = Simulator::Now();
  }
  m_cWndPrev = tcb->m_cWnd;
  m_canDecrease = (Simulator::Now() - m_lastDecrease >= rtt);
*/
/*  if (tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD)
    {
      m_ackedBytesEcn += segmentsAcked * tcb->m_segmentSize;
    }
  if (m_nextSeqFlag == false)
    {
      m_nextSeq = tcb->m_nextTxSequence;
      m_nextSeqFlag = true;
    }
  if (tcb->m_lastAckedSeq >= m_nextSeq)
    {
      double bytesEcn = 0.0; // Corresponds to variable M in RFC 8257
      if (m_ackedBytesTotal >  0)
        {
          bytesEcn = static_cast<double> (m_ackedBytesEcn * 1.0 / m_ackedBytesTotal);
        }
      m_alpha = (1.0 - m_g) * m_alpha + m_g * bytesEcn;
      m_traceCongestionEstimate (m_ackedBytesEcn, m_ackedBytesTotal, m_alpha);
      NS_LOG_INFO (this << "bytesEcn " << bytesEcn << ", m_alpha " << m_alpha);
      Reset (tcb);
    }
*/

  /*
  Old ack code
  if (tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD)
    {
      m_ackedBytesEcn += segmentsAcked * tcb->m_segmentSize;
    }
  if (m_nextSeqFlag == false)
    {
      m_nextSeq = tcb->m_nextTxSequence;
      m_nextSeqFlag = true;
    }
  if (tcb->m_lastAckedSeq >= m_nextSeq)
    {
      double bytesEcn = 0.0; // Corresponds to variable M in RFC 8257
      if (m_ackedBytesTotal >  0)
        {
          bytesEcn = static_cast<double> (m_ackedBytesEcn * 1.0 / m_ackedBytesTotal);
        }
      m_alpha = (1.0 - m_g) * m_alpha + m_g * bytesEcn;
      m_traceCongestionEstimate (m_ackedBytesEcn, m_ackedBytesTotal, m_alpha);
      NS_LOG_INFO (this << "bytesEcn " << bytesEcn << ", m_alpha " << m_alpha);
      Reset (tcb);
    }*/
}

void Swift::InitializeSwiftAlpha (double alpha) {
  NS_LOG_FUNCTION (this << alpha);
  NS_ABORT_MSG_IF (m_initialized, "Swift has already been initialized");
  m_alpha = alpha;
}

void Swift::Reset (Ptr<TcpSocketState> tcb) {
  NS_LOG_FUNCTION (this << tcb);
  m_nextSeq = tcb->m_nextTxSequence;
  m_ackedBytesEcn = 0;
  m_ackedBytesTotal = 0;
}

void Swift::CeState0to1 (Ptr<TcpSocketState> tcb) {
  NS_LOG_FUNCTION (this << tcb);
  if (!m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag) {
      SequenceNumber32 tmpRcvNxt;
      /* Save current NextRxSequence. */
      tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence ();

      /* Generate previous ACK without ECE */
      tcb->m_rxBuffer->SetNextRxSequence (m_priorRcvNxt);
      tcb->m_sendEmptyPacketCallback (TcpHeader::ACK);

      /* Recover current RcvNxt. */
      tcb->m_rxBuffer->SetNextRxSequence (tmpRcvNxt);
    }

  if (m_priorRcvNxtFlag == false) {
      m_priorRcvNxtFlag = true;
    }
  m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence ();
  m_ceState = true;
  tcb->m_ecnState = TcpSocketState::ECN_CE_RCVD;
}

void
Swift::CeState1to0 (Ptr<TcpSocketState> tcb) {
  NS_LOG_FUNCTION (this << tcb);
  if (m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag) {
    SequenceNumber32 tmpRcvNxt;
    /* Save current NextRxSequence. */
    tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence ();

    /* Generate previous ACK with ECE */
    tcb->m_rxBuffer->SetNextRxSequence (m_priorRcvNxt);
    tcb->m_sendEmptyPacketCallback (TcpHeader::ACK | TcpHeader::ECE);

    /* Recover current RcvNxt. */
    tcb->m_rxBuffer->SetNextRxSequence (tmpRcvNxt);
  }

  if (m_priorRcvNxtFlag == false) {
    m_priorRcvNxtFlag = true;
  }
  m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence ();
  m_ceState = false;

  if (tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE) {
    tcb->m_ecnState = TcpSocketState::ECN_IDLE;
  }
}

void
Swift::UpdateAckReserved (Ptr<TcpSocketState> tcb,
                             const TcpSocketState::TcpCAEvent_t event) {
  NS_LOG_FUNCTION (this << tcb << event);
  switch (event) {
    case TcpSocketState::CA_EVENT_DELAYED_ACK:
      if (!m_delayedAckReserved) {
          m_delayedAckReserved = true;
        }
      break;
    case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
      if (m_delayedAckReserved) {
          m_delayedAckReserved = false;
        }
      break;
    default:
      /* Don't care for the rest. */
      break;
    }
}

void Swift::CwndEvent (Ptr<TcpSocketState> tcb,
                     const TcpSocketState::TcpCAEvent_t event) {
  NS_LOG_FUNCTION (this << tcb << event);
  switch (event) {
    case TcpSocketState::CA_EVENT_ECN_IS_CE:
      CeState0to1 (tcb);
      break;
    case TcpSocketState::CA_EVENT_ECN_NO_CE:
      CeState1to0 (tcb);
      break;
    case TcpSocketState::CA_EVENT_DELAYED_ACK:
    case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
      UpdateAckReserved (tcb, event);
      break;
    default:
      /* Don't care for the rest. */
      break;
  }
}

} // namespace ns3
