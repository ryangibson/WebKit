/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#import "APICast.h"
#import "JSCInlines.h"
#import "JSContextInternal.h"
#import "JSContextPrivate.h"
#import "JSContextRefInternal.h"
#import "JSGlobalObject.h"
#import "JSValueInternal.h"
#import "JSVirtualMachineInternal.h"
#import "JSWrapperMap.h"
#import "JavaScriptCore.h"
#import "ObjcRuntimeExtras.h"
#import "StrongInlines.h"


#include "CallFrame.h"
#include "Completion.h"
#include "Exception.h"
#include "GCActivityCallback.h"
#include "InitializeThreading.h"
#include "JSLock.h"
#include "JSObject.h"
#include "OpaqueJSString.h"
#include "JSCInlines.h"
#include "SourceCode.h"
#include "EventLoop.h"
#include <wtf/text/StringHash.h>

#include "Debugger.h"
#include "Parser.h"
#include "ParserError.h"
#import <objc/runtime.h>
#include <iostream>

#if JSC_OBJC_API_ENABLED

namespace JSC {

using namespace JSC;
using namespace std;

class Debugger;
//class Parser;
//class ParserError;

class JS_EXPORT_PRIVATE RPGDebugger : public Debugger {
public:
    RPGDebugger(VM&);
    virtual ~RPGDebugger();

protected:
    void sourceParsed(ExecState*, SourceProvider*, int errorLineNumber, const WTF::String& errorMessage);
//    void recompileAllJSFunctions();
    void handleBreakpointHit(JSGlobalObject*, const Breakpoint&);
    void handleExceptionInBreakpointCondition(ExecState*, Exception*) const;
    void handlePause(JSGlobalObject*, ReasonForPause);
    void notifyDoneProcessingDebuggerEvents();

private:
    void pause();
};

RPGDebugger::RPGDebugger(VM& vm) : Debugger(vm) {
    cout << "RPGDebugger.construct" << endl;
}

RPGDebugger::~RPGDebugger() {
    cout << "RPGDebugger.destruct" << endl;
}

void RPGDebugger::sourceParsed(ExecState *state, SourceProvider *provider, int errorLineNumber, const WTF::String& errorMessage) {
    cout << "RPG: sourceParsed" << endl;
    cout << "state: " << state << endl;
    cout << "provider: " << provider << endl;
    cout << "errorLineNumber: " << errorLineNumber << endl;
    cout << "errorMessage: " << errorMessage.length() << endl;

    /*
    JSC::SourceID sid = provider->asID();
    cout << "RPG.RPGDebugger.sourceParsed.SourceID: " << sid << endl;
    JSC::Breakpoint bp;
    bp.line = 3;
    bp.sourceID = sid;
    bool existing = false;
    resolveBreakpoint(bp, provider);
    setBreakpoint(bp, existing);

    activateBreakpoints();
    */
}

//void RPGDebugger::recompileAllJSFunctions() {
//    cout << "RPG: recompileAllJSFunctions" << endl;
//    Debugger::recompileAllJSFunctions();
//}

void RPGDebugger::handleBreakpointHit(JSGlobalObject *globalObject, const Breakpoint& breakPoint) {
    cout << "RPG: handleBreakpointHit" << endl;
    cout << "globalObject: " << globalObject << endl;
    cout << "breakPoint: " << breakPoint.line << endl;
}

void RPGDebugger::handleExceptionInBreakpointCondition(ExecState *state, Exception *exception) const {
    cout << "RPG: handleExceptionInBreakpointCondition" << endl;
    cout << "state: " << state << endl;
    cout << "exception: " << exception << endl;
}

//void performCallback(void *info);
static void performCallback(void*) {
    cout << "performCallback" << endl;
}

void RPGDebugger::handlePause(JSGlobalObject *globalObject, ReasonForPause reasonForPause) {
    cout << "RPG: handlePause" << endl;
    cout << "globalObject: " << globalObject << endl;
    cout << "reasonForPause: " << reasonForPause << endl;

    cout << "isPaused: " << isPaused() << endl;
    cout << "isStepping: " << isStepping() << endl;
    setPauseOnNextStatement(true);
    cout << "isStepping: " << isStepping() << endl;
    cout << "isInteractivelyDebugging: " << isInteractivelyDebugging() << endl;
    cout << "isAttached: " << isAttached(globalObject) << endl;
    cout << endl;

//    auto l = new JSC::JSLock(vm());
//    l->lock();

    // Drop all locks so another thread can work in the VM while we are nested.
    JSC::JSLock::DropAllLocks dropAllLocks(globalObject->vm());

    JSC::DebuggerCallFrame &f = currentDebuggerCallFrame();
    cout << "FUNC: " << f.functionName().utf8().data() << endl;
    cout << "--line: " << f.line() << endl;
    cout << "--column: " << f.column() << endl;
    cout << endl;

//    pthread_mutex_t l;
//    pthread_mutex_init(&l, NULL);
//    pthread_mutex_lock(&l);
//    pthread_mutex_lock(&l);
//
    this->pause();
}

void RPGDebugger::pause() {
    Inspector::EventLoop loop;
    CFRunLoopSourceContext context = {
        0, // CFIndex    version;
        nullptr, // void *    info;
        nullptr, // const void *(*retain)(const void *info);
        nullptr, // void    (*release)(const void *info);
        nullptr, // CFStringRef    (*copyDescription)(const void *info);
        nullptr, // Boolean    (*equal)(const void *info1, const void *info2);
        nullptr, // CFHashCode    (*hash)(const void *info);
        nullptr, // void    (*schedule)(void *info, CFRunLoopRef rl, CFRunLoopMode mode);
        nullptr, // void    (*cancel)(void *info, CFRunLoopRef rl, CFRunLoopMode mode);
        performCallback // void    (*perform)(void *info);
    };
    CFRunLoopSourceRef source = CFRunLoopSourceCreate(NULL, 0, &context);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, loop.remoteInspectorRunLoopMode());

