#include <iostream>
#include <string>
#include <cstring>
#include "node.h"

Sender sender;
Receiver receiver;

using namespace std;

int main() {
    sender.my_receiver = &receiver;
    receiver.my_sender = &sender;

    //get string from user, add each letter
    //as a separate packet
    string s;
    getline(cin, s);
    for (int i=0; i<s.length(); ++i) {
        packet p;
        char c_string[2] = {0};
        c_string[0] = s[i];
        strcpy((char* )p.data, c_string);
        sender.network_incoming_buffer.push(p);
    }

    while (true) {
        if (!sender.network_incoming_buffer.empty()) {
            sender.event_queue.push(network_layer_ready);
        }
        sender.consume_events();
        receiver.consume_events();
        sender.timer_tick();
        receiver.timer_tick();
    }


    return 0;
}
