#include "sentry_integration_qt.h"

extern "C" {
#include "sentry_alloc.h"
#include "sentry_core.h"
}

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#ifdef QT_MULTIMEDIA_LIB
extern "C" {
#    include "sentry_options.h"
#    include "sentry_path.h"
#    include "sentry_utils.h"
#    include "sentry_value.h"
}

#    include <QtCore/qbuffer.h>
#    include <QtCore/qcoreapplication.h>
#    include <QtCore/qdatetime.h>
#    include <QtCore/qdir.h>
#    include <QtCore/qeventloop.h>
#    include <QtCore/qmutex.h>
#    include <QtCore/qpointer.h>
#    include <QtCore/qthread.h>
#    include <QtCore/qthreadpool.h>
#    include <QtCore/qtimer.h>
#    include <QtCore/qurl.h>
#    include <QtCore/quuid.h>
#    include <QtGui/qevent.h>
#    include <QtGui/qguiapplication.h>
#    include <QtGui/qimage.h>
#    include <QtGui/qpainter.h>
#    include <QtGui/qpixmap.h>
#    include <QtGui/qscreen.h>
#    include <QtGui/qwindow.h>
#    include <QtMultimedia/qmediacapturesession.h>
#    include <QtMultimedia/qmediaformat.h>
#    include <QtMultimedia/qmediarecorder.h>
#    include <QtMultimedia/qvideoframe.h>
#    include <QtMultimedia/qvideoframeinput.h>

#    include <atomic>
#endif

static QtMessageHandler originalMessageHandler = nullptr;

static const char *
logLevelForMessageType(QtMsgType msgType)
{
    switch (msgType) {
    case QtDebugMsg:
        return "debug";
    case QtWarningMsg:
        return "warning";
    case QtCriticalMsg:
        return "error";
    case QtFatalMsg:
        return "fatal";
    case QtInfoMsg:
        Q_FALLTHROUGH();
    default:
        return "info";
    }
}

static void
sentry_qt_messsage_handler(
    QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    sentry_value_t crumb
        = sentry_value_new_breadcrumb("default", qUtf8Printable(message));

    sentry_value_set_by_key(
        crumb, "category", sentry_value_new_string(context.category));
    sentry_value_set_by_key(
        crumb, "level", sentry_value_new_string(logLevelForMessageType(type)));

    sentry_value_t location = sentry_value_new_object();
    sentry_value_set_by_key(
        location, "file", sentry_value_new_string(context.file));
    sentry_value_set_by_key(
        location, "line", sentry_value_new_int32(context.line));
    sentry_value_set_by_key(crumb, "data", location);

    sentry_add_breadcrumb(crumb);

    // Don't interfere with normal logging, by forwarding
    // to any existing message handlers.
    if (originalMessageHandler)
        originalMessageHandler(type, context, message);
}

#ifdef QT_MULTIMEDIA_LIB
// 1 fps like mobile
static const int REPLAY_FRAME_INTERVAL_MS = 1000;
static const int REPLAY_MAX_FRAME_SIZE = 1280;
static const int REPLAY_ENCODE_TIMEOUT_MS = 10000;
// newer clips may still be pending for a crashed run's daemon
static const int REPLAY_STALE_SECS = 3600;
// rrweb's mouse move throttle
static const int REPLAY_MOVE_INTERVAL_MS = 50;

static const int RRWEB_INCREMENTAL_SNAPSHOT = 3;
static const int RRWEB_MOUSE_MOVE = 1;
static const int RRWEB_MOUSE_INTERACTION = 2;
static const int RRWEB_CLICK = 2;
static const int RRWEB_TOUCH_START = 7;
static const int RRWEB_TOUCH_END = 9;
static const int RRWEB_POINTER_MOUSE = 0;
static const int RRWEB_POINTER_TOUCH = 2;

// rolling clip of the focused window in `<database>/replays` for crash handlers
class ReplayRecorder : public QObject {
public:
    ReplayRecorder(
        const sentry_path_t *dir, const QString &replayId, uint32_t durationMs);
    ~ReplayRecorder() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct Frame {
        QByteArray png;
        QSize size;
        uint64_t usec;
    };

    struct Input {
        uint64_t usec;
        // 0..1 in the recorded window
        QPointF position;
        int source;
        int type;
        int pointerType;
        int pointerId;
    };