    while (true && !loop.ended())
        loop.cycle();
}

void RPGDebugger::notifyDoneProcessingDebuggerEvents() {
    cout << "RPG: notifyDoneProcessingDebuggerEvents" << endl;
}

};

@implementation JSDebugger {
    JSGlobalContextRef m_context;
    JSC::RPGDebugger *m_debugger;
}

- (instancetype)initWithContext:(JSGlobalContextRef)ctx
{
    self = [super init];
    if (self) {
        m_context = ctx;
        JSC::ExecState* exec = toJS(m_context);
        JSC::VM& vm = exec->vm();
        m_debugger = new JSC::RPGDebugger(vm);
    }
    return self;
}

- (void)attach
{
    NSLog(@"JSDebugger.attach");
    JSC::JSGlobalObject* globalObject = toJS(m_context)->lexicalGlobalObject();
    m_debugger->attach(globalObject);

}

- (void)detach
{
    NSLog(@"JSDebugger.dettach");
    JSC::JSGlobalObject* globalObject = toJS(m_context)->lexicalGlobalObject();
    m_debugger->detach(globalObject, JSC::RPGDebugger::TerminatingDebuggingSession);
}

- (void)addBreakPoint:(NSUInteger)line
{
    NSLog(@"JSDebugger.addBreakPoint %lu", line);
}

- (void)removeBreakPoint:(NSUInteger)line
{
    NSLog(@"JSDebugger.removeBreakPoint %lu", line);
}

- (void)enableBreakPoint:(NSUInteger)line
{
    NSLog(@"JSDebugger.enableBreakPoint %lu", line);
}

- (void)disableBreakPoint:(NSUInteger)line
{
    NSLog(@"JSDebugger.disableBreakPoint %lu", line);
}

- (void)pause
{
    NSLog(@"JSDebugger.pause");
}

- (void)resume
{
    NSLog(@"JSDebugger.resume");
}

- (void)stepOver
{
    NSLog(@"JSDebugger.stepOver");
}

- (void)stepInto
{
    NSLog(@"JSDebugger.stepInto");
}

