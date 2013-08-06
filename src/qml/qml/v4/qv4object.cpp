/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4argumentsobject_p.h"
#include "qv4mm_p.h"
#include "qv4lookup_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <stdint.h>
#include "qv4alloca_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(Object);

Object::Object(ExecutionEngine *engine)
    : Managed(engine->emptyClass)
    , prototype(0)
    , memberDataAlloc(InlinePropertySize), memberData(inlineProperties)
    , arrayOffset(0), arrayDataLen(0), arrayAlloc(0), arrayAttributes(0), arrayData(0), sparseArray(0)
{
    vtbl = &static_vtbl;
    type = Type_Object;
    memset(memberData, 0, sizeof(Property)*memberDataAlloc);
}

Object::Object(ExecutionContext *context)
    : Managed(context->engine->emptyClass)
    , prototype(0)
    , memberDataAlloc(InlinePropertySize), memberData(inlineProperties)
    , arrayOffset(0), arrayDataLen(0), arrayAlloc(0), arrayAttributes(0), arrayData(0), sparseArray(0)
{
    vtbl = &static_vtbl;
    type = Type_Object;
    memset(memberData, 0, sizeof(Property)*memberDataAlloc);
}

Object::Object(ExecutionEngine *engine, InternalClass *internalClass)
    : Managed(internalClass)
    , prototype(0)
    , memberDataAlloc(InlinePropertySize), memberData(inlineProperties)
    , arrayOffset(0), arrayDataLen(0), arrayAlloc(0), arrayAttributes(0), arrayData(0), sparseArray(0)
{
    vtbl = &static_vtbl;
    type = Type_Object;

    if (internalClass->size >= memberDataAlloc) {
        memberDataAlloc = internalClass->size;
        memberData = new Property[memberDataAlloc];
    }
    memset(memberData, 0, sizeof(Property)*memberDataAlloc);
}

Object::~Object()
{
    if (memberData != inlineProperties)
        delete [] memberData;
    delete [] (arrayData - (sparseArray ? 0 : arrayOffset));
    if (arrayAttributes)
        delete [] (arrayAttributes - (sparseArray ? 0 : arrayOffset));
    delete sparseArray;
    _data = 0;
}

void Object::destroy(Managed *that)
{
    static_cast<Object *>(that)->~Object();
}

void Object::put(ExecutionContext *ctx, const QString &name, const Value &value)
{
    put(ctx->engine->newString(name), value);
}

Value Object::getValue(const Value &thisObject, const Property *p, PropertyAttributes attrs)
{
    if (!attrs.isAccessor())
        return p->value;
    FunctionObject *getter = p->getter();
    if (!getter)
        return Value::undefinedValue();

    return getter->call(thisObject, 0, 0);
}

void Object::putValue(Property *pd, PropertyAttributes attrs, const Value &value)
{
    if (attrs.isAccessor()) {
        if (pd->set) {
            Value args[1];
            args[0] = value;
            pd->set->call(Value::fromObject(this), args, 1);
            return;
        }
        goto reject;
    }

    if (!attrs.isWritable())
        goto reject;

    pd->value = value;
    return;

  reject:
    if (engine()->current->strictMode)
        engine()->current->throwTypeError();

}

void Object::inplaceBinOp(ExecutionContext *ctx, BinOp op, String *name, const Value &rhs)
{
    Value v = get(name);
    Value result;
    op(ctx, &result, v, rhs);
    put(name, result);
}

void Object::inplaceBinOp(ExecutionContext *ctx, BinOp op, const Value &index, const Value &rhs)
{
    uint idx = index.asArrayIndex();
    if (idx < UINT_MAX) {
        bool hasProperty = false;
        Value v = getIndexed(idx, &hasProperty);
        Value result;
        op(ctx, &result, v, rhs);
        putIndexed(idx, result);
        return;
    }
    String *name = index.toString(ctx);
    assert(name);
    inplaceBinOp(ctx, op, name, rhs);
}

void Object::defineDefaultProperty(String *name, Value value)
{
    Property *pd = insertMember(name, Attr_Data|Attr_NotEnumerable);
    pd->value = value;
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value value)
{
    defineDefaultProperty(context->engine->newIdentifier(name), value);
}

void Object::defineDefaultProperty(ExecutionEngine *engine, const QString &name, Value value)
{
    defineDefaultProperty(engine->newIdentifier(name), value);
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(SimpleCallContext *), int argumentCount)
{
    Q_UNUSED(argumentCount);
    String *s = context->engine->newIdentifier(name);
    FunctionObject* function = context->engine->newBuiltinFunction(context, s, code);
    function->defineReadonlyProperty(context->engine->id_length, Value::fromInt32(argumentCount));
    defineDefaultProperty(s, Value::fromObject(function));
}

