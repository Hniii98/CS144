Before Today: Datagrams, User Datagrams, Reliable byte streams on top of unreliable service abstraction  
Today: Packet Switching  
Logistics:

- Come to the lab  
- Help each other out both in the lab and on Ed  
- Extra credits will be given in upcoming checkpoints for new test cases

How does end point drop a “postcard” to its destination?

- Circuit-switched networks (e.g. telephones)  
  - Each telephone is connected to a center office  
  - And a staff worked in the office would connect the wires upon customers’ request  
  - If the person you want to call does not belong to the same office as you, there are circuits between main offices  
  - Any phone call has a real direct electrical circuit  
  - BUT: setting up and tearing down circuit is expensive, it works for telephone calls, but would not make sense if you only want to send a short piece of data  
- Packet Switching  
  - The time it takes for the first bit to be received – **Propagation Delay:**   
    tl \= lc (l \= distance, c \= light speed in that medium) (seconds)  
    - c \= 2108 m/s in cable  
  - The time it takes for the whole packet to be received after the first bi t is received — **Serialization delay: tp \= size of packetlink rate (bits per second) \= pr**  
  - **Total time to send a packet across a link: t \= tp \+ tl  \= pr \+ lc**  
- The path between sender and receiver consists of multiple links   
  -   
  - Each hop on the link receives the whole packet before sending it out, and therefore each hop would have propagation delay \+ serialization delay  
  - And there is **Queueing delay** if the link is busy (a packet needs to wait in line at the FIFO queue).  
  - TIme until packet begins transmission on a link — **Queueing delay**:  
    Q(t) \=  serialization delay of any packet before this packet in the queue   
  -   
  - **End-to-end delay**: i {hops} (tp \+ tl \+ Qi(t))