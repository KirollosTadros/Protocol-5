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

    //get string from user, where each letter will become
    //a single packet
    cout << "Note: sliding window size is " << MAX_SEQ << endl;
    cout << "Enter data to send (each character is a packet): ";
    string s;
    getline(cin, s);
    if (!s.length()) {
        s = "ABCDEF";
    }
    cout << "Choose one of the following:" << endl;
    cout << "1. Drop a specific frame" << endl;
    cout << "2. Delay receiving frames" << endl;
    cout << "3. Continue" << endl;
    int choice = -1;
    cin >> choice;
    int delay_duration = -1;
    switch (choice) {
        case 1:
            cout << "Enter frame number to drop (starting from 0): ";
            cin >> choice;
            if (choice < 0 || choice >= s.length()) {
                cout << "Error: frame number out of bounds" << endl;
                return 1;
            }
            receiver.drop_frame = choice;
        break;
        case 2:
            delay_duration = 15000;
        break;
        case 3:
            //do nothing
        break;
        default:
            cout << "Invalid choice" << endl;
            return 1;
    }

    //add each letter from the string as a single packet
    //in the sender's network layer
    for (size_t i=0; i<s.length(); ++i) {
        packet p;
        char c_string[2] = {0};
        c_string[0] = s[i];
        strcpy((char* )p.data, c_string);
        sender.network_incoming_buffer.push(p);
    }

    //Main loop
    bool running = true;
    while (running) {
        //generate network layer events if network layer is not empty, and also
        //if sender has not disabled network events
        if (!sender.network_incoming_buffer.empty()) {
            if (sender.network_events_enabled) {
                sender.event_queue.push(network_layer_ready);
            }
        } else {
            //no more data to send, check if sender
            //has no buffered data
            if (sender.nbuffered == 0) {
                //this is the last iteration
                running = false;
            }
        }
        sender.consume_events();
        if (delay_duration-- <= -1) {
            receiver.consume_events();
        }
        //simulate time passing
        sender.timer_tick();
        receiver.timer_tick();
    }

    return 0;
}
