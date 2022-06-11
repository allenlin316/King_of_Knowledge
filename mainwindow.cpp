#include "mainwindow.h"
#include "ui_mainwindow.h"
#define MAXLINE 100
#define BraodcastPort 5678
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    //gameStart();
    db = QSqlDatabase::addDatabase("QSQLITE");    
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTotalQuestionNum()
{
    char question_num[10];
    recv(sd, question_num, 10, 0);  // recv total question num
    total_question = atoi(question_num);
}

void MainWindow::gameStart()
{
    player1_total_score = player2_total_score = 0;
    ui->player1_score->setValue(0);
    ui->player2_score->setValue(0);
    ui->timer_lcd->display(0);
    client_ans = -1;
    cur_question = 0;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::setLCDTimer);
}

void MainWindow::gameOver()
{
    closesocket(sd);
    timer->stop();
    ui->stackedWidget->setCurrentIndex(4); // goto gameOVer page
    ui->player_name1->setText(ui->player1_name->text());
    ui->player_name2->setText(ui->player2_name->text());
    ui->player_score1->setText(ui->player1_score->text());
    ui->player_score2->setText(ui->player2_score->text());
    if(player1_total_score > player2_total_score)   ui->game_status->setText(ui->player_name1->text() + " win");
    if(player1_total_score < player2_total_score)   ui->game_status->setText(ui->player_name2->text() + " win");
    if(player1_total_score == player2_total_score)  ui->game_status->setText("平手");
    // recv rank from server by broadcast
    recvRank();
}

void MainWindow::setupGamePage(SOCKET &sd){                                                
        if(cur_question == total_question){
            gameOver();
            return;
        }
        seconds = 10;
        ui->timer_lcd->display(seconds);

        char str[100];
        recv(sd, str, MAXLINE, 0);   // recv question
        ui->question_lbl->setText(charToQString(str)); // set question on the UI
        //memset(str, 0, 100);
        recv(sd, str, MAXLINE, 0);
        ui->choice1_btn->setText(charToQString(str));
        //memset(str, 0, 100);
        recv(sd, str, MAXLINE, 0);
        ui->choice2_btn->setText(charToQString(str));
        //memset(str, 0, 100);
        recv(sd, str, MAXLINE, 0);
        ui->choice3_btn->setText(charToQString(str));
        //memset(str, 0, 100);
        recv(sd, str, MAXLINE, 0);
        ui->choice4_btn->setText(charToQString(str));
        timer->start(1000);
        //char *timer = QStringToChar(QString::number(ui->timer_lcd->value()));
        send(sd, QStringToChar(QString::number(ui->timer_lcd->value())), strlen(QStringToChar(QString::number(ui->timer_lcd->value())))+1, 0);
        cur_question++;
}

void MainWindow::setScore(SOCKET &sd){    
    char player1_score[100], player2_score[100];

    recv(sd, player1_score, 10, 0);
    ui->player1_score->setValue(atoi(player1_score));
    player1_total_score = atoi(player1_score);

    recv(sd, player2_score, 10, 0);
    ui->player2_score->setValue(atoi(player2_score));
    player2_total_score = atoi(player2_score);

    setupGamePage(sd);
}

void MainWindow::recvRank()
{
    SOCKET        	sd;
    struct sockaddr_in serv,cli;
    //char  		str[1024];
    char player_name[MAXLINE], correct_rate[MAXLINE];
    int serv_len, row=1, rank_num=1;
    int flag=1, len=sizeof(int);

    sd=socket(AF_INET, SOCK_DGRAM, 0);
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, len) < 0)
           printf("setsocket() failed");
    bool broadcast = true;
    if(	setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast))<0)
        printf("setsockopt() error!\n");

    cli.sin_family       = AF_INET;
    cli.sin_addr.s_addr  = 0;
    cli.sin_port         = htons(BraodcastPort);
    if( bind(sd, (LPSOCKADDR) &cli, sizeof(cli)) <0 ){
       printf("bind error!\n");
       system("pause");
       return;
    }
    serv_len=sizeof(serv);
    while(1){
        recvfrom(sd, player_name, MAXLINE, 0,(LPSOCKADDR) &serv,&serv_len );
        if(strcmp(player_name, "EOF") == 0) break;
        recvfrom(sd, correct_rate, MAXLINE, 0,(LPSOCKADDR) &serv,&serv_len );
        if(row+1 > ui->tableWidget->rowCount()) // append new row if not enough
            ui->tableWidget->insertRow(ui->tableWidget->rowCount());
        QString name = charToQString(player_name);
        QString correct_rate_qstring = charToQString(correct_rate);
        qDebug() << name << " " << correct_rate;
        QTableWidgetItem *rank_item = new QTableWidgetItem();
        //QTableWidgetItem *correct_rate_item = new QTableWidgetItem();
        rank_item->setData(Qt::EditRole, rank_num);
        //correct_rate_item->setData(Qt::EditRole, correct_rate);
        ui->tableWidget->setItem(row, 0, rank_item);
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(name));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(correct_rate_qstring));
        row++;
        rank_num++;
    }
   //ui->stackedWidget->setCurrentIndex(5);
   closesocket(sd);
   WSACleanup();
}