void Object::defineDefaultProperty(ExecutionEngine *engine, const QString &name, Value (*code)(SimpleCallContext *), int argumentCount)
{
    Q_UNUSED(argumentCount);
    String *s = engine->newIdentifier(name);
    FunctionObject* function = engine->newBuiltinFunction(engine->rootContext, s, code);
    function->defineReadonlyProperty(engine->id_length, Value::fromInt32(argumentCount));
    defineDefaultProperty(s, Value::fromObject(function));
}

void Object::defineAccessorProperty(ExecutionEngine *engine, const QString &name,
                                    Value (*getter)(SimpleCallContext *), Value (*setter)(SimpleCallContext *))
{
    String *s = engine->newString(name);
    defineAccessorProperty(s, getter, setter);
}

void Object::defineAccessorProperty(String *name, Value (*getter)(SimpleCallContext *), Value (*setter)(SimpleCallContext *))
{
    ExecutionEngine *v4 = engine();
    Property *p = insertMember(name, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);
    if (getter)
        p->setGetter(v4->newBuiltinFunction(v4->rootContext, name, getter));
    if (setter)
        p->setSetter(v4->newBuiltinFunction(v4->rootContext, name, setter));
}

void Object::defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value)
{
    defineReadonlyProperty(engine->newIdentifier(name), value);
}

void Object::defineReadonlyProperty(String *name, Value value)
{
    Property *pd = insertMember(name, Attr_ReadOnly);
    pd->value = value;
}

void Object::markObjects(Managed *that)
{
    Object *o = static_cast<Object *>(that);
    if (o->prototype)
        o->prototype->mark();

    for (int i = 0; i < o->internalClass->size; ++i) {
        const Property &pd = o->memberData[i];
        if (o->internalClass->propertyData[i].isData()) {
            if (Managed *m = pd.value.asManaged())
                m->mark();
         } else {
            if (pd.getter())
                pd.getter()->mark();
            if (pd.setter())
                pd.setter()->mark();
        }
    }
    o->markArrayObjects();
}

Property *Object::insertMember(String *s, PropertyAttributes attributes)
{
    uint idx;
    internalClass = internalClass->addMember(s, attributes, &idx);

    if (idx >= memberDataAlloc) {
        memberDataAlloc = qMax((uint)8, 2*memberDataAlloc);
        Property *newMemberData = new Property[memberDataAlloc];
        memcpy(newMemberData, memberData, sizeof(Property)*idx);
        memset(newMemberData + idx, 0, sizeof(Property)*(memberDataAlloc - idx));
        if (memberData != inlineProperties)
            delete [] memberData;
        memberData = newMemberData;
    }
    return memberData + idx;
}

// Section 8.12.1
Property *Object::__getOwnProperty__(String *name, PropertyAttributes *attrs)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __getOwnProperty__(idx, attrs);

    uint member = internalClass->find(name);
    if (member < UINT_MAX) {
        if (attrs)
            *attrs = internalClass->propertyData[member];
        return memberData + member;
    }

    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

