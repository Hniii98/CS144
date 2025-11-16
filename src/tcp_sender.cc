#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t total = 0;
  for ( const auto& seg : outstanding_segments_ ) {
    total += seg.sequence_length();
  }
  return total;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmitted_times_;
}

TCPSenderMessage TCPSender::construct_message( string_view peek_of_input )
{
  TCPSenderMessage segment;
  segment.seqno = Wrap32::wrap( accumulative_used_sequence_, isn_ );
  // RST segment shouldn't contain data including FIN/SYN.
  if ( input_.has_error() ) {
    segment.RST = true;
    return segment;
  }

  size_t effective_window = window_size_of_receiver_ > 0 ? window_size_of_receiver_ : 1;

  size_t bytes_in_flight = sequence_numbers_in_flight();
  int capacity = effective_window > bytes_in_flight ? effective_window - bytes_in_flight : 0;

  if ( !SYN_sent && capacity > 0 ) {
    segment.SYN = true;
    SYN_sent = true;
    capacity--;
  }

  size_t payload_limit = capacity > 0 ? static_cast<size_t>( capacity ) : 0;
  payload_limit = std::min( TCPConfig::MAX_PAYLOAD_SIZE, payload_limit );
  size_t payload_size = std::min( payload_limit, peek_of_input.size() );

  if ( payload_size > 0 ) {
    segment.payload = std::string( peek_of_input.substr( 0, payload_size ) );
    reader().pop( payload_size );
    capacity -= payload_size;
  }

  if ( reader().is_finished() && capacity > 0 && !FIN_sent ) {
    segment.FIN = true;
    FIN_sent = true;
  }

  return segment;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( FIN_sent )
    return;

  while ( true ) {
    TCPSenderMessage msg = construct_message( input_.reader().peek() );

    if ( msg.sequence_length() == 0 ) {
      // RST segment is permitted to update status.
      if ( msg.RST ) {
        transmit( msg );
      }
      break;
    }
    transmit( msg );
    accumulative_used_sequence_ += msg.sequence_length();
    outstanding_segments_.push_back( msg );

    if ( !retransmissions_timer_.is_timer_running() ) {
      retransmissions_timer_.set_alarm( eclapsed_time_ms_ + RTO_ms_ );
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return TCPSenderMessage {
    .seqno = Wrap32::wrap( accumulative_used_sequence_, isn_ ),
    .SYN = false,
    .payload = "",
    .FIN = false,
    .RST = input_.has_error(),
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    input_.set_error();
    return;
  }

  window_size_of_receiver_ = msg.window_size;
  if ( !msg.ackno.has_value() )
    return;

  uint64_t absolute_ackno = msg.ackno.value().unwrap( isn_, accumulative_used_sequence_ );
  if ( absolute_ackno > accumulative_used_sequence_ )
    return;

  bool acked_new_data = false;
  while ( !outstanding_segments_.empty() ) {
    const auto& earliest = outstanding_segments_.front();
    uint64_t seg_end = ( earliest.seqno + earliest.sequence_length() ).unwrap( isn_, accumulative_used_sequence_ );
    if ( seg_end <= absolute_ackno ) {
      outstanding_segments_.pop_front();
      acked_new_data = true;
    } else {
      break;
    }
  }
  // When acknowledged new data, reset status.
  if ( acked_new_data ) {
    RTO_ms_ = initial_RTO_ms_;
    retransmitted_times_ = 0;
    if ( !outstanding_segments_.empty() ) {
      retransmissions_timer_.set_alarm( eclapsed_time_ms_ + RTO_ms_ );
    } else {
      retransmissions_timer_.set_off();
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  eclapsed_time_ms_ += ms_since_last_tick; // Move forward time

  // If retransmissions_timer alarm, start the restansmission logic.
  if ( retransmissions_timer_.is_timer_expired( eclapsed_time_ms_ ) ) {
    if ( !outstanding_segments_.empty() ) {
      const TCPSenderMessage& front = outstanding_segments_.front();
      transmit( front );

      uint64_t seg_len = front.sequence_length();
      /*
       * A zero window probe shouldn't do the back off logic.
       * SYN segment is responsible for constrcuting connection and it's window size
       * happend to be zero (initial value), so we should exclude it.
       *
       *  window size = 0  && seg_len = 1
       *  1. SYN                        w/o back off
       *  2. normal payload             back off
       *  3. FIN                        back off
       */
      bool is_zero_window_probe = ( window_size_of_receiver_ == 0 ) && ( seg_len == 1 ) && ( !front.SYN );
      if ( !is_zero_window_probe ) {
        retransmitted_times_ += 1;
        RTO_ms_ *= 2;
      }

      retransmissions_timer_.set_alarm( eclapsed_time_ms_ + RTO_ms_ );
    } else {
      retransmissions_timer_.set_off();
    }
  }
}

void Timer::set_off()
{
  if ( is_running_ )
    is_running_ = false;
}

void Timer::set_alarm( uint64_t timer_alarm_at_ms )
{
  is_running_ = true;
  alarm_at_ms_ = timer_alarm_at_ms;
}

bool Timer::is_timer_expired( uint64_t eclapsed_time ) const
{
  return is_running_ && ( eclapsed_time >= alarm_at_ms_ );
}

bool Timer::is_timer_running() const
{
  return is_running_;
}
