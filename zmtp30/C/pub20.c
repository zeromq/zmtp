//
//  ZMTP/2.0 raw publisher
//  Implements http://rfc.zeromq.org/spec:15
//
#include "zmtplib.h"

//  This is the 2.0 greeting (64 bytes)
typedef struct {
    byte signature [10];    //  0xFF 8*0x00 0x7F
    byte revision;          //  0x01 = ZMTP/2.0
    byte socktype;          //  0x01 = PUB
    byte identity [2];      //  Empty message
} zmtp_greeting_t;

int main (void)
{
    puts ("I: starting ZMTP 2.0 publisher");
    
    //  Create TCP socket
    int peer;
    if ((peer = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        derp ("socket");

    //  We'll connect peer to subscriber for simplicity
    struct sockaddr_in si_peer = { 0 };
    si_peer.sin_family = AF_INET;
    si_peer.sin_port = htons (9000);
    si_peer.sin_addr.s_addr = inet_addr ("127.0.0.1");
    
    //  Keep trying to connect until we succeed
    while (connect (peer, &si_peer, sizeof (si_peer)) == -1)
        sleep (1);

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
    
    //  Count how many HELLOs we can send in one second
    size_t total = 0;
    int64_t finish_at = time_now () + 1000;

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