Property *Object::__getOwnProperty__(uint index, PropertyAttributes *attrs)
{
    uint pidx = propertyIndexFromArrayIndex(index);
    if (pidx < UINT_MAX) {
        Property *p = arrayData + pidx;
        if (!arrayAttributes || arrayAttributes[pidx].isData()) {
            if (attrs)
                *attrs = arrayAttributes ? arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
            return p;
        } else if (arrayAttributes[pidx].isAccessor()) {
            if (attrs)
                *attrs = arrayAttributes ? arrayAttributes[pidx] : PropertyAttributes(Attr_Accessor);
            return p;
        }
    }
    if (isStringObject()) {
        if (attrs)
            *attrs = Attr_NotConfigurable|Attr_NotWritable;
        return static_cast<StringObject *>(this)->getIndex(index);
    }

    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

// Section 8.12.2
Property *Object::__getPropertyDescriptor__(String *name, PropertyAttributes *attrs) const
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __getPropertyDescriptor__(idx);


    const Object *o = this;
    while (o) {
        uint idx = o->internalClass->find(name);
        if (idx < UINT_MAX) {
            if (attrs)
                *attrs = o->internalClass->propertyData[idx];
            return o->memberData + idx;
        }

        o = o->prototype;
    }
    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

Property *Object::__getPropertyDescriptor__(uint index, PropertyAttributes *attrs) const
{
    const Object *o = this;
    while (o) {
        uint pidx = o->propertyIndexFromArrayIndex(index);
        if (pidx < UINT_MAX) {
            Property *p = o->arrayData + pidx;
            if (!o->arrayAttributes || !o->arrayAttributes[pidx].isGeneric()) {
                if (attrs)
                    *attrs = o->arrayAttributes ? o->arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
                return p;
            }
        }
        if (o->isStringObject()) {
            Property *p = static_cast<const StringObject *>(o)->getIndex(index);
            if (p) {
                if (attrs)
                    *attrs = (Attr_NotWritable|Attr_NotConfigurable);
                return p;
            }
        }
        o = o->prototype;
    }
    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

bool Object::__hasProperty__(String *name) const
{
    if (__getPropertyDescriptor__(name))
        return true;
    return !query(name).isEmpty();
}

Value Object::get(Managed *m, String *name, bool *hasProperty)
{
    return static_cast<Object *>(m)->internalGet(name, hasProperty);
}

Value Object::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    return static_cast<Object *>(m)->internalGetIndexed(index, hasProperty);
}

void Object::put(Managed *m, String *name, const Value &value)
{
    static_cast<Object *>(m)->internalPut(name, value);
}

void Object::putIndexed(Managed *m, uint index, const Value &value)
{
    static_cast<Object *>(m)->internalPutIndexed(index, value);
}

PropertyAttributes Object::query(const Managed *m, String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return queryIndexed(m, idx);

    const Object *o = static_cast<const Object *>(m);
    while (o) {
        uint idx = o->internalClass->find(name);
        if (idx < UINT_MAX)
            return o->internalClass->propertyData[idx];

        o = o->prototype;
    }
    return Attr_Invalid;
}

PropertyAttributes Object::queryIndexed(const Managed *m, uint index)
{
    const Object *o = static_cast<const Object *>(m);
    while (o) {
        uint pidx = o->propertyIndexFromArrayIndex(index);
        if (pidx < UINT_MAX) {
            if (o->arrayAttributes)
                return o->arrayAttributes[pidx];
            return Attr_Data;
        }
        if (o->isStringObject()) {
            Property *p = static_cast<const StringObject *>(o)->getIndex(index);
            if (p)
                return Attr_Data;
        }
        o = o->prototype;
    }
    return Attr_Invalid;
}

bool Object::deleteProperty(Managed *m, String *name)
{
    return static_cast<Object *>(m)->internalDeleteProperty(name);
}

bool Object::deleteIndexedProperty(Managed *m, uint index)
{
    return static_cast<Object *>(m)->internalDeleteIndexedProperty(index);
}

void Object::getLookup(Managed *m, Lookup *l, Value *result)
{
    Object *o = static_cast<Object *>(m);
    PropertyAttributes attrs;
    Property *p = l->lookup(o, &attrs);
    if (p) {
        if (attrs.isData()) {
            if (l->level == 0)
                l->getter = Lookup::getter0;
            else if (l->level == 1)
                l->getter = Lookup::getter1;
            else if (l->level == 2)
                l->getter = Lookup::getter2;
            if (result)
                *result = p->value;
            return;
        } else {
            if (l->level == 0)
                l->getter = Lookup::getterAccessor0;
            else if (l->level == 1)
                l->getter = Lookup::getterAccessor1;
            else if (l->level == 2)
                l->getter = Lookup::getterAccessor2;
            if (result)
                *result = p->value;
            Value res = o->getValue(p, attrs);
            if (result)
                *result = res;
            return;
        }
    } else if (result) {
        *result = Value::undefinedValue();
    }
}

void Object::setLookup(Managed *m, Lookup *l, const Value &value)
{
    Object *o = static_cast<Object *>(m);

    uint idx = o->internalClass->find(l->name);
    if (!o->isArrayObject() || idx != ArrayObject::LengthPropertyIndex) {
        if (idx != UINT_MAX && o->internalClass->propertyData[idx].isData() && o->internalClass->propertyData[idx].isWritable()) {
            l->classList[0] = o->internalClass;
            l->index = idx;
            l->setter = Lookup::setter0;
            o->memberData[idx].value = value;
            return;
        }

        if (idx != UINT_MAX) {
            o->putValue(o->memberData + idx, o->internalClass->propertyData[idx], value);
            return;
        }
    }

    o->put(l->name, value);
}

Property *Object::advanceIterator(Managed *m, ObjectIterator *it, String **name, uint *index, PropertyAttributes *attrs)
{
    Object *o = static_cast<Object *>(m);
    *name = 0;
    *index = UINT_MAX;

    if (!it->arrayIndex)
        it->arrayNode = o->sparseArrayBegin();

    // sparse arrays
    if (it->arrayNode) {
        while (it->arrayNode != o->sparseArrayEnd()) {
            int k = it->arrayNode->key();
            uint pidx = it->arrayNode->value;
            Property *p = o->arrayData + pidx;
            it->arrayNode = it->arrayNode->nextNode();
            PropertyAttributes a = o->arrayAttributes ? o->arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
            if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
                it->arrayIndex = k + 1;
                *index = k;
                if (attrs)
                    *attrs = a;
                return p;
            }
        }
        it->arrayNode = 0;
        it->arrayIndex = UINT_MAX;
    }
    // dense arrays
    while (it->arrayIndex < o->arrayDataLen) {
        uint pidx = o->propertyIndexFromArrayIndex(it->arrayIndex);
        Property *p = o->arrayData + pidx;
        PropertyAttributes a = o->arrayAttributes ? o->arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
        ++it->arrayIndex;
        if ((!o->arrayAttributes || !o->arrayAttributes[pidx].isGeneric())
            && (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable())) {
            *index = it->arrayIndex - 1;
            if (attrs)
                *attrs = a;
            return p;
        }
    }

    while (it->memberIndex < o->internalClass->size) {
        String *n = o->internalClass->nameMap.at(it->memberIndex);
        assert(n);

        Property *p = o->memberData + it->memberIndex;
        PropertyAttributes a = o->internalClass->propertyData[it->memberIndex];
        ++it->memberIndex;
        if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
            *name = n;
            if (attrs)
                *attrs = a;
            return p;
        }
    }

    return 0;
}

