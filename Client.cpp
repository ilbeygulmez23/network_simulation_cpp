//
// Created by alperen on 27.09.2023.
//

#include "Client.h"

Client::Client(string const& _id, string const& _ip, string const& _mac) {
    client_id = _id;
    client_ip = _ip;
    client_mac = _mac;
}

ostream &operator<<(ostream &os, const Client &client) {
    os << "client_id: " << client.client_id << " client_ip: " << client.client_ip << " client_mac: "
       << client.client_mac << endl;
    return os;
}

Client::~Client() {
    // TODO: Free any dynamically allocated memory if necessary.
}

void Client::empty_outgoing_queue() {
    while(!outgoing_queue.empty()){
        outgoing_queue.pop();
    }
}

void Client::empty_incoming_queue() {
    while(!incoming_queue.empty()){
        incoming_queue.pop();
    }
}
