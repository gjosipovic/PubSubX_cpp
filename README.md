
                                 ____        __   _____       __   _  __                    
                                / __ \__  __/ /_ / ___/__  __/ /_ | |/ /                    
                               / /_/ / / / / __ \\__ \/ / / / __ \|   /                     
                              / ____/ /_/ / /_/ /__/ / /_/ / /_/ /   |                      
                             /_/    \__,_/_.___/____/\__,_/_.___/_/|_|                      
                                                                              

# Introduction
PubSubX_cpp is a C++ version of PubSubX framework (https://github.com/gjosipovic/PubSubX) for implementation of basic publish subscribe architecture
on TCP/IP layer written in C++. It currently consists of only one module: Client.
Client module is developed using raw sockets.

Client application connects to the server and is used to interact with it and other 
clients (by publishing and receiveing messages). It is platform independent. 
It implements next set of commands
- CONNECT     \<port>  \<name>  - Connects to a server at port, with name 
- DISCONNECT                    - Disconnects from server
- PUBLISH     \<topic> \<data>  - Sends (ASCII) message on a topic 
- SUBSCRIBE   \<topic>          - Client subscribes to a topic
- UNSUBSCRIBE \<topic>          - Client unsubscribes from a topic




---------------------------------------------------------------------------
# Usage

Client must be built from sources by using cmake. After starting, first command that must be entered is CONNECT with appropriate server port number and client name.
```
PubSubX_cpp $ cmake -B build
PubSubX_cpp $ cd build
PubSubX_cpp/build $ make
PubSubX_cpp/build $./PubSubX_cpp
Enter command or (-h): connect 12000 homer
INFO: Connection successfully established
```
Information about all possible commands is given when -h is entered.


---------------------------------------------------------------------------
# Client module
Client module consists of 1 class - client. Client class uses portable select 
mechanism for nonblocking overview of input and output sockets for external 
and inter-thread communication. Client has 2 loops, one for command line interface 
for issuing commands and other for socket monitoring and communication with server. 
These 2 loops communicate via sockets, command loop sends messages over to socket loop.
