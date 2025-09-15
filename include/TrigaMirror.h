/*
TrigaMirror is a software for GNU operating system to get the flux
data of TrigaServer share in network.
Copyright (C) 2024-2025 Thalles Campagnani

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//TrigaServer.h
#ifndef TRIGA_SERVER
#define TRIGA_SERVER

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include <json/json.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <crow.h>

//#define TestMax

using boost::asio::ip::tcp;


class TrigaMirror
{
    public:
        TrigaMirror(std::string ip, 
                    int port, 
                    int read_tax, 
                    std::string logPath, 
                    std::string privKeyPath);
        ~TrigaMirror();

        //Função que cria servidor TCP
        void createMirror(int port);

    private:
        //Save CSV header
        std::string dataHeader = "";
        std::string dataHeaderSing = "";
        int dataHeaderNumPkgs = 0;
        
        //Log
        std::string logPath;

        //Kind
        int kind;
        int qtdBytes;

        //Ponteiros inteligentes globais
        std::atomic<std::shared_ptr<std::string>> data_global           = std::make_shared<std::string>();
        std::atomic<std::shared_ptr<std::string>> data_global_sing      = std::make_shared<std::string>();
        std::atomic<std::shared_ptr<int>>         data_global_numPkgs   = std::make_shared<int>();

        void logConnection(struct sockaddr_in clientAddr, bool sucesses, int taxAmo, std::string ip_client_http);

        //Função que lida com os clientes (recebe o valor de intervalo e cria uma thread para cada cliente)
        void handleTCPClients(int clientSocket, struct sockaddr_in clientAddr);

        int parser(const char *buffer, int *kind, int *interval, std::string *ip_client_http);

        //Threads de leitura de hardware
        void readFromServer(std::string ip, int port, int read_tax);

        std::string readLine(int clientSocket);

        //Criptografia
        std::string privKeyPath;
        std::string signMessage(const std::string message, int* numPkgs);
    };

#endif
