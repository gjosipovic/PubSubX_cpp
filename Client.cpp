//******************************************************************************#
//                  ____        __   _____       __   _  __                    #
//                  / __ \__  __/ /_ / ___/__  __/ /_ | |/ /                    #
//                 / /_/ / / / / __ \\__ \/ / / / __ \|   /                     #
//                / ____/ /_/ / /_/ /__/ / /_/ / /_/ /   |                      #
//               /_/    \__,_/_.___/____/\__,_/_.___/_/|_|                      #
//                                                                              #
//******************************************************************************#
// File    : client.cpp
// Product : PubSubx
// Brief   : Client implementation of publish subscribe protocol in PubSubX
// Ingroup : PubSubx
// Version : 0.1
// Updated : February 15 2022
//
// Copyright(C) Goran Josipovic.All rights reserved.
//******************************************************************************/

#include "Client.hpp"


/******************************************************************************/
/********************          HELPER FUNCTIONS          **********************/
/******************************************************************************/
/* Function to split vector by delimiter into a vector of strings*/
vector<string> split(const string& s, const string& delim, const bool keep_empty = false) {
    vector<string> result;
    if (delim.empty()) {
        result.push_back(s);
        return result;
    }
    string::const_iterator substart = s.begin(), subend;
    while (true) {
        subend = search(substart, s.end(), delim.begin(), delim.end());
        string temp(substart, subend);
        if (keep_empty || !temp.empty()) {
            result.push_back(temp);
        }
        if (subend == s.end()) {
            break;
        }
        substart = subend + delim.size();
    }
    return result;
}
string toUpper(string* input) {
    std::for_each(input->begin(), input->end(), [](char& c) {
        c = ::toupper(c);
        });
}

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}



/******************************************************************************/
/*********************          ERROR FUNCTIONS          **********************/
/******************************************************************************/
enum errors_enum {
    INIT_FAIL, WRONG_PORT, WRONG_NAME, NAME_TAKEN, CONN_FAIL, SEL_FAIL,
    MSG_TOO_LONG, CONN_LOST, CONN_DOWN, NOT_CONN, WRONG_TOPIC,
    EMPTY_TOPIC, WRONG_CMD, NO_RSP, UNKNOWN_RSP, EXCEPTION, MAX_ERRORS
};

static string errors[] = {
    [INIT_FAIL] = "Initialization of local sockets has failed",
    [WRONG_PORT] = "Server port number is wrong, must be integer in range 1024 < port < 32000",
    [WRONG_NAME] = "Client name is empty/too long, must be between 1 and 64 characters",
    [NAME_TAKEN] = "Client name is alreaday taken, please enter other name",
    [CONN_FAIL] = "Connection to the server has failed, please check port and try again",
    [SEL_FAIL] = "Select function has failed",
    [MSG_TOO_LONG] = "Received/(trying to send) message that is too long",
    [CONN_LOST] = "Client lost connection to the server try to reconnect ",
    [CONN_DOWN] = "Server shut the connection, all subscriptions are lost",
    [NOT_CONN] = "Client is not connected, only CONNECT command is accepted ",
    [WRONG_TOPIC] = "Client received message on a topic he is not subscribed to ",
    [EMPTY_TOPIC] = "Trying to publish/subscribe/unsubscribe to an empty topic",
    [WRONG_CMD] = "Wrong command is entered, to see help enter -h",
    [NO_RSP] = "No response from server: ",
    [UNKNOWN_RSP] = "Unknown response from server: ",
    [EXCEPTION] = "Exception occured: "
};

enum infos_enum {
    CONN_ACC, ALR_CONN, ALR_SUB, NOT_SUB, CONN_RESTORED, MAX_INFOS
};

static string infos[] = {
    [CONN_ACC] = "Connection sucessfully established",
    [ALR_CONN] = "Already connected to server, first disconnect",
    [ALR_SUB] = "Already subscribed to topic:",
    [NOT_SUB] = "Was not subscribed to topic:",
    [CONN_RESTORED] = "Connection restored",
};

