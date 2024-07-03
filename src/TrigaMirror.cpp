/*
TrigaMirror is a software for GNU operating system to get the flux
data of TrigaServer share in network.
Copyright (C) 2024 Thalles Campagnani

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
//TrigaMirror.cpp
#include "TrigaMirror.h"

TrigaMirror::TrigaMirror(std::string ip, int port)
{
    std::thread readFromServerThread   (&TrigaMirror::readFromServer, this, ip, port);
    readFromServerThread.detach();
}

TrigaMirror::~TrigaMirror() {}

void TrigaMirror::createMirror(int port) {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "[startServer] Error opening socket" << std::endl;
        return;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[startServer] Error on binding" << std::endl;
        return;
    }

    listen(serverSocket, 5);
    //std::cout << "[startServer] Server started on port " << port << std::endl;

    while(true) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "[startServer] Error on accept" << std::endl;
            continue;
        }

        //std::cout << "[startServer] Client connected" << std::endl;
        
        std::thread clientThread(&TrigaMirror::handleTCPClients, this, clientSocket);
        clientThread.detach();
    }
}

void TrigaMirror::handleTCPClients(int clientSocket)
{
    int timeout = 2; // 2 seconds timeout
    char buffer[1024];
    int n = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (n <= 0)
    {
        std::cerr << "[handleTCPClients] Error receiving data" << std::endl;
        close(clientSocket);
        return;
    }

    // Parse received data (assuming it's a number)
    int interval = std::stoi(std::string(buffer, n));

    //std::cout << "[handleTCPClients] Received interval: " << interval << "ms" << std::endl;
    // Create new thread
    std::thread([this, interval, clientSocket]()
    {
        while(true)
        {
            std::string data = *data_global.load();
            if(send(clientSocket, data.c_str(), sizeof(data), 0) <= 0) break;

            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }).detach();
}

// Ler dados do servidor
void TrigaMirror::readFromServer(std::string ip, int port) 
{
    std::cout << "ENTREI!\n" << ip << "\n" << port << "\n";
    int clientSocket;
    struct sockaddr_in serverAddr;
    // Configurar endereço do servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    char c;
    auto line = std::shared_ptr <std::string> (new std::string);

    // Loop eterno para tentar conectar o tempo todo
    while (true)
    {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) std::cerr << "Erro ao criar socket.\n";

        // Conectar ao servidor
        std::cout << "Tentando conectar!\n";
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Erro ao conectar ao servidor.\n";
        }
        std::cout << "Conectado!\n";

        // Enviar taxa de amostragem
        const char tax[6] = "10000";
        send(clientSocket, tax, sizeof(tax), 0);

        //Loop eterno para, caso esteja conectado, ler o tempo todo
        while (true)
        {
            //Loop para ler caractere por caractere até encontrar o fim de linha
            c = ' ';
            *line="";
            while (c != 10)//'\n')
            {
                if(recv(clientSocket, &c, 1, 0) < 1)
                {
                    std::cout << "Erro: Nada recebido\n";
                    *line="";
                    break;
                }
                //Ler 1 caractere e armazenar na variavel c
                std::cout << "Caractere recebido: " << c << " (ASCII: " << static_cast<int>(c) << ")\n";
                *line += c;
            }
            data_global.store(line);
            std::cout << line;
        }
    }
}