// Section 8.12.3
Value Object::internalGet(String *name, bool *hasProperty)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return getIndexed(idx, hasProperty);

    name->makeIdentifier();

    Object *o = this;
    while (o) {
        uint idx = o->internalClass->find(name);
        if (idx < UINT_MAX) {
            if (hasProperty)
                *hasProperty = true;
            return getValue(o->memberData + idx, o->internalClass->propertyData.at(idx));
        }

        o = o->prototype;
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value Object::internalGetIndexed(uint index, bool *hasProperty)
{
    Property *pd = 0;
    PropertyAttributes attrs = Attr_Data;
    Object *o = this;
    while (o) {
        uint pidx = o->propertyIndexFromArrayIndex(index);
        if (pidx < UINT_MAX) {
            if (!o->arrayAttributes || !o->arrayAttributes[pidx].isGeneric()) {
                pd = o->arrayData + pidx;
                if (o->arrayAttributes)
                    attrs = o->arrayAttributes[pidx];
                break;
            }
        }
        if (o->isStringObject()) {
            pd = static_cast<StringObject *>(o)->getIndex(index);
            if (pd) {
                attrs = (Attr_NotWritable|Attr_NotConfigurable);
                break;
            }
        }
        o = o->prototype;
    }

    if (pd) {
        if (hasProperty)
            *hasProperty = true;
        return getValue(pd, attrs);
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}


// Section 8.12.5
void Object::internalPut(String *name, const Value &value)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return putIndexed(idx, value);

    name->makeIdentifier();

    uint member = internalClass->find(name);
    Property *pd = 0;
    PropertyAttributes attrs;
    if (member < UINT_MAX) {
        pd = memberData + member;
        attrs = internalClass->propertyData[member];
    }

    // clause 1
    if (pd) {
        if (attrs.isAccessor()) {
            if (pd->setter())
                goto cont;
            goto reject;
        } else if (!attrs.isWritable())
            goto reject;
        else if (isArrayObject() && name->isEqualTo(engine()->id_length)) {
            bool ok;
            uint l = value.asArrayLength(&ok);
            if (!ok)
                engine()->current->throwRangeError(value);
            ok = setArrayLength(l);
            if (!ok)
                goto reject;
        } else {
            pd->value = value;
        }
        return;
    } else if (!prototype) {
        if (!extensible)
            goto reject;
    } else {
        // clause 4
        if ((pd = prototype->__getPropertyDescriptor__(name, &attrs))) {
            if (attrs.isAccessor()) {
                if (!pd->setter())
                    goto reject;
            } else if (!extensible || !attrs.isWritable()) {
                goto reject;
            }
        } else if (!extensible) {
            goto reject;
        }
    }

    cont:

    // Clause 5
    if (pd && attrs.isAccessor()) {
        assert(pd->setter() != 0);

        Value args[1];
        args[0] = value;
        pd->setter()->call(Value::fromObject(this), args, 1);
        return;
    }

    {
        Property *p = insertMember(name, Attr_Data);
        p->value = value;
        return;
    }

  reject:
    if (engine()->current->strictMode) {
        QString message = QStringLiteral("Cannot assign to read-only property \"");
        message += name->toQString();
        message += QLatin1Char('\"');
        engine()->current->throwTypeError(message);
    }
}

void Object::internalPutIndexed(uint index, const Value &value)
{
    Property *pd = 0;
    PropertyAttributes attrs;

    uint pidx = propertyIndexFromArrayIndex(index);
    if (pidx < UINT_MAX) {
        if (arrayAttributes && arrayAttributes[pidx].isGeneric()) {
            pidx = UINT_MAX;
        } else {
            pd = arrayData + pidx;
            attrs = arrayAttributes ? arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
        }
    }

    if (!pd && isStringObject()) {
        pd = static_cast<StringObject *>(this)->getIndex(index);
        if (pd)
            // not writable
            goto reject;
    }

    // clause 1
    if (pd) {
        if (attrs.isAccessor()) {
            if (pd->setter())
                goto cont;
            goto reject;
        } else if (!attrs.isWritable())
            goto reject;
        else
            pd->value = value;
        return;
    } else if (!prototype) {
        if (!extensible)
            goto reject;
    } else {
        // clause 4
        if ((pd = prototype->__getPropertyDescriptor__(index, &attrs))) {
            if (attrs.isAccessor()) {
                if (!pd->setter())
                    goto reject;
            } else if (!extensible || !attrs.isWritable()) {
                goto reject;
            }
        } else if (!extensible) {
            goto reject;
        }
    }

    cont:

    // Clause 5
    if (pd && attrs.isAccessor()) {
        assert(pd->setter() != 0);

        Value args[1];
        args[0] = value;
        pd->setter()->call(Value::fromObject(this), args, 1);
        return;
    }

    arraySet(index, value);
    return;

  reject:
    if (engine()->current->strictMode)
        engine()->current->throwTypeError();
}

// Section 8.12.7
bool Object::internalDeleteProperty(String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return deleteIndexedProperty(idx);

    name->makeIdentifier();

    uint memberIdx = internalClass->find(name);
    if (memberIdx != UINT_MAX) {
        if (internalClass->propertyData[memberIdx].isConfigurable()) {
            internalClass->removeMember(this, name->identifier);
            memmove(memberData + memberIdx, memberData + memberIdx + 1, (internalClass->size - memberIdx)*sizeof(Property));
            return true;
        }
        if (engine()->current->strictMode)
            engine()->current->throwTypeError();
        return false;
    }

    return true;
}

bool Object::internalDeleteIndexedProperty(uint index)
{
    uint pidx = propertyIndexFromArrayIndex(index);
    if (pidx == UINT_MAX)
        return true;
    if (arrayAttributes && arrayAttributes[pidx].isGeneric())
        return true;

    if (!arrayAttributes || arrayAttributes[pidx].isConfigurable()) {
        arrayData[pidx].value = Value::undefinedValue();
        if (!arrayAttributes)
            ensureArrayAttributes();
        arrayAttributes[pidx].clear();
        if (sparseArray) {
            arrayData[pidx].value.int_32 = arrayFreeList;
            arrayFreeList = pidx;
        }
        return true;
    }

    if (engine()->current->strictMode)
        engine()->current->throwTypeError();
    return false;
}

// Section 8.12.9
bool Object::__defineOwnProperty__(ExecutionContext *ctx, String *name, const Property &p, PropertyAttributes attrs)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __defineOwnProperty__(ctx, idx, p, attrs);

    name->makeIdentifier();

    Property *current;
    PropertyAttributes *cattrs;

    if (isArrayObject() && name->isEqualTo(ctx->engine->id_length)) {
        assert(ArrayObject::LengthPropertyIndex == internalClass->find(ctx->engine->id_length));
        Property *lp = memberData + ArrayObject::LengthPropertyIndex;
        cattrs = internalClass->propertyData.data() + ArrayObject::LengthPropertyIndex;
        if (attrs.isEmpty() || p.isSubset(attrs, *lp, *cattrs))
            return true;
        if (!cattrs->isWritable() || attrs.type() == PropertyAttributes::Accessor || attrs.isConfigurable() || attrs.isEnumerable())
            goto reject;
        bool succeeded = true;
        if (attrs.type() == PropertyAttributes::Data) {
            bool ok;
            uint l = p.value.asArrayLength(&ok);
            if (!ok)
                ctx->throwRangeError(p.value);
            succeeded = setArrayLength(l);
        }
        if (attrs.hasWritable() && !attrs.isWritable())
            cattrs->setWritable(false);
        if (!succeeded)
            goto reject;
        return true;
    }

    // Clause 1
    {
        uint member = internalClass->find(name);
        current = (member < UINT_MAX) ? memberData + member : 0;
        cattrs = internalClass->propertyData.data() + member;
    }

    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        Property *pd = insertMember(name, attrs);
        *pd = p;
        pd->fullyPopulated(&attrs);
        return true;
    }

    return __defineOwnProperty__(ctx, current, name, p, attrs);