void Client::print_help(void) {
    cout << "client - list of possible client commands:\n";
    cout << "CONNECT <port> <client_name>    : connect to PubSubX server at specified port with client name\n";
    cout << "DISCONNECT                      : disconect from to PubSubX server, all subscriptions will be removed\n";
    cout << "PUBLISH <topic_name> <message>  : publish message to topic on PubSubX server\n";
    cout << "SUBSCRIBE <topic>               : subscribe client to a topic on a PubSubX server\n";
    cout << "UNSUBSCRIBE <topic_name>        : remove subscription from a topic on PubSubX server\n";
}

void Client::print_error(uint16_t errnum, string msg) {
    assert(errnum < MAX_ERRORS&& errors[errnum] != "");
    cout << "ERROR: " + errors[errnum] + msg + "\n";
    cout.flush();
}


void Client::print_info(uint16_t infonum, string msg) {
    assert(infonum < MAX_INFOS&& infos[infonum] != "");
    cout << "INFO: " + infos[infonum] + msg + "\n";
    cout.flush();
}


/******************************************************************************/
/*************************          CREATOR          **************************/
/******************************************************************************/
Client::Client(string server_name)
    :m_server_name(server_name)
{
    /* Initialize all the local sockets */
    //socket_server_init();
    socket_listen_init();
    socket_inter_init();
}



/******************************************************************************/
/********************          CONNECT FUNCTIONS          *********************/
/******************************************************************************/
bool Client::connect_args_check() {

    // Check if first argument-> port is adequate number
    if (!(m_arg1.find_first_not_of("0123456789") == string::npos)) {
        print_error(WRONG_PORT);
        return false;
    }

    // Check if port is in adequater range
    int l_port = stoi(m_arg1);
    if (l_port < 1024 || l_port > 65535) {
        print_error(WRONG_PORT);
        return false;
    }

    // Check if second argument-> name is adequate
    if (m_arg2 == "" || (m_arg2.length() > MAX_NAME_LEN)) {
        print_error(WRONG_NAME);
        return false;
    }

    return true;
}

void Client::connect_server() {

    // Before any other steps check input arguments
    if (!connect_args_check()) { return; }

    socket_server_init();

    // Update connection address structure
    m_server_addr.sin_port = htons(stoi(m_arg1));

    // Try to establish connection
    if (connect(m_server_socket, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr)) < 0) {
        print_error(CONN_FAIL);
        return;
    }

    // Send and receive message
    int l_valread;
    char l_buffer[BUFFER_SIZE] = { 0 };
    static string l_conn_msg;
    l_conn_msg = "CONNECT " + m_arg2 + EOM;

    // Send connection message
    if (send(m_server_socket, l_conn_msg.c_str(), l_conn_msg.length(), 0) != l_conn_msg.length()) {
        print_error(CONN_FAIL);
        shutdown(m_server_socket, SHUT_RDWR);
        close(m_server_socket);
        return;
    }

    // Blocking read of response message
    l_valread = read(m_server_socket, l_buffer, BUFFER_SIZE);
    if (l_valread == 0 || l_valread == -1) {
        print_error(CONN_FAIL);
        shutdown(m_server_socket, SHUT_RDWR);
        close(m_server_socket);
        return;
    }

    // Connection established
    if (strncmp(l_buffer, "OK", strlen("OK")) == 0) {
        connect_accept();
        return;
    }

    // Connection reestablished
    if (strncmp(l_buffer, "RESTORED", strlen("RESTORED")) == 0) {
        connect_restore(l_buffer);
        return;
    }

    // Name already taken
    if (strncmp(l_buffer, "ERROR", strlen("ERROR")) == 0) {
        print_error(NAME_TAKEN);
        shutdown(m_server_socket, SHUT_RDWR);
        close(m_server_socket);
        return;
    }
    // Unknown error
    else {
        print_error(UNKNOWN_RSP);
        shutdown(m_server_socket, SHUT_RDWR);
        close(m_server_socket);
        return;
    }
}

