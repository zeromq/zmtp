//
//  ZMTP/2.0 0MQ publisher
//  We use an XPUB socket to ensure there's no message loss
//
#include "czmq.h"

int main (void)
{
    zctx_t *ctx = zctx_new ();
    zctx_set_linger (ctx, 1000);
    
    void *pub = zsocket_new (ctx, ZMQ_XPUB);
    zsocket_set_hwm (pub, 0);
    zsocket_connect (pub, "tcp://127.0.0.1:9000");

    //  Wait for subscriber to subscribe
    zframe_t *frame = zframe_recv (pub);
    zframe_destroy (&frame);
    
    //  Send HELLOs for five seconds
    size_t total = 0;
    int64_t finish_at = zclock_time () + 5000;
    
    while (zclock_time () < finish_at) {
        //  Send 100K and then check time again
        int count = 0;
        for (count = 0; count < 100000; count++)
            zstr_send (pub, "HELLO");
        total++;
    }
    printf ("%zd00000 messages sent\n", total);
    
    zstr_send (pub, "WORLD");
    zctx_destroy (&ctx);
    return 0;
}