    void capture();
    void addInput(const QPointF &globalPosition, int source, int type,
        int pointerType, int pointerId);
    void addFrame(const QImage &image, uint64_t usec);
    bool encode();
    QVideoFrame videoFrame(qsizetype index, const QSize &size) const;
    sentry_value_t inputEvents(uint64_t start, uint64_t end, const QSize &size);
    bool publish();

    const QString replayId;
    const qsizetype maxFrames;
    sentry_path_t *mp4 = nullptr;
    sentry_path_t *mp4Tmp = nullptr;
    sentry_path_t *json = nullptr;
    sentry_path_t *jsonTmp = nullptr;
    QList<Frame> frames;
    QTimer timer;
    QThreadPool pool;
    std::atomic<bool> busy { false };
    std::atomic<bool> failed { false };
    QPointer<QWindow> recordedWindow;
    uint64_t lastMoveUsec = 0;
    QMutex inputsMutex;
    QList<Input> inputs;
};

static sentry_path_t *
replay_path(const sentry_path_t *dir, const QString &name)
{
    return sentry__path_join_str(dir, name.toUtf8().constData());
}

ReplayRecorder::ReplayRecorder(
    const sentry_path_t *dir, const QString &replayId, uint32_t durationMs)
    : replayId(replayId)
    , maxFrames(qMax<qsizetype>(1,
          (durationMs + REPLAY_FRAME_INTERVAL_MS - 1)
              / REPLAY_FRAME_INTERVAL_MS))
{
    // stale clips
    sentry__path_create_dir_all(dir);
    QDir staging(QString::fromUtf8(dir->path));
    const QDateTime stale
        = QDateTime::currentDateTime().addSecs(-REPLAY_STALE_SECS);
    for (const QFileInfo &info : staging.entryInfoList(
             QStringList(QStringLiteral("replay-*")), QDir::Files)) {
        if (info.lastModified() < stale) {
            staging.remove(info.fileName());
        }
    }

    const QString base = QStringLiteral("replay-") + replayId;
    mp4 = replay_path(dir, base + QStringLiteral(".mp4"));
    mp4Tmp = replay_path(dir, base + QStringLiteral(".mp4.tmp"));
    json = replay_path(dir, base + QStringLiteral(".json"));
    jsonTmp = replay_path(dir, base + QStringLiteral(".json.tmp"));

    pool.setMaxThreadCount(1);
    QObject::connect(&timer, &QTimer::timeout, &timer, [this] { capture(); });
    timer.start(REPLAY_FRAME_INTERVAL_MS);
    QCoreApplication::instance()->installEventFilter(this);
}

ReplayRecorder::~ReplayRecorder()
{
    timer.stop();
    pool.waitForDone();
    for (sentry_path_t *path : { mp4, mp4Tmp, json, jsonTmp }) {
        if (path) {
            sentry__path_remove(path);
            sentry__path_free(path);
        }
    }
}

void
ReplayRecorder::capture()
{
    if (failed) {
        timer.stop();
        return;
    }
    // previous frame still encoding
    if (busy) {
        return;
    }

    QWindow *window = QGuiApplication::focusWindow();
    if (!window) {
        for (QWindow *topLevel : QGuiApplication::topLevelWindows()) {
            if (topLevel->isVisible()) {
                window = topLevel;
                break;
            }
        }
    }
    QScreen *screen = window ? window->screen() : nullptr;
    if (!screen) {
        return;
    }
    // null on Wayland
    QPixmap pixmap = screen->grabWindow(window->winId());
    if (pixmap.isNull()) {
        return;
    }
    recordedWindow = window;

    busy = true;
    pool.start([this, image = pixmap.toImage(), usec = sentry__usec_time()] {
        addFrame(image, usec);
        if (!encode()) {
            failed = true;
        }
        busy = false;
    });
}

