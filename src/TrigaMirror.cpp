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
//TrigaMirror.cpp
#include "TrigaMirror.h"

TrigaMirror::TrigaMirror(std::string ip, 
                        int port, 
                        int read_tax, 
                        std::string logPath, 
                        std::string privKeyPath)
{
    this->privKeyPath = privKeyPath;
    this->logPath = logPath;
    
    std::thread readFromServerThread   (&TrigaMirror::readFromServer, this, ip, port, read_tax);
    readFromServerThread.detach();
}

TrigaMirror::~TrigaMirror() {}

/*
####################################
## Funções de leitura do servidor ##
####################################
*/


// Ler dados do servidor
void TrigaMirror::readFromServer(std::string ip, int port, int read_tax) 
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    // Configurar endereço do servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // Loop eterno para tentar conectar o tempo todo
    while (true)
    {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            std::cerr << "Erro ao criar socket.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        // Conectar ao servidor
        //std::cout << "Tentando conectar!\n";
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Erro ao conectar ao servidor.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            close(clientSocket);
            continue;
        }
        //std::cout << "Conectado!\n";

        // Enviar taxa de amostragem
        if(!send(clientSocket, std::to_string(read_tax).c_str(), std::to_string(read_tax).length(), 0))
        {
            std::cerr << "Erro ao enviar mensagem para servidor.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            close(clientSocket);
            continue;
        }

        //Salvar header e cópia assinada
        while (dataHeader == "") dataHeader = readLine(clientSocket);
        dataHeaderSing = signMessage(dataHeader, &dataHeaderNumPkgs);


        auto line = std::shared_ptr <std::string> (new std::string);
        auto lineSing = std::shared_ptr <std::string> (new std::string);
        auto numPkgs = std::shared_ptr <int> (new int);

        //Loop eterno para, caso esteja conectado, ler o tempo todo
        while (true)
        {
            //Ler linha do servidor
            *line = readLine(clientSocket);

            //Cria um cópia da linha assinada
            *lineSing = signMessage(*line, numPkgs.get());

            //Salva dados nas variáveis globais
            data_global.store(line);
            data_global_sing.store(lineSing);
            data_global_numPkgs.store(numPkgs);
        }
    }
}

std::string TrigaMirror::readLine(int clientSocket)
{
    //Loop para ler caractere por caractere até encontrar o fim de linha
    char c = ' ';
    std::string line="";
    while (c != 10)//'\n')
    {
        if(recv(clientSocket, &c, 1, 0) < 1)
        {
            std::cerr << "Erro: Nada recebido\n";
            line="";
            break;
        }
        //Ler 1 caractere e armazenar na variavel c
        line += c;
        //std::cout << "Caractere recebido: " << c << " (ASCII: " << static_cast<int>(c) << ")\n";
    }
    //line += '\n';
    //std::cout << line;
    return line;
}

// Função para assinar mensagens
std::string TrigaMirror::signMessage(const std::string message, int* numPkgs)
{
    *numPkgs=0;
    std::string signedMessage;
    size_t start = 0;
    size_t len = message.length();
    while (start < len) //percorre a mensagem em blocos de até 63 caracteres de cada vez, assinando cada bloco separadamente.
    {
        char bytes[256]= {0};
        size_t end = std::min(start + 63, len);
        std::string command  = "echo \'\'\'";
                    command += message.substr(start, end - start);
                    command += "\'\'\' | openssl pkeyutl -sign -inkey ";
                    command += privKeyPath;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Falha ao abrir pipe.\n";
            return "";
        }   
        size_t bytesRead = fread(bytes, 1, 256, pipe);
        pclose(pipe);
        if (bytesRead != 256)
        {
            std::cerr << "Erro ao ler do pipe. Número de bytes lidos: " << bytesRead << "\n";
            return "";
        }
        signedMessage.append(bytes,256);
        (*numPkgs)++;
        start = end;
    }
    return signedMessage;
}


/*
####################################
##       Funções de espelho       ##
####################################
*/



void TrigaMirror::createMirror(int port)
{
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

    while(true) 
    {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            logConnection(clientAddr, false, 0, "");
            continue;
        }        
        std::thread clientThread(&TrigaMirror::handleTCPClients, this, clientSocket,clientAddr);
        clientThread.detach();
    }
}

