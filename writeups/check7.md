# Checkpoint 6 Writeup

**Name:** Hniii98  
**Time Spent:** ~1 hours  


Solo portion:

- Did your implementation successfully start and end a conversation with another copy of itself? Yes.
  
As the `server` part, run this in a terminal: 

```bash
hniii98@ysyx:~/Project/CS144/CS144$ ./build/apps/endtoend server cs144.keithw.org 1024
DEBUG: Network interface has Ethernet address 02:00:00:72:4f:55 and IP address 172.16.0.1
DEBUG: Network interface has Ethernet address 02:00:00:17:2f:be and IP address 10.0.0.172
DEBUG: adding route 172.16.0.0/12 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 192.168.0.0/16 => 10.0.0.192 on interface 1
DEBUG: Network interface has Ethernet address be:5b:60:fe:2c:6c and IP address 172.16.0.100
DEBUG: minnow listening for incoming connection...
```
It cames out to be successful to wait a incoming connection.

And run it as `client` in another terminal:
```bash
hniii98@ysyx:~/Project/CS144/CS144$ ./build/apps/endtoend client cs144.keithw.org 1025
DEBUG: Network interface has Ethernet address 02:00:00:78:b3:84 and IP address 192.168.0.1
DEBUG: Network interface has Ethernet address 02:00:00:18:28:02 and IP address 10.0.0.192
DEBUG: adding route 192.168.0.0/16 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 172.16.0.0/12 => 10.0.0.172 on interface 1
DEBUG: Network interface has Ethernet address be:eb:fb:f8:d8:53 and IP address 192.168.0.50
DEBUG: Connecting from 192.168.0.50:19653...
DEBUG: minnow connecting to 172.16.0.100:1234...
DEBUG: minnow successfully connected to 172.16.0.100:1234.

```
Everthing goes well, client print `DEBUG: minnow successfully connected to 172.16.0.100:1234.` and server print `DEBUG: minnow new connection from 192.168.0.50:36490.`

And then, i try to make a conversation between them.Send `Hello client!` in server and `Hello server!`in client.

Check their status:
- Server:
```bash
hniii98@ysyx:~/Project/CS144/CS144$ ./build/apps/endtoend server cs144.keithw.org 1024
DEBUG: Network interface has Ethernet address 02:00:00:72:4f:55 and IP address 172.16.0.1
DEBUG: Network interface has Ethernet address 02:00:00:17:2f:be and IP address 10.0.0.172
DEBUG: adding route 172.16.0.0/12 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 192.168.0.0/16 => 10.0.0.192 on interface 1
DEBUG: Network interface has Ethernet address be:5b:60:fe:2c:6c and IP address 172.16.0.100
DEBUG: minnow listening for incoming connection...
DEBUG: minnow new connection from 192.168.0.50:36490.
Hello client!
Hello server!
```

- client
```bash
hniii98@ysyx:~/Project/CS144/CS144$ ./build/apps/endtoend client cs144.keithw.org 1025
DEBUG: Network interface has Ethernet address 02:00:00:fe:3b:74 and IP address 192.168.0.1
DEBUG: Network interface has Ethernet address 02:00:00:6c:8d:c6 and IP address 10.0.0.192
DEBUG: adding route 192.168.0.0/16 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 172.16.0.0/12 => 10.0.0.172 on interface 1
DEBUG: Network interface has Ethernet address 32:6f:23:b0:7e:46 and IP address 192.168.0.50
DEBUG: Connecting from 192.168.0.50:36490...
DEBUG: minnow connecting to 172.16.0.100:1234...
DEBUG: minnow successfully connected to 172.16.0.100:1234.
Hello client!
Hello server!
```

Finally, press `ctrl + D` to wait each side to be done:
- server:
```bash
DEBUG: minnow outbound stream to 192.168.0.50:36490 finished (0 seqnos still in flight).
DEBUG: Outbound stream to 172.16.0.100 finished.
DEBUG: minnow inbound stream from 192.168.0.50:36490 finished cleanly.
DEBUG: Inbound stream from 172.16.0.100 finished.
DEBUG: minnow waiting for clean shutdown... DEBUG: minnow outbound stream to 192.168.0.50:36490 has been fully acknowledged.
DEBUG: minnow TCP connection finished cleanly.
done.
Exiting... done.
```

