This is a simple dns server system developped in C Language.
It has been complied in Linux and debugged through dig tool.

When getting this exercise,I think there are 3 key points to consider:
1. how to store and look up A-records effectively
2. how to decode dns query messages from clients and encode response messages to clients.
3. to create an UDP socket server to do data transport

My solution is as below:
1. use hash table algorithm to store and search A-records. 
   When the process starts, A-records are loaded from /etc/hosts file and inserted into hash table.
   Because A records are correspondence between domain name and IP address, so I adopted BKDR string hash algorithm.
   There are 1024 buckets in the hash table totally.
   Hash value is computed through BKDR string hash algorithm, If the value is h, then this item is stored in bucket[h] list.

   When the DNS server receives a dns query message, hash value of the domain name is computed through hash algorithm.
   If the value is h, then comparision between domain name in the query message and domain name of hash node in bucket[h] list is carried out.  If there exists the same domain name, then the corresponding IP address is returned to dns client. 

2. To make clear of frame structure of DNS standard query and response message, I referenced RFC 1034 and RFC 1035.
   When get acquainted of this, the decoding of query and encoding of responce messages becomes clear.

3. An UDP socket using epoll method is created. 
   Different from common select() function, the efficiency of epoll is apparently higher when handling large number of socket connections.  
   Because we just moniter one socket here, so the effect is not that obvious. I'd just want to do an attempt.


Also, there are several points to Note:
1. simpledns process moniters on port 53, so root permission is necessary.
2. dig tool can be used to test dimpledns.
   For example, "dig@127.0.0.1 locolhost" can executed to query IP address of localhost from server 127.0.0.1
   My test result (dns.pcap) is attached. 
3. host and name correspodence must be configured in pairs in /etc/hosts. 
   Multiple addresses and alias are not supported



   Notice:
          How to close dnsmasq ?
           sudo vi /etc/NetworkManager/NetworkManager.conf