#include <iostream>
#include <algorithm>  

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto mapping = cached_mapping_.find(next_hop);
 
  EthernetFrame frame;
  Serializer serializer;

  // Already cached it's IP-to-Ethernet mapping, compose Ethernetframe with InternetDatagram.
  if(mapping != cached_mapping_.end())
  {
    frame.header = EthernetHeader{
                    .dst = mapping->second.MAC, 
                    .src = ethernet_address_,                                
                    .type = EthernetHeader::TYPE_IPv4};
    dgram.serialize(serializer);
    frame.payload = serializer.finish();
  } 
  // Otherwise, with a ARP request.
  else 
  {    
    // Queue the InternetDatagram to wait corresponing mapping.
    datagrams_waiting_mapping_[next_hop].push_back(DgramEntry{
                                                    .dgram = dgram, 
                                                    .expired_at = since_constructed_ + ARP_SENDING_FROZEN});
    // This ip is not at ARP sending available time since since a sending in last 5 ms.
    auto pass = arp_available_at_.find(next_hop);
    if(pass != arp_available_at_.end() && 
       pass->second > since_constructed_) return;
    
    frame.header = EthernetHeader{.dst = ETHERNET_BROADCAST, .src = ethernet_address_,
                                  .type = EthernetHeader::TYPE_ARP};
    
    // Construct ARP message and serialize.
    ARPMessage arp{.opcode = ARPMessage::OPCODE_REQUEST, .sender_ethernet_address = ethernet_address_,
              .sender_ip_address = ip_address_.ipv4_numeric(), 
              .target_ip_address = next_hop.ipv4_numeric(),
            }; 
    arp.serialize(serializer);
    frame.payload = serializer.finish();

    // Update sending availabletime, cover if it exists.
    arp_available_at_[next_hop] = since_constructed_ + ARP_SENDING_FROZEN;

  }
  
  transmit(frame);
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // cerr << frame.header.type << frame.payload.size() << "\n";
  // Ignore any frames are not destined for the network interface
  if(frame.header.dst != ethernet_address_ && 
     frame.header.dst != ETHERNET_BROADCAST) 
    return;
    
  Parser parser(frame.payload);

  if(frame.header.type == EthernetHeader::TYPE_IPv4){  
    // Parse Ethernet frame to a IPDatagram.
    InternetDatagram dgram;

    dgram.parse(parser);
    if(parser.has_error()) return;

    datagrams_received_.push(std::move(dgram));
  }
  else if(frame.header.type == EthernetHeader::TYPE_ARP){
    // Parse Ethernet frame to a ARPMessage.
    ARPMessage arp;
    arp.parse(parser);
    if(parser.has_error()) return;

    Address incoming_ip = Address::from_ipv4_numeric(arp.sender_ip_address);
    EthernetAddress incoming_ethernet = arp.sender_ethernet_address;

    // Learn incoming mapping from both reply and request.
    cached_mapping_[incoming_ip] = MACEntry{.MAC = incoming_ethernet, 
                                          .expired_at = since_constructed_ + MAPPING_ALIVE };
    
    // Send InternetDatagram that was waiting it's IP-to-Ethernet mapping.
  
    send_waiting_dgrams(incoming_ip);
    
    // Reply to ARP request
    if(arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric()) 
      send_arp_reply(incoming_ip, incoming_ethernet);
  }


}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Update time.
  since_constructed_ += ms_since_last_tick;

  // Clean expired mapping.
  clean_expired_mapping(since_constructed_);

  for(auto it = datagrams_waiting_mapping_.begin(); it != datagrams_waiting_mapping_.end(); )
  {
      auto& dgrams_vec = it->second;
      dgrams_vec.erase(remove_if(dgrams_vec.begin(), dgrams_vec.end(), 
                                [this](const DgramEntry &dgram){ return dgram.expired_at <= since_constructed_; }),
                      dgrams_vec.end());

      if(dgrams_vec.empty())
      {
        it = datagrams_waiting_mapping_.erase(it);  
      } else{
        it++;
      }

  }
}

//! \param[in] now the number of milliseconds since the NetworkInterface was constructed
void NetworkInterface::clean_expired_mapping( const ms now )
{
  for(auto it = cached_mapping_.begin(); it != cached_mapping_.end(); )
  {
    if(it->second.expired_at <= now)
    {
      it = cached_mapping_.erase(it);
    } else {
      it++;
    }
  }
}

//! \param[in] comming ip the Address of  IP-to-Ethernet just learn from arp frame
void NetworkInterface::send_waiting_dgrams( const Address& comming_ip)
{
  auto it = datagrams_waiting_mapping_.find(comming_ip);
  if(it != datagrams_waiting_mapping_.end())
  {
    for(auto& d : it->second)
    {
      send_datagram(d.dgram, comming_ip);
    }
    datagrams_waiting_mapping_.erase(it);
  }

}

//! \param[in]  target_ip the IP Address about to send ARP reply to
void NetworkInterface::send_arp_reply( const Address& target_ip, const EthernetAddress& target_ethernet )
{
  // Construct a ARP reply message.
  EthernetFrame frame;
  frame.header = EthernetHeader{
    .dst = target_ethernet,
    .src = ethernet_address_,
    .type = EthernetHeader::TYPE_ARP,
  };

  ARPMessage arp{
    .opcode = ARPMessage::OPCODE_REPLY,
    .sender_ethernet_address = ethernet_address_,
    .sender_ip_address = ip_address_.ipv4_numeric(),
    .target_ethernet_address = target_ethernet,
    .target_ip_address = target_ip.ipv4_numeric(),
  };

  Serializer serializer;
  arp.serialize(serializer);
  frame.payload = serializer.finish();
  
  transmit(frame);
}


