#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <deque>
#include <functional>

class Timer
{
public:
  /* Turn off timer */
  void set_off();

  /* Turn on a timer and alarm at # of milliseconds */
  void set_alarm( uint64_t timer_alarm_at_ms );

  /*
   * Check if timer is expired, this function don't turn off timer and the owner of timer
   * is responsible for doing this.
   * You should pass a time as parameter for function to check.
   */
  bool is_timer_expired( uint64_t eclapsed_time ) const;
  bool is_timer_running() const;

private:
  bool is_running_ { false };
  uint64_t alarm_at_ms_ { UINT64_MAX };
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), RTO_ms_( initial_RTO_ms_ )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

  Timer retransmissions_timer_ {};

private:
  Reader& reader() { return input_.reader(); }

  TCPSenderMessage construct_message( std::string_view peek );

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_;
  uint64_t eclapsed_time_ms_ { 0 };
  uint64_t accumulative_used_sequence_ { 0 }; // Keep track of numbers of bytes have sent
  std::deque<TCPSenderMessage> outstanding_segments_ {};
  uint64_t retransmitted_times_ { 0 };     // Consective retransmissions times.
  uint64_t window_size_of_receiver_ { 0 }; // Record window size of receiver.

  bool SYN_sent {};
  bool FIN_sent {};
};