void Client::connect_accept(void) {

    print_info(CONN_ACC);

    // Update atributes
    m_server_port = stoi(m_arg1);
    m_name = m_arg2;
    m_connected = true;

    // Initi receive stream
    m_receive_stream = "";

    // Start the socket thread    
    m_socket_thread = thread(&Client::socket_loop, this);
    m_socket_thread.detach();
}

void Client::connect_restore(char* str) {

    print_info(CONN_RESTORED);

    // Update atributes
    m_server_port = stoi(m_arg1);
    m_name = m_arg2;
    m_connected = true;

    // Initi receive stream
    m_receive_stream = "";

    string l_message = str;

    vector<string> l_msg_list = split(l_message, EOM);
    // Second message has subscribed topics
    if (l_msg_list.size() > 1) {
        vector<string> l_topics_list = split(l_msg_list[1], " ");
        int i;
        for (i = 0; i < l_topics_list.size(); i++) {
            m_topics.insert(l_topics_list[i]);
        }
    }

    // All the other messages are missed messages on subscribed topics
    if (l_msg_list.size() > 2) {
        int l_prev_msg_size;
        // Remove first 2 messages from string l_message
        l_prev_msg_size = l_msg_list[0].length() + l_msg_list[1].length() + 2 * strlen(EOM);
        l_message.erase(0, l_prev_msg_size);
        // Send rest as normal message stream
        process_message_chunk((char*)l_message.c_str(), l_message.size(), true);
    }


    // Start the socket thread
    m_socket_thread = thread(&Client::socket_loop, this);
    m_socket_thread.detach();
}

