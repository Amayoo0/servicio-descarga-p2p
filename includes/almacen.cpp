#include "almacen.h"

/**
 * Agrega un cliente al almacén.
 * 
 * @param s Cadena que describe el cliente a agregar. Debe tener el siguiente formato 'fichero.txt/127.0.0.1/5050/'
 * @return Verdadero si se agrega el cliente correctamente, falso si hay algún error.
 */
bool AlmacenDirecciones::agregar_cliente(std::string s){
    bool resultado = true;
    try{
        auto [fichero, ip, puerto] = separar_string(s);
        auto donde = tabla.find(fichero);
        par_ip_puerto direccion{ip, puerto};        
        if(donde != tabla.end()){                       //si encuentra la clave en la tabla
            donde->second.push_back(direccion);         //agrega nueva direccion (puede tener varias)
        }else
            tabla.emplace(fichero, std::vector{direccion});
    }catch(const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
        resultado = false;
    }

    return resultado;

}

/**
 * Obtiene un cliente del almacén.
 * 
 * @param fichero Cadena que contiene el nombre del fichero. Por ejemplo: 'fichero.txt'.
 * @param s Devuelve la dirección del fichero si se encuentra con la siguiente estructura 'tcp://127.0.0.1:5050'.
 * @return Verdadero si se obtiene el cliente correctamente, falso si hay algún error.
 */
bool AlmacenDirecciones::obtener_cliente(std::string fichero, std::string &s){
    bool encontrado = false;
    auto donde = tabla.find(fichero);
    if(donde != tabla.end()){ 
        encontrado = true;
        auto [ip, puerto] = ip_puerto_to_string(donde->second[0].first, donde->second[0].second);
        s = "tcp://" + ip + ":" + puerto;
    }else{
        s = "EMPTY";
    }
    return encontrado;
}
/* posible mejora: devolver un vector de string para que el cliente pruebe a conectarse a cada uno en caso de error.*/


std::pair<std::string, std::string> AlmacenDirecciones::ip_puerto_to_string(uint32_t ip, uint16_t puerto){
    std::string ip_str = inet_ntoa(*reinterpret_cast<struct in_addr*>(&ip));
    std::string puerto_str = std::to_string(ntohs(puerto));
    return std::make_pair(ip_str, puerto_str);
}

/**
 * Imprime por pantalla el contenido del almacén.
*/
void AlmacenDirecciones::to_string(){
    for( const auto & f: tabla ){
        std::cout << "Nombre fichero: " << f.first << std::endl;
        for( const auto & d: f.second ){
            uint32_t ip_binario = d.first;
            auto [ip, puerto] = ip_puerto_to_string(d.first, d.second);
            std::cout << "\tip: " << ip << "\n\tpuerto: " << puerto << std::endl;
        }
    }
}

std::tuple<std::string, uint32_t, uint16_t> AlmacenDirecciones::separar_string(std::string s){
    std::string delimiter = "</>";

    size_t pos = 0;
    std::vector<std::string> tokens;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        tokens.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }

    std::string fichero = tokens[0];
    uint32_t ip = inet_addr(tokens[1].c_str());
    uint16_t puerto = htons(std::stoi(tokens[2])); 

    return std::make_tuple(fichero, ip, puerto); 
}


std::string AlmacenDirecciones::obtener_ficheros(){
    std::string mi_almacen{};
    for( const auto & f: tabla ){
        mi_almacen.append(f.first);
        mi_almacen.append(" ");
    }
    return mi_almacen;
}

// Método para guardar los datos del almacén en un archivo
bool AlmacenDirecciones::salvar_datos(const std::string& nombre_archivo) {
    std::ofstream archivo(nombre_archivo);
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo para guardar los datos" << std::endl;
        return false;
    }

    for (const auto& par : tabla) {
        archivo << par.first << " "; // Nombre del fichero
        for (const auto& direccion : par.second) {
            archivo << direccion.first << " " << direccion.second << " "; // Dirección IP y Puerto
        }
        archivo << std::endl;
    }

    archivo.close();
    return true;
}

// Método para recuperar los datos desde un archivo
bool AlmacenDirecciones::recuperar_datos(const std::string& nombre_archivo) {
    std::ifstream archivo(nombre_archivo);
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo para recuperar los datos" << std::endl;
        return false;
    }

    tabla.clear(); // Limpiar la tabla existente antes de recuperar los datos

    std::string fichero;
    uint32_t ip;
    uint16_t puerto;
    while (archivo >> fichero >> ip >> puerto) {
        tabla[fichero].push_back(std::make_pair(ip, puerto));
    }

    archivo.close();
    return true;
}



