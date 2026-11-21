#include <QApplication>
#include <QStyleFactory>

#include "ui/MainWindow.h"
#include "ui/PigeonMascot.h"
#include "ui/Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Utility di build: PigeonVPN --render-icon <file.png> <px>  (genera le icone dal disegno QPainter)
    const QStringList args = app.arguments();
    if (args.size() == 4 && args[1] == QLatin1String("--render-icon")) {
        const int px = args[3].toInt();
        return PigeonMascot::renderPixmap(VpnState::Connected, px > 0 ? px : 256, 0.8).save(args[2]) ? 0 : 1;
    }
    QApplication::setApplicationName(QStringLiteral("PigeonVPN"));
    QApplication::setOrganizationName(QStringLiteral("PigeonVPN"));
    QApplication::setQuitOnLastWindowClosed(false); // resta nel tray

    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setStyleSheet(appStyleSheet());
    app.setWindowIcon(QIcon(PigeonMascot::renderPixmap(VpnState::Connected, 128)));

    MainWindow w;
    w.show();
    return app.exec();
}
