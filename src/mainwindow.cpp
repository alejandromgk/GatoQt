#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* Paleta de colores para los botones, dependiendo del jugador */
    paletteWinner.setColor(QPalette::ButtonText, Qt::black);
    paletteWinner.setColor(QPalette::Button, Qt::green);
    normalPallete.setColor(QPalette::ButtonText, Qt::black);
    normalPallete.setColor(QPalette::Button, Qt::white);
    p1Pallete.setColor(QPalette::ButtonText, Qt::black);
    p1Pallete.setColor(QPalette::Button, Qt::red);
    p2Pallete.setColor(QPalette::ButtonText, Qt::black);
    p2Pallete.setColor(QPalette::Button, Qt::blue);

    /* QList que contiene los botones de la ui para poder trabajar con ellos */
    buttonList.append(ui->pushButton);
    buttonList.append(ui->pushButton_2);
    buttonList.append(ui->pushButton_3);
    buttonList.append(ui->pushButton_4);
    buttonList.append(ui->pushButton_5);
    buttonList.append(ui->pushButton_6);
    buttonList.append(ui->pushButton_7);
    buttonList.append(ui->pushButton_8);
    buttonList.append(ui->pushButton_9);

    for (int i=0;i<BOARDSIZE;i++){
        buttonList.at(i)->setDisabled(true);
        connect(buttonList.at(i), SIGNAL(clicked()), this, SLOT(updateUiBoard()));
        buttonList.at(i)->setPalette (normalPallete);
    }

    /* Conexiones señales-slots */
    connect(&client, SIGNAL(newMessage(QString)), this, SLOT(appendGameState(QString)));
    connect(&client, SIGNAL(newOponent(QString)), this, SLOT(newOponent(QString)));
    connect(&client, SIGNAL(oponentLeft()), this, SLOT(oponentLeft()));

    myNickName = client.nickName();
    ui->label_P1H->setText (myNickName);
    initBoard();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*!
 *Actualiza el tablero y el estado del juego cuando se presiona un botón
 */
void MainWindow::updateUiBoard()
{
    if(playerState == myTurn){
        for (int i=0; i<BOARDSIZE;i++ ){
            if(buttonList.at(i) == dynamic_cast<QPushButton*>(sender())){
                if(canPlayAtPos(i)){
                    if(myMark==Cross){
                        maxPlay++;
                        ui->label->setText ("Turno de tu oponente");
                        buttonList.at(i)->setText ("X");
                        buttonList.at(i)->setPalette (p1Pallete);
                        ui->label_Mark->setText ("'X'");
                        playerState = oponentTurn;
                        board.replace(i, Cross);
                        client.sendMessage(composeGameState());
                    } else {
                        maxPlay++;
                        buttonList.at(i)->setText("O");
                        buttonList.at(i)->setPalette (p2Pallete);
                        ui->label_Mark->setText ("'O'");
                        playerState = oponentTurn;
                        ui->label->setText ("Turno de tu oponente");
                        board.replace(i, Circle);
                        client.sendMessage(composeGameState());
                    }
                }
            }
        }
    }
    checkWinner();
}

/*!
 * Inicializa el trablero y otras variables.
 */
void MainWindow::initBoard()
{
    playerState = myTurn;
    myMark=Cross;
    gameState = Playing;
    for (int i =0; i<BOARDSIZE;i++){
        board.append(Empty);
    }
    maxPlay=4;
}

bool MainWindow::canPlayAtPos(int pos)
{
    return board.at(pos) == Empty;
}

/*!
 * Revisa las combinaciones posibles de gane en busca de un ganador.
 */
bool MainWindow::winner()
{
    for (int i=0; i<3; ++i) {
        if (board[i] != Empty && board[i] == board[i+3] && board[i] == board[i+6]){
            buttonList.at(i)->setPalette(paletteWinner);
            buttonList.at(i+3)->setPalette(paletteWinner);
            buttonList.at(i+6)->setPalette(paletteWinner);
            return true;
        }

        if (board[i*3] != Empty && board[i*3] == board[i*3+1] && board[i*3] == board[i*3+2]){
            buttonList.at(i*3)->setPalette(paletteWinner);
            buttonList.at(i*3+1)->setPalette(paletteWinner);
            buttonList.at(i*3+2)->setPalette(paletteWinner);
            return true;
        }
    }

    if (board[0] != Empty && board[0] == board[4] && board[0] == board[8]){
        buttonList.at(0)->setPalette(paletteWinner);
        buttonList.at(4)->setPalette(paletteWinner);
        buttonList.at(8)->setPalette(paletteWinner);
        return true;
    }

    if (board[2] != Empty && board[2] == board[4] && board[2] == board[6]){
        buttonList.at(2)->setPalette(paletteWinner);
        buttonList.at(4)->setPalette(paletteWinner);
        buttonList.at(6)->setPalette(paletteWinner);
        return true;
    }

    return false;
}
/*!
 * El loop del juego, cuando hay un ganador, empate o el oponente ha abandonado
 * reinicia el juego para comenzar otra partida o hasta que un oponente entra a jugar
 */
