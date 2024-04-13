#ifndef SERVICIO_FICHERO_H
#define SERVICIO_FICHERO_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <fstream>
#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#define PUERTO_CONEXION 22223

using namespace boost::asio;
using namespace boost::asio::ip;



class ServicioFicheros : std::enable_shared_from_this<ServicioFicheros>{

private:
    std::string fichero;
    posix::stream_descriptor file;
    std::array<char, 1024> datos;
public:
    tcp::socket socket_conectado;

    ServicioFicheros(io_context & io_c, std::string nombre_fichero);
    ~ServicioFicheros();
    static std::shared_ptr<ServicioFicheros> create(io_context & io_c, std::string nombre_fichero);
    std::shared_ptr<ServicioFicheros> get_ptr();
    void do_read_from_file_and_write_socket();
    void do_read_from_socket_and_async_write_file();
};

#endif