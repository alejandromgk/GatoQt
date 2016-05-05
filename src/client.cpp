#include "client.h"

Client::Client()
{
    peerManager = new PeerManager(this);
    // El puerto de la clase Server se detecta automáticamente mediante
    // la función predeterminada de Qt serverPort().
    peerManager->setServerPort(server.serverPort());
    peerManager->startBroadcasting();//Comienza la emisión.

    QObject::connect(peerManager, SIGNAL(newConnection(Connection*)),
                     this, SLOT(newConnection(Connection*)));
    QObject::connect(&server, SIGNAL(newConnection(Connection*)),
                     this, SLOT(newConnection(Connection*)));
}

/*!
  Manda el mensaje a los nodos conectados, que están almacenados en peers
*/
void Client::sendMessage(const QString &message)
{
    if (message.isEmpty())
        return;

    QList<Connection *> connections = peers.values();
    foreach (Connection *connection, connections)
        connection->sendMessage(message);
}


/*!
  Método que regresa nuestro nombre de usuario, en el formato username@hostname:puerto
*/
QString Client::nickName() const
{
    return QString(peerManager->userName()) + '@' + QHostInfo::localHostName()
           + ':' + QString::number(server.serverPort());
}

/*!
  Método que comprueba si la ip de quien envía un paquete tiene conexión con nosotros.
*/
bool Client::hasConnection(const QHostAddress &senderIp, int senderPort) const
{
    if (senderPort == -1)
        return peers.contains(senderIp);

    if (!peers.contains(senderIp))
        return false;

    QList<Connection *> connections = peers.values(senderIp);
    foreach (Connection *connection, connections) {
        if (connection->peerPort() == senderPort)
            return true;
    }

    return false;
}

/*!
  Método que es llamado cuando hay una nueva conexión, conecta distintos Slots para comprobar que no hay errores
  de conexión o si está desconectada además si ya está lista para ser usada.
*/
void Client::newConnection(Connection *connection)
{
    connection->setGreetingMessage(peerManager->userName());

    connect(connection, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(connection, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(connection, SIGNAL(readyForUse()), this, SLOT(readyForUse()));
}

/*!
  Actualiza variables, como las señales una vez que la conexión está lista para ser usada.
*/
void Client::readyForUse()
{
    Connection *connection = qobject_cast<Connection *>(sender());
    if (!connection || hasConnection(connection->peerAddress(),
                                     connection->peerPort()))
        return;

    connect(connection, SIGNAL(newMessage(QString)),
            this, SIGNAL(newMessage(QString)));

    peers.insert(connection->peerAddress(), connection);
    QString nick = connection->name();
    if (!nick.isEmpty())
        emit newOponent(nick);
}

/*!
  Método que emite la señal de desconectado
*/
void Client::disconnected()
{
    if (Connection *connection = qobject_cast<Connection *>(sender()))
        removeConnection(connection);
}

void Client::connectionError(QAbstractSocket::SocketError /* socketError */)
{
    if (Connection *connection = qobject_cast<Connection *>(sender()))
        removeConnection(connection);
}

void Client::removeConnection(Connection *connection)
{
    if (peers.contains(connection->peerAddress())) {
        peers.remove(connection->peerAddress());
        emit oponentLeft();
    }
    connection->deleteLater();
}
