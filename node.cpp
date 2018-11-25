#include "node.h"
#include <iostream>
#include <iomanip>

using namespace std;

Node::Node() {
    ack_expected = 0;           /* next ack_expected inbound */
    next_frame_to_send = 0;     /* next frame going out */
    frame_expected = 0;         /* number of frame_expected inbound */
    nbuffered = 0;              /* initially no packets are buffered */
    for (int i=0; i<MAX_SEQ+1; ++i) {
        stop_timer(i);  //initially all timers are stopped
    }
    timeout_default = 10000;
}

bool Node::between(seq_nr a, seq_nr b, seq_nr c) {
    /* Return true if a <= b < c circularly; false otherwise. */
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return true;
    else
        return false;
}

void Node::send_data(seq_nr frame_nr, packet buffer[]) {
    /* Construct and send a data frame. */
    frame s;                                            /* scratch variable */
    s.kind = frame_kind::data;
    s.info = buffer[frame_nr];                          /* insert packet into frame */
    s.seq = frame_nr;                                   /* insert sequence number into frame */
    to_physical_layer(&s);                              /* transmit the frame */
    start_timer(frame_nr);                              /* start the timer running */
}

void Node::send_ack(seq_nr frame_nr) {
    frame s;
    s.kind = ack;
    s.ack = frame_nr;
    to_physical_layer(&s);
}

bool Node::has_event() {
    return !event_queue.empty();
}

event_type Node::get_event() {
    event_type result = event_queue.front();
    event_queue.pop();
    return result;
}

/* Fetch a packet from the network layer for transmission on the channel. */
void Node::from_network_layer(packet *p) {
    //override
}
/* Deliver information from an inbound frame to the network layer. */
void Node::to_network_layer(packet *p) {
    //override
}

/* Go get an inbound frame from the physical layer and copy it to r. */
void Node::from_physical_layer(frame *r) {
    //override
}
/* Pass the frame to the physical layer for transmission. */
void Node::to_physical_layer(frame *s) {
    //override
}

/* Start the clock running and enable the timeout event. */
void Node::start_timer(seq_nr k) {
    timers[k] = timeout_default;
}
/* Stop the clock and disable the timeout event. */
void Node::stop_timer(seq_nr k) {
    timers[k] = -1;
}

void Node::consume_events() {
    while (has_event()) {
        event = get_event();
        switch (event) {
            case network_layer_ready: /* the network layer has a packet to send */
                // cout << "net layer ready" << endl; //todo: remove
                //check first if we didn't exceed the sender sliding window
                if (nbuffered < MAX_SEQ) {
                    /* Accept, save, and transmit a new frame. */
                    from_network_layer(&buffer[next_frame_to_send]);    /* fetch new packet */
                    nbuffered = nbuffered + 1;                          /* expand the sender’s window */
                    send_data(next_frame_to_send, buffer);              /* transmit the frame */
                    inc(next_frame_to_send);                            /* advance sender’s upper window edge */
                }
                // cout << "net layer done" << endl; //todo: remove
                break;
            case frame_arrival:          /* a data or control frame has arrived */
                from_physical_layer(&r); /* get incoming frame from physical layer */
                switch (r.kind) {
                    case frame_kind::data:
                        received_data(&r);
                    break;
                    case ack:
                        /* Ack n implies n - 1, n - 2, etc. Check for this. */
                        while (between(ack_expected, r.ack, next_frame_to_send)) {
                            nbuffered = nbuffered - 1; /* one frame fewer buffered */
                            stop_timer(ack_expected);  /* frame arrived intact; stop timer */
                            inc(ack_expected);         /* contract sender’s window */
                        }
                        received_ack(r.ack);
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

void Node::received_ack(seq_nr frame_nr) {
    //override
}

void Node::received_data(frame *r) {
    if (r->seq == frame_expected) {
        /* Frames are accepted only in order. */
        to_network_layer(&r->info); /* pass packet to network layer */
        send_ack(frame_expected);
        inc(frame_expected);       /* advance lower edge of receiver’s window */
    }
}

void Node::timer_tick() {
    for (int i=0; i<MAX_SEQ+1; ++i) {
        if (timers[i] > -1) {
            timers[i] -= 1;
        }
        if (timers[i] == 0) {
            event_queue.push(timeout);
            timers[i] = -1;
        }
    }
}



/* Sender */

const int padding = 13;
const int space = 40;

void Sender::send_data(seq_nr frame_nr, packet buffer[]) {
    Node::send_data(frame_nr, buffer);
    cout << setw(padding) << left << "(Sender)";
    cout << "Sent: " << buffer[frame_nr].data << endl;
}


void Sender::from_network_layer(packet *p) {
    *p = network_incoming_buffer.front();
    network_incoming_buffer.pop();
}

void Sender::to_network_layer(packet *p) {
    //will not happen (except for ACK which
    //is handled by received_ack() method)
}

void Sender::from_physical_layer(frame *r) {
    *r = physical_incoming_buffer.front();
    physical_incoming_buffer.pop();
}

void Sender::to_physical_layer(frame *s) {
    my_receiver->physical_incoming_buffer.push(*s);
    my_receiver->event_queue.push(frame_arrival);
}

void Sender::received_ack(seq_nr frame_nr) {
    cout << setw(padding) << left << "(Sender)";
    cout << "Received ACK #" << frame_nr << endl;    
}



/* Receiver */


void Receiver::received_data(frame *r) {
    if (r->seq == frame_expected) {
        /* Frames are accepted only in order. */
        cout << setw(space) << "";
        cout << setw(padding) << left << "(Receiver)";
        cout << "Received: " << r->info.data << " (#" << r->seq << ")" << endl;
        to_network_layer(&r->info); /* pass packet to network layer */
        send_ack(frame_expected);
        inc(frame_expected);       /* advance lower edge of receiver’s window */
    } else {
        cout << setw(space) << "";
        cout << setw(padding) << left << "(Receiver)";
        cout << "Received unexpected: " << r->info.data << " (#" << r->seq << ") [DISCARDED]" << endl;
    }
}

void Receiver::send_ack(seq_nr frame_nr) {
    Node::send_ack(frame_nr);
    cout << setw(space) << "";
    cout << setw(padding) << left << "(Receiver)";
    cout << "Sent ACK (#" << frame_nr << ")" << endl;
}

void Receiver::from_network_layer(packet *p) {
    //will not happen
}

void Receiver::to_network_layer(packet *p) {
    //do nothing, will print received data
    //inside Receiver::received_data() method
}

void Receiver::from_physical_layer(frame *r) {
    *r = physical_incoming_buffer.front();
    physical_incoming_buffer.pop();
}

void Receiver::to_physical_layer(frame *s) {
    my_sender->physical_incoming_buffer.push(*s);
    my_sender->event_queue.push(frame_arrival);
}