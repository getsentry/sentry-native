#include "sentry.h"

#include <QtCore/qtimer.h>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qrasterwindow.h>

#include <cstdio>
#include <cstdlib>

class Window : public QRasterWindow {
public:
    int frame = 0;

protected:
    void
    paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const QRect rect(QPoint(), size());
        painter.fillRect(rect, QColor::fromHsv((frame * 40) % 360, 200, 255));
        painter.drawText(rect, Qt::AlignCenter, QString::number(frame));
    }
};

int
main(int argc, char **argv)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_database_path(options, ".sentry-native");
    sentry_options_set_debug(options, 1);
    sentry_options_set_attach_session_replay(options, 1);
    // deferred start
    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_uuid_t test_uuid = sentry_uuid_new_v4();
    char test_id[37];
    sentry_uuid_as_string(&test_uuid, test_id);
    sentry_set_tag("test.id", test_id);
    sentry_set_tag("test.suite", "e2e");
    printf("TEST_ID:%s\n", test_id);
    fflush(stdout);

    QGuiApplication app(argc, argv);
    Window window;
    window.resize(640, 400);
    window.show();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &window, [&window] {
        window.frame++;
        window.update();
    });
    timer.start(500);

    QTimer::singleShot(3000, &window, [&window] {
        const QPointF position(window.width() / 2.0, window.height() / 2.0);
        QMouseEvent press(QEvent::MouseButtonPress, position,
            window.mapToGlobal(position), Qt::LeftButton, Qt::LeftButton,
            Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &press);
    });

    // after a few frames
    QTimer::singleShot(5000, [] { *(volatile int *)nullptr = 0; });

    return app.exec();
}
