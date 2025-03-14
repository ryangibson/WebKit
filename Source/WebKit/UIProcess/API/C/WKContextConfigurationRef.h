/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef WKContextConfigurationRef_h
#define WKContextConfigurationRef_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKContextConfigurationRef WKContextConfigurationCreate();
WK_EXPORT WKContextConfigurationRef WKContextConfigurationCreateWithLegacyOptions();

WK_EXPORT WKStringRef WKContextConfigurationCopyApplicationCacheDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetApplicationCacheDirectory(WKContextConfigurationRef configuration, WKStringRef applicationCacheDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyDiskCacheDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetDiskCacheDirectory(WKContextConfigurationRef configuration, WKStringRef diskCacheDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyIndexedDBDatabaseDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetIndexedDBDatabaseDirectory(WKContextConfigurationRef configuration, WKStringRef indexedDBDatabaseDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyInjectedBundlePath(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetInjectedBundlePath(WKContextConfigurationRef configuration, WKStringRef injectedBundlePath);

WK_EXPORT WKStringRef WKContextConfigurationCopyLocalStorageDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetLocalStorageDirectory(WKContextConfigurationRef configuration, WKStringRef localStorageDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyWebSQLDatabaseDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetWebSQLDatabaseDirectory(WKContextConfigurationRef configuration, WKStringRef webSQLDatabaseDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyMediaKeysStorageDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetMediaKeysStorageDirectory(WKContextConfigurationRef configuration, WKStringRef mediaKeysStorageDirectory);

WK_EXPORT WKStringRef WKContextConfigurationCopyResourceLoadStatisticsDirectory(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetResourceLoadStatisticsDirectory(WKContextConfigurationRef configuration, WKStringRef resourceLoadStatisticsDirectory);

WK_EXPORT bool WKContextConfigurationFullySynchronousModeIsAllowedForTesting(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetFullySynchronousModeIsAllowedForTesting(WKContextConfigurationRef configuration, bool allowed);

WK_EXPORT WKArrayRef WKContextConfigurationCopyOverrideLanguages(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetOverrideLanguages(WKContextConfigurationRef configuration, WKArrayRef overrideLanguages);

WK_EXPORT bool WKContextConfigurationShouldCaptureAudioInUIProcess(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetShouldCaptureAudioInUIProcess(WKContextConfigurationRef configuration, bool allowed);

WK_EXPORT bool WKContextConfigurationProcessSwapsOnNavigation(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetProcessSwapsOnNavigation(WKContextConfigurationRef configuration, bool swaps);

WK_EXPORT bool WKContextConfigurationPrewarmsProcessesAutomatically(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetPrewarmsProcessesAutomatically(WKContextConfigurationRef configuration, bool prewarms);

WK_EXPORT bool WKContextConfigurationAlwaysKeepAndReuseSwappedProcesses(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetAlwaysKeepAndReuseSwappedProcesses(WKContextConfigurationRef configuration, bool keepAndReuse);

WK_EXPORT bool WKContextConfigurationProcessSwapsOnWindowOpenWithOpener(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetProcessSwapsOnWindowOpenWithOpener(WKContextConfigurationRef configuration, bool swaps);

WK_EXPORT int64_t WKContextConfigurationDiskCacheSizeOverride(WKContextConfigurationRef configuration);
WK_EXPORT void WKContextConfigurationSetDiskCacheSizeOverride(WKContextConfigurationRef configuration, int64_t size);
    
#ifdef __cplusplus
}
#endif


#endif // WKContextConfigurationRef_h
