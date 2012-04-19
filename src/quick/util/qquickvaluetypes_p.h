/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKVALUETYPES_P_H
#define QQUICKVALUETYPES_P_H

#include <qqml.h>
#include <qtquickglobal.h>
#include <private/qqmlvaluetype_p.h>

#include <QtGui/QColor>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#include <QtGui/QQuaternion>
#include <QtGui/QMatrix4x4>
#include <QtGui/QFont>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

namespace QQuickValueTypes {

void registerValueTypes();

}

class Q_AUTOTEST_EXPORT QQuickColorValueType : public QQmlValueTypeBase<QColor>
{
    Q_PROPERTY(qreal r READ r WRITE setR)
    Q_PROPERTY(qreal g READ g WRITE setG)
    Q_PROPERTY(qreal b READ b WRITE setB)
    Q_PROPERTY(qreal a READ a WRITE setA)
    Q_OBJECT
public:
    QQuickColorValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal r() const;
    qreal g() const;
    qreal b() const;
    qreal a() const;
    void setR(qreal);
    void setG(qreal);
    void setB(qreal);
    void setA(qreal);
};

class Q_AUTOTEST_EXPORT QQuickVector2DValueType : public QQmlValueTypeBase<QVector2D>
{
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
    Q_OBJECT
public:
    QQuickVector2DValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal x() const;
    qreal y() const;
    void setX(qreal);
    void setY(qreal);
};

class Q_AUTOTEST_EXPORT QQuickVector3DValueType : public QQmlValueTypeBase<QVector3D>
{
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
    Q_PROPERTY(qreal z READ z WRITE setZ)
    Q_OBJECT
public:
    QQuickVector3DValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);
};

class Q_AUTOTEST_EXPORT QQuickVector4DValueType : public QQmlValueTypeBase<QVector4D>
{
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
    Q_PROPERTY(qreal z READ z WRITE setZ)
    Q_PROPERTY(qreal w READ w WRITE setW)
    Q_OBJECT
public:
    QQuickVector4DValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;
    qreal w() const;
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);
    void setW(qreal);
};

class Q_AUTOTEST_EXPORT QQuickQuaternionValueType : public QQmlValueTypeBase<QQuaternion>
{
    Q_PROPERTY(qreal scalar READ scalar WRITE setScalar)
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
    Q_PROPERTY(qreal z READ z WRITE setZ)
    Q_OBJECT
public:
    QQuickQuaternionValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal scalar() const;
    qreal x() const;
    qreal y() const;
    qreal z() const;
    void setScalar(qreal);
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);
};

class Q_AUTOTEST_EXPORT QQuickMatrix4x4ValueType : public QQmlValueTypeBase<QMatrix4x4>
{
    Q_PROPERTY(qreal m11 READ m11 WRITE setM11)
    Q_PROPERTY(qreal m12 READ m12 WRITE setM12)
    Q_PROPERTY(qreal m13 READ m13 WRITE setM13)
    Q_PROPERTY(qreal m14 READ m14 WRITE setM14)
    Q_PROPERTY(qreal m21 READ m21 WRITE setM21)
    Q_PROPERTY(qreal m22 READ m22 WRITE setM22)
    Q_PROPERTY(qreal m23 READ m23 WRITE setM23)
    Q_PROPERTY(qreal m24 READ m24 WRITE setM24)
    Q_PROPERTY(qreal m31 READ m31 WRITE setM31)
    Q_PROPERTY(qreal m32 READ m32 WRITE setM32)
    Q_PROPERTY(qreal m33 READ m33 WRITE setM33)
    Q_PROPERTY(qreal m34 READ m34 WRITE setM34)
    Q_PROPERTY(qreal m41 READ m41 WRITE setM41)
    Q_PROPERTY(qreal m42 READ m42 WRITE setM42)
    Q_PROPERTY(qreal m43 READ m43 WRITE setM43)
    Q_PROPERTY(qreal m44 READ m44 WRITE setM44)
    Q_OBJECT
public:
    QQuickMatrix4x4ValueType(QObject *parent = 0);

    virtual QString toString() const;

