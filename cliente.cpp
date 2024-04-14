#include <string>
#include <iostream>
#include <utility>     //para std::pair
#include <arpa/inet.h> //para inet_addr e inet_ntoa
#include <includes/zmq.hpp>
#include <thread>
#include <array>
#include <boost/asio.hpp>
#include <memory>
#include <fstream>
#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <cstdlib>

// #define DEBUG_PRINTF

/* --- CONSTANTES --- */
const std::string SERVIDOR = "tcp://localhost:5555";
const std::string SEPARADOR = "</>";

/* --- FUNCIONES --- */
void hebra_servidora_ficheros(uint16_t puerto, std::string nombre_fichero);
void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente);
void do_accept(boost::asio::io_context &ioc, boost::asio::ip::tcp::acceptor &acceptor, std::string nombre_fichero);
void do_connect(boost::asio::io_context &ioc, const std::string server_address, std::string server_port, std::string nombre_fichero);

int main()
{
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
    while (noFin)
    {
        std::string solicitud{};
        int opcion = 0;
        std::cout << std::endl
                  << "Qué opción desea:" << std::endl;
        std::cout << "0 - Mostrar el listado de ficheros del servidor." << std::endl;
        std::cout << "1 - Subir un nuevo fichero al servidor." << std::endl;
        std::cout << "2 - Descargar un fichero" << std::endl;
        std::cout << "Opción: ";
        std::cin >> opcion;

        switch (opcion)
        {
        case 0: // Muestra el listado de ficheros.
            // REQ: pidiendo datos del almacen
            ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
            }
            // RES: recibe los ficheros del servidor
            ret = socket.recv(reply, zmq::recv_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al recibir los ficheros del servidor." << std::endl;
            }

            std::cout << "Listado de ficheros del servidor:" << std::endl;
            std::cout << reply.to_string() << std::endl;

            break;
        case 1: // Subir un nuevo fichero al servidor
            // REQ: pidiendo datos del almacen
            ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
            }
            // RES: recibe confirmación
            ret = socket.recv(reply, zmq::recv_flags::none);
            if (!ret || reply.to_string() != ack)
            {
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
            if (!ret)
            {
                std::cerr << "Fallo al enviar datos del fichero." << std::endl;
                exit(-1);
            }
            // RES: recibe confirmación
            ret = socket.recv(reply, zmq::recv_flags::none);
            if (!ret || reply.to_string() != ack)
            {
                std::cerr << "Fallo al recivir ACK" << std::endl;
                exit(-1);
            }

            // lanzar hebra que escuche peticiones en el puerto indicado.
            servidoras_escuchando.push_back(std::thread(hebra_servidora_ficheros, std::stoi(puerto), nombre));
            break;
        case 2: // Descargar un fichero
            // REQ: enviamos opción al servidor
            ret = socket.send(zmq::buffer(std::to_string(opcion)), zmq::send_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al solicitar datos del almacen." << std::endl;
            }
            // RES: recibe confirmación
            ret = socket.recv(reply, zmq::recv_flags::none);
            if (!ret || reply.to_string() != ack)
            {
                std::cerr << "Fallo al recivir ACK" << std::endl;
                exit(-1);
            }

            std::cout << "Noombre fichero? ";
            std::cin >> solicitud;
            // REQ: enviamos nombre de fichero
            ret = socket.send(zmq::buffer(solicitud), zmq::send_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al solicitar datos del fichero." << std::endl;
            }
            // RES: recibe los datos del cliente con fichero
            ret = socket.recv(reply, zmq::recv_flags::none);
            if (!ret)
            {
                std::cerr << "Fallo al recibir los datos del cliente con fichero." << std::endl;
            }

            if (reply.to_string() == "EMPTY")
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
        } // end switch
    }     // end while
    for (auto &hebra : servidoras_escuchando)
    {
        hebra.join();
    }

    return 0;
}

struct FileService : std::enable_shared_from_this<FileService>
{
private:
    std::string fichero;
    boost::asio::posix::stream_descriptor file;
    std::array<char, 1024> datos;

public:
    boost::asio::ip::tcp::socket socket_conectado;

