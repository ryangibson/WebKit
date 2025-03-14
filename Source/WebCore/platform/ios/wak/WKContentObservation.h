/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKContentObservation_h
#define WKContentObservation_h

#if TARGET_OS_IPHONE

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    WKContentNoChange               = 0,
    WKContentVisibilityChange       = 2,
    WKContentIndeterminateChange    = 1
}   WKContentChange;

WEBCORE_EXPORT void WKStartObservingContentChanges(void);
WEBCORE_EXPORT void WKStopObservingContentChanges(void);

WEBCORE_EXPORT void WKStartObservingDOMTimerScheduling(void);
WEBCORE_EXPORT void WKStopObservingDOMTimerScheduling(void);
WEBCORE_EXPORT bool WKIsObservingDOMTimerScheduling(void);

WEBCORE_EXPORT WKContentChange WKObservedContentChange(void);

WEBCORE_EXPORT int WebThreadCountOfObservedDOMTimers(void);
WEBCORE_EXPORT void WebThreadClearObservedDOMTimers(void);

#ifdef __cplusplus
}
#endif

#endif // TARGET_OS_IPHONE

#endif // WKContentObservation_h
