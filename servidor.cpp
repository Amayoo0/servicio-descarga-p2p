#include <string>
#include <thread>
#include <iostream>
#include <includes/zmq.hpp>
#include "includes/almacen.h"
#include <csignal> // Para manejo de señales
#include <thread>
#include "includes/crow_all.h"
#include <unordered_map>
#include <sstream>

// #define DEBUG_PRINTF
#define PUERTO_CROW 8080

/* --- CONSTANTES --- */
const std::string ip_puerto_escucha = "tcp://*:5555";
const std::string fichero_backup = "./storage/datos_almacen.txt";
const std::string SEPARADOR = "</>";



/* --- VARIABLES GLOBALES --- */
AlmacenDirecciones almacen;
std::thread servidor_crow;


/* --- FUNCIONES --- */
void sigint_handler(int signal) // manejadora Ctrl+C
{
    std::cout << "\n\nSe recibió una señal SIGINT (Ctrl+C) \nGuardando datos..." << std::endl;
    almacen.salvar_datos(fichero_backup);
    servidor_crow.join();
    exit(signal); // Sale del programa con el código de señal recibido
}

// Función de manejo para la ruta "/guardar"
void guardar_string(crow::response& res, const crow::request& req) {

}

std::string formatea_solicitud(const std::string body){
    std::string resultado{};
    // Parsear los datos y almacenarlos en un mapa
    std::unordered_map<std::string, std::string> datos;
    std::istringstream iss(body);
    std::string token;
    while (std::getline(iss, token, '&')) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string clave = token.substr(0, pos);
            std::string valor = token.substr(pos + 1);
            // Decodificar valor (si es necesario)
            // Aquí puedes implementar tu propia lógica de decodificación de URL
            // En este ejemplo, no se realiza decodificación
            datos[clave] = valor;
        }
    }
    resultado = datos["nombre_fichero"] + SEPARADOR + datos["ip"] + SEPARADOR + datos["puerto"] + SEPARADOR ;
    return resultado;
}

void servidor_crow_handler()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        std::string datos_almacen = almacen.to_html();
        return datos_almacen;        
    });

    CROW_ROUTE(app, "/guardar")
    .methods("POST"_method)
    ([](const crow::request &req){
        std::string resultado = formatea_solicitud(req.body);
        if( almacen.agregar_cliente(resultado) ){
            std::string ack = "ACK";
            return ack;
        }else{
            return resultado;
        }
    });

    // Ruta para mostrar el perfil del usuario
    CROW_ROUTE(app, "/perfil")
    ([]() {
        // Datos del usuario (nombre y edad)
        std::string nombre = "Antonio";
        int edad = 22;

        crow::mustache::context ctx;
        ctx["nombre"] = nombre;
        ctx["edad"] = edad;

        // Renderizar la plantilla HTML con los datos del usuario
        return crow::mustache::load("perfil.html").render(ctx);
    });

    CROW_ROUTE(app, "/agregar_fichero/<string>/<string>/<string>")
    ([](const std::string& nombre_fichero, const std::string& ip, const std::string puerto)
    {
        std::string solicitud = nombre_fichero + SEPARADOR + ip + SEPARADOR + puerto + SEPARADOR;
        almacen.agregar_cliente(solicitud);
        crow::response res;
        res.body = (boost::format(R"(<h2>Fichero agregado correctamente.</h2> <a href='localhost:%1%/'>Inicio</a>)") % PUERTO_CROW).str();
        return res;
    });

    // Configurar la ruta "/guardar" para manejar solicitudes POST
    // CROW_ROUTE(app, "/guardar").methods("POST"_method)
    // ([&](, const crow::request& req){
    //     // Verificar si el cuerpo de la solicitud contiene datos
    //     if (!req.body.empty()) {
    //         // Obtener el string del cuerpo de la solicitud
    //         std::string nuevo_string = req.body;

    //         // Aquí podrías almacenar el string en algún lugar, por ejemplo, en una variable global o en una base de datos

    //         // Enviar una respuesta al cliente indicando que se ha guardado el string correctamente
    //         res.code = 200;
    //         res.body = "String guardado correctamente: " + nuevo_string;
    //     } else {
    //         // Enviar una respuesta de error si el cuerpo de la solicitud está vacío
    //         res.code = 400;
    //         res.body = "Error: El cuerpo de la solicitud está vacío";
    //     }
    // });

    app.loglevel(crow::LogLevel::Error);


    app.port(PUERTO_CROW).multithreaded().run();
}

