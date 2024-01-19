#include "Network.h"
#include <sstream>
Network::Network() = default;

void Network::process_commands(vector<Client> &clients, vector<string> &commands, int message_limit,
                      const string &sender_port, const string &receiver_port) {
    // TODO: Execute the commands given as a vector of strings while utilizing the remaining arguments.
    for(const auto& command : commands){

        vector<std::string> command_frags;

        std::cout << "--------------------" << std::endl;
        std::cout << "Command : " << command << std::endl;
        std::cout << "--------------------" << std::endl;
        stringstream ss {command};
        std::string frag;
        while(ss >> frag) {
            command_frags.emplace_back(frag);
        }

        if(command_frags.at(0) == "MESSAGE"){
            Client sender_client;
            Client receiver_client;
            Client next_hop_client;

            std::string sender_ID = command_frags.at(1);
            std::string receiver_ID = command_frags.at(2);
            std::string message;
            //Hold the message
            bool begin_message {false};
            for(char letter : command){
                if(letter == '#'){
                    begin_message = true;
                }
                else if(begin_message){
                    message += letter;
                }
            }

            // Find the necessary clients

            for(auto &client : clients){ //consider the & in case of error
                if(client.client_id == sender_ID){
                    sender_client = client;
                }
                if(client.client_id == receiver_ID){
                    receiver_client = client;
                }
            }
            for(auto &client : clients){
                if(client.client_id == sender_client.routing_table[receiver_client.client_id]){
                    next_hop_client = client;
                }
            }

            // Divide the message to chunks and generate frames for each chunk, add them to queue

            std::vector<std::string> chunks;
            int message_size = (int)message.length();

            for(int i = 0 ; i <= message_size ; i += message_limit){
                std::string chunk = message.substr(i,message_limit);
                if(!chunk.empty()){
                    chunks.emplace_back(chunk);
                }
            }
            int remainder = message_size % message_limit;
            if(remainder == 0){
                if(!message.substr(message_size - remainder, message_size).empty()){
                    chunks.emplace_back(message.substr(message_size - remainder, message_size));
                }

            }
            queue<stack<Packet*>> queue;
            for(const auto &chunk : chunks){
                stack<Packet*> frame = build_frame(sender_client,receiver_client,next_hop_client,chunk,sender_port,receiver_port,message_limit); //frame
                queue.push(frame);
            }
            // Keep the queue inside the sender's outgoing queue
            sender_client.outgoing_queue = queue;
            for(auto &client : clients) { //consider the & in case of error
                if (client.client_id == sender_ID) {
                    client.outgoing_queue = queue;
                }
            }
            // Generate output

            std::cout << "Message to be sent: \"" << message << "\"" << std::endl;
            print_queue(queue);

        }
        else if(command_frags.at(0) == "SHOW_FRAME_INFO"){
            std::string client_ID  = command_frags.at(1);
            std::string queue_selection = command_frags.at(2);
            std::string frame_number_str = command_frags.at(3);
            int frame_number = stoi(frame_number_str);

            // Reach the frame and call the print_layers method
            Client current_client;
            for(const auto& client : clients){
                if(client.client_id == client_ID){
                    current_client = client;
                }
            }


            std::cout << "Current Frame #" << frame_number << " on the ";
            if(queue_selection == "out"){
                std::cout << "outgoing queue of client " << client_ID << std::endl;
                stack<Packet*> current_frame = current_client.outgoing_queue.front();
                if(!current_client.outgoing_queue.empty()){
                    while(frame_number > 1){
                        current_client.outgoing_queue.front().pop(); //crashes here due to empty queue
                        frame_number--;
                    }

                    print_layers(current_frame);
                }
            }
            else if(queue_selection == "in"){
                std::cout << "incoming queue of client " << client_ID << std::endl;
                if(!current_client.incoming_queue.empty()){
                    while(frame_number > 1){
                        current_client.incoming_queue.pop();
                        frame_number--;
                    }
                    stack<Packet*> current_frame = current_client.incoming_queue.front();
                    print_layers(current_frame);
                }
            }


        }
        else if(command_frags.at(0) == "SHOW_Q_INFO"){ //didn't code the invalid frame case
            std::string client_ID = command_frags.at(1);
            std::string queue_selection = command_frags.at(2);
            Client current_client;
            for(const auto& client : clients){
                if(client.client_id == client_ID){
                    current_client = client;
                }
            }
            std::cout << "Client " << client_ID << " ";
            size_t number_of_frames {0};
            queue<stack<Packet*>> curr_queue;
            if(queue_selection == "out"){
                curr_queue = current_client.outgoing_queue ;
                std::cout << "Outgoing Queue Status" << std::endl;
            }
            else if(queue_selection == "in"){
                curr_queue = current_client.incoming_queue;
                std::cout << "Incoming Queue Status" << std::endl;
            }
            while(!curr_queue.empty()){
                number_of_frames++;
                curr_queue.pop();
            }
            std::cout << "Current total number of frames: " << number_of_frames << std::endl;
        }
        else if(command_frags.at(0) == "SEND"){

            for(auto& client: clients){
                if(!client.outgoing_queue.empty()){
                    auto* phy_layer = dynamic_cast<PhysicalLayerPacket*>(client.outgoing_queue.front().top());
                    //get the next hop's client
                    std::string hops_mac = phy_layer->receiver_MAC_address;
                    Client next_hop_client;
                    for(auto& i_client: clients){
                        if(i_client.client_mac == hops_mac){
                            next_hop_client = i_client;
                        }
                    }


                    std::cout << "Client " << client.client_id << " sending frame #1" << " to client " << next_hop_client.client_id << std::endl;

                    print_queue_for_send(client.outgoing_queue);

                    client.outgoing_queue = next_hop_client.incoming_queue;

                    client.empty_outgoing_queue();

                    //INCREMENT NEXT HOP COUNT

                }
            }

        }
        else if(command_frags.at(0) == "RECEIVE"){
            for(auto &client: clients){
                if(!client.incoming_queue.empty()){
                    client.incoming_queue = client.outgoing_queue;
                    client.empty_incoming_queue();
                    
                }
            }
        }
        else if(command_frags.at(0) == "PRINT_LOG"){}
        else{
            std::cout << "Invalid command." << std::endl;
        }

    }

    /* Don't use any static variables, assume this method will be called over and over during testing.
     Don't forget to update the necessary member variables after processing each command. For example,
     after the MESSAGE command, the outgoing queue of the sender must have the expected frames ready to send. */
}

