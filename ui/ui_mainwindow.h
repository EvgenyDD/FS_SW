/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *lineedit_fw_path_ctrl;
    QPushButton *btn_off_ctrl;
    QPushButton *btn_reboot_ctrl;
    QPushButton *btn_upd_ctrl;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QLineEdit *lineedit_fw_path_head;
    QPushButton *btn_off_head;
    QPushButton *btn_reboot_head;
    QPushButton *btn_upd_head;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLineEdit *lineedit_fw_path_tail;
    QPushButton *btn_off_tail;
    QPushButton *btn_reboot_tail;
    QPushButton *btn_upd_tail;
    QTextBrowser *logBrowser;
    QHBoxLayout *horizontalLayout_4;
    QLineEdit *lineedit_term;
    QRadioButton *radio_term_ctrl;
    QRadioButton *radio_term_head;
    QRadioButton *radio_term_tail;
    QPushButton *btn_term_send;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(640, 360);
        QIcon icon;
        icon.addFile(QString::fromUtf8("paw.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        lineedit_fw_path_ctrl = new QLineEdit(centralWidget);
        lineedit_fw_path_ctrl->setObjectName(QString::fromUtf8("lineedit_fw_path_ctrl"));

        horizontalLayout->addWidget(lineedit_fw_path_ctrl);

        btn_off_ctrl = new QPushButton(centralWidget);
        btn_off_ctrl->setObjectName(QString::fromUtf8("btn_off_ctrl"));
        btn_off_ctrl->setMaximumSize(QSize(30, 16777215));

        horizontalLayout->addWidget(btn_off_ctrl);

        btn_reboot_ctrl = new QPushButton(centralWidget);
        btn_reboot_ctrl->setObjectName(QString::fromUtf8("btn_reboot_ctrl"));
        btn_reboot_ctrl->setMaximumSize(QSize(50, 16777215));

        horizontalLayout->addWidget(btn_reboot_ctrl);

        btn_upd_ctrl = new QPushButton(centralWidget);
        btn_upd_ctrl->setObjectName(QString::fromUtf8("btn_upd_ctrl"));
        btn_upd_ctrl->setMaximumSize(QSize(45, 16777215));

        horizontalLayout->addWidget(btn_upd_ctrl);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        lineedit_fw_path_head = new QLineEdit(centralWidget);
        lineedit_fw_path_head->setObjectName(QString::fromUtf8("lineedit_fw_path_head"));

        horizontalLayout_2->addWidget(lineedit_fw_path_head);

        btn_off_head = new QPushButton(centralWidget);
        btn_off_head->setObjectName(QString::fromUtf8("btn_off_head"));
        btn_off_head->setMaximumSize(QSize(30, 16777215));

        horizontalLayout_2->addWidget(btn_off_head);

        btn_reboot_head = new QPushButton(centralWidget);
        btn_reboot_head->setObjectName(QString::fromUtf8("btn_reboot_head"));
        btn_reboot_head->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_2->addWidget(btn_reboot_head);

        btn_upd_head = new QPushButton(centralWidget);
        btn_upd_head->setObjectName(QString::fromUtf8("btn_upd_head"));
        btn_upd_head->setMaximumSize(QSize(45, 16777215));

        horizontalLayout_2->addWidget(btn_upd_head);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_3->addWidget(label_3);

        lineedit_fw_path_tail = new QLineEdit(centralWidget);
        lineedit_fw_path_tail->setObjectName(QString::fromUtf8("lineedit_fw_path_tail"));

        horizontalLayout_3->addWidget(lineedit_fw_path_tail);

        btn_off_tail = new QPushButton(centralWidget);
        btn_off_tail->setObjectName(QString::fromUtf8("btn_off_tail"));
        btn_off_tail->setMaximumSize(QSize(30, 16777215));

        horizontalLayout_3->addWidget(btn_off_tail);

        btn_reboot_tail = new QPushButton(centralWidget);
        btn_reboot_tail->setObjectName(QString::fromUtf8("btn_reboot_tail"));
        btn_reboot_tail->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_3->addWidget(btn_reboot_tail);

        btn_upd_tail = new QPushButton(centralWidget);
        btn_upd_tail->setObjectName(QString::fromUtf8("btn_upd_tail"));
        btn_upd_tail->setMaximumSize(QSize(45, 16777215));

        horizontalLayout_3->addWidget(btn_upd_tail);


        gridLayout->addLayout(horizontalLayout_3, 2, 0, 1, 1);

        logBrowser = new QTextBrowser(centralWidget);
        logBrowser->setObjectName(QString::fromUtf8("logBrowser"));

        gridLayout->addWidget(logBrowser, 3, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        lineedit_term = new QLineEdit(centralWidget);
        lineedit_term->setObjectName(QString::fromUtf8("lineedit_term"));

        horizontalLayout_4->addWidget(lineedit_term);

        radio_term_ctrl = new QRadioButton(centralWidget);
        radio_term_ctrl->setObjectName(QString::fromUtf8("radio_term_ctrl"));
        radio_term_ctrl->setChecked(true);

        horizontalLayout_4->addWidget(radio_term_ctrl);

        radio_term_head = new QRadioButton(centralWidget);
        radio_term_head->setObjectName(QString::fromUtf8("radio_term_head"));

        horizontalLayout_4->addWidget(radio_term_head);

        radio_term_tail = new QRadioButton(centralWidget);
        radio_term_tail->setObjectName(QString::fromUtf8("radio_term_tail"));

        horizontalLayout_4->addWidget(radio_term_tail);

        btn_term_send = new QPushButton(centralWidget);
        btn_term_send->setObjectName(QString::fromUtf8("btn_term_send"));
        btn_term_send->setMaximumSize(QSize(40, 16777215));

        horizontalLayout_4->addWidget(btn_term_send);


        gridLayout->addLayout(horizontalLayout_4, 4, 0, 1, 1);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 640, 21));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "ASuit Manager", nullptr));
        label->setText(QApplication::translate("MainWindow", "CTRL", nullptr));
        btn_off_ctrl->setText(QApplication::translate("MainWindow", "OFF", nullptr));
        btn_reboot_ctrl->setText(QApplication::translate("MainWindow", "Reboot", nullptr));
        btn_upd_ctrl->setText(QApplication::translate("MainWindow", "Update", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "HEAD", nullptr));
        btn_off_head->setText(QApplication::translate("MainWindow", "OFF", nullptr));
        btn_reboot_head->setText(QApplication::translate("MainWindow", "Reboot", nullptr));
        btn_upd_head->setText(QApplication::translate("MainWindow", "Update", nullptr));
        label_3->setText(QApplication::translate("MainWindow", "TAIL", nullptr));
        btn_off_tail->setText(QApplication::translate("MainWindow", "OFF", nullptr));
        btn_reboot_tail->setText(QApplication::translate("MainWindow", "Reboot", nullptr));
        btn_upd_tail->setText(QApplication::translate("MainWindow", "Update", nullptr));
        radio_term_ctrl->setText(QApplication::translate("MainWindow", "CTRL", nullptr));
        radio_term_head->setText(QApplication::translate("MainWindow", "HEAD", nullptr));
        radio_term_tail->setText(QApplication::translate("MainWindow", "TAIL", nullptr));
        btn_term_send->setText(QApplication::translate("MainWindow", "Send", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
