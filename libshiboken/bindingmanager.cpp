/*
 * This file is part of the Shiboken Python Bindings Generator project.
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: PySide team <contact@pyside.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation. Please
 * review the following information to ensure the GNU Lesser General
 * Public License version 2.1 requirements will be met:
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 *
 * As a special exception to the GNU Lesser General Public License
 * version 2.1, the object code form of a "work that uses the Library"
 * may incorporate material from a header file that is part of the
 * Library.  You may distribute such object code under terms of your
 * choice, provided that the incorporated material (i) does not exceed
 * more than 5% of the total size of the Library; and (ii) is limited to
 * numerical parameters, data structure layouts, accessors, macros,
 * inline functions and templates.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "basewrapper.h"
#include "bindingmanager.h"
#include "google/dense_hash_map"

namespace Shiboken
{

typedef google::dense_hash_map<const void*, PyObject*> WrapperMap;

struct BindingManager::BindingManagerPrivate {
    WrapperMap wrapperMapper;
};

BindingManager::BindingManager()
{
    m_d = new BindingManager::BindingManagerPrivate;
    m_d->wrapperMapper.set_empty_key((WrapperMap::key_type)0);
    m_d->wrapperMapper.set_deleted_key((WrapperMap::key_type)1);
}

BindingManager::~BindingManager()
{
    delete m_d;
}

BindingManager& BindingManager::instance() {
    static BindingManager singleton;
    return singleton;
}

bool BindingManager::hasWrapper(const void* cptr)
{
    return m_d->wrapperMapper.count(cptr);
}

void BindingManager::assignWrapper(PyObject* wrapper, const void* cptr)
{
    WrapperMap::iterator iter = m_d->wrapperMapper.find(cptr);
    if (iter == m_d->wrapperMapper.end())
        m_d->wrapperMapper.insert(std::make_pair(cptr, wrapper));
    else
        iter->second = wrapper;
}

void BindingManager::releaseWrapper(void *cptr)
{
    WrapperMap::iterator iter = m_d->wrapperMapper.find(cptr);
    if (iter != m_d->wrapperMapper.end())
        m_d->wrapperMapper.erase(iter);
}

void BindingManager::releaseWrapper(PyObject* wrapper)
{
    releaseWrapper(PyBaseWrapper_cptr(wrapper));
}

PyObject* BindingManager::retrieveWrapper(const void* cptr)
{
    WrapperMap::iterator iter = m_d->wrapperMapper.find(cptr);
    if (iter == m_d->wrapperMapper.end())
        return 0;
    return iter->second;
}

PyObject* BindingManager::getOverride(const void* cptr, const char* methodName)
{
    PyObject* wrapper = retrieveWrapper(cptr);

    if (wrapper) {
        PyTypeObject* baseWrapperType = ((Shiboken::PyBaseWrapper*)wrapper)->baseWrapperType;
        PyObject* method = PyObject_GetAttrString(wrapper, const_cast<char*>(methodName));
        if (method) {
            PyObject* defaultMethod = 0;
            if (PyMethod_Check(method) &&
                ((PyMethodObject*) method)->im_self == wrapper &&
                baseWrapperType->tp_dict != 0) {
                defaultMethod = PyDict_GetItemString(baseWrapperType->tp_dict, const_cast<char*>(methodName));
            }

            if (defaultMethod && ((PyMethodObject*)method)->im_func != defaultMethod)
                return method;

            Py_DECREF(method);
        }
    }

    return 0;
}

} // namespace Shiboken