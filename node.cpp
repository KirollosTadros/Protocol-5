#include "node.h"

class Node {
public:
    seq_nr next_frame_to_send;  /* (SENDER) MAX_SEQ > 1; used for outbound stream */
    seq_nr ack_expected;        /* (SENDER) oldest frame as yet unacknowledged */
    seq_nr frame_expected;      /* (RECEIVER) next frame_expected on inbound stream */
    packet buffer[MAX_SEQ + 1]; /* (SENDER) buffers for the outbound stream */
    seq_nr nbuffered;           /* (SENDER) number of output buffers currently in use */
    frame r;                    /* scratch variable */
    event_type event;

    Node() {
        ack_expected = 0;           /* next ack_expected inbound */
        next_frame_to_send = 0;     /* next frame going out */
        frame_expected = 0;         /* number of frame_expected inbound */
        nbuffered = 0;              /* initially no packets are buffered */
    }
    
    bool between(seq_nr a, seq_nr b, seq_nr c) {
        /* Return true if a <= b < c circularly; false otherwise. */
        if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
            return true;
        else
            return false;
    }

    void send_data(seq_nr frame_nr, packet buffer[]) {
        /* Construct and send a data frame. */
        frame s;                                            /* scratch variable */
        s.kind = data;
        s.info = buffer[frame_nr];                          /* insert packet into frame */
        s.seq = frame_nr;                                   /* insert sequence number into frame */
        to_physical_layer(&s);                              /* transmit the frame */
        start_timer(frame_nr);                              /* start the timer running */
    }

    void send_ack(seq_nr frame_nr) {
        frame s;
        s.kind = ack;
        s.ack = frame_nr;
        to_physical_layer(&s);
    }

    bool has_event() {
        //todo: implement
    }

    event_type get_event() {
        //todo: implement
    }

    /* Fetch a packet from the network layer for transmission on the channel. */
    void from_network_layer(packet *p) {
        
    }
    /* Deliver information from an inbound frame to the network layer. */
    void to_network_layer(packet *p) {
        
    }

    /* Go get an inbound frame from the physical layer and copy it to r. */
    void from_physical_layer(frame *r) {
        
    }
    /* Pass the frame to the physical layer for transmission. */
    void to_physical_layer(frame *s) {
        
    }

    /* Start the clock running and enable the timeout event. */
    void start_timer(seq_nr k) {
        
    }
    /* Stop the clock and disable the timeout event. */
    void stop_timer(seq_nr k) {
        
    }

    void consume_events() {
        while (has_event()) {
            event = get_event();
            switch (event) {
                case network_layer_ready: /* the network layer has a packet to send */
                    //check first if we didn't exceed the sender sliding window
                    if (nbuffered < MAX_SEQ) {
                        /* Accept, save, and transmit a new frame. */
                        from_network_layer(&buffer[next_frame_to_send]);    /* fetch new packet */
                        nbuffered = nbuffered + 1;                          /* expand the sender’s window */
                        send_data(next_frame_to_send, buffer);              /* transmit the frame */
                        inc(next_frame_to_send);                            /* advance sender’s upper window edge */
                    }
                    break;
                case frame_arrival:          /* a data or control frame has arrived */
                    from_physical_layer(&r); /* get incoming frame from physical layer */
                    switch (r.kind) {
                        case data:
                            if (r.seq == frame_expected) {
                                /* Frames are accepted only in order. */
                                to_network_layer(&r.info); /* pass packet to network layer */
                                send_ack(frame_expected);
                                inc(frame_expected);       /* advance lower edge of receiver’s window */
                            }
                        break;
                        case ack:
                            /* Ack n implies n - 1, n - 2, etc. Check for this. */
                            while (between(ack_expected, r.ack, next_frame_to_send)) {
                                /* Handle piggybacked ack. */
                                nbuffered = nbuffered - 1; /* one frame fewer buffered */
                                stop_timer(ack_expected);  /* frame arrived intact; stop timer */
                                inc(ack_expected);         /* contract sender’s window */
                            }
                        break;
                        default:
                            ; //do nothing, not reached
                    }
                    break;
                case timeout:                          /* trouble; retransmit all outstanding frames */
                    next_frame_to_send = ack_expected; /* start retransmitting here */
                    for (seq_nr i = 1; i <= nbuffered; i++)
                    {
                        send_data(next_frame_to_send, buffer); /* resend frame */
                        inc(next_frame_to_send);                               /* prepare to send the next one */
                    }
            }
        }
    }

};
