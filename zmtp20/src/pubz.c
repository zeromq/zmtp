//
//  ZMTP/2.0 raw publisher
//  Implements http://rfc.zeromq.org/spec:15
//
#include "zmtplib.h"

int main (void)
{
    //  Create TCP socket
    int peer;
    if ((peer = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        derp ("socket");

    //  We'll connect peer to subscriber for simplicity
    struct sockaddr_in si_peer = { 0 };
    si_peer.sin_family = AF_INET;
    si_peer.sin_port = htons (9000);
    si_peer.sin_addr.s_addr = inet_addr ("127.0.0.1");
    if (connect (peer, &si_peer, sizeof (si_peer)) == -1)
        derp ("connect");

    //  Send greeting to peer
    zmtp_greeting_t greeting = { { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F }, 1, 0, { 0, 0 } };
    greeting.socktype = 1;
    tcp_send (peer, &greeting, sizeof (greeting));
    
    //  Get greeting from peer
    tcp_recv (peer, &greeting, sizeof (greeting));
    assert (greeting.socktype == 2);
    assert (greeting.revision >= 1);

    //  Get subscription from peer
    zmtp_msg_t msg = { 0 };
    zmtp_recv (peer, &msg);
    assert (msg.size == 1);
    assert (msg.data [0] == 1);     //  SUBSCRIBE
    
    //  Count how many HELLOs we can send in five seconds
    size_t total = 0;
    int64_t finish_at = time_now () + 5000;

    msg.size = 5;
    memcpy (msg.data, "HELLO", 5);
    while (time_now () < finish_at) {
        //  Send 100K and then check time again
        int count = 0;
        for (count = 0; count < 100000; count++)
            zmtp_send (peer, &msg);
        total++;
    }
    printf ("%zd00000 messages sent\n", total);
    
    //  Send WORLD to end broadcast
    memcpy (msg.data, "WORLD", 5);
    zmtp_send (peer, &msg);
    
    close (peer);
    return 0;
}
