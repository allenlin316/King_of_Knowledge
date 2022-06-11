#include <QCoreApplication>
#include <stdio.h>
#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDriver>
#include <vector>
#include <time.h>
#include <string>
#include <winsock2.h>
#define MAXLINE 100
#define BraodcastPort 5678
using namespace std;

QSqlDatabase db;
const int question_num = 5;
double player1_correct_num=0, player2_correct_num=0;
char player1[100], player2[100];
char timer[10];
int player1_total_score=0, player2_total_score=0;
vector<QString> questions_vec;
vector<vector<QString>> choices_vec;
vector<QString> answers;
void initializeServer();    // 初始化 server，讓 server 可以重複執行
void gameRoom();    // 遊戲主程式碼
void checkGameStatus(int q_index, SOCKET &clnt_sd, SOCKET &clnt_sd2);   // 判斷玩家答案
void setQuestionsAndChoices();  // 從資料庫抓題目選項與答案
void setScoreToDB();    // 把玩家成績儲存到資料庫 Rank
void sendRankToClient();    // 把資料庫 Rank 的所有成績傳給玩家
char* QStringToChar(QString str);   // 從 QString 轉換成 char 陣列

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // setup the Questions db
    db = QSqlDatabase::addDatabase("QSQLITE");     
    while(1){
        initializeServer();
        gameRoom();
        Sleep(500);
    }
    system("pause");
    return a.exec();
}

void initializeServer(){
    player1_correct_num=0, player2_correct_num=0;
    player1_total_score=0, player2_total_score=0;
    memset(player1, 0, sizeof(player1));
    memset(player2, 0, sizeof(player2));
    memset(timer, 0, sizeof(timer));
    questions_vec.clear();
    choices_vec.clear();
    answers.clear();
}

void gameRoom(){
    // connect two players to the server
    SOCKET sd, clnt_sd, clnt_sd2;
    struct sockaddr_in serv, clnt;
    int q_index=0;
    char total_question[10];
    WSADATA wsadata;

    WSAStartup(0x101, &wsadata);
    sd = socket(AF_INET, SOCK_STREAM, 0);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(5678);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(sd, (struct sockaddr *) &serv, sizeof(serv));

    listen(sd, 5);
    int clnt_len = sizeof(clnt);
    printf("Server waits.\n");
    clnt_sd = accept(sd, (struct sockaddr *) &clnt, &clnt_len);
    recv(clnt_sd, player1, 100, 0); // recv client name
    printf("%s is connected\n", player1);
    clnt_sd2 = accept(sd, (struct sockaddr *) &clnt, &clnt_len);
    recv(clnt_sd2, player2, 100, 0);
    printf("%s is connected\n", player2);
    send(clnt_sd, player2, strlen(player2)+1, 0);
    send(clnt_sd2, player1, strlen(player1)+1, 0);
    printf("send %s to client %s\n", player2, player1);
    printf("send %s to client %s\n", player1, player2);
    setQuestionsAndChoices();   // retrieve questions and answers from database

    std::string question_num_string = std::to_string(question_num);
    const char* c = question_num_string.c_str();
    strcpy(total_question, c);
    send(clnt_sd, total_question, strlen(total_question)+1, 0);     // send clients question num
    send(clnt_sd2, total_question, strlen(total_question)+1, 0);
    while(q_index < question_num){
        QString q_tmp = questions_vec[q_index];
        qDebug() << q_tmp;
        //char *question = QStringToChar(q_tmp);
        qDebug() << QStringToChar(q_tmp);
        send(clnt_sd, QStringToChar(q_tmp), strlen(QStringToChar(q_tmp))+1, 0); // send question to client
        send(clnt_sd2, QStringToChar(q_tmp), strlen(QStringToChar(q_tmp))+1, 0);

        for(int i=0; i<4; i++){ // send four choices to clients
            QString choice_tmp = choices_vec[q_index][i];
            //char *choice = QStringToChar(choice_tmp);
            qDebug() << QStringToChar(choice_tmp);
            send(clnt_sd, QStringToChar(choice_tmp), strlen(QStringToChar(choice_tmp))+1, 0);
            send(clnt_sd2, QStringToChar(choice_tmp), strlen(QStringToChar(choice_tmp))+1, 0);
            Sleep(200);
        }
        // recv timer count from client
        recv(clnt_sd, timer, 10, 0);
        recv(clnt_sd2, timer, 10, 0);
        qDebug() << "recv time from client" << timer;
        // recv answers from clients
        checkGameStatus(q_index, clnt_sd, clnt_sd2);
        q_index++;
    }    
    // set player score into database
    setScoreToDB();
    sendRankToClient();
    closesocket(clnt_sd);
    closesocket(clnt_sd2);
    closesocket(sd);
    WSACleanup();
}