- (void)stepOut
{
    NSLog(@"JSDebugger.stepOut");
}

- (NSArray<JSDebugFrame *> *)frames
{
    NSLog(@"JSDebugger.frames");
    return @[];
}

- (void)selectFrameAtIndex:(NSUInteger)index
{
    NSLog(@"JSDebugger.selectFrameAtIndex %lu", (unsigned long)index);
}

@end

@implementation JSDebugFrame {

}

- (NSDictionary<NSString *, JSValue *> *)variables
{
    NSLog(@"JSDebugFrame.variables");
    return @{};
}

@end

@implementation JSContext {
    JSVirtualMachine *m_virtualMachine;
    JSGlobalContextRef m_context;
    JSC::Strong<JSC::JSObject> m_exception;
    JSC::RPGDebugger *m_debugger;
}

- (JSGlobalContextRef)JSGlobalContextRef
{
    return m_context;
}

- (void)ensureWrapperMap
{
    if (!toJS([self JSGlobalContextRef])->lexicalGlobalObject()->wrapperMap()) {
        // The map will be retained by the GlobalObject in initialization.
        [[[JSWrapperMap alloc] initWithGlobalContextRef:[self JSGlobalContextRef]] release];
    }
}

- (instancetype)init
{
    return [self initWithVirtualMachine:[[[JSVirtualMachine alloc] init] autorelease]];
}

- (instancetype)initWithVirtualMachine:(JSVirtualMachine *)virtualMachine
{
    self = [super init];
    if (!self)
        return nil;

    m_virtualMachine = [virtualMachine retain];
    m_context = JSGlobalContextCreateInGroup(getGroupFromVirtualMachine(virtualMachine), 0);

    self.exceptionHandler = ^(JSContext *context, JSValue *exceptionValue) {
        context.exception = exceptionValue;
    };

    [self ensureWrapperMap];
    [m_virtualMachine addContext:self forGlobalContextRef:m_context];

    return self;
}

- (void)dealloc
{
    m_exception.clear();
    JSGlobalContextRelease(m_context);
    [m_virtualMachine release];
    [_exceptionHandler release];
    [super dealloc];
}

- (NSString *)ryTestMethod
{
    JSC::ExecState* exec = toJS(m_context);
    JSC::VM& vm = exec->vm();
    m_debugger = new JSC::RPGDebugger(vm);


    JSC::JSGlobalObject* globalObject = toJS(m_context)->lexicalGlobalObject();
//    JSC::Debugger::attach(globalObject);
    m_debugger->attach(globalObject);
//    m_debugger->activateBreakpoints();
    return @"THIS IS RYAN'S TEST METHOD RESULT (NEW 2)";
}

- (void)parseJavaScript:(NSString *)script
{
    NSLog(@"> %@", script);
}
/*
- (void)_parseJavaScript:(JSStringRef)script
               sourceURL:(JSStringRef)sourceURL
      startingLineNumber:(int)startingLineNumber
{
    JSC::ExecState* exec = toJS(m_context);
    JSC::VM& vm = exec->vm();

    //JSC::JSGlobalObject* globalObject = vm.vmEntryGlobalObject(exec);
    auto sourceURLString = sourceURL ? sourceURL->string() : String();
    JSC::SourceCode source = makeSource(script->string(), JSC::SourceOrigin { sourceURLString }, sourceURLString, TextPosition(OrdinalNumber::fromOneBasedInt(startingLineNumber), OrdinalNumber()));
    JSC::ParserError error;


    JSC::Parser<JSC::Lexer<LChar>> parser(
        &vm,
        source,
        JSC::JSParserBuiltinMode::NotBuiltin,
        JSC::JSParserStrictMode::NotStrict,
        JSC::JSParserScriptMode::Classic,
        JSC::SourceParseMode::ProgramMode,
        JSC::SuperBinding::NotNeeded,
        JSC::ConstructorKind::None,
        JSC::DerivedContextType::None,
        false,
        JSC::EvalContextType::None,
        nullptr
    );
    auto rootNode = parser.parse(&error, JSC::Identifier(), JSC::JSParserScriptMode::Classic, JSC::ParsingContext::Program);
    */
