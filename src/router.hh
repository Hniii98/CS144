#pragma once

#include "exception.hh"
#include "network_interface.hh"

#include <optional>

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    interfaces_.push_back( notnull( "add_interface", std::move( interface ) ) );
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return interfaces_.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

  // The routing table entry
  struct RouteEntry
  {
    static constexpr uint8_t PREFIX_MAXLEN = 32;

    std::optional<Address> next_hop; // 16B aligned
    uint32_t route_prefix;
    uint32_t prefix_mask;
    size_t interface_num;

    static uint32_t to_mask( uint8_t prefix_length )
    {
      if ( prefix_length > PREFIX_MAXLEN )
        throw std::runtime_error( "prefix_length must be <= 32" );
      return prefix_length == 0 ? 0 : ~( ( 1U << ( PREFIX_MAXLEN - prefix_length ) ) - 1U );
    }
  };

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> interfaces_ {};
};