reject:
  if (ctx->strictMode)
      ctx->throwTypeError();
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs)
{
    Property *current = 0;

    // 15.4.5.1, 4b
    if (isArrayObject() && index >= arrayLength() && !internalClass->propertyData[ArrayObject::LengthPropertyIndex].isWritable())
        goto reject;

    if (isNonStrictArgumentsObject)
        return static_cast<ArgumentsObject *>(this)->defineOwnProperty(ctx, index, p, attrs);

    // Clause 1
    {
        uint pidx = propertyIndexFromArrayIndex(index);
        if (pidx < UINT_MAX && (!arrayAttributes || !arrayAttributes[pidx].isGeneric()))
            current = arrayData + pidx;
        if (!current && isStringObject())
            current = static_cast<StringObject *>(this)->getIndex(index);
    }

    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        Property *pd = arrayInsert(index, attrs);
        *pd = p;
        pd->fullyPopulated(&attrs);
        return true;
    }

    return __defineOwnProperty__(ctx, current, 0 /*member*/, p, attrs);
reject:
  if (ctx->strictMode)
      ctx->throwTypeError();
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, Property *current, String *member, const Property &p, PropertyAttributes attrs)
{
    // clause 5
    if (attrs.isEmpty())
        return true;

    PropertyAttributes cattrs = Attr_Data;
    if (member)
        cattrs = internalClass->propertyData[current - memberData];
    else if (arrayAttributes)
        cattrs = arrayAttributes[current - arrayData];

    // clause 6
    if (p.isSubset(attrs, *current, cattrs))
        return true;

    // clause 7
    if (!cattrs.isConfigurable()) {
        if (attrs.isConfigurable())
            goto reject;
        if (attrs.hasEnumerable() && attrs.isEnumerable() != cattrs.isEnumerable())
            goto reject;
    }

    // clause 8
    if (attrs.isGeneric())
        goto accept;

    // clause 9
    if (cattrs.isData() != attrs.isData()) {
        // 9a
        if (!cattrs.isConfigurable())
            goto reject;
        if (cattrs.isData()) {
            // 9b
            cattrs.setType(PropertyAttributes::Accessor);
            cattrs.clearWritable();
            current->setGetter(0);
            current->setSetter(0);
        } else {
            // 9c
            cattrs.setType(PropertyAttributes::Data);
            cattrs.setWritable(false);
            current->value = Value::undefinedValue();
        }
    } else if (cattrs.isData() && attrs.isData()) { // clause 10
        if (!cattrs.isConfigurable() && !cattrs.isWritable()) {
            if (attrs.isWritable() || !current->value.sameValue(p.value))
                goto reject;
        }
    } else { // clause 10
        assert(cattrs.isAccessor() && attrs.isAccessor());
        if (!cattrs.isConfigurable()) {
            if (p.getter() && !(current->getter() == p.getter() || (!current->getter() && (quintptr)p.getter() == 0x1)))
                goto reject;
            if (p.setter() && !(current->setter() == p.setter() || (!current->setter() && (quintptr)p.setter() == 0x1)))
                goto reject;
        }
    }

  accept:

    current->merge(cattrs, p, attrs);
    if (member) {
        internalClass = internalClass->changeMember(member, cattrs);
    } else {
        if (cattrs != Attr_Data)
            ensureArrayAttributes();
        if (arrayAttributes)
            arrayAttributes[current - arrayData] = cattrs;
    }
    return true;
  reject:
    if (ctx->strictMode)
        ctx->throwTypeError();
    return false;
}


