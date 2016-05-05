#include "server.h"

Server::Server(QObject *parent)
    : QTcpServer(parent)
{
    /* Le dice al server que "escuche" las conexiones de todas las interfaces de red */
    listen(QHostAddress::Any);
}

/*! Conexión entrante, cada que una nueva conexión es detectada, se crea una instancia de la clase Conecction
 * Y se emite la señal de nueva conexión pasando como argumento la conexión recién detectada.
 */
void Server::incomingConnection(int socketDescriptor)
{
    Connection *connection = new Connection(this);
    connection->setSocketDescriptor(socketDescriptor);
    emit newConnection(connection);
}
