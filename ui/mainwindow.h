#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "tools/queue_threadsafe.h"
#include "asuit_protocol.h"
#include "asuit_protocol.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::queue_threadsafe<char> &q, Flasher &flasher, QWidget *parent = nullptr);
    ~MainWindow();

    void set_protocol(ASuitProtocol *p)
    {
        protocol = p;
        upd_ui_flag = true;
    }

private slots:
    void timer_update();
    void on_btn_upd_ctrl_clicked();
    void on_btn_upd_head_clicked();
    void on_btn_upd_tail_clicked();

    void on_btn_off_ctrl_clicked();
    void on_btn_off_head_clicked();
    void on_btn_off_tail_clicked();

    void on_btn_reboot_ctrl_clicked();
    void on_btn_reboot_head_clicked();
    void on_btn_reboot_tail_clicked();

    void on_btn_term_send_clicked();

    void on_lineedit_term_returnPressed();

private:
    void debug(std::string s)
    {
        for(auto &i : s)
        {
            q_log.push(i);
        }
        q_log.push('\n');
    }


    Ui::MainWindow *ui;
    QTimer *timer;
    std::queue_threadsafe<char> &q_log;
    void save_paths();
    Flasher &flasher;
    ASuitProtocol *protocol{nullptr};

    std::atomic_bool upd_ui_flag{true};

    void enable_ui(bool state);
    void send_terminal();
};

#endif // MAINWINDOW_H
