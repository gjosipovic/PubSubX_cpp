//******************************************************************************//
//                  ____        __   _____       __   _  __                     //
//                  / __ \__  __/ /_ / ___/__  __/ /_ | |/ /                    //
//                 / /_/ / / / / __ \\__ \/ / / / __ \|   /                     //
//                / ____/ /_/ / /_/ /__/ / /_/ / /_/ /   |                      //
//               /_/    \__,_/_.___/____/\__,_/_.___/_/|_|                      //
//                                                                              //
//******************************************************************************//
// File    : client.h
// Product : PubSubx
// Brief   : CLient implementation of publish subscribe protocol in PubSubX
// Ingroup : PubSubx
// Version : 0.1
// Updated : February 15 2022
//
// Copyright(C) Goran Josipovic.All rights reserved.
//******************************************************************************/


/******************************************************************************/
/************************          INCLUDES           *************************/
/******************************************************************************/

#ifndef PUBSUBX_H
#define PUBSUBX_H


#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex> 
#include <chrono>
#include <fcntl.h>
#include <set>
#include <deque>

using namespace std;

/******************************************************************************/
/***********************          DEFINITIONS          ************************/
/******************************************************************************/
#define MAX_NAME_LEN 64         // Maximum length of client name
#define BUFFER_SIZE 1024        // Size of the single receive / send buffer
#define MAX_MESSAGE_SIZE ((10)*(BUFFER_SIZE))
#define EOM "\n\nx"             // End of message string

const vector<string> commands = { "-H", "CONNECT", "DISCONNECT", "PUBLISH", "SUBSCRIBE", "UNSUBSCRIBE" };


/******************************************************************************/
/**********************          CLIENT CLASS           ***********************/
/******************************************************************************/
class Client {

public:
    // Creator 
    Client(string server_name);

    // Main command loop 
    void command_loop(void);


    /******************************************************************************/
    /********************          CLIENT ATTRIBUTES          *********************/
    /******************************************************************************/
private:

    // Basic server data 
    string m_server_name;           // Server host to connet to
    int    m_server_port;           // Server port number to connect to
    int    m_server_socket;         // Server socket file descriptor
    struct sockaddr_in m_server_addr;// Server address strucutre 

    // Command data
    string m_command;               // Input command
    string m_arg1;                  // Input argument 1  
    string m_arg2;                  // Input argument 2

    // Inter-thread communication sockets
    int    m_listen_sock;           // Listening socket for communication establishment
    int    m_listen_port;           // Local listening port number

    int    m_msg_in_sock;           // Input end of the message sockets, on command loop side
    int    m_close_in_sock;         // Input end of close signal socket, on command loop side

    int    m_msg_out_sock;          // Output end of the message sockets, on socket loop side
    int    m_close_out_sock;        // Output end of close signal socket, on socket loop side

    // Socket thread data
    thread m_socket_thread;         // Thread class          
    mutex  m_mutex;                 // Mutex to avoid race conditions between socket and command loop

    // Connection flag
    bool   m_connected;

    // Client name and topics/messages attributes
    string        m_name;            // Name of the client
    set<string>   m_topics;          // Set of subscribed topics
    deque<string> m_out_messages;    // Queue of ooutgoing messages
    string        m_receive_stream;  // Input receive stream


/******************************************************************************/
/********************          CLIENT OPERATIONS          *********************/
/******************************************************************************/

    // Print functions
    void print_help(void);
    void print_error(uint16_t errnum, string msg = "");
    void print_info(uint16_t infonum, string msg = "");

    // Command functions    
    bool command_parse(string input);
    void command_process(void);
    void command_disconnect(void);
    void command_publish(void);
    void command_subscribe(void);
    void command_unsubscribe(void);


    // Connection establishment functions
    bool connect_args_check(void);
    void connect_server(void);
    void connect_accept(void);
    void connect_restore(char* str);

    // Socket functions 
    void socket_server_init(void);      // Initialize main server socket
    void socket_listen_init(void);      // Initialize listening socket for inter thread communication
    void socket_inter_init(void);       // Initialize inter thread sockets 

    void socket_command_msg(void);      // Message sent from command loop to socket loop
    void socket_close_msg(void);        // Close message sent from command loop to socket loop
    void socket_server_msg(void);       // Message sent from server
    bool socket_write(void);            // Write message to server, returns true if last message is sent

    void socket_loop(void);             // Main socket loop function

    // IO messages functions
    void process_message_chunk(char* msg_chunk, int size, bool from_restore);
    void print_received_message(string msg);
    string get_send_chunk(bool* last);

};

#endif