void TrigaMirror::handleTCPClients(int clientSocket, struct sockaddr_in clientAddr)
{
    const int timeout = 0;
    char buffer[1024];
    
    
    //Espere receber o dado do cliente: formato de dado a ser enviado e a taxa de envio
    int n = recv(clientSocket, buffer, sizeof(buffer)-1, timeout);
    if (n <= 0)
    {
        logConnection(clientAddr, false, 0, "");
        close(clientSocket);
        return;
    }
    
    //Processe o dado enviado pelo cliente
    int kind, interval;
    std::string ip_cliente_http;
    if (parser(buffer, &kind, &interval, &ip_cliente_http))
    {
        logConnection(clientAddr, false, 0, ip_cliente_http);
        close(clientSocket);
        return;
    }

    // Envie dados conforme tipo de formato
    if (kind==0)//Tipo RAW
    {
        send(clientSocket, dataHeader.c_str(), dataHeader.length(), 0);
        while(true)
        {
            std::string data = *data_global.load(); //Leia do vetor global os dados
            if(send(clientSocket, data.c_str(), data.length(), 0) <= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }        
    }
    else if (kind==1)//Tipo RAW assinado digitalmente
    {
        send(clientSocket, dataHeaderSing.c_str(), dataHeaderNumPkgs*256, 0);
        while(true)
        {
            std::string data = *data_global_sing.load(); //Leia do vetor global os dados assinados
            if(send(clientSocket, data.c_str(), *data_global_numPkgs.load()*256, 0) <= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }
    else if (kind==2)// Tipo HTTP SSE
    {
        // enviar cabeçalho SSE
        std::string httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\ndata: ";
        httpHeader += dataHeader.c_str();
        httpHeader += "\n\n";
        send(clientSocket, httpHeader.c_str(), httpHeader.length(), 0);
        while(true)
        {
            std::string data = "data: ";
            data += *data_global.load(); //Leia do vetor global os dados
            data += "\n\n";
            if(send(clientSocket, data.c_str(), data.length(), 0) <= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }        
    }

}

int TrigaMirror::parser(const char *buffer, int *kind, int *interval, std::string *ip_client_http)
{
    if (!buffer || !kind || !interval)
        return 1; // ponteiros inválidos

    std::string buf(buffer);

    // Remove espaços e quebras de linha no início e fim
    auto start = buf.find_first_not_of(" \t\r\n");
    auto end   = buf.find_last_not_of(" \t\r\n");
    if (start == std::string::npos)
        return 1; // string vazia
    buf = buf.substr(start, end - start + 1);

    // Caso 1: string é só número inteiro
    if (!buf.empty() && std::all_of(buf.begin(), buf.end(), ::isdigit)) {
        *kind = 0;
        *interval = std::stoi(buf);
        return 0;
    }

    // Caso 2: começa com 's' seguido de número inteiro
    if (buf.size() > 1 && (buf[0] == 's' || buf[0] == 'S')) {
        std::string numpart = buf.substr(1);
        if (!numpart.empty() && std::all_of(numpart.begin(), numpart.end(), ::isdigit)) {
            *kind = 1;
            *interval = std::stoi(numpart);
            return 0;
        }
    }

    // Caso 3: HTTP request com "taxAmo="
    {
        std::size_t pos = buf.find("taxAmo=");
        if (pos != std::string::npos) {
            pos += 7; // pula "taxAmo="
            std::size_t posEnd = pos;
            while (posEnd < buf.size() && std::isdigit(static_cast<unsigned char>(buf[posEnd]))) {
                posEnd++;
            }
            if (posEnd > pos) {
                *kind = 2;
                *interval = std::stoi(buf.substr(pos, posEnd - pos));

                // Capturar X-Forwarded-For (enviado pelo apache quando é proxy reverso)
                std::size_t xf_pos = buf.find("X-Forwarded-For:");
                if (xf_pos != std::string::npos) {
                    xf_pos += 16; // pular "X-Forwarded-For:"
                    // pula espaços
                    while (xf_pos < buf.size() && (buf[xf_pos] == ' ' || buf[xf_pos] == '\t'))
                        xf_pos++;
                    std::size_t xf_end = buf.find_first_of("\r\n", xf_pos);
                    if (xf_end != std::string::npos)
                        *ip_client_http = buf.substr(xf_pos, xf_end - xf_pos);
                    else
                        *ip_client_http = buf.substr(xf_pos);
                } else {
                    ip_client_http->clear();
                }
                return 0;
            }
        }
    }

    // Nenhum dos formatos pré-estabelecidos
    return 1;
}



/*
####################################
##         Outras Funções         ##
####################################
*/

// Função para criar log de conexão
void TrigaMirror::logConnection(struct sockaddr_in clientAddr, bool sucesses, int taxAmo, std::string ip_client_http)
{
    //Se logPath for nulo, não salve log de conexão.
    if (logPath=="") return;

    //Montar mensagem
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *parts = std::localtime(&now_c);

    std::ostringstream message;
    message <<  std::put_time(parts, "%Y-%m-%d %H:%M:%S");
    message << ";";
    message << inet_ntoa(clientAddr.sin_addr);
    message << ";";
    message << std::to_string(clientAddr.sin_port);
    message << ";";
    message << std::to_string(sucesses);
    message << ";";
    message << std::to_string(taxAmo);
    message << ";";
    message << ip_client_http;
    message << ";\n";

    //Verificar se arquivo já existe
    bool alreadyExist = std::filesystem::exists(logPath);

    //Abrir arquivo
    std::ofstream outfile(logPath,std::ios::app);
    if (outfile.is_open()) // Se o arquivo foi aberto com sucesso
    {
        if(!alreadyExist) outfile << "TIME;IP;PORT;SUCESSES;TaxAmo;IP_HTTP;\n";//Caso o arquivo esteja sendo criado agora, escreva o cabeçalho
        outfile << message.str(); // Escrever a linha no arquivo
        outfile.close(); // Fechar o arquivo
    }
    else
    {
        message.str("[ logConnection() ] Unable to create/open log file: ");
        message << logPath;
        message << "\n";
        std::cerr << message.str();
    }
}
