#include "connection.h"

static const int TransferTimeout = 30 * 1000;
static const int PongTimeout = 60 * 1000;
static const int PingInterval = 5 * 1000;
static const char SeparatorToken = ' ';

Connection::Connection(QObject *parent)
    : QTcpSocket(parent)
{
    greetingMessage = tr("undefined");
    username = tr("unknown");
    state = WaitingForGreeting;
    currentDataType = Undefined;
    numBytesForCurrentDataType = -1;
    transferTimerId = 0;
    isGreetingMessageSent = false;
    pingTimer.setInterval(PingInterval);

    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(processReadyRead()));
    QObject::connect(this, SIGNAL(disconnected()), &pingTimer, SLOT(stop()));
    QObject::connect(&pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));
    QObject::connect(this, SIGNAL(connected()),
                     this, SLOT(sendGreetingMessage()));
}

/*!
 * Regresa el nombre de usuario del nodo conectado (No el nuestro)
*/
QString Connection::name() const
{
    return username;
}

/*!
 Asigna nuestro nombre de usuario al primer mensaje que se enviará.
*/
void Connection::setGreetingMessage(const QString &message)
{
    greetingMessage = message; // usuario
}

/*!
 * Escribe el mensaje al flujo de datos de la conexión.
 */
bool Connection::sendMessage(const QString &message)
{
    if (message.isEmpty())
        return false;

    QByteArray msg = message.toUtf8();
    QByteArray data = "MESSAGE " + QByteArray::number(msg.size()) + ' ' + msg;
    qDebug()<<"sendMessage:"<<data;
    return write(data) == data.size();
}

/*!
  Crea un temporizador para controlar el tiempo de tranferencia no pase de un valor determinado
 */
void Connection::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == transferTimerId) {
        abort();
        killTimer(transferTimerId);
        transferTimerId = 0;
    }
}

/*!
  Asegura que la conexión está lista para ser usada, y por medio del mensaje de saludo Greeting
  se guarda el hostname del nodo que se acaba de conectar en la variable username
 */
void Connection::processReadyRead()
{
    if (state == WaitingForGreeting) {
        if (!readProtocolHeader())
            return;
        if (currentDataType != Greeting) {
            abort();
            return;
        }
        state = ReadingGreeting;
    }

    if (state == ReadingGreeting) {
        if (!hasEnoughData())
            return;

        buffer = read(numBytesForCurrentDataType);
        if (buffer.size() != numBytesForCurrentDataType) {
            abort();
            return;
        }

        username = QString(buffer) + '@' + peerAddress().toString() + ':'
                   + QString::number(peerPort());
        currentDataType = Undefined;
        numBytesForCurrentDataType = 0;
        buffer.clear();

        if (!isValid()) {
            abort();
            return;
        }

        if (!isGreetingMessageSent)
            sendGreetingMessage();

        pingTimer.start();
        pongTime.start();
        state = ReadyForUse;
        emit readyForUse();
    }

    do {
        if (currentDataType == Undefined) {
            if (!readProtocolHeader())
                return;
        }
        if (!hasEnoughData())
            return;
        processData();
    } while (bytesAvailable() > 0);
}

/*!
  Manda un mensaje de ping y comprueba si el tiempo de respuesta (Pong) es mayor
  al definido, de ser así, manda de nuevo el mensaje de Ping.
 */
void Connection::sendPing()
{
    if (pongTime.elapsed() > PongTimeout) {
        abort();
        return;
    }

    write("PING 1 p");
}

/*!
  Manda nuestro usuario en forma de primer mensaje con fines de identificación.
 */
void Connection::sendGreetingMessage()
{
    QByteArray greeting = greetingMessage.toUtf8();
    QByteArray data = "GREETING " + QByteArray::number(greeting.size()) + ' ' + greeting;
    //qDebug()<<"sendGretingMsg"<<data;
    if (write(data) == data.size())
        isGreetingMessageSent = true;
}

