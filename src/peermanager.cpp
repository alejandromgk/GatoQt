#include "peermanager.h"


static const qint32 BroadcastInterval = 2000;
static const unsigned broadcastPort = 45000;

/*
 * El constructor recibe una instancia de la clase cliente
*/

PeerManager::PeerManager(Client *client)
    : QObject(client)
{
    this->client = client;

/*
  Busca dentro de las variables de entorno los campos señalados para fines de identificacion
*/
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";

    QStringList environment = QProcess::systemEnvironment();
    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2) {
                username = stringList.at(1).toUtf8();
                break;
            }
        }
    }

    if (username.isEmpty())
        username = "RandomPlayer";

    updateAddresses();
    serverPort = 0;

    broadcastSocket.bind(QHostAddress::Any, broadcastPort, QUdpSocket::ShareAddress
                         | QUdpSocket::ReuseAddressHint);
    connect(&broadcastSocket, SIGNAL(readyRead()) ,
            this, SLOT(readBroadcastDatagram()));

/*
  Comienza un timmer que va a conectar el slot que va a difundir el datagrama a la red cada 2 segundos
*/
    broadcastTimer.setInterval(BroadcastInterval);
    connect(&broadcastTimer, SIGNAL(timeout()),
            this, SLOT(sendBroadcastDatagram()));
}

/*!
  Puerto del servidor anteriormente creado
*/
void PeerManager::setServerPort(int port)
{
    serverPort = port;
}

/*!
  Regresa el nombre de usuario.
*/
QByteArray PeerManager::userName() const
{
    return username;
}

/*!
 * Comienza la emición del datagrama cada 2 segundo para que sea detectado por otra instancia
 */
void PeerManager::startBroadcasting()
{
    broadcastTimer.start();
}

/*!
 * Filtra las direcciones en la variable ipAddresses para buscar solo por IPs locales.
 */

bool PeerManager::isLocalHostAddress(const QHostAddress &address)
{
    foreach (QHostAddress localAddress, ipAddresses) {
        if (address == localAddress)
            return true;
    }
    return false;
}

/*!
 * Genera un datagrama válido para la aplicación en el formato usuario@puerto y lo difunde en la red
 * con el fin de que otra instancia del programa la detecte con readBroadcastDatagram()
 * para poder estrablecer la conexión entre los nodos.
 */

void PeerManager::sendBroadcastDatagram()
{
    QByteArray datagram(username);
    datagram.append('@');
    datagram.append(QByteArray::number(serverPort));

    bool validBroadcastAddresses = true;
    foreach (QHostAddress address, broadcastAddresses) {
        if (broadcastSocket.writeDatagram(datagram, address,
                                          broadcastPort) == -1)
            validBroadcastAddresses = false;
    }

    if (!validBroadcastAddresses)
        updateAddresses();
}

/*!
 * Lee la emisión de datagramas (usuario/ip + puerto) en la red y comprueba si es tiene el formato
 * que lo identifica como un nodo de este progrma
 */
void PeerManager::readBroadcastDatagram()
{
    while (broadcastSocket.hasPendingDatagrams()) {
        QHostAddress senderIp;
        quint16 senderPort;
        QByteArray datagram;
        datagram.resize(broadcastSocket.pendingDatagramSize());
        // Guarda la información del datagrama pendiente, la ip y el puerto de quien lo envía.
        // Y comprueba que el valor de retorno sea -1, de serlo hubo un fallo en la recepción
        if (broadcastSocket.readDatagram(datagram.data(), datagram.size(),
                                         &senderIp, &senderPort) == -1)
            continue;
        // Comprueba que el datagrama tenga el formato usuario@puerto para saber si es un nodo de esta aplicación.
        QList<QByteArray> list = datagram.split('@');
        if (list.size() != 2)
            continue;

        //Que no sea esta instancia
        int senderServerPort = list.at(1).toInt();
        if (isLocalHostAddress(senderIp) && senderServerPort == serverPort)
            continue;

        // Una vez comprobado que es un nodo de este programa, se crea la conexión con él
        // y se emite la señal de nueva conexión y se detiene la búsqueda de otros jugadores
        if (!client->hasConnection(senderIp)) {
            Connection *connection = new Connection(this);
            emit newConnection(connection);
            connection->connectToHost(senderIp, senderServerPort);
        }
    }
}

/*!
 * Actualiza la lista de ips y puertos disponibles en las distintas interfaces de red del equipo
 */
void PeerManager::updateAddresses()
{
    broadcastAddresses.clear();
    ipAddresses.clear();
    foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
        foreach (QNetworkAddressEntry entry, interface.addressEntries()) {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null && entry.ip() != QHostAddress::LocalHost) {
                broadcastAddresses << broadcastAddress;
                ipAddresses << entry.ip();
            }
        }
    }
}
