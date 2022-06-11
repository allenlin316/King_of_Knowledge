#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <winsock2.h>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    SOCKET sd;
    QTimer *timer;
    int cur_question;
    int total_question;
    int client_ans;
    int seconds;
    int player1_total_score;
    int player2_total_score;

public:    
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setTotalQuestionNum(); // 接收來自 server 傳送過來的題目數量，設定總共幾題題目
    void gameStart();   // 遊戲開始初始化
    void gameOver();    // 遊戲結束
    void setupGamePage(SOCKET &sd); //  從 server 接收題目選項，設定在 UI 上
    void setScore(SOCKET &sd);  // sever 傳回玩家得分
    void recvRank();    // 玩家收到 server 傳回的所有玩家排名
    char* QStringToChar(QString);   // QString 轉換成 char 陣列
    QString charToQString(char *str);   // char 陣列轉換成 QString

public slots:
    void setLCDTimer(); // 計時器

private slots:
    void on_connect_btn_clicked();
    void on_gameStart_btn_clicked();
    void on_choice1_btn_clicked();
    void on_choice2_btn_clicked();
    void on_choice3_btn_clicked();
    void on_choice4_btn_clicked();
    void on_rank_page_btn_clicked();
    void on_home_page_btn_clicked();
    void on_about_btn_clicked();
    void on_back_to_home_page_btn_clicked();
    void on_back_home_page_btn_clicked();
    void on_back_home_btn_clicked();
};
#endif // MAINWINDOW_H
