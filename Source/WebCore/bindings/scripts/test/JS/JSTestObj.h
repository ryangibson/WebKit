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

#include "JSDOMConvertDictionary.h"
#include "JSDOMConvertEnumeration.h"
#include "JSDOMWrapper.h"
#include "TestObj.h"
#include <JavaScriptCore/CallData.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestObj : public JSDOMWrapper<TestObj> {
public:
    using Base = JSDOMWrapper<TestObj>;
    static JSTestObj* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestObj>&& impl)
    {
        JSTestObj* ptr = new (NotNull, JSC::allocateCell<JSTestObj>(globalObject->vm().heap)) JSTestObj(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestObj* toWrapped(JSC::VM&, JSC::JSValue);
    static bool getOwnPropertySlot(JSC::JSObject*, JSC::ExecState*, JSC::PropertyName, JSC::PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSC::JSObject*, JSC::ExecState*, unsigned propertyName, JSC::PropertySlot&);
    static void getOwnPropertyNames(JSC::JSObject*, JSC::ExecState*, JSC::PropertyNameArray&, JSC::EnumerationMode = JSC::EnumerationMode());
    static JSC::CallType getCallData(JSC::JSCell*, JSC::CallData&);

    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
    static JSC::JSObject* serialize(JSC::ExecState&, JSTestObj& thisObject, JSDOMGlobalObject&, JSC::ThrowScope&);
    mutable JSC::WriteBarrier<JSC::Unknown> m_cachedAttribute1;
    mutable JSC::WriteBarrier<JSC::Unknown> m_cachedAttribute2;
    static void visitChildren(JSCell*, JSC::SlotVisitor&);

    static void heapSnapshot(JSCell*, JSC::HeapSnapshotBuilder&);

    // Custom attributes
    JSC::JSValue customAttr(JSC::ExecState&) const;
    void setCustomAttr(JSC::ExecState&, JSC::JSValue);

    // Custom functions
    JSC::JSValue customMethod(JSC::ExecState&);
    JSC::JSValue customMethodWithArgs(JSC::ExecState&);
    static JSC::JSValue classMethod2(JSC::ExecState&);
    JSC::JSValue testCustomPromiseFunction(JSC::ExecState&, Ref<DeferredPromise>&&);
    static JSC::JSValue testStaticCustomPromiseFunction(JSC::ExecState&, Ref<DeferredPromise>&&);
    JSC::JSValue testCustomReturnsOwnPromiseFunction(JSC::ExecState&);
public:
    static const unsigned StructureFlags = JSC::HasStaticPropertyTable | JSC::InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | JSC::OverridesGetCallData | JSC::OverridesGetOwnPropertySlot | JSC::OverridesGetPropertyNames | Base::StructureFlags;
protected:
    JSTestObj(JSC::Structure*, JSDOMGlobalObject&, Ref<TestObj>&&);

    void finishCreation(JSC::VM&);
};

class JSTestObjOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&, const char**);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestObj*)
{
    static NeverDestroyed<JSTestObjOwner> owner;
    return &owner.get();
}

inline void* wrapperKey(TestObj* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject*, TestObj&);
inline JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestObj* impl) { return impl ? toJS(state, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject*, Ref<TestObj>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::ExecState* state, JSDOMGlobalObject* globalObject, RefPtr<TestObj>&& impl) { return impl ? toJSNewlyCreated(state, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestObj> {
    using WrapperClass = JSTestObj;
    using ToWrappedReturnType = TestObj*;
};
String convertEnumerationToString(TestObj::EnumType);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::EnumType);

template<> std::optional<TestObj::EnumType> parseEnumeration<TestObj::EnumType>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::EnumType>();

String convertEnumerationToString(TestObj::Optional);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::Optional);

template<> std::optional<TestObj::Optional> parseEnumeration<TestObj::Optional>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::Optional>();

String convertEnumerationToString(AlternateEnumName);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, AlternateEnumName);

template<> std::optional<AlternateEnumName> parseEnumeration<AlternateEnumName>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<AlternateEnumName>();

#if ENABLE(Condition1)

String convertEnumerationToString(TestObj::EnumA);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::EnumA);

template<> std::optional<TestObj::EnumA> parseEnumeration<TestObj::EnumA>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::EnumA>();

#endif

#if ENABLE(Condition1) && ENABLE(Condition2)

String convertEnumerationToString(TestObj::EnumB);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::EnumB);

template<> std::optional<TestObj::EnumB> parseEnumeration<TestObj::EnumB>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::EnumB>();

#endif

#if ENABLE(Condition1) || ENABLE(Condition2)

String convertEnumerationToString(TestObj::EnumC);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::EnumC);

template<> std::optional<TestObj::EnumC> parseEnumeration<TestObj::EnumC>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::EnumC>();

#endif

String convertEnumerationToString(TestObj::Kind);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::Kind);

template<> std::optional<TestObj::Kind> parseEnumeration<TestObj::Kind>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::Kind>();

String convertEnumerationToString(TestObj::Size);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::Size);

template<> std::optional<TestObj::Size> parseEnumeration<TestObj::Size>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::Size>();

String convertEnumerationToString(TestObj::Confidence);
template<> JSC::JSString* convertEnumerationToJS(JSC::ExecState&, TestObj::Confidence);

template<> std::optional<TestObj::Confidence> parseEnumeration<TestObj::Confidence>(JSC::ExecState&, JSC::JSValue);
template<> const char* expectedEnumerationValues<TestObj::Confidence>();

template<> TestObj::Dictionary convertDictionary<TestObj::Dictionary>(JSC::ExecState&, JSC::JSValue);

JSC::JSObject* convertDictionaryToJS(JSC::ExecState&, JSDOMGlobalObject&, const TestObj::Dictionary&);

template<> TestObj::DictionaryThatShouldNotTolerateNull convertDictionary<TestObj::DictionaryThatShouldNotTolerateNull>(JSC::ExecState&, JSC::JSValue);

template<> TestObj::DictionaryThatShouldTolerateNull convertDictionary<TestObj::DictionaryThatShouldTolerateNull>(JSC::ExecState&, JSC::JSValue);

template<> AlternateDictionaryName convertDictionary<AlternateDictionaryName>(JSC::ExecState&, JSC::JSValue);

template<> TestObj::ParentDictionary convertDictionary<TestObj::ParentDictionary>(JSC::ExecState&, JSC::JSValue);

template<> TestObj::ChildDictionary convertDictionary<TestObj::ChildDictionary>(JSC::ExecState&, JSC::JSValue);

#if ENABLE(Condition1)

template<> TestObj::ConditionalDictionaryA convertDictionary<TestObj::ConditionalDictionaryA>(JSC::ExecState&, JSC::JSValue);

#endif

#if ENABLE(Condition1) && ENABLE(Condition2)

template<> TestObj::ConditionalDictionaryB convertDictionary<TestObj::ConditionalDictionaryB>(JSC::ExecState&, JSC::JSValue);

#endif

#if ENABLE(Condition1) || ENABLE(Condition2)

template<> TestObj::ConditionalDictionaryC convertDictionary<TestObj::ConditionalDictionaryC>(JSC::ExecState&, JSC::JSValue);

#endif


} // namespace WebCore