    FileService(boost::asio::io_context &io_c, std::string nombre_fichero) : 
        socket_conectado(io_c),
        file(io_c, open(nombre_fichero.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
    {
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << nombre_fichero << std::endl;
        }
    }
    ~FileService()
    {
#ifdef DEBUG_PRINTF
        std::cout << "Destructor FileService\n";
#endif
        if (file.is_open())
        {
            file.close();
        }
    }

    static std::shared_ptr<FileService> create(boost::asio::io_context &io_c, std::string nombre_fichero)
    {
        return std::make_shared<FileService>(io_c, nombre_fichero); // solo funciona gracia a enable_shared_from_this
    }
    std::shared_ptr<FileService> get_ptr()
    {
        return shared_from_this(); // aumenta el contador de referencia. Devuelve un shared_ptr apuntando a lo mismo
    }
    void do_read()
    {
        async_read(file, boost::asio::buffer(datos), boost::asio::transfer_at_least(1),
                   [self = shared_from_this()](const boost::system::error_code &ec, std::size_t leidos)
                   {
                       if (!ec)
                       {
#ifdef DEBUG_PRINTF
                           std::cout << "Servidor lee (" << leidos << " bytes): " << self->datos.data() << std::endl;
#endif
                           write(self->socket_conectado, boost::asio::buffer(self->datos, leidos));
                           self->do_read();
                       }
                   });
    }

    void do_write()
    {
        async_read(socket_conectado, boost::asio::buffer(datos), boost::asio::transfer_at_least(1),
                   [self = shared_from_this()](const boost::system::error_code &ec, std::size_t leidos)
                   {
                       if (!ec)
                       {
#ifdef DEBUG_PRINTF
                           std::cout << "Cliente lee (" << leidos << " bytes): " << self->datos.data() << std::endl;
#endif
                           write(self->file, boost::asio::buffer(self->datos, leidos));
                           self->do_write();
                       }
                   });
    }
};

void do_accept(boost::asio::io_context &ioc, boost::asio::ip::tcp::acceptor &acceptor, std::string nombre_fichero)
{
    std::shared_ptr<FileService> servicio = FileService::create(ioc, nombre_fichero);
#ifdef DEBUG_PRINTF
    std::cout << "use_count: " << servicio.use_count() << std::endl;
#endif
    acceptor.async_accept(servicio->socket_conectado,
                          [&ioc, &acceptor, servicio, nombre_fichero](const boost::system::error_code &ec) { // la lambda necesita: &ioc, &acceptor y servicio como copia (se puede porque es shared)
                              if (!ec)
                              {
                                  servicio->do_read();
                              }
                              do_accept(ioc, acceptor, nombre_fichero);
                          });
}

void do_connect(boost::asio::io_context &ioc, const std::string server_address, std::string server_port, std::string nombre_fichero)
{
#ifdef DEBUG_PRINTF
    std::cout << "Cliente descargando " << nombre_fichero << ":\n\tIP: " << server_address << "\n\tPuerto: " << server_port << std::endl;
#endif
    std::ofstream outfile(nombre_fichero);
    if (outfile.is_open())
    {
        outfile.close();
        std::shared_ptr<FileService> servicio = FileService::create(ioc, nombre_fichero);
        // Resolve the host and port
        boost::asio::ip::tcp::resolver resolver(ioc);
        boost::asio::ip::tcp::resolver::query query(server_address, server_port);
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        // Connect to the server
        boost::asio::connect(servicio->socket_conectado, endpoint_iterator);
        servicio->do_write();
    }
    else
    {
        std::cerr << "Error: Failed to create empty file: " << nombre_fichero << std::endl;
    }
}

void hebra_servidora_ficheros(uint16_t puerto, std::string nombre_fichero)
{
#ifdef DEBUG_PRINTF
    std::cout << "[Hebra servidora] Escuchando peticiones en el puerto: " << puerto << std::endl;
#endif
    try
    {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor{io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), puerto)};

        acceptor.listen();
        do_accept(io, acceptor, nombre_fichero);

        io.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void hebra_cliente_ficheros(std::string fichero, std::string dir_cliente)
{
#ifdef DEBUG_PRINTF
    std::cout << "Hebra cliente solicitando descarga en: " << dir_cliente << std::endl;
#endif
    try
    {
        std::size_t double_slash_pos = dir_cliente.find("//");
        std::size_t colon_pos = dir_cliente.find(":", double_slash_pos);
        std::string ip = dir_cliente.substr(double_slash_pos + 2, colon_pos - double_slash_pos - 2);
        std::string port_str = dir_cliente.substr(colon_pos + 1);
        uint16_t port = htons(std::stoi(port_str));

        boost::asio::io_context ioc;
        do_connect(ioc, ip, port_str, fichero);

        ioc.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}