- client
```bash
DEBUG: Outbound stream to 172.16.0.100 finished.
DEBUG: minnow outbound stream to 172.16.0.100:1234 finished (0 seqnos still in flight).
DEBUG: minnow inbound stream from 172.16.0.100:1234 finished cleanly.
DEBUG: Inbound stream from 172.16.0.100 finished.
DEBUG: minnow waiting for clean shutdown... DEBUG: minnow outbound stream to 172.16.0.100:1234 has been fully acknowledged.
DEBUG: minnow TCP connection finished cleanly.
done.
Exiting... done.
```

- Did it successfully transfer a one-megabyte file, with contents identical upon receipt? Yes.

In `server` termial, construct the `big.txt` and send it.

```bash
dd if=/dev/urandom bs=1M count=1 of=/tmp/big.txt
```

```bash
./build/apps/endtoend server cs144.keithw.org even number < /tmp/big.txt
```


In the `client` terminal, connect to receive this file.

```bash
hniii98@ysyx:~/Project/CS144/CS144$ </dev/null ./build/apps/endtoend client cs144.keithw.org 1025 > /tmp/big-received.txt
```

Waiting...

And `server` fully sent `big.txt` and `client` receives all.

`server`prints:
```bash
hniii98@ysyx:~/Project/CS144/CS144$ ./build/apps/endtoend server cs144.keithw.org 1024 < /tmp/big.txt
DEBUG: Network interface has Ethernet address 02:00:00:3f:ed:fb and IP address 172.16.0.1
DEBUG: Network interface has Ethernet address 02:00:00:b5:db:fd and IP address 10.0.0.172
DEBUG: adding route 172.16.0.0/12 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 192.168.0.0/16 => 10.0.0.192 on interface 1
DEBUG: Network interface has Ethernet address 02:46:49:61:69:8d and IP address 172.16.0.100
DEBUG: minnow listening for incoming connection...
DEBUG: minnow new connection from 192.168.0.50:29360.
DEBUG: minnow inbound stream from 192.168.0.50:29360 finished cleanly.
DEBUG: Inbound stream from 172.16.0.100 finished.
DEBUG: Outbound stream to 172.16.0.100 finished.
DEBUG: minnow waiting for clean shutdown... DEBUG: minnow outbound stream to 192.168.0.50:29360 finished (63000 seqnos still in flight).
DEBUG: minnow outbound stream to 192.168.0.50:29360 has been fully acknowledged.
DEBUG: minnow TCP connection finished cleanly.
done.
Exiting... done.
hniii98@ysyx:~/Project/CS144/CS144$ 
```

- `client` prints
```bash
hniii98@ysyx:~/Project/CS144/CS144$ </dev/null ./build/apps/endtoend client cs144.keithw.org 1025 > /tmp/big-received.txt
DEBUG: Network interface has Ethernet address 02:00:00:2b:c5:21 and IP address 192.168.0.1
DEBUG: Network interface has Ethernet address 02:00:00:97:91:4e and IP address 10.0.0.192
DEBUG: adding route 192.168.0.0/16 => (direct) on interface 0
DEBUG: adding route 10.0.0.0/8 => (direct) on interface 1
DEBUG: adding route 172.16.0.0/12 => 10.0.0.172 on interface 1
DEBUG: Network interface has Ethernet address 92:d4:94:a2:7c:08 and IP address 192.168.0.50
DEBUG: Connecting from 192.168.0.50:29360...
DEBUG: minnow connecting to 172.16.0.100:1234...
DEBUG: minnow successfully connected to 172.16.0.100:1234.
DEBUG: Outbound stream to 172.16.0.100 finished.
DEBUG: minnow outbound stream to 172.16.0.100:1234 finished (0 seqnos still in flight).
DEBUG: minnow outbound stream to 172.16.0.100:1234 has been fully acknowledged.
DEBUG: minnow inbound stream from 172.16.0.100:1234 finished cleanly.
DEBUG: Inbound stream from 172.16.0.100 finished.
DEBUG: minnow waiting for clean shutdown... DEBUG: minnow TCP connection finished cleanly.
done.
Exiting... done.
```

Diff these two files:

```bash
hniii98@ysyx:~/Project/CS144/CS144$ sha256sum /tmp/big.txt 
53dbc7e1ec01a8bbd5afa289a431cce7439c56ba526b8726b2d0e37f10897521  /tmp/big.txt
hniii98@ysyx:~/Project/CS144/CS144$ sha256sum /tmp/big-received.txt 
53dbc7e1ec01a8bbd5afa289a431cce7439c56ba526b8726b2d0e37f10897521  /tmp/big-received.txt
hniii98@ysyx:~/Project/CS144/CS144$ 
```

All go well !!