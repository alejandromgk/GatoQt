#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QMessageBox>
#include <QButtonGroup>
#include "client.h"

const short int BOARDSIZE = 9;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    enum PlayerMark { Cross, Circle, Empty };
    enum StatePlayer { myTurn, oponentTurn };
    enum StateGame {Playing, P1Won, P2Won, NobodyWon, P2Left };

public slots:
    void updateUiBoard();
    void initBoard();
    bool canPlayAtPos(int pos_);
    bool winner();
    void restart();

private slots:
    QString composeGameState();
    void newOponent(const QString &nick);
    void oponentLeft();
    void appendGameState(const QString &message);
    void checkWinner();

private:
    Ui::MainWindow *ui;
    QList<PlayerMark> board; // El tablero se representa como un QList de PlayerMarks.
    StatePlayer playerState;
    StateGame gameState;
    PlayerMark myMark;
    int maxPlay;
    Client client;
    QString myNickName;
    QList<QPushButton*> buttonList;
    QPalette paletteWinner, normalPallete, p1Pallete, p2Pallete;
};

#endif // MAINWINDOW_H