void checkGameStatus(int q_index, SOCKET &clnt_sd, SOCKET &clnt_sd2){
    bool first_to_ans = true, player1_done=false, player2_done=false;
    fd_set readfds; // for select()
    int activity;
    TIMEVAL timeout;
    char player1_ans[10], player2_ans[10], player1_score[10], player2_score[10];
    strcpy(player1_ans, "");    strcpy(player2_ans, ""); // initialize
    int count_down = atoi(timer);


    while(count_down--){
        qDebug() << "remaining time: " << count_down;
        qDebug() << "clear the socket fd set";
        FD_ZERO(&readfds);
        qDebug() << "add 2 client to fd set";
        FD_SET(clnt_sd, &readfds);
        FD_SET(clnt_sd2, &readfds);
        qDebug() << "call select()";
        timeout.tv_sec = count_down;
        timeout.tv_usec = 0;
        activity = select(0, &readfds, NULL, NULL, &timeout);

        if(activity == 0){
            // no answer from clients
            qDebug() << "activity=0";
            if(!player1_done || !player2_done){
                send(clnt_sd, QStringToChar(QString::number(player1_total_score)), strlen(QStringToChar(QString::number(player1_total_score)))+1, 0);
                send(clnt_sd, QStringToChar(QString::number(player2_total_score)), strlen(QStringToChar(QString::number(player2_total_score)))+1, 0);
                send(clnt_sd2, QStringToChar(QString::number(player2_total_score)), strlen(QStringToChar(QString::number(player2_total_score)))+1, 0);
                send(clnt_sd2, QStringToChar(QString::number(player1_total_score)), strlen(QStringToChar(QString::number(player1_total_score)))+1, 0);
            }
            break;
        }

        if ( activity == SOCKET_ERROR ){
                qDebug() << "select call failed with error code: " << WSAGetLastError();
                exit(EXIT_FAILURE);
         }

        if(FD_ISSET(clnt_sd, &readfds)){
            recv(clnt_sd, player1_ans, 10, 0);
            qDebug() << "recv " << player1_ans << " from player1";
        }
        if(FD_ISSET(clnt_sd2, &readfds)){
             recv(clnt_sd2, player2_ans, 10, 0);
             qDebug() << "recv " << player2_ans << " from player2";
        }

        if(QString(player1_ans) == answers[q_index] && !player1_done){    // answer is correct
            if(first_to_ans){
                // got 10 points
                player1_total_score += 10;
                strcpy(player1_score, "10");
                player1_correct_num++;
                first_to_ans = false;
            }
            else{
                // got 5 points
                player1_total_score += 5;
                player1_correct_num++;
                strcpy(player1_score, "5");
            }
            printf("client %s answer is correct\n", player1);
            player1_done = true;
        }
        else {
            if(strcmp(player1_ans, "") != 0){
                printf("wrong answer from %s\n", player1);
                player1_done = true;
            }
        }
        if(QString(player2_ans) == answers[q_index] && !player2_done){
            if(first_to_ans){
                // got 10 points
                player2_total_score += 10;
                strcpy(player2_score, "10");
                first_to_ans = false;
                player2_correct_num++;
            }
            else{
                // got 5 points
                player2_total_score += 5;
                player2_correct_num++;
                strcpy(player2_score, "5");
            }
            printf("client %s answer is correct\n", player2);
            player2_done = true;
        }
        else{
            if(strcmp(player2_ans, "") != 0){
                printf("wrong answer from %s\n", player2);
                player2_done = true;
            }
        }
        if(player1_done && player2_done){
            //char *tmp2_score = QStringToChar(QString::number(player2_total_score));
           // char *tmp1_score = QStringToChar(QString::number(player1_total_score));
            send(clnt_sd, QStringToChar(QString::number(player1_total_score)), strlen(QStringToChar(QString::number(player1_total_score)))+1, 0);
            send(clnt_sd, QStringToChar(QString::number(player2_total_score)), strlen(QStringToChar(QString::number(player2_total_score)))+1, 0);
            send(clnt_sd2, QStringToChar(QString::number(player2_total_score)), strlen(QStringToChar(QString::number(player2_total_score)))+1, 0);
            send(clnt_sd2, QStringToChar(QString::number(player1_total_score)), strlen(QStringToChar(QString::number(player1_total_score)))+1, 0);
            break;
        }
    }
    //qDebug() << "10 sec time is over";
}

