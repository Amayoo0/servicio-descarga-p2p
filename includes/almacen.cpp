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


std::string AlmacenDirecciones::to_html() {
    std::string html{
    R"(
        <!DOCTYPE html>
        <html>
        <head>
        <style>
        body {
            background-color: #f0f0f0;
            font-family: Arial, sans-serif;
        }

        h2 {
            color: #ff4500;
            text-align: center;
        }

        form {
            width: 50%;
            margin: 0 auto;
            padding: 20px;
            background-color: #fff;
            border-radius: 10px;
            box-shadow: 0 0 20px rgba(0, 0, 0, 0.1);
        }

        label {
            display: block;
            margin-bottom: 10px;
            color: #333;
        }

        input[type="text"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 20px;
            border: 1px solid #ccc;
            border-radius: 5px;
            box-sizing: border-box;
            font-size: 16px;
        }

        input[type="submit"] {
            width: 100%;
            padding: 10px;
            background-color: #ff4500;
            color: #fff;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 18px;
            transition: background-color 0.3s ease;
        }

        input[type="submit"]:hover {
            background-color: #ff6347;
        }
        header {
            background-color: rgba(255, 255, 255, 0.3);
            padding: 10px 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }

        h1 {
            text-align: center;
            font-size: 3rem;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
        }

        div {
            background-color: rgba(255, 255, 255, 0.3);
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 20px rgba(0, 0, 0, 0.2);
            text-align: center;
        }

        p {
            font-size: 1.2rem;
            margin-bottom: 15px;
        }

        strong {
            color: #ff6699;
        }

        .botones {
            display: flex;
        }

        a.button {
            background-color: #ff6347;;
            color: #fff;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            margin-left: 10px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }

        table {
        font-family: arial, sans-serif;
        border-collapse: collapse;
        width: 100%;
        }

        td, th {
        border: 1px solid #dddddd;
        text-align: left;
        padding: 8px;
        }

        tr:nth-child(even) {
        background-color: #dddddd;
        }
        </style>
        </head>
        <body>
            <header>
                <h1>Inicio</h1>
                <div class="botones">
                    <a href="/" class="button">Inicio</a>
                    <a href="/perfil" class="button">Perfil</a>
                </div>
            </header>

        <h2>Enviar fichero al Servidor</h2>
        <form action="/guardar" method="POST">
            <label for="nombre_fichero">Nombre fichero? </label>
            <input type="text" id="nombre_fichero" name="nombre_fichero"><br>
            <label for="ip">IP? </label>
            <input type="text" id="ip" name="ip"><br>
            <label for="puerto">Puerto? </label>
            <input type="text" id="puerto" name="puerto"><br>
            <input type="submit" value="Enviar">
        </form>

        <h2>Almacen de direcciones</h2>
        <table>
            <tr>
                <th>Fichero</th>
                <th>Direccion IP</th>
                <th>Puerto</th>
            </tr>
    )"
    };
    for( const auto & f: tabla ){
        html.append("<tr>");

        for( const auto & d: f.second ){
            auto [ip, puerto] = ip_puerto_to_string(d.first, d.second);
            html.append((boost::format(R"(
                <tr>
                    <td>%1%</td>
                    <td>%2%</td>
                    <td>%3%</td>
                </tr>
            )") %f.first % ip % puerto).str());
        }
    }
    html.append(
        R"(
            </table>
            </body>
            </html>
        )"
    );
    return html;
}