/*
    std::unique_ptr<JSC::ProgramNode> rootNode = parser.parse(
        vm,
        source,
        JSC::Identifier(),
        JSC::JSParserBuiltinMode::NotBuiltin,
        JSC::JSParserStrictMode::NotStrict,
        JSC::JSParserScriptMode::Classic,
        JSC::SourceParseMode::ProgramMode,
        JSC::SuperBinding::NotNeeded,
        &error,
        nullptr,
        JSC::ConstructorKind::None,
        JSC::DerivedContextType::None,
        JSC::EvalContextType::None,
        nullptr
    );
 */
//}

- (JSValue *)evaluateAndDebug:(NSString *)script
{
    JSValueRef exceptionValue = nullptr;
    auto scriptJS = OpaqueJSString::tryCreate(script);
    NSURL *sourceURL = nil;
    auto sourceURLJS = OpaqueJSString::tryCreate([sourceURL absoluteString]);

    JSValueRef result = [self debugEvaluateWithContext:m_context
                                                script:scriptJS.get()
                                            thisObject:nullptr
                                             sourceURL:sourceURLJS.get()
                                    startingLineNumber:0
                                             exception:&exceptionValue];

    if (exceptionValue)
        return [self valueFromNotifyException:exceptionValue];

    return [JSValue valueWithJSValueRef:result inContext:self];
}

- (JSValueRef)debugEvaluateWithContext:(JSContextRef)ctx
                                script:(JSStringRef)script
                            thisObject:(JSObjectRef)thisObject
                             sourceURL:(JSStringRef)sourceURL
                    startingLineNumber:(int)startingLineNumber
                             exception:(JSValueRef*)exception
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    JSC::ExecState* exec = toJS(ctx);
    JSC::VM& vm = exec->vm();
    JSC::JSLockHolder locker(vm);

    JSC::JSObject *jsThisObject = toJS(thisObject);

    startingLineNumber = std::max(1, startingLineNumber);

    // evaluate sets "this" to the global object if it is NULL
    JSC::JSGlobalObject* globalObject = vm.vmEntryGlobalObject(exec);
    auto sourceURLString = sourceURL ? sourceURL->string() : String();
    JSC::SourceCode source = makeSource(script->string(), JSC::SourceOrigin { sourceURLString }, sourceURLString, TextPosition(OrdinalNumber::fromOneBasedInt(startingLineNumber), OrdinalNumber()));


    //
    auto provider = source.provider();
    JSC::SourceID sid = provider->asID();
    std::cout << "RPG.RPGDebugger.debugEvaluateWithContext.SourceID: " << sid << std::endl;
    JSC::Breakpoint bp;
    bp.line = 6;
    bp.sourceID = sid;
    bool existing = false;
    m_debugger->resolveBreakpoint(bp, provider);
    m_debugger->setBreakpoint(bp, existing);

//    JSC::Breakpoint bp2;
//    bp2.line = 3;
//    bp2.sourceID = sid;
////    bool existing = false;
//    m_debugger->resolveBreakpoint(bp2, provider);
//    m_debugger->setBreakpoint(bp2, existing);

    m_debugger->activateBreakpoints();
    //

    NakedPtr<JSC::Exception> evaluationException;
    JSC::JSValue returnValue = JSC::profiledEvaluate(globalObject->globalExec(), JSC::ProfilingReason::API, source, jsThisObject, evaluationException);

    if (evaluationException) {
        if (exception)
            *exception = toRef(exec, evaluationException->value());
        return 0;
    }

    if (returnValue)
        return toRef(exec, returnValue);

    // happens, for example, when the only statement is an empty (';') statement
    return toRef(exec, JSC::jsUndefined());
}

- (JSValue *)evaluateScript:(NSString *)script
{
    return [self evaluateScript:script withSourceURL:nil];
}

