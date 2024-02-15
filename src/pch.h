#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <deque>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <time.h> // note: Only for to_time_t(std::tm& tm)
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QModelIndex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSize>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QStringLiteral>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QXmlStreamReader>
#include <Qt>
#include <QtGlobal>
#include <QtLogging>
#include <QtSystemDetection>
