//
//  ZMTP/2.0 0MQ subscriber
//
#include "czmq.h"

int main (void)
{
    zctx_t *ctx = zctx_new ();
    void *sub = zsocket_new (ctx, ZMQ_SUB);
    zsocket_set_hwm (sub, 0);
    zsocket_bind (sub, "tcp://*:9000");
    zsocket_set_subscribe (sub, "");

    //  Get broadcast until it's done
    size_t count = 0;
    while (true) {
        char *msg = zstr_recv (sub);
        if (msg == NULL || streq (msg, "WORLD"))
            break;      //  Interrupted or finished
        count++;
        free (msg);
    }
    printf ("%zd messages received\n", count);
    
    zctx_destroy (&ctx);
    return 0;
}
