#ifndef ALMACEN_H
#define ALMACEN_H

#include <iostream>
#include <map>
#include <vector>
#include <utility>      //para std::pair
#include <arpa/inet.h>  //para inet_addr e inet_ntoa
#include <cstdint>
#include <fstream>      //para manejo de ficheros



class AlmacenDirecciones{
    private:
    using par_ip_puerto = std::pair<uint32_t, uint16_t>;
    using tabla_ficheros =  std::map<std::string, std::vector<par_ip_puerto> >;
    
    tabla_ficheros tabla;

    std::tuple<std::string, uint32_t, uint16_t> separar_string(std::string);
    std::pair<std::string, std::string> ip_puerto_to_string(uint32_t ip, uint16_t puerto);

    public:
    bool agregar_cliente(std::string s);
    bool obtener_cliente(std::string fichero, std::string &s);
    void to_string();
    std::string obtener_ficheros();
    bool salvar_datos(const std::string &nombre_archivo);
    bool recuperar_datos(const std::string &nombre_archivo);

};

#endif