void MainWindow::on_connect_btn_clicked()
{
    gameStart();
    // setup connection to server and go to game page
    struct sockaddr_in serv;
    char client_name[10];
    WSADATA wsadata;

    WSAStartup(0x101, (LPWSADATA) &wsadata);

    sd = socket(AF_INET, SOCK_STREAM, 0); // create a TCP socket

    //char *ip_str = QStringToChar(ui->server_ip->text());
    qDebug() << "server IP: " << QStringToChar(ui->server_ip->text());
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(QStringToChar(ui->server_ip->text()));
    serv.sin_port = htons(5678);

    if(::connect(sd, (LPSOCKADDR) &serv, sizeof(serv)) == SOCKET_ERROR){
        qDebug() << "error code: " << WSAGetLastError();
        return;
    }
    qDebug() << ui->client_name->text() << " connects to server.";
    //char *client_name_char = QStringToChar(ui->client_name->text());
    send(sd, QStringToChar(ui->client_name->text()), strlen(QStringToChar(ui->client_name->text()))+1, 0);
    qDebug() << "send " << QStringToChar(ui->client_name->text()) << " client name to server";
    ui->player1_name->setText(ui->client_name->text()); // set ur own player name on the game page
    //memset(client_name_char, '\0', 10);
    recv(sd, client_name, MAXLINE, 0);
    ui->player2_name->setText(charToQString(client_name)); // set the other player name on the game page

   ui->stackedWidget->setCurrentIndex(3);

   setTotalQuestionNum();
   setupGamePage(sd);
}

QString MainWindow::charToQString(char *str)
{
    QString qstring = QString::fromUtf8(str, strlen(str)+1);
    return qstring;
}

char* MainWindow::QStringToChar(QString str){
    QByteArray qArr = str.toLatin1(); // to ASCII (QString to QByteArray)
    char *ip_str = qArr.data();
    return ip_str;
}

void MainWindow::on_gameStart_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(2); // switch to connect page
}

void MainWindow::on_choice1_btn_clicked()
{
        char str[10];
        client_ans = 1;         
         strcpy(str, "1");
         send(sd, str, strlen(str)+1, 0);
         setScore(sd);
}

void MainWindow::on_choice2_btn_clicked()
{
        char str[10];
        client_ans = 2;
        strcpy(str, "2");
        send(sd, str, strlen(str)+1, 0);
        setScore(sd);
}

void MainWindow::on_choice3_btn_clicked()
{
        char str[10];
        client_ans = 3;
        strcpy(str, "3");
        send(sd, str, strlen(str)+1, 0);
        setScore(sd);
}

void MainWindow::on_choice4_btn_clicked()
{
        char str[10];
        client_ans = 4;
        strcpy(str, "4");
        send(sd, str, strlen(str)+1, 0);
        setScore(sd);
}

void MainWindow::setLCDTimer()
{
    seconds--;
    if(seconds <= 0)
        setScore(sd);
    ui->timer_lcd->display(seconds);
}

void MainWindow::on_rank_page_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_home_page_btn_clicked()
{
    // go to home page
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_about_btn_clicked()
{
    // go to about game page
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_back_to_home_page_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_back_home_page_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_back_home_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}
