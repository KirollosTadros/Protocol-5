#ifndef NODE_H
#define NODE_H

#include <queue>

#define MAX_SEQ 3

typedef enum {
    frame_arrival,
    timeout,
    network_layer_ready
} event_type;

#define MAX_PKT 1024                /* determines packet size in bytes */

typedef unsigned int seq_nr;        /* sequence or ack numbers */
typedef struct {unsigned char data[MAX_PKT];} packet;   /* packet definition */
typedef enum {data, ack} frame_kind;                    /* frame_kind definition */
typedef struct {        /* frames are transported in this layer */
    frame_kind kind;    /* what kind of frame is it? */
    seq_nr seq;         /* sequence number */
    seq_nr ack;         /* acknowledgement number */
    packet info;        /* the network layer packet */
} frame;
typedef int timer_t;

/* Macro inc is expanded in-line: increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0

class Node {
public:
    seq_nr next_frame_to_send;  /* (SENDER) MAX_SEQ > 1; used for outbound stream */
    seq_nr ack_expected;        /* (SENDER) oldest frame as yet unacknowledged */
    seq_nr frame_expected;      /* (RECEIVER) next frame_expected on inbound stream */
    packet buffer[MAX_SEQ + 1]; /* (SENDER) buffers for the outbound stream */
    seq_nr nbuffered;           /* (SENDER) number of output buffers currently in use */
    frame r;                    /* scratch variable */
    event_type event;
    timer_t timers[MAX_SEQ + 1];    //a timer for each frame in the sliding window
    std::queue<event_type> event_queue;
    unsigned int timeout_default;   //value of timeout before re-sending a frame

    Node();
    bool between(seq_nr a, seq_nr b, seq_nr c);
    virtual void send_data(seq_nr frame_nr, packet buffer[]);
    virtual void send_ack(seq_nr frame_nr);

    bool has_event();
    event_type get_event();

    /* Fetch a packet from the network layer for transmission on the channel. */
    virtual void from_network_layer(packet *p);
    /* Deliver information from an inbound frame to the network layer. */
    virtual void to_network_layer(packet *p);

    /* Go get an inbound frame from the physical layer and copy it to r. */
    virtual void from_physical_layer(frame *r);
    /* Pass the frame to the physical layer for transmission. */
    virtual void to_physical_layer(frame *s);

    /* Start the clock running and enable the timeout event. */
    void start_timer(seq_nr k);
    /* Stop the clock and disable the timeout event. */
    void stop_timer(seq_nr k);

    /* Allow the network layer to cause a network layer ready event. */
    virtual void enable_network_layer();
    /* Forbid the network layer from causing a network layer ready event. */
    virtual void disable_network_layer();

    void consume_events();
    virtual void handle_network_layer_ready();
    virtual void handle_timeout();
    virtual void received_ack(seq_nr frame_nr);
    virtual void received_data(frame *r);
    void timer_tick();
};

class Receiver;     //for circular reference

class Sender : public Node {
public:
    Receiver* my_receiver;
    std::queue<frame> physical_incoming_buffer;
    std::queue<packet> network_incoming_buffer;
    bool network_events_enabled;
    Sender();
    void enable_network_layer();
    void disable_network_layer();
    void handle_network_layer_ready();
    void handle_timeout();
    void send_data(seq_nr frame_nr, packet buffer[]);
    void from_network_layer(packet *p);
    void to_network_layer(packet *p);
    void from_physical_layer(frame *r);
    void to_physical_layer(frame *s);
    void received_ack(seq_nr frame_nr);
};

class Receiver : public Node {
public:
    Sender* my_sender;
    std::queue<frame> physical_incoming_buffer;
    void send_ack(seq_nr frame_nr);
    void received_data(frame *r);
    void from_network_layer(packet *p);
    void to_network_layer(packet *p);
    void from_physical_layer(frame *r);
    void to_physical_layer(frame *s);
};

#endif //NODE_H