- (JSValue *)evaluateScript:(NSString *)script withSourceURL:(NSURL *)sourceURL
{
    JSValueRef exceptionValue = nullptr;
    auto scriptJS = OpaqueJSString::tryCreate(script);
    auto sourceURLJS = OpaqueJSString::tryCreate([sourceURL absoluteString]);

    NSLog(@"RPG.CheckSyntax");
    bool syntaxCheckResult = JSCheckScriptSyntax(m_context, scriptJS.get(), sourceURLJS.get(), 0, &exceptionValue);
    NSLog(@"RPG.CheckedSyntax: %@", syntaxCheckResult ? @"Passed" : @"Failed");

    JSValueRef result = JSEvaluateScript(m_context, scriptJS.get(), nullptr, sourceURLJS.get(), 0, &exceptionValue);

    if (exceptionValue)
        return [self valueFromNotifyException:exceptionValue];

    return [JSValue valueWithJSValueRef:result inContext:self];
}

- (void)setException:(JSValue *)value
{
    JSC::ExecState* exec = toJS(m_context);
    JSC::VM& vm = exec->vm();
    JSC::JSLockHolder locker(vm);
    if (value)
        m_exception.set(vm, toJS(JSValueToObject(m_context, valueInternalValue(value), 0)));
    else
        m_exception.clear();
}

- (JSValue *)exception
{
    if (!m_exception)
        return nil;
    return [JSValue valueWithJSValueRef:toRef(m_exception.get()) inContext:self];
}

- (JSWrapperMap *)wrapperMap
{
    return toJS(m_context)->lexicalGlobalObject()->wrapperMap();
}

- (JSValue *)globalObject
{
    return [JSValue valueWithJSValueRef:JSContextGetGlobalObject(m_context) inContext:self];
}

+ (JSContext *)currentContext
{
    Thread& thread = Thread::current();
    CallbackData *entry = (CallbackData *)thread.m_apiData;
    return entry ? entry->context : nil;
}

+ (JSValue *)currentThis
{
    Thread& thread = Thread::current();
    CallbackData *entry = (CallbackData *)thread.m_apiData;
    if (!entry)
        return nil;
    return [JSValue valueWithJSValueRef:entry->thisValue inContext:[JSContext currentContext]];
}

+ (JSValue *)currentCallee
{
    Thread& thread = Thread::current();
    CallbackData *entry = (CallbackData *)thread.m_apiData;
    // calleeValue may be null if we are initializing a promise.
    if (!entry || !entry->calleeValue)
        return nil;
    return [JSValue valueWithJSValueRef:entry->calleeValue inContext:[JSContext currentContext]];
}

+ (NSArray *)currentArguments
{
    Thread& thread = Thread::current();
    CallbackData *entry = (CallbackData *)thread.m_apiData;

    if (!entry)
        return nil;

    if (!entry->currentArguments) {
        JSContext *context = [JSContext currentContext];
        size_t count = entry->argumentCount;
        JSValue * argumentArray[count];
        for (size_t i =0; i < count; ++i)
            argumentArray[i] = [JSValue valueWithJSValueRef:entry->arguments[i] inContext:context];
        entry->currentArguments = [[NSArray alloc] initWithObjects:argumentArray count:count];
    }

    return entry->currentArguments;
}

- (JSVirtualMachine *)virtualMachine
{
    return m_virtualMachine;
}

- (NSString *)name
{
    JSStringRef name = JSGlobalContextCopyName(m_context);
    if (!name)
        return nil;

    return CFBridgingRelease(JSStringCopyCFString(kCFAllocatorDefault, name));
}

- (void)setName:(NSString *)name
{
    JSGlobalContextSetName(m_context, OpaqueJSString::tryCreate(name).get());
}

- (BOOL)_remoteInspectionEnabled
{
    return JSGlobalContextGetRemoteInspectionEnabled(m_context);
}

- (void)_setRemoteInspectionEnabled:(BOOL)enabled
{
    JSGlobalContextSetRemoteInspectionEnabled(m_context, enabled);
}

