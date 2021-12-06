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

#ifndef SWIFT_H
#define SWIFT_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/traced-callback.h"

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief An implementation of SWIFT. This model implements all of the
 * endpoint capabilities mentioned in the SWIFT SIGCOMM paper.
 */

class Swift : public TcpLinuxReno {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  Swift ();

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  Swift (const Swift& sock);

  /**
   * \brief Destructor
   */
  virtual ~Swift (void);

  // Documented in base class
  virtual std::string GetName () const;

  /**
   * \brief Set configuration required by congestion control algorithm,
   *        This method will force SwiftEcn mode and will force usage of
   *        either ECT(0) or ECT(1) (depending on the 'UseEct0' attribute),
   *        despite any other configuration in the base classes.
   *
   * \param tcb internal congestion state
   */
  virtual void Init (Ptr<TcpSocketState> tcb);

  /**
   * TracedCallback signature for SWIFT update of congestion state
   *
   * \param [in] bytesAcked Bytes acked in this observation window
   * \param [in] bytesMarked Bytes marked in this observation window
   * \param [in] alpha New alpha (congestion estimate) value
   */
  typedef void (* CongestionEstimateTracedCallback)(uint32_t bytesAcked, uint32_t bytesMarked, double alpha);

  // Documented in base class
  /*virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);*/
  virtual Ptr<TcpCongestionOps> Fork ();
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time &rtt);
  virtual void CwndEvent (Ptr<TcpSocketState> tcb,
                          const TcpSocketState::TcpCAEvent_t event);
private:
  /**
   * \brief Changes state of m_ceState to true
   *
   * \param tcb internal congestion state
   */
  void CeState0to1 (Ptr<TcpSocketState> tcb);

  /**
   * \brief Changes state of m_ceState to false
   *
   * \param tcb internal congestion state
   */
  void CeState1to0 (Ptr<TcpSocketState> tcb);

  /**
   * \brief Updates the value of m_delayedAckReserved
   *
   * \param tcb internal congestion state
   * \param event the congestion window event
   */
  void UpdateAckReserved (Ptr<TcpSocketState> tcb,
                          const TcpSocketState::TcpCAEvent_t event);

  /**
   * \brief Resets the value of m_ackedBytesEcn, m_ackedBytesTotal and m_nextSeq
   *
   * \param tcb internal congestion state
   */
  void Reset (Ptr<TcpSocketState> tcb);

  /**
   * \brief Initialize the value of m_alpha
   *
   * \param alpha SWIFT alpha parameter
   */
  uint32_t clamp(uint32_t a, uint32_t b, uint32_t c);
  void InitializeSwiftAlpha (double alpha);

  void UpdateTargetDelay(Ptr<TcpSocketState> tcb);
  void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  uint32_t m_ackedBytesEcn;             //!< Number of acked bytes which are marked
  uint32_t m_ackedBytesTotal;           //!< Total number of acked bytes
  SequenceNumber32 m_priorRcvNxt;       //!< Sequence number of the first missing byte in data
  bool m_priorRcvNxtFlag;               //!< Variable used in setting the value of m_priorRcvNxt for first time
  double m_alpha;                       //!< Parameter used to estimate the amount of network congestion
  SequenceNumber32 m_nextSeq;           //!< TCP sequence number threshold for beginning a new observation window
  bool m_nextSeqFlag;                   //!< Variable used in setting the value of m_nextSeq for first time
  bool m_ceState;                       //!< SWIFT Congestion Experienced state
  bool m_delayedAckReserved;            //!< Delayed Ack state
  double m_g;                           //!< Estimation gain
  bool m_useEct0;                       //!< Use ECT(0) for ECN codepoint
  bool m_initialized;                   //!< Whether SWIFT has been initialized
  
  bool m_canDecrease;
  double m_maxCwnd;
  double m_minCwnd;
  uint32_t m_fSrange;
  uint32_t m_retransmitCount;
  double m_cWndPrev;
  uint32_t m_maxRetries;
  double m_beta;
  uint32_t m_maxDecrease;
  double m_pacingDelay;
  double m_addIncrease;
  double m_baseDelay;
  double m_targetDelay;
  uint32_t m_hops;
  double m_hopScale;
  Time m_lastDecrease;
  /**
   * \brief Callback pointer for congestion state update
   */
  TracedCallback<uint32_t, uint32_t, double> m_traceCongestionEstimate;
};

} // namespace ns3

#endif /* SWIFT_H */

