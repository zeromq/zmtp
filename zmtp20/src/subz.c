//
//  ZMTP/2.0 raw subscriber
//  Implements http://rfc.zeromq.org/spec:15
//
#include "zmtplib.h"

int main (void)
{
    //  Create TCP socket
    int listener;
    if ((listener = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        derp ("socket");

    //  We'll connect publisher to subscriber for simplicity
    struct sockaddr_in si_this = { 0 };
    si_this.sin_family = AF_INET;
    si_this.sin_port = htons (9000);
    si_this.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (listener, &si_this, sizeof (si_this)) == -1)
        derp ("bind");

    if (listen (listener, 1) == -1)
        derp ("listen");
    
    //  Wait for one connection and handle it
    int peer;
    if ((peer = accept (listener, NULL, NULL)) == -1)
        derp ("accept");
        
    //  Send greeting to peer
    zmtp_greeting_t greeting = { { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F }, 1, 0, { 0, 0 } };
    greeting.socktype = 2;
    tcp_send (peer, &greeting, sizeof (greeting));

    //  Get greeting from peer
    tcp_recv (peer, &greeting, sizeof (greeting));
    assert (greeting.socktype == 1);
    assert (greeting.revision >= 1);

    //  Send subscription to peer
    zmtp_msg_t msg = { 0 };
    msg.size = 1;
    msg.data [0] = 1;               //  SUBSCRIBE
    zmtp_send (peer, &msg);

    //  Get broadcast until it's done
    size_t count = 0;
    while (true) {
        zmtp_recv (peer, &msg);
        if (msg.size == 5 && memcmp (msg.data, "WORLD", 5) == 0)
            break;      //  Finished
        count++;
    }
    printf ("%zd messages received\n", count);

    close (peer);
    return 0;
}
