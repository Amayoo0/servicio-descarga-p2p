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

struct FileService: std::enable_shared_from_this<FileService>{  
private:
    std::string fichero;
    posix::stream_descriptor file;
    std::array<char, 1024> datos;
public:
    tcp::socket socket_conectado;

    FileService(io_context & io_c, std::string nombre_fichero): 
        socket_conectado(io_c),
        file(io_c, open(nombre_fichero.c_str(), std::ios::binary))
    { 
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << nombre_fichero << std::endl;
        }
    }
    ~FileService(){
        std::cout << "Destructor\n";
        if( file.is_open() ) 
        {
            file.close();
        }
    }
    
    static std::shared_ptr<FileService> create(io_context & io_c, std::string nombre_fichero){
        return std::make_shared<FileService>(io_c, nombre_fichero);  // solo funciona gracia a enable_shared_from_this
    }
    std::shared_ptr<FileService> get_ptr(){
        return shared_from_this(); //aumenta el contador de referencia. Devuelve un shared_ptr apuntando a lo mismo
    }
    void do_read(){
        async_read(file, buffer(datos),  transfer_at_least(1), 
        [self = shared_from_this()](const boost::system::error_code & ec, std::size_t leidos){
            if (!ec){
                std::cout << "Servidor lee: " << leidos << std::endl;
                write(self->socket_conectado, buffer(self->datos, leidos));
                std::cout << "Servidor escribe!" << std::endl;
                self->do_read();
            }
        });
   }
};

void do_accept(io_context & ioc, tcp::acceptor & acceptor){
    std::shared_ptr<FileService> servicio = FileService::create(ioc, "fichero.txt");
    std::cout << "use_count: " << servicio.use_count() << std::endl;
    acceptor.async_accept(servicio->socket_conectado,
        [&ioc,&acceptor,servicio](const boost::system::error_code & ec){    //la lambda necesita: &ioc, &acceptor y servicio como copia (se puede porque es shared)
          if (!ec){
            servicio->do_read();
          }
          do_accept(ioc, acceptor);
        });
}

int main(int argc, char* argv[]){
    
    try{
        io_context io;
        tcp::acceptor acceptor{io, tcp::endpoint(tcp::v4(), PUERTO_CONEXION)};

        acceptor.listen();
        do_accept(io, acceptor);
      
        io.run();
    }catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    } 
    
    return 0;
}