/*!
  Descifra si es un mensaje del tipo MESSAGE, PING, PONG o GREETING,
  Y con ayuda de dataLengthForCurrentDataType() se obtiene el tamaño
  del mensaje para leerlo del buffer
 */
int Connection::readDataIntoBuffer(int maxSize)
{
    if (maxSize > MaxBufferSize)
        return 0;

    int numBytesBeforeRead = buffer.size();
    if (numBytesBeforeRead == MaxBufferSize) {
        abort();
        return 0;
    }

    /* Una vez usado read(), los bytes leídos dejan de estar disponibles
     * En una primera iteración leerá el tipo del mensaje, ej. 'MESSAGE '
     * Posteriormente, cuando de llame de nuevo la función leerá el número
     * de bytes del mensaje en sí, ej. '5 ' porque la primera parte ('MESSAGE ')
     * ya no está disponible en el buffer.
     */
    while (bytesAvailable() > 0 && buffer.size() < maxSize) {
        buffer.append(read(1));
        if (buffer.endsWith(SeparatorToken))
            break;
    }
    return buffer.size() - numBytesBeforeRead;
}

/*!
  Regresa el número de bytes a leer del mensaje en sí.
 */
int Connection::dataLengthForCurrentDataType()
{
    if (bytesAvailable() <= 0 || readDataIntoBuffer() <= 0
            || !buffer.endsWith(SeparatorToken))
        return 0;

    // Se elminia el espacio final del mensaje para obtener solo el número
    // de bytes que nos interesa leer del buffer
    buffer.chop(1);
    int number = buffer.toInt();

    buffer.clear();
    return number;
}

/*!
  Lee el tipo de mensaje que está en el buffer y mediante dataLengthForCurrentDataType()
  detecta el número de bytes a leer del mensaje.
*/
bool Connection::readProtocolHeader()
{
    if (transferTimerId) {
        killTimer(transferTimerId);
        transferTimerId = 0;
    }

    if (readDataIntoBuffer() <= 0) {
        transferTimerId = startTimer(TransferTimeout);
        return false;
    }

    if (buffer == "PING ") {
        currentDataType = Ping;
    } else if (buffer == "PONG ") {
        currentDataType = Pong;
    } else if (buffer == "MESSAGE ") {
        currentDataType = PlainText;
    } else if (buffer == "GREETING ") {
        currentDataType = Greeting;
    } else {
        currentDataType = Undefined;
        abort();
        return false;
    }

    buffer.clear();
    numBytesForCurrentDataType = dataLengthForCurrentDataType();
    return true;
}

/*!
  Comprueba que hay suficientes bytes en el buffer para trabajar
 */
bool Connection::hasEnoughData()
{
    if (transferTimerId) {
        QObject::killTimer(transferTimerId);
        transferTimerId = 0;
    }

    if (numBytesForCurrentDataType <= 0)
        numBytesForCurrentDataType = dataLengthForCurrentDataType();

    if (bytesAvailable() < numBytesForCurrentDataType
            || numBytesForCurrentDataType <= 0) {
        transferTimerId = startTimer(TransferTimeout);
        return false;
    }

    return true;
}

/*!
  En este punto ya tenemos el tamaño de bytes que tiene el mensaje, ahora solo queda
  leerlo y emitir la señal de nuevo Mensaje pasándole como argumento el buffer con el mensaje
  para que lo procese el motor del juego.
 */
void Connection::processData()
{
    buffer = read(numBytesForCurrentDataType);
    if (buffer.size() != numBytesForCurrentDataType) {
        abort();
        return;
    }

    switch (currentDataType) {
    case PlainText:
        emit newMessage(QString::fromUtf8(buffer));
        break;
    case Ping:
        write("PONG 1 p");
        break;
    case Pong:
        pongTime.restart();
        break;
    default:
        break;
    }

    currentDataType = Undefined;
    numBytesForCurrentDataType = 0;
    buffer.clear();
}