    qreal m11() const { return v(0, 0); }
    qreal m12() const { return v(0, 1); }
    qreal m13() const { return v(0, 2); }
    qreal m14() const { return v(0, 3); }
    qreal m21() const { return v(1, 0); }
    qreal m22() const { return v(1, 1); }
    qreal m23() const { return v(1, 2); }
    qreal m24() const { return v(1, 3); }
    qreal m31() const { return v(2, 0); }
    qreal m32() const { return v(2, 1); }
    qreal m33() const { return v(2, 2); }
    qreal m34() const { return v(2, 3); }
    qreal m41() const { return v(3, 0); }
    qreal m42() const { return v(3, 1); }
    qreal m43() const { return v(3, 2); }
    qreal m44() const { return v(3, 3); }

    void setM11(qreal value) { v(0, 0) = value; }
    void setM12(qreal value) { v(0, 1) = value; }
    void setM13(qreal value) { v(0, 2) = value; }
    void setM14(qreal value) { v(0, 3) = value; }
    void setM21(qreal value) { v(1, 0) = value; }
    void setM22(qreal value) { v(1, 1) = value; }
    void setM23(qreal value) { v(1, 2) = value; }
    void setM24(qreal value) { v(1, 3) = value; }
    void setM31(qreal value) { v(2, 0) = value; }
    void setM32(qreal value) { v(2, 1) = value; }
    void setM33(qreal value) { v(2, 2) = value; }
    void setM34(qreal value) { v(2, 3) = value; }
    void setM41(qreal value) { v(3, 0) = value; }
    void setM42(qreal value) { v(3, 1) = value; }
    void setM43(qreal value) { v(3, 2) = value; }
    void setM44(qreal value) { v(3, 3) = value; }
};

class Q_AUTOTEST_EXPORT QQuickFontValueType : public QQmlValueTypeBase<QFont>
{
    Q_OBJECT
    Q_ENUMS(FontWeight)
    Q_ENUMS(Capitalization)

    Q_PROPERTY(QString family READ family WRITE setFamily)
    Q_PROPERTY(bool bold READ bold WRITE setBold)
    Q_PROPERTY(FontWeight weight READ weight WRITE setWeight)
    Q_PROPERTY(bool italic READ italic WRITE setItalic)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline)
    Q_PROPERTY(bool overline READ overline WRITE setOverline)
    Q_PROPERTY(bool strikeout READ strikeout WRITE setStrikeout)
    Q_PROPERTY(qreal pointSize READ pointSize WRITE setPointSize)
    Q_PROPERTY(int pixelSize READ pixelSize WRITE setPixelSize)
    Q_PROPERTY(Capitalization capitalization READ capitalization WRITE setCapitalization)
    Q_PROPERTY(qreal letterSpacing READ letterSpacing WRITE setLetterSpacing)
    Q_PROPERTY(qreal wordSpacing READ wordSpacing WRITE setWordSpacing)

public:
    enum FontWeight { Light = QFont::Light,
                       Normal = QFont::Normal,
                       DemiBold = QFont::DemiBold,
                       Bold = QFont::Bold,
                       Black = QFont::Black };
    enum Capitalization { MixedCase = QFont::MixedCase,
                           AllUppercase = QFont::AllUppercase,
                           AllLowercase = QFont::AllLowercase,
                           SmallCaps = QFont::SmallCaps,
                           Capitalize = QFont::Capitalize };

    QQuickFontValueType(QObject *parent = 0);

    virtual QString toString() const;

    QString family() const;
    void setFamily(const QString &);

    bool bold() const;
    void setBold(bool b);

    FontWeight weight() const;
    void setWeight(FontWeight);

    bool italic() const;
    void setItalic(bool b);

    bool underline() const;
    void setUnderline(bool b);

    bool overline() const;
    void setOverline(bool b);

    bool strikeout() const;
    void setStrikeout(bool b);

    qreal pointSize() const;
    void setPointSize(qreal size);

    int pixelSize() const;
    void setPixelSize(int size);

    Capitalization capitalization() const;
    void setCapitalization(Capitalization);

    qreal letterSpacing() const;
    void setLetterSpacing(qreal spacing);

    qreal wordSpacing() const;
    void setWordSpacing(qreal spacing);

    void onLoad();

private:
    bool pixelSizeSet;
    bool pointSizeSet;
    mutable QQmlNullableValue<int> dpi;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKVALUETYPES_P_H