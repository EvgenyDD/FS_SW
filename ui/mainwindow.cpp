#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include "tools/helper.h"
#include "tools/debug_tools.h"
#include "../lib/air_protocol.h"

MainWindow::MainWindow(std::queue_threadsafe<char> &q, Flasher &flasher, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    q_log(q),
    flasher(flasher)
{
    ui->setupUi(this);

    AnyFile f("fw_path.ini");
    if(f.open_r() == false)
    {
        auto b = f.read();
        auto pieces = StringHelper::split(std::string(reinterpret_cast<char*>(&b[0]), b.size()), '\n');
        if(pieces.size() > 0) ui->lineedit_fw_path_ctrl->setText(QString::fromStdString(pieces[0]+"\0"));
        if(pieces.size() > 1) ui->lineedit_fw_path_head->setText(QString::fromStdString(pieces[1]+"\0"));
        if(pieces.size() > 2) ui->lineedit_fw_path_tail->setText(QString::fromStdString(pieces[2]+"\0"));
    }
    else
    {
        ui->logBrowser->append("Failed to open path file!");
    }

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_update()));
    timer->start(20);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timer_update()
{
    if(upd_ui_flag)
    {
        upd_ui_flag = false;
        enable_ui(protocol != nullptr);
        debug(protocol != nullptr ? "USB Connected!" : "USB Disconnected!");
    }

    QString log;
    char v;
    while(q_log.try_pop(v))
    {
        if(v == '\n') break;
        log.append(v);
    }
    if(log.size())
    {
        ui->logBrowser->append(log);
    }
}

void MainWindow::save_paths()
{
    AnyFile f("fw_path.ini");
    if(f.open_w() == false)
    {
        f.append(ui->lineedit_fw_path_ctrl->text().toStdString() + "\n");
        f.append(ui->lineedit_fw_path_head->text().toStdString() + "\n");
        f.append(ui->lineedit_fw_path_tail->text().toStdString() + "\n");
    }
    else
    {
        ui->logBrowser->append("Failed to save path file!");
    }
}

void MainWindow::enable_ui(bool state)
{
    ui->btn_off_ctrl->setEnabled(state);
    ui->btn_off_head->setEnabled(state);
    ui->btn_off_tail->setEnabled(state);

    ui->btn_upd_ctrl->setEnabled(state);
    ui->btn_upd_head->setEnabled(state);
    ui->btn_upd_tail->setEnabled(state);

    ui->btn_reboot_ctrl->setEnabled(state);
    ui->btn_reboot_head->setEnabled(state);
    ui->btn_reboot_tail->setEnabled(state);

    ui->btn_term_send->setEnabled(state);
}

void MainWindow::on_btn_upd_ctrl_clicked()
{
    save_paths();
    AnyFile file(ui->lineedit_fw_path_ctrl->text().toStdString());
    if(file.open_r(true))
    {
        ui->logBrowser->append("Can't open file " + ui->lineedit_fw_path_ctrl->text());
        return;
    }
    auto v = file.read();
    flasher.start(RFM_NET_ID_CTRL, v, 3000);
}

void MainWindow::on_btn_upd_head_clicked()
{
    save_paths();
    AnyFile file(ui->lineedit_fw_path_head->text().toStdString());
    if(file.open_r(true))
    {
        ui->logBrowser->append("Can't open file " + ui->lineedit_fw_path_head->text());
        return;
    }
    auto v = file.read();
    flasher.start(RFM_NET_ID_HEAD, v, 3000);
}

void MainWindow::on_btn_upd_tail_clicked()
{
    save_paths();
    AnyFile file(ui->lineedit_fw_path_tail->text().toStdString());
    if(file.open_r(true))
    {
        ui->logBrowser->append("Can't open file " + ui->lineedit_fw_path_tail->text());
        return;
    }
    auto v = file.read();
    flasher.start(RFM_NET_ID_TAIL, v, 3000);
}

void MainWindow::on_btn_off_ctrl_clicked() { protocol->off(RFM_NET_ID_CTRL); }
void MainWindow::on_btn_off_head_clicked() { protocol->off(RFM_NET_ID_HEAD); }
void MainWindow::on_btn_off_tail_clicked() { protocol->off(RFM_NET_ID_TAIL); }

void MainWindow::on_btn_reboot_ctrl_clicked() { protocol->reset(RFM_NET_ID_CTRL); }
void MainWindow::on_btn_reboot_head_clicked() { protocol->reset(RFM_NET_ID_HEAD); }
void MainWindow::on_btn_reboot_tail_clicked() { protocol->reset(RFM_NET_ID_TAIL); }

void MainWindow::on_btn_term_send_clicked() {send_terminal();}
void MainWindow::on_lineedit_term_returnPressed() {send_terminal();}

void MainWindow::send_terminal()
{
    protocol->terminal(ui->radio_term_ctrl->isChecked() ? RFM_NET_ID_CTRL
                     : ui->radio_term_head->isChecked() ? RFM_NET_ID_HEAD
                                                        : RFM_NET_ID_TAIL,
                       ui->lineedit_term->text().toStdString());
}