/******************************************************************************/
/********************          SOCKET FUNCTIONS          **********************/
/******************************************************************************/
void Client::socket_server_init(void) {

    // Server socket and address setup
    if ((m_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error(INIT_FAIL);
        exit(0);
    }

    // Currently only localhost support
    if (inet_pton(AF_INET, "127.0.0.1", &m_server_addr.sin_addr) <= 0) {
        print_error(INIT_FAIL);
        exit(0);
    }
    m_server_addr.sin_family = AF_INET;
}


void Client::socket_listen_init(void) {

    // Setup listening port for inter thread communication
    struct sockaddr_in l_listen_addr;

    l_listen_addr.sin_family = AF_INET;
    l_listen_addr.sin_addr.s_addr = INADDR_ANY;

    int i, l_optval = 1;

    // Search for empty port and try to bind socket to it
    for (i = 1024; i < 65535; i++) {

        if ((m_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            print_error(INIT_FAIL);
            exit(0);
        }

        if (setsockopt(m_listen_sock, SOL_SOCKET, SO_REUSEADDR, &l_optval, sizeof(l_optval)) == -1) {
            print_error(INIT_FAIL);
            exit(0);
        }
        l_listen_addr.sin_port = htons(i);

        if (bind(m_listen_sock, (struct sockaddr*)&l_listen_addr, sizeof(l_listen_addr)) == 0) {
            m_listen_port = i;
            break; // Success
        }

        // Failed to bind port, try again
        close(m_listen_sock);
    }

    if (i == 65535) {
        print_error(INIT_FAIL);
        exit(0);
    }

    // Start listening
    listen(m_listen_sock, 5);
}

void Client::socket_inter_init(void) {

    struct sockaddr_in l_conn_addr;
    struct sockaddr_storage l_claddr;
    int addrlen = sizeof(struct sockaddr_storage);

    // Create output sockets
    if ((m_msg_out_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        print_error(INIT_FAIL);
        exit(0);
    }
    if ((m_close_out_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        print_error(INIT_FAIL);
        exit(0);
    }

    // Local address setup
    l_conn_addr.sin_family = AF_INET;
    l_conn_addr.sin_port = htons(m_listen_port);
    if (inet_pton(AF_INET, "127.0.0.1", &l_conn_addr.sin_addr) <= 0) {
        print_error(INIT_FAIL);
        exit(0);
    }

    // Set ouput sockets as nonblocking
    fcntl(m_msg_out_sock, F_SETFL,  fcntl(m_listen_sock, F_GETFL, 0) | O_NONBLOCK);
    fcntl(m_close_out_sock, F_SETFL, fcntl(m_listen_sock, F_GETFL, 0) | O_NONBLOCK);

    // Creating connections
    connect(m_msg_out_sock, (struct sockaddr*)&l_conn_addr, sizeof(l_conn_addr));
    if ((m_msg_in_sock = accept(m_listen_sock, NULL, NULL)) == 0) {
        print_error(INIT_FAIL);
        exit(0);
    }

    connect(m_close_out_sock, (struct sockaddr*)&l_conn_addr, sizeof(l_conn_addr));
    if ((m_close_in_sock = accept(m_listen_sock, NULL, NULL)) == 0) {
        print_error(INIT_FAIL);
        exit(0);
    }

    // Close listening sockets
    close(m_listen_sock);
}

void Client::socket_command_msg(void) {

    static char l_buffer[MAX_MESSAGE_SIZE] = { 0 };
    int l_recvcount;
    l_recvcount = recv(m_msg_out_sock, l_buffer, MAX_MESSAGE_SIZE, 0);
    l_buffer[l_recvcount] = '\0';

    m_out_messages.push_back(l_buffer);
}

void Client::socket_close_msg(void) {

    // First read input message to clear up the buffer
    static char l_buffer[BUFFER_SIZE];
    recv(m_close_out_sock, l_buffer, BUFFER_SIZE, 0);

    // Then send the reply message to notify server of disconnect
    strcpy(l_buffer, "DISCONNECT");
    strcat(l_buffer, EOM);
    send(m_server_socket, l_buffer, strlen(l_buffer), 0);

    // Then shutdown the socket
    shutdown(m_server_socket, SHUT_RDWR);
    close(m_server_socket);
}

void Client::socket_server_msg(void) {

    // Read messge from socket
    static char l_buffer[BUFFER_SIZE];
    int l_size;
    l_size = recv(m_server_socket, l_buffer, BUFFER_SIZE, 0);
    l_buffer[l_size] = 0;

    // If buffer is empty connection is down
    if (l_buffer[0] == '\0') {
        cout << "\n";
        print_error(CONN_DOWN);
        shutdown(m_server_socket, SHUT_RDWR);
        close(m_server_socket);
    }
    else {
        process_message_chunk(l_buffer, l_size, false);
    }
}


bool Client::socket_write(void) {

    static bool l_last;
    static string l_chunk;

    l_chunk = get_send_chunk(&l_last);

    send(m_server_socket, l_chunk.c_str(), strlen(l_chunk.c_str()), 0);

    return l_last;

}

void Client::socket_loop(void) {

    fd_set l_readfds, l_writefds, l_errorfds;
    // Maximum file descriptor number
    int l_nfds = max({ m_server_socket, m_msg_out_sock, m_close_out_sock });
    int l_ready;

    // Set file descriptors sets
    FD_ZERO(&l_readfds);
    FD_ZERO(&l_writefds);
    FD_ZERO(&l_errorfds);

    FD_SET(m_server_socket,  &l_readfds);
    FD_SET(m_msg_out_sock,   &l_readfds);
    FD_SET(m_close_out_sock, &l_readfds);

    FD_SET(m_server_socket, &l_errorfds);


    while (1) {

        // Blocking wait untill some fd becomes available
        m_mutex.unlock();
        l_ready = select(l_nfds + 1, &l_readfds, &l_writefds, &l_errorfds, NULL);
        m_mutex.lock();

        // Connection down
        if (l_ready == -1) {
            m_connected = false;
            
            cout << "Enter command or (-h):";
            cout.flush();
            m_mutex.unlock();
            return;
        }

        // Error on server socket
        if (FD_ISSET(m_server_socket, &l_errorfds)) {
            print_error(CONN_LOST);
            close(m_server_socket);
            m_connected = false;
            m_mutex.unlock();       // Don't forget to unlock the mutex
            return;
        }

        // Input message from server
        if (FD_ISSET(m_server_socket, &l_readfds)) {
            socket_server_msg();
        }

        // Input message from command loop
        if (FD_ISSET(m_msg_out_sock, &l_readfds)) {
            // Read the message and set the send set
            socket_command_msg();
            // Arm write fd set
            FD_SET(m_server_socket, &l_writefds);
        }

        // Close (disconnect commands) message from command loop
        if (FD_ISSET(m_close_out_sock, &l_readfds)) {
            socket_close_msg();
            m_connected = false;
            m_mutex.unlock();
            return;            
        }

        // Server ready to receive message
        if (FD_ISSET(m_server_socket, &l_writefds)) {
            // Returns true if last written, disarm write set if last
            if (socket_write()) {
                FD_ZERO(&l_writefds);
            }
        }

        // Reset basic fd sets
        FD_SET(m_server_socket, &l_readfds);
        FD_SET(m_msg_out_sock, &l_readfds);
        FD_SET(m_close_out_sock, &l_readfds);
        FD_SET(m_server_socket, &l_errorfds);
    }
}


/******************************************************************************/
/****************           I/O PROCESSING FUNCTIONS          *****************/
/******************************************************************************/
void Client::process_message_chunk(char* msg_chunk, int size, bool from_restore) {


    static string l_msg_chunk;
    l_msg_chunk = msg_chunk;
    m_receive_stream += string(msg_chunk);



    if (m_receive_stream.find(EOM)) {

        //cout << m_receive_stream;
        // Split the stream into a list of messages
        vector<string> l_msg_list;
        l_msg_list = split(m_receive_stream, EOM);
        int l_last_msg = l_msg_list.size() - 1;

        // If stream ends with EOM only entire messages are in list
        if (ends_with(m_receive_stream, EOM)) {
            // << "Skinuo sve";
            m_receive_stream = "";
        }
        // If not it contains parts of future messages
        else {
            m_receive_stream = l_msg_list[l_last_msg];
            l_msg_list.pop_back();
            l_last_msg--;
        }

        // If not called from restore print new line
        if (!from_restore) {
            cout << "\n";
        }

        // Print all the messages
        int i;
        for (i = 0; i <= l_last_msg; i++) {
            if (l_msg_list[i] == "") { continue; };
            print_received_message(l_msg_list[i]);
        }

        // Print prompt
        if (!from_restore) {
            cout << "Enter command or (-h): ";
            cout.flush();
        }
    }

}

void Client::print_received_message(string msg) {
    // Split message into words
    vector<string> word_list;
    static string topic, data;
    data = "";

    word_list = split(msg, " ");
    topic = word_list[0];

    // Parse topic and data 
    if (word_list.size() > 1) {
        data = word_list[1];
    }
    //cout << "Topic:";
    //cout << topic + "\n";
    // If topic in list of subscribed topics print topic name and data
    if (m_topics.find(topic) != m_topics.end()) {
        cout << "Topic: " + topic + " Data: " + data + "\n";
        cout.flush();
    }
    else {
        print_error(WRONG_TOPIC);
    }

}

string Client::get_send_chunk(bool* last) {

    int l_maxsz = BUFFER_SIZE - strlen(EOM);

    static string l_chunk;
    *last = false;

    if (m_out_messages[0].size() > l_maxsz) {
        l_chunk = m_out_messages[0].substr(0, l_maxsz);
        m_out_messages[0].erase(0, l_maxsz);
    }
    else {
        l_chunk = m_out_messages[0];
        m_out_messages.pop_front();
        if (m_out_messages.size() == 0) {
            *last = true;
        }
    }
    l_chunk += EOM;

    return l_chunk;
}




/******************************************************************************/
/*******************          COMMANDS FUNCTIONS          ********************/
/******************************************************************************/
bool Client::command_parse(string input) {
    assert(input != "");

    vector<string> list;

    list = split(input, " ");
    m_command = list[0];
    toUpper(&m_command);

    // Check if command is in commands vector
    if (find(commands.begin(), commands.end(), m_command) == commands.end()) {
        return false;
    }

    // Parse arguments if they exist
    if (list.size() >= 2) {
        m_arg1 = list[1];
    }
    else {
        m_arg1 = "";
    }

    if (list.size() >= 3) {
        m_arg2 = list[2];
    }
    else {
        m_arg2 = "";
    }

    return true;

}

void Client::command_process(void) {

    if (m_command == "DISCONNECT") {
        command_disconnect();
    }
    else if (m_command == "PUBLISH") {
        command_publish();
    }
    else if (m_command == "SUBSCRIBE") {
        command_subscribe();
    }
    else if (m_command == "UNSUBSCRIBE") {
        command_unsubscribe();
    }
    else {
        cout << "Error in command process";
        assert(0);
    }
}

void Client::command_disconnect(void) {

    // Send message to socket loop
    static string l_message;
    l_message = "DISCONNECT";

    int snt = send(m_close_in_sock, l_message.c_str(), l_message.size(), 0);

    // Delete subscribed topics
    m_topics.clear();

}

void Client::command_publish(void) {

    if (m_arg1 == "") {
        print_error(EMPTY_TOPIC);
        return;
    }

    static string l_message;
    l_message = "PUBLISH " + m_arg1 + " " + m_arg2;

    send(m_msg_in_sock, l_message.c_str(), l_message.size(), 0);

}

void Client::command_subscribe(void) {

    if (m_arg1 == "") {
        print_error(EMPTY_TOPIC);
        return;
    }
    static string l_message;

    // Check that not already subscribed
    if (m_topics.find(m_arg1) == m_topics.end()) {

        m_topics.insert(m_arg1);

        l_message = "SUBSCRIBE " + m_arg1;
        send(m_msg_in_sock, l_message.c_str(), l_message.size(), 0);
    }
    else {
        print_info(ALR_SUB, m_arg1);
    }


}

void Client::command_unsubscribe(void) {
    if (m_arg1 == "") {
        print_error(EMPTY_TOPIC);
        return;
    }
    static string l_message;

    // Check that already subscribed
    if (m_topics.find(m_arg1) != m_topics.end()) {
        m_topics.erase(m_arg1);
        l_message = "UNSUBSCRIBE " + m_arg1;
        send(m_msg_in_sock, l_message.c_str(), l_message.size(), 0);
    }
    else {
        print_info(NOT_SUB, m_arg1);
    }
}



void Client::command_loop(void) {

    string input;
    string response;

    // The main loop
    while (1) {

        cout << "Enter command or (-h): ";

        // Waiting for user input so mutex is locked
        m_mutex.unlock();
        getline(std::cin, input);
        m_mutex.lock();

        if (input == "\n") {
            continue;
        }

        if (!command_parse(input)) {
            print_error(WRONG_CMD);
            continue;
        }

        if (m_command == "-H") {
            print_help();
            continue;
        }

        if (!m_connected) {
            if (m_command == "CONNECT") {
                connect_server();
            }
            else {
                print_error(NOT_CONN);
            }
        }
        else {
            if (m_command == "CONNECT") {
                print_info(ALR_CONN);
            }
            else {
                command_process();
            }
        }
    }
}