bool
ReplayRecorder::eventFilter(QObject *watched, QEvent *event)
{
    // windows only, widgets get the same input
    if (!qobject_cast<QWindow *>(watched)) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        // synthesized from touch
        if (mouse->pointingDevice()
            && mouse->pointingDevice()->type()
                == QInputDevice::DeviceType::TouchScreen) {
            break;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            addInput(mouse->globalPosition(), RRWEB_MOUSE_INTERACTION,
                RRWEB_CLICK, RRWEB_POINTER_MOUSE, 0);
            break;
        }
        const uint64_t usec = sentry__usec_time();
        if (usec - lastMoveUsec >= REPLAY_MOVE_INTERVAL_MS * 1000) {
            lastMoveUsec = usec;
            addInput(mouse->globalPosition(), RRWEB_MOUSE_MOVE, 0,
                RRWEB_POINTER_MOUSE, 0);
        }
        break;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        for (const QEventPoint &point :
            static_cast<QTouchEvent *>(event)->points()) {
            if (point.state() == QEventPoint::Pressed) {
                addInput(point.globalPosition(), RRWEB_MOUSE_INTERACTION,
                    RRWEB_TOUCH_START, RRWEB_POINTER_TOUCH, point.id());
            } else if (point.state() == QEventPoint::Released) {
                addInput(point.globalPosition(), RRWEB_MOUSE_INTERACTION,
                    RRWEB_TOUCH_END, RRWEB_POINTER_TOUCH, point.id());
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void
ReplayRecorder::addInput(const QPointF &globalPosition, int source, int type,
    int pointerType, int pointerId)
{
    // also from popups
    QWindow *window = recordedWindow;
    if (!window || window->width() <= 0 || window->height() <= 0) {
        return;
    }
    const QPointF local = window->mapFromGlobal(globalPosition);
    const QPointF position(
        local.x() / window->width(), local.y() / window->height());
    if (position.x() < 0.0 || position.x() > 1.0 || position.y() < 0.0
        || position.y() > 1.0) {
        return;
    }

    const uint64_t usec = sentry__usec_time();
    const uint64_t retentionUsec = static_cast<uint64_t>(maxFrames + 1)
        * REPLAY_FRAME_INTERVAL_MS * 1000;
    QMutexLocker locker(&inputsMutex);
    while (!inputs.isEmpty() && inputs.first().usec + retentionUsec < usec) {
        inputs.removeFirst();
    }
    inputs.append({ usec, position, source, type, pointerType, pointerId });
}

void
ReplayRecorder::addFrame(const QImage &image, uint64_t usec)
{
    QSize size = image.size();
    if (size.width() > REPLAY_MAX_FRAME_SIZE
        || size.height() > REPLAY_MAX_FRAME_SIZE) {
        size.scale(
            REPLAY_MAX_FRAME_SIZE, REPLAY_MAX_FRAME_SIZE, Qt::KeepAspectRatio);
    }
    // H.264 4:2:0 needs even dimensions
    size = QSize(size.width() & ~1, size.height() & ~1);
    if (size.isEmpty()) {
        return;
    }

    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    const QImage scaled = image.size() == size
        ? image
        : image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (!scaled.save(&buffer, "PNG")) {
        return;
    }

    frames.append({ png, size, usec });
    while (frames.size() > maxFrames) {
        frames.removeFirst();
    }
}

QVideoFrame
ReplayRecorder::videoFrame(qsizetype index, const QSize &size) const
{
    QImage image = QImage::fromData(frames[index].png, "PNG");
    // from before a resize
    if (image.size() != size) {
        QImage canvas(size, QImage::Format_RGB32);
        canvas.fill(Qt::black);
        const QImage scaled
            = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainter(&canvas).drawImage((size.width() - scaled.width()) / 2,
            (size.height() - scaled.height()) / 2, scaled);
        image = canvas;
    }

    const uint64_t origin = frames.first().usec;
    const uint64_t start = frames[index].usec - origin;
    const uint64_t end = index + 1 < frames.size()
        ? frames[index + 1].usec - origin
        : start + REPLAY_FRAME_INTERVAL_MS * 1000;
    QVideoFrame frame(image);
    frame.setStartTime(static_cast<qint64>(start));
    frame.setEndTime(static_cast<qint64>(end));
    return frame;
}

bool
ReplayRecorder::encode()
{
    if (frames.isEmpty()) {
        return true;
    }
    const QSize size = frames.last().size;

    QMediaCaptureSession session;
    QVideoFrameInput input;
    QMediaRecorder recorder;
    session.setVideoFrameInput(&input);
    session.setRecorder(&recorder);

    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    recorder.setMediaFormat(format);
    recorder.setVideoResolution(size);
    recorder.setVideoFrameRate(1000.0 / REPLAY_FRAME_INTERVAL_MS);
    recorder.setOutputLocation(
        QUrl::fromLocalFile(QString::fromUtf8(mp4Tmp->path)));

    QEventLoop loop;
    bool ok = true;
    qsizetype next = 0;
    auto send = [&] {
        for (; next < frames.size(); ++next) {
            if (!input.sendVideoFrame(videoFrame(next, size))) {
                // resumed on readyToSendVideoFrame
                return;
            }
        }
        recorder.stop();
    };
    QObject::connect(
        &input, &QVideoFrameInput::readyToSendVideoFrame, &loop, send);
    QObject::connect(&recorder, &QMediaRecorder::recorderStateChanged, &loop,
        [&](QMediaRecorder::RecorderState state) {
            if (state == QMediaRecorder::StoppedState) {
                loop.quit();
            }
        });
    QObject::connect(&recorder, &QMediaRecorder::errorOccurred, &loop,
        [&](QMediaRecorder::Error, const QString &message) {
            SENTRY_WARNF(
                "session replay encoding failed: %s", qUtf8Printable(message));
            ok = false;
            loop.quit();
        });
    QTimer::singleShot(REPLAY_ENCODE_TIMEOUT_MS, &loop, [&] {
        SENTRY_WARN("session replay encoding timed out");
        ok = false;
        loop.quit();
    });

    recorder.record();
    QTimer::singleShot(0, &loop, send);
    loop.exec();

    if (!ok) {
        sentry__path_remove(mp4Tmp);
        return false;
    }
    return publish();
}

// ms since `start`, video pixels
sentry_value_t
ReplayRecorder::inputEvents(uint64_t start, uint64_t end, const QSize &size)
{
    QList<Input> captured;
    {
        QMutexLocker locker(&inputsMutex);
        captured = inputs;
    }

    sentry_value_t events = sentry_value_new_list();
    for (const Input &input : captured) {
        if (input.usec < start || input.usec > end) {
            continue;
        }
        const double x = input.position.x() * size.width();
        const double y = input.position.y() * size.height();

        sentry_value_t data = sentry_value_new_object();
        sentry_value_set_by_key(
            data, "source", sentry_value_new_int32(input.source));
        if (input.source == RRWEB_MOUSE_MOVE) {
            sentry_value_t position = sentry_value_new_object();
            sentry_value_set_by_key(position, "x", sentry_value_new_double(x));
            sentry_value_set_by_key(position, "y", sentry_value_new_double(y));
            sentry_value_set_by_key(position, "id", sentry_value_new_int32(0));
            sentry_value_set_by_key(
                position, "timeOffset", sentry_value_new_int32(0));
            sentry_value_t positions = sentry_value_new_list();
            sentry_value_append(positions, position);
            sentry_value_set_by_key(data, "positions", positions);
        } else {
            sentry_value_set_by_key(
                data, "type", sentry_value_new_int32(input.type));
            sentry_value_set_by_key(data, "id", sentry_value_new_int32(0));
            sentry_value_set_by_key(data, "x", sentry_value_new_double(x));
            sentry_value_set_by_key(data, "y", sentry_value_new_double(y));
            sentry_value_set_by_key(
                data, "pointerType", sentry_value_new_int32(input.pointerType));
            sentry_value_set_by_key(
                data, "pointerId", sentry_value_new_int32(input.pointerId));
        }

        sentry_value_t event = sentry_value_new_object();
        sentry_value_set_by_key(
            event, "type", sentry_value_new_int32(RRWEB_INCREMENTAL_SNAPSHOT));
        sentry_value_set_by_key(event, "timestamp",
            sentry_value_new_double(
                static_cast<double>(input.usec - start) / 1000.0));
        sentry_value_set_by_key(event, "data", data);
        sentry_value_append(events, event);
    }
    return events;
}

bool
ReplayRecorder::publish()
{
    const Frame &first = frames.first();
    const Frame &last = frames.last();
    const double durationMs
        = static_cast<double>(last.usec - first.usec) / 1000.0
        + REPLAY_FRAME_INTERVAL_MS;

    sentry_value_t meta = sentry_value_new_object();
    sentry_value_set_by_key(meta, "replayId",
        sentry_value_new_string(replayId.toUtf8().constData()));
    sentry_value_set_by_key(
        meta, "replayType", sentry_value_new_string("buffer"));
    sentry_value_set_by_key(meta, "segmentId", sentry_value_new_int32(0));
    sentry_value_set_by_key(
        meta, "durationMs", sentry_value_new_double(durationMs));
    sentry_value_set_by_key(meta, "endTimestampSec",
        sentry_value_new_double(static_cast<double>(last.usec) / 1000000.0
            + REPLAY_FRAME_INTERVAL_MS / 1000.0));
    sentry_value_set_by_key(
        meta, "width", sentry_value_new_int32(last.size.width()));
    sentry_value_set_by_key(
        meta, "height", sentry_value_new_int32(last.size.height()));
    sentry_value_set_by_key(meta, "frameCount",
        sentry_value_new_int32(static_cast<int32_t>(frames.size())));
    sentry_value_set_by_key(meta, "frameRate",
        sentry_value_new_int32(1000 / REPLAY_FRAME_INTERVAL_MS));
    sentry_value_set_by_key(meta, "inputEvents",
        inputEvents(first.usec, last.usec + REPLAY_FRAME_INTERVAL_MS * 1000,
            last.size));

    size_t len = 0;
    char *buf = sentry__value_to_json(meta, &len);
    sentry_value_decref(meta);

    // mp4 last, crash handlers look for it
    const bool ok = buf && sentry__path_write_buffer(jsonTmp, buf, len) == 0
        && sentry__path_rename(jsonTmp, json) == 0
        && sentry__path_rename(mp4Tmp, mp4) == 0;
    sentry_free(buf);
    return ok;
}

static ReplayRecorder *replayRecorder = nullptr;
static sentry_path_t *replayDir = nullptr;
static uint32_t replayDurationMs = 0;

static void
stop_replay()
{
    qRemovePostRoutine(stop_replay);
    delete replayRecorder;
    replayRecorder = nullptr;
}

static sentry_value_t
start_replay()
{
    if (!replayDir || replayRecorder
        || !qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        return sentry_value_new_null();
    }

    const QString replayId = QUuid::createUuid().toString(QUuid::Id128);
    replayRecorder = new ReplayRecorder(replayDir, replayId, replayDurationMs);
    // stop before QCoreApplication tears down
    qAddPostRoutine(stop_replay);

    sentry_value_t replay = sentry_value_new_object();
    sentry_value_set_by_key(replay, "replay_id",
        sentry_value_new_string(replayId.toUtf8().constData()));
    return replay;
}

static void
start_replay_deferred()
{
    // after construction
    QTimer::singleShot(0, QCoreApplication::instance(), [] {
        sentry_value_t replay = start_replay();
        if (!sentry_value_is_null(replay)) {
            sentry_set_context("replay", replay);
        }
    });
}
#endif

static void
register_qt(
    void *UNUSED(data), sentry_scope_t *scope, const sentry_options_t *options)
{
    SENTRY_DEBUG("registering Qt integration");
    originalMessageHandler = qInstallMessageHandler(sentry_qt_messsage_handler);

#ifdef QT_MULTIMEDIA_LIB
    if (!options->attach_session_replay) {
        return;
    }
    replayDir = sentry__path_join_str(options->database_path, "replays");
    replayDurationMs = options->session_replay_duration;
    if (QCoreApplication::instance()) {
        sentry_value_t replay = start_replay();
        if (!sentry_value_is_null(replay)) {
            sentry_scope_set_context(scope, "replay", replay);
        }
    } else {
        // pre-routines cannot be removed
        static bool deferred = false;
        if (!deferred) {
            qAddPreRoutine(start_replay_deferred);
            deferred = true;
        }
    }
#else
    (void)scope;
    (void)options;
#endif
}

static void
unregister_qt(void *UNUSED(data), sentry_scope_t *UNUSED(scope),
    const sentry_options_t *UNUSED(options))
{
    qInstallMessageHandler(originalMessageHandler);
    originalMessageHandler = nullptr;

#ifdef QT_MULTIMEDIA_LIB
    QCoreApplication *app = QCoreApplication::instance();
    if (app && app->thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(app, stop_replay, Qt::QueuedConnection);
    } else {
        stop_replay();
    }
    sentry__path_free(replayDir);
    replayDir = nullptr;
#endif
}

sentry_integration_t *
sentry_integration_qt_new(void)
{
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    if (!integration) {
        return nullptr;
    }

    integration->name = "qt";
    integration->register_func = register_qt;
    integration->unregister_func = unregister_qt;

    return integration;
}
