/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <qdir.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QDebug>

#include "../shared/testhttpserver.h"
#include "../../../shared/util.h"

#define SERVER_ADDR "http://127.0.0.1:14450"
#define SERVER_PORT 14450


class tst_qdeclarativemoduleplugin : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativemoduleplugin()
    {
    }

private slots:
    void importsPlugin();
    void importsPlugin2();
    void importsPlugin21();
    void importsMixedQmlCppPlugin();
    void incorrectPluginCase();
    void importPluginWithQmlFile();
    void remoteImportWithQuotedUrl();
    void remoteImportWithUnquotedUri();
    void versionNotInstalled();
    void versionNotInstalled_data();
    void implicitQmldir();
    void implicitQmldir_data();
};

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
#define SRCDIR "."
#endif

#define VERIFY_ERRORS(errorfile) \
    if (!errorfile) { \
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty()) \
            qWarning() << "Unexpected Errors:" << component.errors(); \
        QVERIFY(!component.isError()); \
        QVERIFY(component.errors().isEmpty()); \
    } else { \
        QString verify_errors_file_name = QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("data") + QDir::separator() + QLatin1String(errorfile); \
        QFile file(verify_errors_file_name); \
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); \
        QByteArray data = file.readAll(); \
        file.close(); \
        QList<QByteArray> expected = data.split('\n'); \
        expected.removeAll(QByteArray("")); \
        QList<QDeclarativeError> errors = component.errors(); \
        QList<QByteArray> actual; \
        for (int ii = 0; ii < errors.count(); ++ii) { \
            const QDeclarativeError &error = errors.at(ii); \
            QByteArray errorStr = QByteArray::number(error.line()) + ":" +  \
                                  QByteArray::number(error.column()) + ":" + \
                                  error.description().toUtf8(); \
            actual << errorStr; \
        } \
        if (qgetenv("DEBUG") != "" && expected != actual) { \
            qWarning() << "Expected:" << expected << "Actual:" << actual; \
        } \
        if (qgetenv("QDECLARATIVELANGUAGE_UPDATEERRORS") != "" && expected != actual) {\
            QFile file(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("data") + QDir::separator() + QLatin1String(errorfile)); \
            QVERIFY(file.open(QIODevice::WriteOnly)); \
            for (int ii = 0; ii < actual.count(); ++ii) { \
                file.write(actual.at(ii)); file.write("\n"); \
            } \
            file.close(); \
        } else { \
            QCOMPARE(expected, actual); \
        } \
    }

inline QUrl TEST_FILE(const QString &filename)
{
    QFileInfo fileInfo(__FILE__);
    return QUrl::fromLocalFile(fileInfo.absoluteDir().filePath(filename));
}

void tst_qdeclarativemoduleplugin::importsPlugin()
{
    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));
    QTest::ignoreMessage(QtWarningMsg, "plugin created");
    QTest::ignoreMessage(QtWarningMsg, "import worked");
    QDeclarativeComponent component(&engine, TEST_FILE("data/works.qml"));
    foreach (QDeclarativeError err, component.errors())
    	qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qdeclarativemoduleplugin::importsPlugin2()
{
    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));
    QTest::ignoreMessage(QtWarningMsg, "plugin2 created");
    QTest::ignoreMessage(QtWarningMsg, "import2 worked");
    QDeclarativeComponent component(&engine, TEST_FILE("data/works2.qml"));
    foreach (QDeclarativeError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qdeclarativemoduleplugin::importsPlugin21()
{
    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));
    QTest::ignoreMessage(QtWarningMsg, "plugin2.1 created");
    QTest::ignoreMessage(QtWarningMsg, "import2.1 worked");
    QDeclarativeComponent component(&engine, TEST_FILE("data/works21.qml"));
    foreach (QDeclarativeError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qdeclarativemoduleplugin::incorrectPluginCase()
{
    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));

    QDeclarativeComponent component(&engine, TEST_FILE("data/incorrectCase.qml"));

    QList<QDeclarativeError> errors = component.errors();
    QCOMPARE(errors.count(), 1);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
#if defined(Q_OS_MAC)
    QString libname = "libPluGin.dylib";
#elif defined(Q_OS_WIN32)
    QString libname = "PluGin.dll";
#endif
    QString expectedError = QLatin1String("plugin cannot be loaded for module \"com.nokia.WrongCase\": File name case mismatch for \"") + QFileInfo(__FILE__).absoluteDir().filePath("imports/com/nokia/WrongCase/" + libname) + QLatin1String("\"");
