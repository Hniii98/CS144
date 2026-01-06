Today: The eras tour of home networking

- Level 0: terminals (in 1970s)  
  - USB (universal serial bus)  
  - In 1970s, each computer has several serial ports, and transmit bytes on each serial lines  
    - E.g. the computer is connected to a teletype machine (tty) through a serial port  
    - There is no backspace or edit what you wrote in on teletype machines  
    - Later teletype machines were replaced by glass terminals (or glass tty). And glass terminals would allow editing and erasing  
    - Glass terminals — serial port — computers

(Sections in color red are newly added in that level)

- Level 0.1: terminals \+ modems  
  - Then, people started to want  to have glass terminals at their home. Anything input into the glass terminals was transmitted to a computer at a different location through telephone lines  
  - Glass terminals — serial port — modulator/demodulator — telephone lines — modulator/demodulator — serial port — computers  
  - modulator/demodulator \=\> modem (transform between bits and telephone signals)  
  - (modem — telephone lines — modem) is an interposing (x-in-the-middle), and the glass terminals and computers on the two sides are not aware of these modems and telephone lines and consider they are directly connected to each other through a serial port.  
- Level 1: Internet at home  
  - Glass terminals were replaced by computers (PC) in home, and PCs were speaking TCP/IP.  
  - PC — TCP/IP — SLIP — serial port — modem — telephone lines — modem — serial port — SLIP — TCP/IP — PC  
  - SLIP tags a length on each IP datagrams so that given bytesteams, SLIP can cut them into datagrams  
  - SLIP was later replaced by PPP  
- Given level 1, if we abstract modems/computers on the path between a PC and the destination it want to talk to, it becomes this:  
  - PC – TCP/IP – modem – ISP (which is actually – modem — (PPP7) — router — other network interfaces (e.g. eth0)) – Internet – router — remote network (reddit)  
  - Each part of this graph needs to keep some states for having a TCP connection between PC and reddit:  
    - PC and reddit each needs a TCP Socket  
    - And the routers of ISP and reddit need to know next hops for datagrams  
  - Then the PC would need to keep:  
    - TCP Socket:  
      - 18.1.2.7:55000 (src address:src port)  
      - 151.3.2.9:443   (dst address:dst port)  
  - And reddit has:  
    - TCP Socket:  
      - 151.3.2.9:443   (src address:src port)  
      - 18.1.2.7:55000 (dst address:dst port)  
  - ISP needs to track how IP addresses should be routed:  
    - ISP:  (to the home PC) — modem — ppp7 — router — eth0 — internet  
    - In the routing table:  
      - 0/0: eth0 via 8.7.6.5 (8.7.6.5 is the address of its ISP)  
      - 18.1.2.7/32: ppp7  
  - Reddit’s router’s routing table:  
    - 0/0: eth0 via 14.14.14.14 (14.14.14.14 is the address of reddit’s ISP)  
    - 151.3/16: eth1  
- Level 2: cable modem  
  - modem also speaks Ethernet instead of SLIP/PPP7  
  - ISP:  (to the home PC) – modem — eth1 — router — eth0 — internet  
  - And in the routing table of ISP’s route (with address 18.1.0.1)r:  
    - In the routing table:  
    - 0/0: eth0 via 8.7.6.5 (8.7.6.5 is the address of its ISP)  
    - 18.1.2.7/32: eth1  
    - 18.1.0.1/32: is me  
- Level 3: home network  
  - Multiple PCs are connected to the same modem. All PCs are connected to different ports of a switch and the switch is connected to the modem. A switch keeps an ethernet to port mapping. (**A switch does not look at IP headers).**  
  - PC1: 18.1.2.7 and PC2: 18.1.2.8 are connected to the same switch  
    - PC1 – switch – modem  
    - PC2 – switch – modem  
  - And at the ISP’s routing table:  
    - 0/0: eth0 via 8.7.6.5 (8.7.6.5 is the address of its ISP)  
    - 18.1.2.7/32: eth1  
    - 18.1.0.1/32: is me  
    - 18.1.2.8/32: eth1  
- Level 4: home subnet  
  - Level 3 is annoying because the ISP’s routing table has an entry for both PC1 and PC2, and a “delegation” would make this easier.  
  - At the home, there is a router between switch and modem:  
    - PC1(18.1.2.7) – switch – (eth0) – router(18.1.2.2) – (eth1) – modem – (to ISP)  
    - PC2(18.1.2.8) – switch – (eth0) – router(18.1.2.2) – (eth1) – modem – (to ISP)  
  - At the home router’s routing table:  
    - 0/0: eth1 via 18.1.0.1 (ISP address)  
    - 18.1.2.0/24: eth0       (home network)  
  - At the ISP’s routing table:  
    - 0/0: eth0 via 8.7.6.5  
    - 18.1.2.0/24: eth1 via 18.1.2.2  
    - 18.1.0.1/32: is me  
- Level 5: home wireless network  
  - The home switch is replaced by Wi-Fi (AP)  
  - And then it gets harder for an ISP to assign an IP address to every device connected to the Wi-Fi in every its customers’ home

Last time: level 5 home subnetwork  
Home subnet:

ISP:

The reddit:

- If we live in the world of IPv6 (each hosts get assigned a unique IPv6 address):  
  - As long as PC1 (18.1.14.4) and the reddit (151.2.3.9) stores the source and dst IP addresses of the TCP Connection in their socket, the TCP connection can be done  
    - PC1: 18.1.14.4: 5678 ← —\> 151.3.2.4:443: reddit  
  - DNS \+ DHCP \+ switch \+ router \+ modem used to be independent parts, but now they goes in to a “home router” you can buy from a shop

- Level 6: Proxy server/ jump host / bastion / socks5  
  - Each subnets have a local range of IPv4 addresses

- The computer relays the bytes between TCP connection 1 and TCP connection 2\.  
- But this is annoying for asking every new PC to also set up the proxy


  


- Level 7: Transparent Proxy

- The PC does not know the existence of the proxy  
- And the proxy computer acts as if it is the reddit server in the home subnet, and relay the connection out with its own public IP address

 

- Level 8: network address/port translation ( NAT )  
  - For the proxy, it no longer reconstruct the byte stream, but only do translation on the IP address and port to appear in the public network