void MainWindow::restart()
{
    QMessageBox info;
    QString msgboxtext;

    if(gameState==P1Won)
        msgboxtext="Haz ganado";
    if(gameState==P2Won)
        msgboxtext="Tu oponente ha ganado";
    if(gameState==NobodyWon)
        msgboxtext="El juego ha terminado en empate";
    if(gameState==P2Left)
        msgboxtext="El oponente ha abandonado";

    //gameState = Playing;

    info.setText(msgboxtext);
    info.setIcon (QMessageBox::Information);
    info.show();
    QPoint pos = mapToGlobal(QPoint( width()/2  - info.width()/2, height()/2 - info.height()/2 ));
    info.move(pos);
    info.exec();

    board.clear ();
    initBoard ();

    for (int i=0;i<BOARDSIZE;i++){
        buttonList.at(i)->setText("");
        buttonList.at(i)->setPalette (normalPallete);
    }
    ui->label->setText ("A jugar!");
    ui->label_Mark->setText ("...");

}

/*!
 * Compone el mensaje que se va a enviar de a cuerdo al estado actual del juego
 */
QString MainWindow::composeGameState()
{
    QString gState;

    if(gameState==Playing){
        gState.append("P");
    } else if(gameState==P1Won){
        gState.append("1");
    } else if(gameState==P2Won){
        gState.append("2");
    } else {
        gState.append("N");
    }

    if(myMark==Circle){
        gState.append("C");
    } else {
        gState.append("E");
    }


    foreach(PlayerMark Mark, board){
        if(Mark==Cross){
            gState.append ("X");
        } else if (Mark==Circle){
            gState.append ("O");
        } else {
            gState.append ("-");
        }
    }
    qDebug()<<"compose GS - gState"<<gState;
    return gState;
}

/*!
 * Decodifica el mensaje recibido y actualiza el estado del juego como corresponde.
 */
void MainWindow::appendGameState(const QString &message)
{
    /* Formato mensaje: {1/2/N/P}{E/C}{Tablero}
     * Ej: 'PE--XX--O-O' Donde P indica que aún se está jugando (P para Playing, N nadie ganó,
     * 1 y 2 determinan quién fue el ganador), E que el jugador enviando el mensaje juega
     * con el símbolo X (E = X, C = O) y '--XX--O-O' Es el estado actual del tablero.
     */

    playerState = myTurn;

    /* Lee el tablero actualizado con el movimiento del contrincante recién hecho */
    ui->label->setText ("Tu turno");
    int index = 0;
    for (int i = 2; i< BOARDSIZE+2; i++ ){
        if(message.at(i) == 'X'){
            board.replace(index, Cross);
            buttonList.at(index)->setText ("X");
            buttonList.at(index)->setPalette (p1Pallete);
        } else if (message.at(i)=='O') {
            board.replace(index, Circle);
            buttonList.at(index)->setText ("O");
            buttonList.at(index)->setPalette (p2Pallete);
        } else {
            board.replace(index, Empty);
            buttonList.at(index)->setText ("");
            buttonList.at(index)->setPalette (normalPallete);
        }
        index++;
    }

    /* Actualiza el símbolo con el que jugamos */
    if(message.at(1)=='E'){
        myMark=Circle;
        ui->label_Mark->setText ("'O'");
    } else {
        myMark=Cross;
        ui->label_Mark->setText ("'X'");
    }

    /* Actualiza el estado del juego y comprueba si hay algún ganador */
    if(message.at(0)=='P'){
        gameState=Playing;
    } else if(message.at(0)=='1'){
        gameState=P2Won;
        ui->label->setText ("Tu oponente ha ganado");
        winner();
        restart();
    } else if(message.at(0)=='2'){
        gameState=P1Won;
        ui->label->setText ("Haz ganado!");
        winner();
        restart();
    } else {
        gameState=NobodyWon;
        winner();
        ui->label->setText ("Juego empatado");
        restart();
    }
}

/*!
 * Actualiza variables, la ui y reinicia el juego si encuentra un ganador
 */
void MainWindow::checkWinner()
{
    if(winner()){
        if(playerState==myTurn){

            gameState = P2Won;
            ui->label->setText ("Tu oponente ha ganado");
            client.sendMessage(composeGameState());
            restart ();
        }
        else{

            gameState = P1Won;
            ui->label->setText ("Haz ganado");
            client.sendMessage(composeGameState());
            restart ();

        }
    } else {
        if(maxPlay==BOARDSIZE){
            gameState = NobodyWon;
            ui->label->setText ("Juego empatado");
            client.sendMessage(composeGameState());
            restart ();
        }
    }
}

/*!
 * Slot que se activa cuando se emite la señal un nuevo oponente en la red
 */
void MainWindow::newOponent(const QString &nick)
{
    ui->label_P2H->setText(nick);
    ui->label->setText ("Oponente encontrado!");
    for (int i=0;i<BOARDSIZE;i++){
        buttonList.at(i)->setEnabled(true);
    }
}

/*!
 *Slot que actualiza el juego cuando el oponente se desconecta
 */
void MainWindow::oponentLeft()
{
    ui->label_P2H->setText ("-");
    for (int i=0;i<BOARDSIZE;i++){
        buttonList.at(i)->setDisabled(true);
    }
    gameState=P2Left;
    restart();
    ui->label->setText ("Buscando oponente...");
}
