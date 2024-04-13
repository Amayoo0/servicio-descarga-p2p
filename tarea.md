[Clase 03/04] 

sustituir accept por un bind

hacer el acept del tutorial y luego un close

.async_accept(socket, [con = newconnection](const boost::system::error_code &err){
    aquí va mi codigo
    con->start();
})

[Clase 10/04]
g++ -I ./asio-1.28.0/include -I. -o ejemplo ejemplo.cpp

2preguntas de http
preguntas sobre 


Ampliación con crow en otro puerto: pedir los ficheros del torrent por http.
Ampliación ASIO async.





WebSocket. 
El codigo websocket_basisco.html está en el servidor en /template y cuando el cliente accede a este fichero 
se lo descarga y con un moustache en la ip para que el servidor le devuelva la variable con su propia ip.



### Lineas futuras
1. Utilizar un mutex cuando se acceda a memoria compartida (si dos clientes quieren escribir en el almacen)