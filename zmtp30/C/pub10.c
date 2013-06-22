//
//  ZMTP 1.0 raw publisher
//  Implements http://rfc.zeromq.org/spec:13
//
#include "zmtplib.h"

int main (void)
{
    puts ("I: starting ZMTP 1.0 publisher");
    
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

    //  Exchange identities with peer
    zmtp10_msg_t identity = { 1, 0 };
    zmtp10_send (peer, &identity);
    zmtp10_recv (peer, &identity);

    //  Count how many HELLOs we can send in one second
    size_t total = 0;
    int64_t finish_at = time_now () + 1000;

    zmtp10_msg_t msg = { 0, 0 };
    msg.size = 6;
    memcpy (msg.data, "HELLO", 5);
    while (time_now () < finish_at) {
        //  Send 100K and then check time again
        int count = 0;
        for (count = 0; count < 10000; count++)
            zmtp10_send (peer, &msg);
        total++;
    }
    printf ("I: %zd0000 messages sent\n", total);
    
    //  Send WORLD to end broadcast
    memcpy (msg.data, "WORLD", 5);
    zmtp10_send (peer, &msg);
    
    close (peer);
    return 0;
}
