//
//  ZMTP 3.0 subscriber proof-of-concept
//  Implements http://rfc.zeromq.org/spec:23 with NULL mechanism
//  Implements backwards compatibility with ZMTP 1.0 and 2.0

#include "zmtplib.h"

//  This is the 3.0 greeting (64 bytes)
typedef struct {
    byte signature [10];
    byte version [2];
    byte mechanism [20];
    byte as_server [1];
    byte filler [31];
} zmtp_greeting_t;

int main (void)
{
    puts ("I: starting subscriber");
    
    //  Create TCP socket
    int peer;
    if ((peer = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        derp ("socket");
    
    //  Connect to publisher
    struct sockaddr_in si_peer = { 0 };
    si_peer.sin_family = AF_INET;
    si_peer.sin_port = htons (9000);
    si_peer.sin_addr.s_addr = inet_addr ("127.0.0.1");

    //  Keep trying to connect until we succeed
    while (connect (peer, &si_peer, sizeof (si_peer)) == -1)
        sleep (1);

    //  Do full version detection (1.0, 2.0, or 3.0)

    //  This is our greeting (64 octets)
    zmtp_greeting_t outgoing = {
        { 0xFF, 0, 0, 0, 0, 0, 0, 0, 1, 0x7F },
        { 3, 0 },
        { 'N', 'U', 'L', 'L', 0 },
        { 0 },
        { 0 }
    };
    //  Do full backwards version detection following RFC23
    //  Send first ten bytes of greeting to peer
    tcp_send (peer, &outgoing, 10);

    //  1 = ZMTP 1.0, 2 = 2.0 or higher
    int frame_level;

    //  Read first byte from peer
    zmtp_greeting_t incoming;
    tcp_recv (peer, &incoming, 1);
    byte length = incoming.signature [0];
    if (length == 0xFF) {
        //  Looks like 2.0+, read 9 more bytes to be sure
        tcp_recv (peer, (byte *) &incoming + 1, 9);
        if ((incoming.signature [9] & 1) == 1) {
            //  Peer is 2.0 or later
            frame_level = 2;

            //  Exchange major version numbers
            tcp_send (peer, (byte *) &outgoing + 10, 1);
            tcp_recv (peer, (byte *) &incoming + 10, 1);

            if (incoming.version [0] == 1 || incoming.version [0] == 2) {
                //  if version is 1 or 2, the peer is using ZMTP 2.0, so
                //  send ZMTP 2.0 socket type and identity and continue
                //  with ZMTP 2.0.
                //  Socket type = XPUB (1), identity is empty (0, 0)
                puts ("I: peer is talking ZMTP 2.0");
                byte socktypeid [3] = { 2, 0, 0 };
                tcp_send (peer, socktypeid, 3);
                //  Get peer's socket type and identity
                tcp_recv (peer, socktypeid, 3);
                //  Check peer is a XPUB socket
                assert (socktypeid [0] == 1);
            }
            else
            if (incoming.version [0] >= 3) {
                //  If version >= 3, the peer is using ZMTP 3.0, so send
                //  rest of the greeting and continue with ZMTP 3.0.
                puts ("I: peer is talking ZMTP 3.0");
                tcp_send (peer, (byte *) &outgoing + 11, 53);
                //  Get remainder of greeting from peer
                tcp_recv (peer, (byte *) &incoming + 11, 53);
                //  Do NULL handshake - send READY command
                //  For now, empty dictionary
                zmtp_msg_t ready = { 0x04, 8 };
                memcpy (ready.data, "READY   ", 8);
                zmtp_send (peer, &ready);
                //  Now wait for peer's READY command
                zmtp_recv (peer, &ready);
                assert (ready.flags == 0x04);
                assert (ready.size == 8);
                assert (memcmp (ready.data, "READY   ", 8) == 0);
                puts ("I: NULL security handshake completed");
            }
            else {
                puts ("E: peer sent invalid version (0)");
                exit (1);
            }
        }
        else {
            //  Peer is 1.0 but sent a long identity, so forget it
            puts ("I: peer sent ZMTP 1.0 long identity frame");
            puts ("E: we're not going to handle this... ending test");
            exit (1);
        }
    }
    else {
        //  Peer is 1.0 and has sent us identity frame
        //  Here we read it, and discard it
        puts ("I: peer is talking ZMTP 1.0");
        frame_level = 1;
        byte identity [255];
        tcp_recv (peer, identity, length);
    }

    size_t count = 0;
    if (frame_level == 2) {
        //  Send subscription to peer
        puts ("I: subscribing to test data stream");
        zmtp_msg_t msg = { 0 };
        msg.size = 1;
        msg.data [0] = 1;               //  SUBSCRIBE
        zmtp_send (peer, &msg);
        //  Get broadcast until it's done
        while (true) {
            zmtp_recv (peer, &msg);
            if (msg.size == 5 && memcmp (msg.data, "WORLD", 5) == 0)
                break;      //  Finished
            count++;
        }
    }
    else {
        //  Get broadcast until it's done
        zmtp10_msg_t msg = { 0, 0 };
        while (true) {
            zmtp10_recv (peer, &msg);
            if (msg.size == 6 && memcmp (msg.data, "WORLD", 5) == 0)
                break;      //  Finished
            count++;
        }
    }
    printf ("I: %zd messages received\n", count);

    close (peer);
    return 0;
}
