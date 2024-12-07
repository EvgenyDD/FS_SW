#include "mainwindow.h"
#include <QApplication>

#include "rapi/rapi.h"

#include "interface/serial_port/serial_interface_provider.h"
#include "tools/queue_threadsafe.h"

#include "asuit_protocol.h"


static std::queue_threadsafe<char> q_log;
static Flasher flasher(q_log);
static int argc_fake = 0;
static QApplication a(argc_fake, nullptr);
static MainWindow w(q_log, flasher);

void RAPI::set_parser_cb(AbstractInterface* iface)
{
    auto *r = new ASuitProtocol(iface->get_id(), iface->get_if_impl_mutex(), q_log);
    flasher.set_protocol(r);
    w.set_protocol(r);
    iface->set_parser(r);
    iface->get_ptr_impl()->set_tx(std::bind(&AbstractInterface::tx_data, iface, std::placeholders::_1));
}

void RAPI::remove_interface_signal(int id)
{
    (void)id;
    flasher.set_protocol(nullptr);
    w.set_protocol(nullptr);
}

void RAPI::setup_interfaces()
{
    interface_providers.emplace_back(std::unique_ptr<AbstractInterfaceProvider>(new SerialInterfaceProvider(interface_collector)));
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    RAPI::instance();

    w.show();

    return a.exec();
}
