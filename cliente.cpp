#include <string>
#include <iostream>
#include <utility>      //para std::pair
#include <arpa/inet.h>  //para inet_addr e inet_ntoa
#include <zmq.hpp>
#include <thread>
#include <fstream>

#define DEBUG_PRINTF

/* --- CONSTANTES --- */
const std::string SERVIDOR = "tcp://localhost:5555";
const std::string SEPARADOR = "</>";

/* --- FUNCIONES --- */
void hebra_servidora_ficheros(uint16_t puerto);
void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente);



int main()
{
    std::string solicitud{};
    std::string nombre, ip, puerto;
    std::string ack{"ACK"};
    zmq::send_result_t ret = 0;
    int res = 0;
    std::vector<std::thread> servidoras_escuchando;
    std::thread cliente_descarga;

    // initialize the zmq context with a single IO thread
    zmq::context_t context{1};

    // construct a REQ (request) socket and connect to interface
    zmq::socket_t socket{context, zmq::socket_type::req};
    zmq::message_t reply{};

    socket.connect(SERVIDOR);
    
    bool noFin = true;
    while(noFin)
    {
        int opcion = 0;
        std::cout << std::endl << "Qué opción desea:" << std::endl;
        std::cout << "0 - Mostrar el listado de ficheros del servidor." << std::endl;
        std::cout << "1 - Subir un nuevo fichero al servidor." << std::endl;
        std::cout << "2 - Descargar un fichero" << std::endl;
        std::cin >> opcion;

        switch (opcion)
        {
            case 0: // Muestra el listado de ficheros.
                // REQ: pidiendo datos del almacen
                ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
                }                
                // RES: recibe los ficheros del servidor
                ret = socket.recv(reply, zmq::recv_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al recibir los ficheros del servidor." << std::endl;
                }

                std::cout << "Listado de ficheros del servidor:" << std::endl;
                std::cout << reply.to_string() << std::endl;

                break;
            case 1: // Subir un nuevo fichero al servidor
                // REQ: pidiendo datos del almacen
                ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
                }
                // RES: recibe confirmación
                ret = socket.recv(reply, zmq::recv_flags::none);
                if (!ret || reply.to_string() != ack) {
                    std::cerr << "Fallo al recivir ACK" << std::endl;
                    exit(-1);
                }
                std::cout << "Nombre fichero? ";
                std::cin >> nombre;
                solicitud.append(nombre).append(SEPARADOR);

                std::cout << "IP de escucha? ";
                std::cin >> ip;
                solicitud.append(ip).append(SEPARADOR);

                std::cout << "Puerto de escucha? ";
                std::cin >> puerto;
                solicitud.append(puerto).append(SEPARADOR);

                
                // REQ: envía datos ficheros
                ret = socket.send(zmq::buffer(solicitud), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al enviar datos del fichero." << std::endl;
                    exit(-1);
                } 
                // RES: recibe confirmación
                ret = socket.recv(reply, zmq::recv_flags::none);
                if (!ret || reply.to_string() != ack) {
                    std::cerr << "Fallo al recivir ACK" << std::endl;
                    exit(-1);
                }

                // lanzar hebra que escuche peticiones en el puerto indicado.
                servidoras_escuchando.push_back(std::thread(hebra_servidora_ficheros, std::stoi(puerto)));
                break;
            case 2: // Descargar un fichero
                std::cout << "Descargar un fichero" << std::endl;
                // REQ: enviamos opción al servidor
                ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
                }
                // RES: recibe confirmación
                ret = socket.recv(reply, zmq::recv_flags::none);
                if (!ret || reply.to_string() != ack) {
                    std::cerr << "Fallo al recivir ACK" << std::endl;
                    exit(-1);
                } 


                std::cout << "Indique el nombre completo del fichero que desea descargar:" << std::endl;
                std::cin >> solicitud;
                // REQ: enviamos nombre de fichero
                ret = socket.send(zmq::buffer(solicitud), zmq::send_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al solicitar datos del fichero." << std::endl;
                }     
                // RES: recibe los datos del cliente con fichero
                ret = socket.recv(reply, zmq::recv_flags::none);
                if (!ret) {
                    std::cerr << "Fallo al recibir los datos del cliente con fichero." << std::endl;
                }

                if (reply.to_string() == "EMPTY" )
                {
                    std::cout << "Introduzca un fichero existente en el servidor. Eliga la opción 0 para ver los fichros actuales" << std::endl;
                    break;
                }