bool Object::__defineOwnProperty__(ExecutionContext *ctx, const QString &name, const Property &p, PropertyAttributes attrs)
{
    return __defineOwnProperty__(ctx, ctx->engine->newString(name), p, attrs);
}


void Object::copyArrayData(Object *other)
{
    arrayReserve(other->arrayDataLen);
    arrayDataLen = other->arrayDataLen;
    memcpy(arrayData, other->arrayData, arrayDataLen*sizeof(Property));
    arrayOffset = 0;
    if (other->sparseArray) {
        sparseArray = new SparseArray(*other->sparseArray);
        arrayFreeList = other->arrayFreeList;
    }
    if (isArrayObject())
        setArrayLengthUnchecked(other->arrayLength());
}


Value Object::arrayIndexOf(Value v, uint fromIndex, uint endIndex, ExecutionContext *ctx, Object *o)
{
    bool protoHasArray = false;
    Object *p = o;
    while ((p = p->prototype))
        if (p->arrayDataLen)
            protoHasArray = true;

    if (protoHasArray || o->arrayAttributes) {
        // lets be safe and slow
        for (uint i = fromIndex; i < endIndex; ++i) {
            bool exists;
            Value value = o->getIndexed(i, &exists);
            if (exists && __qmljs_strict_equal(value, v))
                return Value::fromDouble(i);
        }
    } else if (sparseArray) {
        for (SparseArrayNode *n = sparseArray->lowerBound(fromIndex); n != sparseArray->end() && n->key() < endIndex; n = n->nextNode()) {
            Value value = o->getValue(arrayData + n->value, arrayAttributes ? arrayAttributes[n->value] : Attr_Data);
            if (__qmljs_strict_equal(value, v))
                return Value::fromDouble(n->key());
        }
    } else {
        if ((int) endIndex > arrayDataLen)
            endIndex = arrayDataLen;
        Property *pd = arrayData;
        Property *end = pd + endIndex;
        pd += fromIndex;
        while (pd < end) {
            if (!arrayAttributes || !arrayAttributes[pd - arrayData].isGeneric()) {
                Value value = o->getValue(pd, arrayAttributes ? arrayAttributes[pd - arrayData] : Attr_Data);
                if (__qmljs_strict_equal(value, v))
                    return Value::fromDouble(pd - arrayData);
            }
            ++pd;
        }
    }
    return Value::fromInt32(-1);
}