vector<Client> Network::read_clients(const string &filename) {
    vector<Client> clients;
    // TODO: Read clients from the given input file and return a vector of Client instances.
    std::ifstream file {filename};
    std::string line;
    if(!file){
        std::cout << "Error opening file: " << filename << std::endl;
    }
    else{
        while(std::getline(file,line)){
            std::vector<std::string> attributes;
            if(line.length() != 1){
                stringstream ss {line};
                std::string attribute;

                while(ss >> attribute){
                    attributes.emplace_back(attribute);
                }
                Client client(attributes.at(0),attributes.at(1),attributes.at(2));
                clients.emplace_back(client);
            }
        }
    }
    return clients;
}

void Network::read_routing_tables(vector<Client> &clients, const string &filename) {
    // TODO: Read the routing tables from the given input file and populate the clients' routing_table member variable.
    ifstream file {filename};
    if(!file){
        std::cout << "Error opening file: " << filename << std::endl;
    }
    else{
        std::string line;
        unordered_map<string, string> map;
        size_t it = 0;
        while(std::getline(file,line)){
            if(line != "-"){
                std::string l0{line[0]};
                std::string l1{line[2]};
                map.insert({l0,l1});
            }
            else{
                clients.at(it++).routing_table = map;
                map.clear();
            }
        }
        //add the last elements' map
        clients.at(it).routing_table = map;
    }
}

// Returns a list of token lists for each command
vector<string> Network::read_commands(const string &filename) {
    vector<string> commands;
    // TODO: Read commands from the given input file and return them as a vector of strings.

    std::ifstream file {filename};
    if(!file){
        std::cout << "Error opening file: " << filename << std::endl;
    }
    else{
        std::string line;
        while(std::getline(file,line)){
            commands.emplace_back(line);
        }
        commands.erase(commands.begin(),commands.begin() + 1);
    }

    return commands;
}