#ifdef DEBUG_PRINTF
                std::cout << "Los datos del cliente relacionado con el fichero son: " << reply.to_string() << std::endl;
#endif
                // lanzamos hebra para conectar con los datos del cliente.
                cliente_descarga = std::thread(hebra_cliente_ficheros, solicitud, reply.to_string());
                break;
            default:
                noFin = false;
                break;
        }   // end switch
    }   // end while
    for ( auto & hebra: servidoras_escuchando )
    {
        hebra.join();
    }
    cliente_descarga.join();
    
    return 0;
}


void hebra_servidora_ficheros( uint16_t puerto) {
#ifdef DEBUG_PRINTF
    std::cout << "[Hebra servidora] Escuchando peticiones en el puerto: " << puerto << std::endl;
#endif
    zmq::message_t request;
    zmq::send_result_t ret = 0;
    
    // initialize the zmq context with a single IO thread
    zmq::context_t context{1};

    // construct a REP (reply) socket and bind to interface
    zmq::socket_t socket{context, zmq::socket_type::rep};
    std::string dir_connection = {"tcp://*:"};
    socket.bind(dir_connection.append(std::to_string(puerto)));

    // prepare some static data for responses
    std::string fin{"FIN"};

    for(;;)
    {
        // REQ: recibe una petición de descarga de un cliente
        ret = socket.recv(request, zmq::recv_flags::none);
        if (!ret) {
            std::cerr << "Fallo al recibir datos" << std::endl;
            exit(-1);
        }

        std::ifstream f_in(request.to_string(), std::ios::binary);
        std::array<char, 1024> buffer;
        std::streamsize leidos = f_in.readsome(buffer.data(), 1024);
        while( leidos > 0 ){
#ifdef DEBUG_PRINTF
            std::cout << "[Servidor Descarga] --- Enviando nuevo paquete ---" << std::endl;
            std::cout << std::string_view(buffer.data(), leidos) << std::endl;
            std::cout << "[Servidor Descarga] --- Fin nuevo paquete ---" << std::endl;
#endif
            // RES: envia buffer (sndmore activo)
            ret = socket.send(zmq::buffer(buffer.data(), leidos), zmq::send_flags::sndmore);
            if (!ret) {
                std::cerr << "Fallo al enviar fichero al cliente" << std::endl;
                exit(-1);
            }
            leidos = f_in.readsome(buffer.data(), 1024);
        }


        ret = socket.send(zmq::buffer(fin), zmq::send_flags::none);
        if (!ret) {
            std::cerr << "Fallo al enviar paquete FIN" << std::endl;
            exit(-1);
        }
        f_in.close();
    }
    
}

void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente){
#ifdef DEBUG_PRINTF
    std::cout << "Hebra cliente solicitando descarga en: " << dir_cliente << std::endl;
#endif
    zmq::message_t request;
    zmq::send_result_t ret = 0;
    std::ofstream f_out(fichero, std::ios::binary);
    bool recibiendo_datos = true;

    // initialize the zmq context with a single IO thread
    zmq::context_t context{1};

    // construct a REQ (request) socket and connect to interface
    zmq::socket_t socket{context, zmq::socket_type::req};

    socket.connect(dir_cliente);

    // REQ: solicita fichero al cliente
    ret = socket.send(zmq::buffer(fichero), zmq::send_flags::none);
    if (!ret) {
        std::cerr << "Fallo al enviar almacen al cliente" << std::endl;
        exit(-1);
    }
    
    // RES: recibiendo datos del fichero
    ret = socket.recv(request, zmq::recv_flags::none);
    while( ret && request.to_string() != "FIN" ){
#ifdef DEBUG_PRINTF
    std::cout << "[Cliente Descarga] --- Nuevo paquete recibido --- " << std::endl;
    std::cout << request.to_string() << std::endl;
    std::cout << "[Cliente Descarga] --- Fin paquete recibido ---" << std::endl;
#endif
        f_out.write(static_cast<const char*>(request.data()), request.size());
        // recibimos proximo paquete
        ret = socket.recv(request, zmq::recv_flags::none);
    }

    f_out.close();

}