void Object::arrayConcat(const ArrayObject *other)
{
    int newLen = arrayDataLen + other->arrayLength();
    if (other->sparseArray)
        initSparse();
    // ### copy attributes as well!
    if (sparseArray) {
        if (other->sparseArray) {
            for (const SparseArrayNode *it = other->sparseArray->begin(); it != other->sparseArray->end(); it = it->nextNode())
                arraySet(arrayDataLen + it->key(), other->arrayData + it->value);
        } else {
            int oldSize = arrayDataLen;
            arrayReserve(oldSize + other->arrayLength());
            memcpy(arrayData + oldSize, other->arrayData, other->arrayLength()*sizeof(Property));
            if (arrayAttributes)
                std::fill(arrayAttributes + oldSize, arrayAttributes + oldSize + other->arrayLength(), PropertyAttributes(Attr_Data));
            for (uint i = 0; i < other->arrayLength(); ++i) {
                SparseArrayNode *n = sparseArray->insert(arrayDataLen + i);
                n->value = oldSize + i;
            }
        }
    } else {
        int oldSize = arrayLength();
        arrayReserve(oldSize + other->arrayDataLen);
        if (oldSize > arrayDataLen) {
            ensureArrayAttributes();
            std::fill(arrayAttributes + arrayDataLen, arrayAttributes + oldSize, PropertyAttributes());
        }
        arrayDataLen = oldSize + other->arrayDataLen;
        if (other->arrayAttributes) {
            for (int i = 0; i < arrayDataLen; ++i) {
                bool exists;
                arrayData[oldSize + i].value = const_cast<ArrayObject *>(other)->getIndexed(i, &exists);
                if (arrayAttributes)
                    arrayAttributes[oldSize + i] = Attr_Data;
                if (!exists) {
                    ensureArrayAttributes();
                    arrayAttributes[oldSize + i].clear();
                }
            }
        } else {
            memcpy(arrayData + oldSize, other->arrayData, other->arrayDataLen*sizeof(Property));
            if (arrayAttributes)
                std::fill(arrayAttributes + oldSize, arrayAttributes + oldSize + other->arrayDataLen, PropertyAttributes(Attr_Data));
        }
    }
    setArrayLengthUnchecked(newLen);
}

void Object::arraySort(ExecutionContext *context, Object *thisObject, const Value &comparefn, uint len)
{
    if (!arrayDataLen)
        return;

    if (sparseArray) {
        context->throwUnimplemented("Object::sort unimplemented for sparse arrays");
        return;
    }

    if (len > arrayDataLen)
        len = arrayDataLen;

    // The spec says the sorting goes through a series of get,put and delete operations.
    // this implies that the attributes don't get sorted around.
    // behavior of accessor properties is implementation defined. We simply turn them all
    // into data properties and then sort. This is in line with the sentence above.
    if (arrayAttributes) {
        for (uint i = 0; i < len; i++) {
            if (arrayAttributes[i].isGeneric()) {
                while (--len > i)
                    if (!arrayAttributes[len].isGeneric())
                        break;
                arrayData[i].value = getValue(arrayData + len, arrayAttributes[len]);
                arrayAttributes[i] = Attr_Data;
                arrayAttributes[len].clear();
            } else if (arrayAttributes[i].isAccessor()) {
                arrayData[i].value = getValue(arrayData + i, arrayAttributes[i]);
                arrayAttributes[i] = Attr_Data;
            }
        }
    }

    ArrayElementLessThan lessThan(context, thisObject, comparefn);

    Property *begin = arrayData;
    // We deliberately choose qSort over std::sort here, because with
    // MSVC in debug builds, std::sort has an ASSERT() that verifies
    // that the return values of lessThan are perfectly consistent
    // and aborts otherwise. We do not want JavaScript to easily crash
    // the entire application and therefore choose qSort, which doesn't
    // have this property.
    qSort(begin, begin + len, lessThan);
}


void Object::initSparse()
{
    if (!sparseArray) {
        sparseArray = new SparseArray;
        for (int i = 0; i < arrayDataLen; ++i) {
            if (!arrayAttributes || !arrayAttributes[i].isGeneric()) {
                SparseArrayNode *n = sparseArray->insert(i);
                n->value = i + arrayOffset;
            }
        }

        uint off = arrayOffset;
        if (!arrayOffset) {
            arrayFreeList = arrayDataLen;
        } else {
            arrayFreeList = 0;
            arrayData -= off;
            arrayAlloc += off;
            int o = off;
            for (int i = 0; i < o - 1; ++i) {
                arrayData[i].value = Value::fromInt32(i + 1);
            }
            arrayData[o - 1].value = Value::fromInt32(arrayDataLen + off);
        }
        for (int i = arrayDataLen + off; i < arrayAlloc; ++i) {
            arrayData[i].value = Value::fromInt32(i + 1);
        }
    }
}

void Object::arrayReserve(uint n)
{
    if (n < 8)
        n = 8;
    if (n >= arrayAlloc) {
        uint off;
        if (sparseArray) {
            assert(arrayFreeList == arrayAlloc);
            // ### FIXME
            arrayDataLen = arrayAlloc;
            off = 0;
        } else {
            off = arrayOffset;
        }
        arrayAlloc = qMax(n, 2*arrayAlloc);
        Property *newArrayData = new Property[arrayAlloc];
        if (arrayData) {
            memcpy(newArrayData, arrayData, sizeof(Property)*arrayDataLen);
            delete [] (arrayData - off);
        }
        arrayData = newArrayData;
        if (sparseArray) {
            for (uint i = arrayFreeList; i < arrayAlloc; ++i) {
                arrayData[i].value = Value::emptyValue();
                arrayData[i].value = Value::fromInt32(i + 1);
            }
        } else {
            arrayOffset = 0;
        }

        if (arrayAttributes) {
            PropertyAttributes *newAttrs = new PropertyAttributes[arrayAlloc];
            memcpy(newAttrs, arrayAttributes, sizeof(PropertyAttributes)*arrayDataLen);
            delete [] (arrayAttributes - off);

            arrayAttributes = newAttrs;
            if (sparseArray) {
                for (uint i = arrayFreeList; i < arrayAlloc; ++i)
                    arrayAttributes[i] = Attr_Invalid;
            }
        }
    }
}

