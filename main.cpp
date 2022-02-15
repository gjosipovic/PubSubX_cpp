//******************************************************************************#
//                  ____        __   _____       __   _  __                    #
//                  / __ \__  __/ /_ / ___/__  __/ /_ | |/ /                    #
//                 / /_/ / / / / __ \\__ \/ / / / __ \|   /                     #
//                / ____/ /_/ / /_/ /__/ / /_/ / /_/ /   |                      #
//               /_/    \__,_/_.___/____/\__,_/_.___/_/|_|                      #
//                                                                              #
//******************************************************************************#
// File    : main.cpp
// Product : PubSubx
// Brief   : Client implementation of publish subscribe protocol in PubSubX
// Ingroup : PubSubx
// Version : 0.1
// Updated : February 15 2022
//
// Copyright(C) Goran Josipovic.All rights reserved.
//******************************************************************************/

#include "Client.hpp"

int main(int argc, char* argv[])
{

    Client client("localhost");
    client.command_loop();

    return 0;
}