#include <string>
#include <iostream>
#include <utility>      //para std::pair
#include <arpa/inet.h>  //para inet_addr e inet_ntoa
#include <zmq.hpp>
#include <thread>
#include <fstream>
#include "servicio_ficheros.h"
#include <array>
#include <boost/asio.hpp>

#define DEBUG_PRINTF

/* --- CONSTANTES --- */
const std::string SERVIDOR = "tcp://localhost:5555";
const std::string SEPARADOR = "</>";

/* --- FUNCIONES --- */
void hebra_servidora_ficheros(uint16_t puerto);
void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente);
void do_accept(io_context & ioc, tcp::acceptor & acceptor);
void do_connect(io_context& ioc, const std::string server_address, uint16_t server_port, std::string nombre_fichero);


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
                cliente_descarga.detach();
                
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
    
    return 0;
}

void do_accept(boost::asio::io_context & ioc, boost::asio::ip::tcp::acceptor & acceptor){
    std::shared_ptr<ServicioFicheros> servicio = ServicioFicheros::create(ioc, "fichero.txt");
    std::cout << "use_count: " << servicio.use_count() << std::endl;
    acceptor.async_accept(servicio->socket_conectado,
        [&ioc,&acceptor,servicio](const boost::system::error_code & ec){    //la lambda necesita: &ioc, &acceptor y servicio como copia (se puede porque es shared)
          if (!ec){
            servicio->do_read_from_file_and_write_socket();
          }
          do_accept(ioc, acceptor);
        });
}

void do_connect(io_context& ioc, const std::string server_address, uint16_t server_port, std::string nombre_fichero) {
    // Create an output file stream object
    std::ofstream outfile(nombre_fichero);

    // Check if the file was successfully opened
    if (outfile.is_open()) {
        // Close the file
        outfile.close();
    
        std::shared_ptr<ServicioFicheros> servicio = ServicioFicheros::create(ioc, "fichero.txt");
        // Resolve the host and port
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(server_address), server_port);

        // Connect to the server
        servicio->socket_conectado.connect(endpoint);
        servicio->do_read_from_socket_and_async_write_file();

    } else {
        std::cerr << "Error: Failed to create empty file: " << nombre_fichero << std::endl;
    }
    
}

void hebra_servidora_ficheros( uint16_t puerto) {
#ifdef DEBUG_PRINTF
    std::cout << "[Hebra servidora] Escuchando peticiones en el puerto: " << puerto << std::endl;
#endif
    try{
        io_context io;
        boost::asio::ip::tcp::acceptor acceptor{io, boost::asio::ip::tcp::endpoint(tcp::v4(), puerto)};

        acceptor.listen();
        do_accept(io, acceptor);
      
        io.run();
    }catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    } 

        
}

void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente){
#ifdef DEBUG_PRINTF
    std::cout << "Hebra cliente solicitando descarga en: " << dir_cliente << std::endl;
#endif
    std::size_t double_slash_pos = dir_cliente.find("//");
    std::size_t colon_pos = dir_cliente.find(":", double_slash_pos);
    std::string ip = dir_cliente.substr(double_slash_pos + 2, colon_pos - double_slash_pos - 2);
    std::string port_str = dir_cliente.substr(colon_pos + 1);
    uint16_t port = htons(std::stoi(port_str));
    io_context ioc;
    boost::asio::ip::tcp::socket socket(ioc);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

    socket.connect(endpoint);
    std::array<char, 1024> datos;

    // Abrir el archivo de salida para escritura binaria
    std::ofstream output_file("output_file.txt", std::ios::binary);
    if (!output_file.is_open())
    {
        std::cerr << "Error opening output file" << std::endl;
    }

    size_t len = 0;
    do{
        boost::system::error_code ec; 
        len = socket.read_some(boost::asio::buffer(datos), ec);
        if (!ec) {
            std::cout << "Cliente lee: " << len << " bytes" << std::endl;
            output_file.write(datos.data(), len);
        }
        else {
            std::cerr << "Error de lectura del socket: " << ec.message() << std::endl;
        }
    }while(len > 0);
    
    // Cerrar el archivo de salida
    output_file.close();
}