#else
    QString expectedError = QLatin1String("module \"com.nokia.WrongCase\" plugin \"PluGin\" not found");
#endif

    QCOMPARE(errors.at(0).description(), expectedError);
}

void tst_qdeclarativemoduleplugin::importPluginWithQmlFile()
{
    QString path = QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports");

    // QTBUG-16885: adding an import path with a lower-case "c:" causes assert failure
    // (this only happens if the plugin includes pure QML files)
    #ifdef Q_OS_WIN
        QVERIFY(path.at(0).isUpper() && path.at(1) == QLatin1Char(':'));
        path = path.at(0).toLower() + path.mid(1);
    #endif

    QDeclarativeEngine engine;
    engine.addImportPath(path);

    QDeclarativeComponent component(&engine, TEST_FILE("data/pluginWithQmlFile.qml"));
    foreach (QDeclarativeError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativemoduleplugin::remoteImportWithQuotedUrl()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/imports");

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData("import \"http://127.0.0.1:14450/com/nokia/PureQmlModule\" \nComponentA { width: 300; ComponentB{} }", QUrl());

    QTRY_COMPARE(component.status(), QDeclarativeComponent::Ready);
    QObject *object = component.create();
    QCOMPARE(object->property("width").toInt(), 300);
    QVERIFY(object != 0);
    delete object;

    foreach (QDeclarativeError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

void tst_qdeclarativemoduleplugin::remoteImportWithUnquotedUri()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/imports");

    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));
    QDeclarativeComponent component(&engine);
    component.setData("import com.nokia.PureQmlModule 1.0 \nComponentA { width: 300; ComponentB{} }", QUrl());


    QTRY_COMPARE(component.status(), QDeclarativeComponent::Ready);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("width").toInt(), 300);
    delete object;

    foreach (QDeclarativeError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

// QTBUG-17324
void tst_qdeclarativemoduleplugin::importsMixedQmlCppPlugin()
{
    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));

    {
    QDeclarativeComponent component(&engine, TEST_FILE("data/importsMixedQmlCppPlugin.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
    }

    {
    QDeclarativeComponent component(&engine, TEST_FILE("data/importsMixedQmlCppPlugin.2.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    delete o;
    }


}

void tst_qdeclarativemoduleplugin::versionNotInstalled_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("versionNotInstalled") << "data/versionNotInstalled.qml" << "versionNotInstalled.errors.txt";
    QTest::newRow("versionNotInstalled") << "data/versionNotInstalled.2.qml" << "versionNotInstalled.2.errors.txt";
}

void tst_qdeclarativemoduleplugin::versionNotInstalled()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QDeclarativeEngine engine;
    engine.addImportPath(QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("imports"));

    QDeclarativeComponent component(&engine, TEST_FILE(file));
    VERIFY_ERRORS(errorFile.toLatin1().constData());
}


// test that errors are reporting correctly for plugin loading and qmldir parsing
void tst_qdeclarativemoduleplugin::implicitQmldir_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    // parsing qmldir succeeds, but plugin specified in the qmldir file doesn't exist
    QTest::newRow("implicitQmldir") << "implicit1" << "temptest.qml" << "implicitQmldir.errors.txt";

    // parsing qmldir fails due to syntax errors, etc.
    QTest::newRow("implicitQmldir2") << "implicit2" << "temptest2.qml" << "implicitQmldir.2.errors.txt";
}
void tst_qdeclarativemoduleplugin::implicitQmldir()
{
    QFETCH(QString, directory);
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QString importPath = QLatin1String(SRCDIR) + QDir::separator() + QLatin1String("data") + QDir::separator() + directory;
    QString fileName = QLatin1String("data") + QDir::separator() + directory + QDir::separator() + file;
    QString errorFileName = directory + QDir::separator() + errorFile;
    QUrl testFileUrl = TEST_FILE(fileName);

    QDeclarativeEngine engine;
    engine.addImportPath(importPath);

    QDeclarativeComponent component(&engine, testFileUrl);
    QList<QDeclarativeError> errors = component.errors();
    VERIFY_ERRORS(errorFileName.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
    QObject *obj = component.create();
    QVERIFY(!obj);
    delete obj;
}


QTEST_MAIN(tst_qdeclarativemoduleplugin)

#include "tst_qdeclarativemoduleplugin.moc"
