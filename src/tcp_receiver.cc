#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // RST flag set, resembler set to error
  if ( message.RST ) {
    reassembler_.set_error();
    return;
  }
  // First time receive seqno, set it to isn.
  if ( message.SYN && !isn_.has_value() )
    isn_ = message.seqno;
  // Reject all fragments before isn will be set.
  if ( !isn_.has_value() )
    return;
  // Bare ack message, don't need to insert.
  if ( message.sequence_length() == 0 )
    return;

  uint64_t checkpoint = reassembler_.first_unassembled_index();
  uint64_t absolute_seqno = message.seqno.unwrap( isn_.value(), checkpoint );

  // If SYN flag set, move seqno to real data.
  if ( message.SYN )
    absolute_seqno += 1;

  uint64_t stream_index = absolute_seqno - 1;
  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;
  // The next sequence number needed by receiver. If isn haven't set yet, ackno will be nullopt.
  if ( isn_.has_value() ) {
    uint64_t stream_index = reassembler_.first_unassembled_index();
    uint64_t absolute_seqno = stream_index + 1;
    // If FIN flag have received yet, absolute_seqno plus one .
    if ( writer().is_closed() )
      absolute_seqno += 1;
    message.ackno = Wrap32::wrap( absolute_seqno, isn_.value() );
  } else {
    message.ackno = std::nullopt;
  }

  message.RST = writer().has_error();
  uint64_t capacity = writer().available_capacity();
  // TCP only use uint16_t to indicate window_size, so we trucate capacity here.
  if ( capacity >= UINT16_MAX )
    message.window_size = UINT16_MAX;
  else {
    message.window_size = capacity;
  }

  return message;
}