void Object::ensureArrayAttributes()
{
    if (arrayAttributes)
        return;

    arrayAttributes = new PropertyAttributes[arrayAlloc];
    for (uint i = 0; i < arrayDataLen; ++i)
        arrayAttributes[i] = Attr_Data;
    for (uint i = arrayDataLen; i < arrayAlloc; ++i)
        arrayAttributes[i] = Attr_Invalid;
}


bool Object::setArrayLength(uint newLen) {
    assert(isArrayObject());
    const Property *lengthProperty = memberData + ArrayObject::LengthPropertyIndex;
    if (lengthProperty && !internalClass->propertyData[ArrayObject::LengthPropertyIndex].isWritable())
        return false;
    uint oldLen = arrayLength();
    bool ok = true;
    if (newLen < oldLen) {
        if (sparseArray) {
            SparseArrayNode *begin = sparseArray->lowerBound(newLen);
            if (begin != sparseArray->end()) {
                SparseArrayNode *it = sparseArray->end()->previousNode();
                while (1) {
                    Property &pd = arrayData[it->value];
                    if (arrayAttributes) {
                        if (!arrayAttributes[it->value].isConfigurable()) {
                            ok = false;
                            newLen = it->key() + 1;
                            break;
                        } else {
                            arrayAttributes[it->value].clear();
                        }
                    }
                    pd.value.tag = Value::_Empty_Type;
                    pd.value.int_32 = arrayFreeList;
                    arrayFreeList = it->value;
                    bool brk = (it == begin);
                    SparseArrayNode *prev = it->previousNode();
                    sparseArray->erase(it);
                    if (brk)
                        break;
                    it = prev;
                }
            }
        } else {
            Property *it = arrayData + arrayDataLen;
            const Property *begin = arrayData + newLen;
            while (--it >= begin) {
                if (arrayAttributes) {
                    if (!arrayAttributes[it - arrayData].isEmpty() && !arrayAttributes[it - arrayData].isConfigurable()) {
                        ok = false;
                        newLen = it - arrayData + 1;
                        break;
                    } else {
                        arrayAttributes[it - arrayData].clear();
                    }
                    it->value = Value::emptyValue();
                }
            }
            arrayDataLen = newLen;
        }
    } else {
        if (newLen >= 0x100000)
            initSparse();
    }
    setArrayLengthUnchecked(newLen);
    return ok;
}

void Object::markArrayObjects() const
{
    for (uint i = 0; i < arrayDataLen; ++i) {
        const Property &pd = arrayData[i];
        if (!arrayAttributes || arrayAttributes[i].isData()) {
            if (Managed *m = pd.value.asManaged())
                m->mark();
         } else if (arrayAttributes[i].isAccessor()) {
            if (pd.getter())
                pd.getter()->mark();
            if (pd.setter())
                pd.setter()->mark();
        }
    }
}

ArrayObject::ArrayObject(ExecutionEngine *engine, const QStringList &list)
    : Object(engine)
{
    init(engine);

    // Converts a QStringList to JS.
    // The result is a new Array object with length equal to the length
    // of the QStringList, and the elements being the QStringList's
    // elements converted to JS Strings.
    int len = list.count();
    arrayReserve(len);
    for (int ii = 0; ii < len; ++ii)
        arrayData[ii].value = Value::fromString(engine->newString(list.at(ii)));
    setArrayLengthUnchecked(len);
}

void ArrayObject::init(ExecutionEngine *engine)
{
    type = Type_ArrayObject;
    internalClass = engine->arrayClass;

    memberData = new Property[4];
    memberData[LengthPropertyIndex].value = Value::fromInt32(0);
}

QStringList ArrayObject::toQStringList() const
{
    QStringList result;

    QV4::ExecutionEngine *engine = internalClass->engine;

    uint32_t length = arrayLength();
    for (uint32_t i = 0; i < length; ++i)
        result.append(const_cast<ArrayObject *>(this)->getIndexed(i).toString(engine->current)->toQString());
    return result;
}


DEFINE_MANAGED_VTABLE(ForEachIteratorObject);

void ForEachIteratorObject::markObjects(Managed *that)
{
    ForEachIteratorObject *o = static_cast<ForEachIteratorObject *>(that);
    Object::markObjects(that);
    if (o->it.object)
        o->it.object->mark();
}