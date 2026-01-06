- Last time: “best effort” delivery as the service abstraction  
  - Not delivered \-\> [**timeout \+ retransmit**](#bookmark=id.cziqnj72gch4)  
  - Delivered n\> 1 times \-\> [**transform operations to be idempotent**](#bookmark=id.s75spauyho6)  
  - Delivered altered \-\> **checksum or crypto**  
  - Delivered out of order \-\> **[sequence number](#bookmark=id.3mogw4tdk2z4)**  
  - On top of this service abstraction, we can build:   
    - VoIP  
    - User Datagrams  
    - VPN (IP-in-UDP/IP-in-IP/IPsec)  
      - Q: How does Netflix determine where an IP address is actually from?  
      - A: Netflix would look at the IP addresses provided by VPN services and ban those IP addresses.  
- Short get: get(key) \-\> value   
  - E.g. host: what is the IP address that corresponds to a host?  
    - With packet loss, it takes a longer time to reply, but would still give an answer  
    - This service is “reliable” despite the fact that it is built on a unreliable “best effort” service abstraction  
      

```c
// Server
void recv ( const string& service ) {
UDPSocket sock;
sock.bind ( Address (“0”, service) );
Address source (“0”);
string payload;
while (true) {
sock.recv( source, payload);
cout << “Message from” << source.to_string() << “: “ << payload << endl;
if (payload == "best_class_ever" ) {
sock.sendto( source, "EE180");
}
}
}
```

```c
// Sender
void run( const string& host, const string& service, const string& query) {
UDPSocket sock;
sock.set_blocking( false );
Address source ("0");
string answer;

// retransmit the query (with a small timeout), until there is a reply
do {
sock.sendto(Address(host, service), query);
	this_thread::sleep_for(seconds(1));
sock.recv(source, answer);
if (answer.empty()) {
	cerr << "No reply, retransmitting" << endl;
	}
} while (answer.empty())

cout << "Got reply to " << query << ": " << answer << endl;
}
```

  - By doing this, we implement a “reliable” service on top of an “unreliable” service abstraction, and this is also how many real-word reliable services are built (e.g. host).  
    - And also: Domain Name System (DNS): what is the IP address of an internet domain name?  
    - DHCP (Dynamic Host Configuration Protocol): what is the IP address I am supposed to use?  
- Set: (e.g. set the back door open)  
  - Both short get and set (the back door open), you could say how ever many times you want and it does not change the ending state  
  - But for \`pop(7)\`, \`push(“hi”)\`, it matters how many times you say it.  
  - Idempotent: doing one time or more than one time does not change the ending state (GET PUT). The strategy we used above works for something idempotent, but not for non-idempotent action  
- Do a non-idempotent operation (POST):  
  	By having a set of launched missiles, we make launch\_missle idempotent


```c
// Server
void launch_missle() {
	cout << "Launching one missle" << endl;
}

void recv ( const string& service ) {
unordered_set<uint64_t> launched_missle;
UDPSocket sock;
sock.bind ( Address (“0”, service) );
Address source (“0”);
string payload;
while (true) {
sock.recv( source, payload);
cout << “Message from” << source.to_string() << “: “ << payload << endl;
if (payload == "best_class_ever" ) {
sock.sendto(source, "EE180");
} else if (payload == "launch_one_missle" + missle_id ) {
	if (missle_id not in launched_missle ) {
		launch_missle();
		launched_missle.insert(missle_id);
	}
	sock.sendto(source, "ack");
}
}
}
```

- ByteStream: push, pop, peek needs to be transformed into idempotent operations, and this is achieved by **TCP**  
-   
- What should be in the TCP Sender message to make these operations idempotent?  
  - \`push (“abcd”)\` works iff each message is delivered exactly once  
  - \`push(“abcd”) \+ message unique id\`, but the sender needs to keep a set of any message sent  
  - Create a reassembler, \`first\_index: 0, data: “abcd”\` \`first\_index: 4, data: “efgh”, \`first\_index \= 8, FIN=true\`  
- 

- Stacks of service abstraction  
  - Short gets (DNS, DHCP) \-\> User datagrams \-\> Internet Datagrams  
  - Byte Stream –(TCP)--\> User datagrams \-\> Internet Datagrams  
    - Web requests/responses (HTTP) \-\> Byte Stream  
      - Youtube/Wikipedia \-\> Web requests/responses  
    - Email sending (SMTP) \-\> Byte Stream  
    - Email receiving (IMAP) \-\> Byte Stream  
- Multiplexing ByteStream  
  - “u8 u8” (Which byte stream; what is the byte)  
    - Any reading and writing of one byte would be actually two bytes, the first byte for which byte stream and the second for the actual byte  
  - “u8 u8 {u8 u8 … u8}” (which byte stream; size of payload; sequence of characters of the string chunk)  
    - Any reading and writing of n bytes would be actually n \+ 2 bytes  
    - Tagged byte stream: HTTP/2 | SPDY  
- How to make ByteStream push idempotent?  
  - TCP Sender Message  
    	first\_index  
    	data   
    	FIN  
      
  - This works for out-of-order or multiple deliveries. Since UDP has a checksum field, altered TCP Message would be ignored on the UDP layer.  
  - What if datagrams are missing?  
    - How does the sender know that a datagram needs to be sent multiple times?  
    - DNS/DHCP: if we don’t receive an answer, then we retransmit. But such response/answer does not exist in \`pushing\` (void push())  
  - Acknowledgement  
    - TCP Receiver Message  
      - “A B C D E F G” each byte sent as a separate TCP sender message, and “D” is not received.  
      - “I got the sender message with first-index \= 2, length \= 1.”  
        - Valid but more work. There will be one receiver message for each sender message.  
      - “Got anything? Y/N. Next needed: \#3.”  
        - Acknowledgements are accumulative, and that make life simpler.  
      - Give FIN flag a number: “A B C D E F G FIN”.  
        TCP Sender Message: {sequence number, data}  
        TCP Receiver Message: {Next needed: optional\<int\>}  
        and TCP Receiver Message {Next needed: optional(8)} would mean FIN is received.  
      - TCP Receiver Message: {Next needed: optional\<int\>; available capacity:int}  
        - {Next needed: optional(3); available capacity: 2} \=== Receiver wants to here about “DE”.  
        - “DE” is the **window**. (Red area in that picture of lab 1\)  
    - **TCP Receiver Message: {Ack no: optional\<int\>; window size: int}**

  - TCP Segment  
      
  - This is the service abstraction that TCP is providing:  
  - And this is what happens under the hood (and also what you will be implementing in the labs)  
      
      
    	  
  - Rules of TCP  
    - Reply to any nonempty TCP Server Message

- What happens when a stream ends?  
  - My sender has ended its outgoing bytestream, but the incoming bytestream from the peer may not be ended.  
  - When a stream ends, can the same pair of ports be used? Reusing the same pair of ports makes it not clear to tell whether a datagram belongs to the old stream or the new stream.  
  - We want a new INCARNATION of the connection (new connection on the same pair of ports)  
  - **Sequence number**: start from a random big number \+ **SYN**: this sequence number should be viewed as the beginning of a stream  
    - If the sequence number doesn’t make sense on the old stream, and the SYN flag is true, the receiver knows this is a new incarnation of the connection.  
    - e.g. {sequnce\_no=12345, data=”abcdef”, SYN=true, FIN=true}, and {sequence\_no=99999, data=”xyz”, SYN=true, FIN=true}  
  - First seqno belongs to SYN flag, next seqnos belong to each byte of stream, final seqno belongs to FIN flag.  
    - It is very important to have SYN flag and FIN flag delivered reliably, so therefore receiver need to acknowledge SYN seqno and FIN seqno  
- What happens to TCP receiver message’s next\_needed\_idx field before receiving the SYN flag from the peer?  
  - Without seqno:  
    - I: {{first\_index=0, data=”abcdef”, FIN=true}, {next\_needed=0, window\_size=1000}}  
    - Peer: {{first\_index=0, data=””, FIN=true}, {next\_needed=7, window\_size=1000}}  
    - I: {{first\_index=7, data=””, FIN=false}, {next\_needed=1, window\_size=1000}}  
  - With seqno and SYN:  
    - I: {{seqno=12340, SYN=true, data=”abcdef”, FIN=true}, {**What should this be? (before seeing 9999 from the Peer**)}}  
    - Peer: {{seqno=9999, SYN=true, data=””, FIN=true}, {next\_needed=12348, window\_size=1000}}  
    - I: {{next\_needed=10001, window size \=1000}}  
  - ackno \= optional\<int\> (a pair of ACK flag and ackno int)  
    - I: {{seqno=12340, SYN=true, data=”abcdef”, FIN=true}, {ACK=false, ackno={missing}, window\_size=1000}}          (SYN)  
    - Peer: {{seqno=9999, SYN=true, data=””, FIN=true}, {ACK=true, ackno=12348, window\_size=1000}}              (SYN+ACK)  
    - I: {{ACK=true, ackno=10001, window size \=1000}}        (ACK)  
  - (SYN) \+ (SYN+ACK) \+ (ACK) \= “the three-way handshake”  
  - What if the two SYN messages are sent at the same time?  
    - ![][image1]  
    - Not a classic “three-way handshake” but still a valid way of starting a TCP connection.  
- Standardized TCP Message:  
  - Sender: {sequence number, SYN, data, FIN}  
  - Receiver: {ackno: optional\<int\>, window\_size}  
  - “User Datagram” info

[https://www.rfc-editor.org/rfc/rfc9293.html\#name-header-format](https://www.rfc-editor.org/rfc/rfc9293.html#name-header-format)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |       |C|E|U|A|P|R|S|F|                               |
   | Offset| Rsrvd |W|C|R|C|S|S|Y|I|            Window             |
   |       |       |R|E|G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           [Options]                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               :
   :                             Data                              :
   :                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         Note that one tick mark represents one bit position.
```

- Wireshark tool

[image1]: <data:image/png;base64,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>