void setQuestionsAndChoices(){
    db.setDatabaseName("Questions.db");
    if(db.open())
        printf("Database connection success\n");
    else{
        printf("Database connection failed");
        return;
    }
    QSqlQuery query;
    query.exec("SELECT question FROM Questions"); // get all the questions from db
    while(query.next()){
        QString question = query.value(0).toString();
        questions_vec.push_back(question);
    }
    query.exec("SELECT * FROM Questions");
    while(query.next()){
        QString choice1 = query.value(2).toString(); // choice 1
        QString choice2 = query.value(3).toString();
        QString choice3 = query.value(4).toString();
        QString choice4 = query.value(5).toString();
        vector<QString> choice_tmp;
        choice_tmp.push_back(choice1);
        choice_tmp.push_back(choice2);
        choice_tmp.push_back(choice3);
        choice_tmp.push_back(choice4);
        choices_vec.push_back(choice_tmp);
    }
    query.exec("SELECT ans FROM Questions");
    while(query.next()){
        int ans = query.value(0).toInt();
        answers.push_back(QString::number(ans));
    }

    db.close();
}

void setScoreToDB(){
    db.setDatabaseName("Rank.db");
    if(db.open())
        printf("Database connection success\n");
    else{
        printf("Database connection failed");
        return;
    }
    QSqlQuery query;
    query.prepare("INSERT INTO Rank (correct_rate, name) "
                  "VALUES (:correct_rate, :name)");
    query.bindValue(":correct_rate", (player1_correct_num/question_num)*100);
    qDebug() << "player1: " << player1_correct_num << " / " << question_num;
    qDebug() << "correct_rate: " << (player1_correct_num/question_num)*100;
    query.bindValue(":name", player1);
    query.exec();

    query.prepare("INSERT INTO Rank (correct_rate, name) "
                  "VALUES (:correct_rate, :name)");
    query.bindValue(":correct_rate", (player2_correct_num/question_num)*100);
    qDebug() << "player2: " << player2_correct_num << " / " << question_num;
    qDebug() << "correct_rate: " << (player2_correct_num/question_num)*100;
    query.bindValue(":name", player2);
    query.exec();

    query.exec("SELECT * FROM Rank");
    while(query.next()){
        qDebug() << query.value(1).toInt();
        qDebug() << query.value(2).toString();
    }

    db.close();
}

void sendRankToClient(){
    SOCKET serv_sd;
    int cli_len;
    char str[10];
    struct sockaddr_in cli;
    db.setDatabaseName("Rank.db");
    if(db.open())
        printf("Database connection success\n");
    else{
        printf("Database connection failed");
        return;
    }
    serv_sd = socket(AF_INET, SOCK_DGRAM, 0); // open UDP socket
    bool broadcast = true;
    setsockopt(serv_sd, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast));

    cli_len = sizeof(cli);

    printf("server start broadcasting on port: %d \n", BraodcastPort);

    cli.sin_family = AF_INET;
    cli.sin_addr.s_addr = inet_addr("255.255.255.255");
    cli.sin_port        = htons(BraodcastPort);

    QSqlQuery query;
    query.exec("SELECT * FROM Rank ORDER BY correct_rate DESC");

    while(query.next()){
        int correct_rate = query.value(1).toInt();
        QString player_name = query.value(2).toString();
        //char *player_name_ch = QStringToChar(player_name);
        //char *correct_rate_ch = QStringToChar(QString::number(correct_rate));
        sendto(serv_sd, QStringToChar(player_name), strlen(QStringToChar(player_name))+1, 0,(LPSOCKADDR)&cli,cli_len);
        sendto(serv_sd, QStringToChar(QString::number(correct_rate)), strlen(QStringToChar(QString::number(correct_rate)))+1, 0, (LPSOCKADDR)&cli, cli_len);
        qDebug() << "player_name: " << QStringToChar(player_name) << "correct_rate: " << QStringToChar(QString::number(correct_rate));
    }
    strcpy(str, "EOF");
    sendto(serv_sd, str, strlen(str)+1, 0, (LPSOCKADDR)&cli, cli_len);    // notify client that sending is complete
    qDebug() << "send to clients: " << str;
    db.close();
}

char* QStringToChar(QString str){
    QByteArray qArr = str.toUtf8(); // to ASCII (QString to QByteArray)
    char *c_str = qArr.data();
    return c_str;
}