stack<Packet*> Network::build_frame(Client& sender, Client& receiver,Client& next_hop, const std::string& message_chunk, const string &sender_port, const string &receiver_port,
                          int &message_limit) { //deletion of the packets can be made here
    stack<Packet*> frame;

    Packet* app_layer = new ApplicationLayerPacket(0,sender.client_id,receiver.client_id,message_chunk);
    frame.push(app_layer);

    Packet* tra_layer = new TransportLayerPacket(1,sender_port,receiver_port);
    frame.push(tra_layer);

    Packet* net_layer = new NetworkLayerPacket(2,sender.client_ip,receiver.client_ip);
    frame.push(net_layer);

    Packet* phy_layer = new PhysicalLayerPacket(3,sender.client_mac,next_hop.client_mac); //not the receiver's mac, next hop's mac
    frame.push(phy_layer);

    return frame;
}

void Network::print_queue(queue<stack<Packet*>> queue){ //app layerin print fonksiyonunu düzenle, dynamic cast kullanarak message dataya erişebilirsin
    size_t it = 0;
    while(!queue.empty()){

        std::cout << "Frame #" << ++it << std::endl;

        auto* phy_layer = dynamic_cast<PhysicalLayerPacket *>(queue.front().top());
        phy_layer->print(); //print physical layer
        queue.front().pop(); //pop physical layer

        auto* net_layer = dynamic_cast<NetworkLayerPacket*>(queue.front().top());
        net_layer->print(); //print network layer
        queue.front().pop(); //pop network layer

        auto* tra_layer = dynamic_cast<TransportLayerPacket*>(queue.front().top());
        tra_layer->print(); //print transport layer
        queue.front().pop(); //pop transport layer

        auto* app_layer = dynamic_cast<ApplicationLayerPacket*>(queue.front().top());
        app_layer->print(); //print application layer
        std::cout << "Message chunk caried: \"" << app_layer->message_data << "\"" << std::endl;
        std::cout << "Number of hops so far: 0" << std::endl; //to be configured
        std::cout << "--------" << std::endl;

        queue.pop();
    }

}
void Network::print_layers(stack<Packet*> s){
    stack<Packet*> rev_stack;
    while(!s.empty()){
        rev_stack.push(s.top());
        s.pop();
    }
    auto* app_layer = dynamic_cast<ApplicationLayerPacket*>(rev_stack.top());
    std::cout << "Carried Message: \"" << app_layer->message_data << "\"" << std::endl;
    std::cout << "Layer 0 info: ";
    app_layer->print();
    rev_stack.pop();

    auto* tra_layer = dynamic_cast<TransportLayerPacket*>(rev_stack.top());
    std::cout << "Layer 1 info: ";
    tra_layer->print();
    rev_stack.pop();

    auto* net_layer = dynamic_cast<NetworkLayerPacket*>(rev_stack.top());
    std::cout << "Layer 2 info: ";
    net_layer->print();
    rev_stack.pop();

    auto* phy_layer = dynamic_cast<PhysicalLayerPacket*>(rev_stack.top());
    std::cout << "Layer 3 info: ";
    phy_layer->print();
    rev_stack.pop();

    std::cout << "Number of hops so far: 0" << std::endl;

}

Network::~Network() {
    // TODO: Free any dynamically allocated memory if necessary.
}

void Network::print_queue_for_send(queue<stack<Packet *>> queue) {
    while(!queue.empty()){

        auto* phy_layer = dynamic_cast<PhysicalLayerPacket *>(queue.front().top());
        phy_layer->print(); //print physical layer
        queue.front().pop(); //pop physical layer

        auto* net_layer = dynamic_cast<NetworkLayerPacket*>(queue.front().top());
        net_layer->print(); //print network layer
        queue.front().pop(); //pop network layer

        auto* tra_layer = dynamic_cast<TransportLayerPacket*>(queue.front().top());
        tra_layer->print(); //print transport layer
        queue.front().pop(); //pop transport layer

        auto* app_layer = dynamic_cast<ApplicationLayerPacket*>(queue.front().top());
        app_layer->print(); //print application layer
        std::cout << "Message chunk caried: \"" << app_layer->message_data << "\"" << std::endl;
        std::cout << "Number of hops so far: 0" << std::endl; //to be configured
        std::cout << "--------" << std::endl;

        queue.pop();
    }
}


