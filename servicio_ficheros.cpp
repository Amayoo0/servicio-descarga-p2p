#include "servicio_ficheros.h"


ServicioFicheros::ServicioFicheros(io_context & io_c, std::string nombre_fichero): 
    socket_conectado(io_c),
    file(io_c, open(nombre_fichero.c_str(), std::ios::out | std::ios::binary))
{ 
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << nombre_fichero << std::endl;
    }
}
ServicioFicheros::~ServicioFicheros(){
    std::cout << "Destructor\n";
    if( file.is_open() ) 
    {
        file.close();
    }
}

std::shared_ptr<ServicioFicheros> ServicioFicheros::create(io_context & io_c, std::string nombre_fichero){
    return std::make_shared<ServicioFicheros>(io_c, nombre_fichero);  // solo funciona gracia a enable_shared_from_this
}
std::shared_ptr<ServicioFicheros> ServicioFicheros::get_ptr(){
    return shared_from_this(); //aumenta el contador de referencia. Devuelve un shared_ptr apuntando a lo mismo
}
void ServicioFicheros::do_read_from_file_and_write_socket(){
    async_read(file, buffer(datos),  transfer_at_least(1), 
    [self = shared_from_this()](const boost::system::error_code & ec, std::size_t leidos){
        if (!ec){
            std::cout << "Servidor lee: " << leidos << std::endl;
            write(self->socket_conectado, buffer(self->datos, leidos));
            std::cout << "Servidor escribe!" << std::endl;
            self->do_read_from_file_and_write_socket();
        }
    });
}

void ServicioFicheros::do_read_from_socket_and_async_write_file() {
    async_read(socket_conectado, buffer(datos), transfer_at_least(1),
        [self = shared_from_this()](const boost::system::error_code& ec, std::size_t leidos) {
            if (!ec) {
                std::cout << "Servidor lee: " << leidos << " bytes" << std::endl;
                async_write(self->file, buffer(self->datos, leidos),
                    [self](const boost::system::error_code& ec, std::size_t) {
                        if (!ec) {
                            std::cout << "Servidor escribe!" << std::endl;
                            self->do_read_from_socket_and_async_write_file();
                        }
                        else {
                            std::cerr << "Error de escritura en el archivo: " << ec.message() << std::endl;
                        }
                    });
            }
            else {
                std::cerr << "Error de lectura del socket: " << ec.message() << std::endl;
            }
        });
}