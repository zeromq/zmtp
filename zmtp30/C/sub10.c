//
//  ZMTP 1.0 raw subscriber
//  Implements http://rfc.zeromq.org/spec:13
//
#include "zmtplib.h"

int main (void)
{
    puts ("I: starting ZMTP 1.0 subscriber");
    
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
        
    //  Exchange identities with peer
    zmtp10_msg_t identity = { 1, 0 };
    zmtp10_send (peer, &identity);
    zmtp10_recv (peer, &identity);

    //  Get broadcast until it's done
    zmtp10_msg_t msg = { 0, 0 };
    size_t count = 0;
    while (true) {
        zmtp10_recv (peer, &msg);
        if (msg.size == 6 && memcmp (msg.data, "WORLD", 5) == 0)
            break;      //  Finished
        count++;
    }
    printf ("I: %zd messages received\n", count);

    close (peer);
    return 0;
}
