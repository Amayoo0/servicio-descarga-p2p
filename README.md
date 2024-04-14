# Servicio de Descarga P2P con Persistencia en Base de Datos

Este proyecto implementa un servicio de descarga peer-to-peer (P2P) utilizando ZeroMQ (zmq) y Boost.Asio en C++. El servidor se encarga de gestionar las conexiones de los clientes y de almacenar los archivos subidos en una base de datos para garantizar la persistencia de los datos.

## Características

- Utiliza el protocolo ZeroMQ para la comunicación entre los clientes y el servidor.
- Se basa en Boost.Asio para la programación asíncrona de E/S y la gestión de sockets.
- Implementa la funcionalidad de lectura y escritura asíncrona de archivos utilizando `async_read` y `async_write`.
- Los archivos descargados se almacenan en una base de datos para garantizar la persistencia de los datos incluso después de que el servidor se reinicie.

## Requisitos

- C++17
- ZeroMQ (librería zmq)
- Boost.Asio

## Instalación

1. Clona el repositorio desde GitHub:

```bash
git clone https://github.com/tu_usuario/servicio-descarga-p2p.git
```
2. Compila el servidor y el cliente:

```bash
cd servicio-descarga-p2p
g++ -I. -o ./builds/cliente cliente.cpp -std=c++17 -lzmq -pthread
g++ -I. -o ./builds/servidor servidor.cpp ./includes/almacen.cpp -std=c++17 -lzmq
```
3. Ejecuta el servidor:
```bash
./builds/servidor
```
4. Ejecuta el cliente:
```bash
./builds/cliente
```

## Uso

1. Ejecuta el servidor en una máquina accesible desde los clientes.
2. Ejecuta el cliente en las máquinas de los usuarios que deseen ofrecer/descargar archivos.
3. El cliente puede enviar solicitudes al servidor (opción 1) para encontrar clientes disponibles para un fichero.
4. El servidor busca el archivo en su base de datos y le envía la dirección IP y puerto.
5. El cliente que desee descargar un fichero (opción 2) establecerá un socket con el cliente registado en el servidor y se realizará la transferencia de fichero.


## Lineas futuras
1. Utilizar un mutex cuando se acceda a memoria compartida (si dos clientes quieren escribir en el almacen)
2. Separar FileService en una clase externa. Problema: Exception: bad_weak_ptr
3. Si un cliente_descarga no está disponible, solicitar otro cliente al servidor.
4. Servicio Crow en otro puerto: pedir los ficheros del servicio-p2p por http.