- (BOOL)_includesNativeCallStackWhenReportingExceptions
{
    return JSGlobalContextGetIncludesNativeCallStackWhenReportingExceptions(m_context);
}

- (void)_setIncludesNativeCallStackWhenReportingExceptions:(BOOL)includeNativeCallStack
{
    JSGlobalContextSetIncludesNativeCallStackWhenReportingExceptions(m_context, includeNativeCallStack);
}

- (CFRunLoopRef)_debuggerRunLoop
{
    return JSGlobalContextGetDebuggerRunLoop(m_context);
}

- (void)_setDebuggerRunLoop:(CFRunLoopRef)runLoop
{
    JSGlobalContextSetDebuggerRunLoop(m_context, runLoop);
}

@end

@implementation JSContext(SubscriptSupport)

- (JSValue *)objectForKeyedSubscript:(id)key
{
    return [self globalObject][key];
}

- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key
{
    [self globalObject][key] = object;
}

@end

@implementation JSContext (Internal)

- (instancetype)initWithGlobalContextRef:(JSGlobalContextRef)context
{
    self = [super init];
    if (!self)
        return nil;

    JSC::JSGlobalObject* globalObject = toJS(context)->lexicalGlobalObject();
    m_virtualMachine = [[JSVirtualMachine virtualMachineWithContextGroupRef:toRef(&globalObject->vm())] retain];
    ASSERT(m_virtualMachine);
    m_context = JSGlobalContextRetain(context);
    [self ensureWrapperMap];

    self.exceptionHandler = ^(JSContext *context, JSValue *exceptionValue) {
        context.exception = exceptionValue;
    };

    [m_virtualMachine addContext:self forGlobalContextRef:m_context];

    return self;
}

- (void)notifyException:(JSValueRef)exceptionValue
{
    self.exceptionHandler(self, [JSValue valueWithJSValueRef:exceptionValue inContext:self]);
}

- (JSValue *)valueFromNotifyException:(JSValueRef)exceptionValue
{
    [self notifyException:exceptionValue];
    return [JSValue valueWithUndefinedInContext:self];
}

- (BOOL)boolFromNotifyException:(JSValueRef)exceptionValue
{
    [self notifyException:exceptionValue];
    return NO;
}

- (void)beginCallbackWithData:(CallbackData *)callbackData calleeValue:(JSValueRef)calleeValue thisValue:(JSValueRef)thisValue argumentCount:(size_t)argumentCount arguments:(const JSValueRef *)arguments
{
    Thread& thread = Thread::current();
    [self retain];
    CallbackData *prevStack = (CallbackData *)thread.m_apiData;
    *callbackData = (CallbackData){ prevStack, self, [self.exception retain], calleeValue, thisValue, argumentCount, arguments, nil };
    thread.m_apiData = callbackData;
    self.exception = nil;
}

- (void)endCallbackWithData:(CallbackData *)callbackData
{
    Thread& thread = Thread::current();
    self.exception = callbackData->preservedException;
    [callbackData->preservedException release];
    [callbackData->currentArguments release];
    thread.m_apiData = callbackData->next;
    [self release];
}

- (JSValue *)wrapperForObjCObject:(id)object
{
    JSC::JSLockHolder locker(toJS(m_context));
    return [[self wrapperMap] jsWrapperForObject:object inContext:self];
}

- (JSValue *)wrapperForJSObject:(JSValueRef)value
{
    JSC::JSLockHolder locker(toJS(m_context));
    return [[self wrapperMap] objcWrapperForJSValueRef:value inContext:self];
}

+ (JSContext *)contextWithJSGlobalContextRef:(JSGlobalContextRef)globalContext
{
    JSVirtualMachine *virtualMachine = [JSVirtualMachine virtualMachineWithContextGroupRef:toRef(&toJS(globalContext)->vm())];
    JSContext *context = [virtualMachine contextForGlobalContextRef:globalContext];
    if (!context)
        context = [[[JSContext alloc] initWithGlobalContextRef:globalContext] autorelease];
    return context;
}

@end

#endif
