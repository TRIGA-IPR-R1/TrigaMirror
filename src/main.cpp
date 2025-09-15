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

//main.cpp
#include "TrigaMirror.h"
#include <cxxopts.hpp>
#include <fstream>
#include <chrono>
#include <csignal>

// Função para mostrar a versão do programa
void showVersion()
{
    std::cout << "Development Version" << std::endl;
}

// Função para mostrar a licença do programa
void showLicense()
{
    std::cout << "TrigaMirror      Copyright (C) 2024-2025      Thalles Campagnani" << std::endl;
    std::cout << "This    program    comes    with    ABSOLUTELY    NO   WARRANTY;" << std::endl;
    std::cout << "This is free software,    and you are welcome to redistribute it" << std::endl;
    std::cout << "under certain conditions; For more details read the file LICENSE" << std::endl;
    std::cout << "that came together with the source code." << std::endl << std::endl;
}

// Estrutura para armazenar as configurações do servidor
struct CONFIG
{
    //Objeto para encerrar o programa após exibir uma mensagem no terminal
    int         close            = false;

    //Objetos referentes as configurações do servidor a ser lido
    std::string server_ip        = "localhost";
    int         server_port      = 123;
    int         read_tax         = 1000;

    //Objetos referentes as configurações gerais do programa
    std::string log_path         = "";
    std::string key_path         = "";

    //Objetos referentes as portas espelho
    int         mirror_port      = 0;   //Caso não seja alterado pela linha de comando, será igualado a server_port
    int         mirror_port_assing=0;   //Caso não seja alterado para !=0, não será criado
    int         mirror_port_http = 0;   //Caso não seja alterado para !=0, não será criado
};

// Função para alterar as configurações a partir de parâmtros da linha de comando
CONFIG configOptions(int argc, char* argv[])
{
    CONFIG config;

    cxxopts::Options options("trigamirror","TrigaMirror is a software for GNU operating system to get flux data from TrigaServer and share in network.\n");
    options.add_options()
        ("v,version",  "Show the program version")
        ("h,help",     "Show this help message")
        ("l,license",  "Show info of the license")
        ("i,ip",       "Ip of TrigaServer", cxxopts::value<std::string>())
        ("p,port",     "Port of TrigaServer and TrigaMirror",cxxopts::value<int>())
        ("t,tax",      "Read tax of TrigaServer in ms",cxxopts::value<int>())
        ("g,log",      "Save log of connections and choose a place to save",cxxopts::value<std::string>())
        ("s,sing",     "Sing (encrypt) data before send",cxxopts::value<std::string>())
        ("m,mirror",   "Change port of TrigaMirror",cxxopts::value<int>());


    auto result = options.parse(argc, argv);

    if (result.count("version") || result.count("v"))
    {
        showVersion();
        config.close = 1;
        return config;
    } 

    if (result.count("help") || result.count("h"))
    {
        std::cout << options.help() << std::endl;
        config.close = 1;
        return config;
    } 

    if (result.count("license") || result.count("l")) 
    {
        showLicense();
        config.close = 1;
        return config;
    } 

    if (result.count("ip")          || result.count("i"))   config.server_ip        = result["ip"].as<std::string>();
    if (result.count("port")        || result.count("p"))   config.server_port      = result["port"].as<int>();
    if (result.count("tax")         || result.count("t"))   config.read_tax         = result["tax"].as<int>();
    if (result.count("log")         || result.count("g"))   config.log_path         = result["log"].as<std::string>();
    if (result.count("sing")        || result.count("s"))   config.key_path         = result["sing"].as<std::string>();
    if (result.count("mirror")      || result.count("m"))   config.mirror_port      = result["mirror"].as<int>();
    
    //Se não foi selecionada uma porta para o mirror: Replique a mesma porta do server
    if(config.mirror_port==0) config.mirror_port = config.server_port; 
    
    return config;
}

// Função principal
int main(int argc, char* argv[])
{
    //Ignorar sinal SIGPIPE quando cliente se desconecta
    signal(SIGPIPE, SIG_IGN); 

    //Ler configurações da linha de comando
    CONFIG config = configOptions(argc, argv);
    
    //Em caso de erro no parâmetro da linha de comando (ou opções extras), encerre o programa.
    if(config.close) return config.close;

    //Criar objeto e conectar ao servidor
    TrigaMirror mirror( config.server_ip, 
                        config.server_port, 
                        config.read_tax, 
                        config.log_path,
                        config.key_path); 
    
    //Crie servidor espelho
    std::thread mirrorThreadRaw (&TrigaMirror::createMirror, &mirror, config.mirror_port);
    mirrorThreadRaw.join();

    return 1;
}
