/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once

#include "JSDOMWrapper.h"
#include "TestNamedGetterNoIdentifier.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestNamedGetterNoIdentifier : public JSDOMWrapper<TestNamedGetterNoIdentifier> {
public:
    using Base = JSDOMWrapper<TestNamedGetterNoIdentifier>;
    static JSTestNamedGetterNoIdentifier* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestNamedGetterNoIdentifier>&& impl)
    {
        JSTestNamedGetterNoIdentifier* ptr = new (NotNull, JSC::allocateCell<JSTestNamedGetterNoIdentifier>(globalObject->vm().heap)) JSTestNamedGetterNoIdentifier(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestNamedGetterNoIdentifier* toWrapped(JSC::VM&, JSC::JSValue);
    static bool getOwnPropertySlot(JSC::JSObject*, JSC::ExecState*, JSC::PropertyName, JSC::PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSC::JSObject*, JSC::ExecState*, unsigned propertyName, JSC::PropertySlot&);
    static void getOwnPropertyNames(JSC::JSObject*, JSC::ExecState*, JSC::PropertyNameArray&, JSC::EnumerationMode = JSC::EnumerationMode());
    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
    static void heapSnapshot(JSCell*, JSC::HeapSnapshotBuilder&);
public:
    static const unsigned StructureFlags = JSC::GetOwnPropertySlotIsImpureForPropertyAbsence | JSC::InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | JSC::OverridesGetOwnPropertySlot | JSC::OverridesGetPropertyNames | Base::StructureFlags;
protected:
    JSTestNamedGetterNoIdentifier(JSC::Structure*, JSDOMGlobalObject&, Ref<TestNamedGetterNoIdentifier>&&);

    void finishCreation(JSC::VM&);
};

class JSTestNamedGetterNoIdentifierOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&, const char**);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestNamedGetterNoIdentifier*)
{
    static NeverDestroyed<JSTestNamedGetterNoIdentifierOwner> owner;
    return &owner.get();
}

inline void* wrapperKey(TestNamedGetterNoIdentifier* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject*, TestNamedGetterNoIdentifier&);
inline JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestNamedGetterNoIdentifier* impl) { return impl ? toJS(state, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject*, Ref<TestNamedGetterNoIdentifier>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::ExecState* state, JSDOMGlobalObject* globalObject, RefPtr<TestNamedGetterNoIdentifier>&& impl) { return impl ? toJSNewlyCreated(state, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestNamedGetterNoIdentifier> {
    using WrapperClass = JSTestNamedGetterNoIdentifier;
    using ToWrappedReturnType = TestNamedGetterNoIdentifier*;
};

} // namespace WebCore
