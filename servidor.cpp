#include <string>
#include <thread>
#include <iostream>
#include <zmq.hpp>
#include "almacen.h"
#include <csignal> // Para manejo de señales

#define DEBUG_PRINTF

/* --- CONSTANTES --- */
const std::string ip_puerto_escucha = "tcp://*:5555";
const std::string fichero_backup = "./storage/datos_almacen.txt";



/* --- VARIABLES GLOBALES --- */
AlmacenDirecciones almacen;



/* --- FUNCIONES --- */
void sigint_handler(int signal) // manejadora Ctrl+C
{
#ifdef DEBUG_PRINTF
    std::cout << "Se recibió una señal SIGINT (Ctrl+C)" << std::endl;
#endif
    almacen.salvar_datos(fichero_backup);
    exit(signal); // Sale del programa con el código de señal recibido
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