int main() 
{
    // Instalar el manejador de señales para SIGINT
    std::signal(SIGINT, sigint_handler);

    // initialize the zmq context with a single IO thread
    zmq::context_t context{1};

    // construct a REP (reply) socket and bind to interface
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind(ip_puerto_escucha);

    // prepare some static data for responses
    std::string ack{"ACK"};


    // recuperamos los datos de la última sesión si los hubiera
    almacen.recuperar_datos(fichero_backup);

    
    // inicializamos servidor Crow HTTP
    servidor_crow = std::thread(servidor_crow_handler);

    for (;;) 
    {
        zmq::message_t request;
        zmq::send_result_t ret = 0;
        bool res{};
        std::string mi_almacen{};
        std::string cliente_con_fichero{};

        // REQ: recibe una petición {0, 1, 2}
        ret = socket.recv(request, zmq::recv_flags::none);
        if (!ret) {
            std::cerr << "Fallo al recibir datos" << std::endl;
            exit(-1);
        }

        int opcion = std::stoi(request.to_string());
        switch (opcion)
        {
            case 0: // Muestra el listado de ficheros.
#ifdef DEBUG_PRINTF
                std::cout << "Enviando ficheros al cliente..." << std::endl;
#endif
                mi_almacen = almacen.obtener_ficheros();
                if ( mi_almacen.empty() )
                {
                    mi_almacen = "EMPTY";
                }

                // RES: envía almacen
                ret = socket.send(zmq::buffer(mi_almacen), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar almacen al cliente" << std::endl;
                    exit(-1);
                }
                break;

            case 1: // Subir un nuevo fichero al servidor
                // RES: enviando ack
                ret = socket.send(zmq::buffer(ack), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar ACK" << std::endl;
                    exit(-1);
                } 
#ifdef DEBUG_PRINTF
                std::cout << "Recibiendo fichero..." << std::endl;
#endif
                // REQ: recibiendo datos fichero
                ret = socket.recv(request, zmq::recv_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al recibir datos del fichero" << std::endl;
                    exit(-1);
                }
#ifdef DEBUG_PRINTF
                std::cout << "Almacenando: " << request.to_string() << std::endl;
#endif
                res = almacen.agregar_cliente(request.to_string());
                if (!res) {
                    std::cerr << "Fallo al guardar los datos en el almacen" << std::endl;
                    exit(-1);
                }
                // RES: enviando ack
                ret = socket.send(zmq::buffer(ack), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar ACK" << std::endl;
                    exit(-1);
                } 
                break;
            case 2: // Descargar un fichero
#ifdef DEBUG_PRINTF
                std::cout << "Se ha solicitado una descarga de fichero." << std::endl;
#endif
                // RES: enviando ack
                ret = socket.send(zmq::buffer(ack), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar ACK" << std::endl;
                    exit(-1);
                } 

                // REQ: recibiendo el nombre de fichero a consultar
                ret = socket.recv(request, zmq::recv_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al recibir datos" << std::endl;
                    exit(-1);
                }
#ifdef DEBUG_PRINTF
                std::cout << "Cliente solicita los datos del fichero: " << request.to_string() << std::endl;
#endif
                
                res = almacen.obtener_cliente(request.to_string(), cliente_con_fichero);
                if (!res) {
                    std::cerr << "Fallo al obtener datos del almacen" << std::endl;
                }
#ifdef DEBUG_PRINTF
                std::cout << "Datos del cliente: " << cliente_con_fichero << std::endl;
#endif
                // enviando datos del cliente
                ret = socket.send(zmq::buffer(cliente_con_fichero), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar datos del cliente." << std::endl;
                    exit(-1);
                }

                break;
            default:
                std::cout << "Se ha recibido un caracter no deseado." << std::endl;
        }   //end switch

    }   //end for

    return 0;
}
