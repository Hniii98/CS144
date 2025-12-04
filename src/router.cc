#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  routing_table_.emplace_back( RouteEntry { .next_hop = next_hop,
                                            .route_prefix = route_prefix,
                                            .prefix_mask = RouteEntry::to_mask( prefix_length ),
                                            .interface_num = interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto it : interfaces_ ) {
    do_interface_transfer( it );
  }
}

// Go through routing_table_ to try to find a longest match.
Router::RouteEntry* Router::find_longest_prefix_match( uint32_t destination )
{
  RouteEntry* best = nullptr;
  for ( auto& entry : routing_table_ ) {
    if ( ( destination & entry.prefix_mask ) == ( entry.route_prefix & entry.prefix_mask ) ) {
      if ( !best || entry.prefix_mask > best->prefix_mask ) {
        best = &entry;
      }
    }
  }
  return best;
}

// Parse a InternetDatagram.
void Router::process_datagram( InternetDatagram& dgram_wrapper )
{
  if ( dgram_wrapper.header.ttl <= 1 ) {
    return; // Dgram is out of date, wait drop.
  }

  Router::RouteEntry* entry = find_longest_prefix_match( dgram_wrapper.header.dst );

  // No RouterEntry matched, early return to make InternetData wait simple drop.
  if ( !entry ) {
    return;
  }

  // Update header and re-serialize.
  dgram_wrapper.header.ttl -= 1;
  dgram_wrapper.header.compute_checksum();

  // Transfer to relative interface to send.
  if ( entry->next_hop == nullopt ) { // Direct
    auto& iface = *interface( entry->interface_num );
    iface.send_datagram( dgram_wrapper, Address::from_ipv4_numeric( dgram_wrapper.header.dst ) );
  } else { // Transfer to next hop
    auto& iface = *interface( entry->interface_num );
    iface.send_datagram( dgram_wrapper, entry->next_hop.value() );
  }
}

// Transfer all received data in a interface
void Router::do_interface_transfer( shared_ptr<NetworkInterface> interface_wrapper )
{
  queue<InternetDatagram>& dgrams = interface_wrapper->datagrams_received();
  while ( not dgrams.empty() ) {
    process_datagram( dgrams.front() );
    // Drop after processing.
    dgrams.pop();
  }
}
