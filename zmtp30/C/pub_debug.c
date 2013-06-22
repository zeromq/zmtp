//
//  ZMTP 3.0 publisher proof-of-concept

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>              
#include <arpa/inet.h>              

typedef struct {
    uint8_t flags;             //  Must be zero
    uint8_t size;              //  Size, 0 to 255 uint8_ts
    uint8_t data [255];        //  Message data
} zmtp_msg_t;

static void
derp (char *s)
{
    perror (s);
    exit (1);
}

static void
tcp_send (int handle, void *buffer, size_t len)
{
    if (send (handle, buffer, len, 0) == -1)
        derp ("send");
}

static void
tcp_recv (int handle, void *buffer, size_t len)
{
    printf (" - reading %d bytes: ", (int) len);
    fflush (stdout);
    size_t len_recd = 0;
    while (len_recd < len) {
        ssize_t bytes = recv (handle, buffer + len_recd, len - len_recd, 0);
        if (bytes == 0)
            break;              //  Peer has shutdown
        printf (" [%d]", (int) bytes);
        fflush (stdout);
        if (bytes == -1)
            derp ("recv");
        len_recd += bytes;
    }
    printf ("\n");
    fflush (stdout);
}

static void
zmtp_recv (int handle, zmtp_msg_t *msg)
{
    tcp_recv (handle, (uint8_t *) msg, 2);
    tcp_recv (handle, msg->data, msg->size);
}

static void
zmtp_send (int handle, zmtp_msg_t *msg)
{
    tcp_send (handle, (uint8_t *) msg, msg->size + 2);
}

//  This is the 3.0 greeting (64 uint8_ts)
typedef struct {
    uint8_t signature [10];
    uint8_t version [2];
    uint8_t mechanism [20];
    uint8_t as_server [1];
    uint8_t filler [31];
} zmtp_greeting_t;


int main (void)
{
    puts ("I: starting publisher");
    
    //  Create TCP listener socket
    int listener;
    if ((listener = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        derp ("socket");

    int on = 1;
    if (setsockopt (listener, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &on, sizeof (on)) == -1)
        derp ("setsockopt (SO_REUSEADDR)");
    
    //  Wait for one subscriber to connect, and handle it
    struct sockaddr_in si_this = { 0 };
    si_this.sin_family = AF_INET;
    si_this.sin_port = htons (9000);
    si_this.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (listener, &si_this, sizeof (si_this)) == -1)
        derp ("bind");

    if (listen (listener, 1) == -1)
        derp ("listen");
    
    puts ("I: waiting for connection");
    while (true) {
        int peer;
        if ((peer = accept (listener, NULL, NULL)) == -1)
            derp ("accept");
        puts ("I: ************  have new peer connection  ************");

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

        //  Read first uint8_t from peer
        zmtp_greeting_t incoming;
        tcp_recv (peer, &incoming, 1);
        uint8_t length = incoming.signature [0];
        if (length != 0xFF) {
            puts ("E: signature not valid (1)");
            close (peer);
            break;
        }
        //  Looks like 2.0+, read 9 more uint8_ts to be sure
        tcp_recv (peer, (uint8_t *) &incoming + 1, 9);
        if ((incoming.signature [9] & 1) != 1) {
            puts ("E: signature not valid (2)");
            close (peer);
            break;
        }
        //  Exchange major version numbers 
        puts ("I: signature valid, exchanging major versions");
        tcp_send (peer, (uint8_t *) &outgoing + 10, 1);
        tcp_recv (peer, (uint8_t *) &incoming + 10, 1);

        if (incoming.version [0] >= 3) {
            //  If version >= 3, the peer is using ZMTP 3.0, so send 
            //  rest of the greeting and continue with ZMTP 3.0.
            puts ("I: peer is talking ZMTP 3.0");
            puts ("I: sending rest of greeting...");
            tcp_send (peer, (uint8_t *) &outgoing + 11, 53);
            //  Get remainder of greeting from peer
            puts ("I: waiting for greeting from peer...");
            tcp_recv (peer, (uint8_t *) &incoming + 11, 53);
            //  Do NULL handshake - send READY command
            //  For now, empty dictionary
            puts ("I: have full greeting from peer");
            zmtp_msg_t ready = { 0x04, 8 };
            memcpy (ready.data, "READY   ", 8);
            puts ("I: sending READY");
            zmtp_send (peer, &ready);
            //  Now wait for peer's READY command
            puts ("I: expecting READY from peer");
            zmtp_recv (peer, &ready);
            assert (memcmp (ready.data, "READY   ", 8) == 0);
            puts ("I: OK! NULL security handshake completed");
        }
        else {
            puts ("E: major version not valid");
            close (peer);
            break;
        }
        //  Get subscription from peer
        zmtp_msg_t msg = { 0 };
        puts ("I: expecting subscription from peer...");
        zmtp_recv (peer, &msg);
        if (msg.size != 1 || msg.data [0] != 1) {
            puts ("E: subscription not complete");
            close (peer);
            break;
        }
        puts ("I: sending one message to peer");
        
        msg.size = 5;
        memcpy (msg.data, "HELLO", 5);
        zmtp_send (peer, &msg);
        
        //  Send WORLD to end broadcast
        memcpy (msg.data, "WORLD", 5);
        zmtp_send (peer, &msg);
        puts ("I: done, test ended");
        close (peer);
    }
    return 0;
}
