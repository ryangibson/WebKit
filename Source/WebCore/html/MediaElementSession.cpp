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

#include "config.h"

#if ENABLE(VIDEO)

#include "MediaElementSession.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLAudioElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "HitTestResult.h"
#include "Logging.h"
#include "Page.h"
#include "PlatformMediaSessionManager.h"
#include "RenderMedia.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "Settings.h"
#include "SourceBuffer.h"
#include <wtf/text/StringBuilder.h>

#if PLATFORM(IOS_FAMILY)
#include "AudioSession.h"
#include "RuntimeApplicationChecks.h"
#include <wtf/spi/darwin/dyldSPI.h>
#endif

namespace WebCore {

static const Seconds clientDataBufferingTimerThrottleDelay { 100_ms };
static const Seconds elementMainContentCheckInterval { 250_ms };

static bool isElementRectMostlyInMainFrame(const HTMLMediaElement&);
static bool isElementLargeEnoughForMainContent(const HTMLMediaElement&, MediaSessionMainContentPurpose);

#if !RELEASE_LOG_DISABLED
static String restrictionNames(MediaElementSession::BehaviorRestrictions restriction)
{
    StringBuilder restrictionBuilder;
#define CASE(restrictionType) \
    if (restriction & MediaElementSession::restrictionType) { \
        if (!restrictionBuilder.isEmpty()) \
            restrictionBuilder.appendLiteral(", "); \
        restrictionBuilder.append(#restrictionType); \
    } \

    CASE(NoRestrictions)
    CASE(RequireUserGestureForLoad)
    CASE(RequireUserGestureForVideoRateChange)
    CASE(RequireUserGestureForAudioRateChange)
    CASE(RequireUserGestureForFullscreen)
    CASE(RequirePageConsentToLoadMedia)
    CASE(RequirePageConsentToResumeMedia)
    CASE(RequireUserGestureToShowPlaybackTargetPicker)
    CASE(WirelessVideoPlaybackDisabled)
    CASE(RequireUserGestureToAutoplayToExternalDevice)
    CASE(MetadataPreloadingNotPermitted)
    CASE(AutoPreloadingNotPermitted)
    CASE(InvisibleAutoplayNotPermitted)
    CASE(OverrideUserGestureRequirementForMainContent)
    CASE(RequireUserGestureToControlControlsManager)
    CASE(RequirePlaybackToControlControlsManager)
    CASE(RequireUserGestureForVideoDueToLowPowerMode)

    return restrictionBuilder.toString();
}
#endif

static bool pageExplicitlyAllowsElementToAutoplayInline(const HTMLMediaElement& element)
{
    Document& document = element.document();
    Page* page = document.page();
    return document.isMediaDocument() && !document.ownerElement() && page && page->allowsMediaDocumentInlinePlayback();
}

MediaElementSession::MediaElementSession(HTMLMediaElement& element)
    : PlatformMediaSession(element)
    , m_element(element)
    , m_restrictions(NoRestrictions)
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , m_targetAvailabilityChangedTimer(*this, &MediaElementSession::targetAvailabilityChangedTimerFired)
#endif
    , m_mainContentCheckTimer(*this, &MediaElementSession::mainContentCheckTimerFired)
    , m_clientDataBufferingTimer(*this, &MediaElementSession::clientDataBufferingTimerFired)
#if !RELEASE_LOG_DISABLED
    , m_logIdentifier(element.logIdentifier())
#endif
{
}

void MediaElementSession::registerWithDocument(Document& document)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.addPlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(document);
#endif
}

void MediaElementSession::unregisterWithDocument(Document& document)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.removePlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(document);
#endif
}

void MediaElementSession::clientWillBeginAutoplaying()
{
    PlatformMediaSession::clientWillBeginAutoplaying();
    m_elementIsHiddenBecauseItWasRemovedFromDOM = false;
    updateClientDataBuffering();
}

bool MediaElementSession::clientWillBeginPlayback()
{
    if (!PlatformMediaSession::clientWillBeginPlayback())
        return false;

    m_elementIsHiddenBecauseItWasRemovedFromDOM = false;
    updateClientDataBuffering();
    return true;
}

bool MediaElementSession::clientWillPausePlayback()
{
    if (!PlatformMediaSession::clientWillPausePlayback())
        return false;

    updateClientDataBuffering();
    return true;
}

void MediaElementSession::visibilityChanged()
{
    scheduleClientDataBufferingCheck();

    if (m_element.elementIsHidden() && !m_element.isFullscreen())
        m_elementIsHiddenUntilVisibleInViewport = true;
    else if (m_element.isVisibleInViewport())
        m_elementIsHiddenUntilVisibleInViewport = false;
}

void MediaElementSession::isVisibleInViewportChanged()
{
    scheduleClientDataBufferingCheck();

    if (m_element.isFullscreen() || m_element.isVisibleInViewport())
        m_elementIsHiddenUntilVisibleInViewport = false;
}

void MediaElementSession::inActiveDocumentChanged()
{
    m_elementIsHiddenBecauseItWasRemovedFromDOM = !m_element.inActiveDocument();
    scheduleClientDataBufferingCheck();
}

void MediaElementSession::scheduleClientDataBufferingCheck()
{
    if (!m_clientDataBufferingTimer.isActive())
        m_clientDataBufferingTimer.startOneShot(clientDataBufferingTimerThrottleDelay);
}

void MediaElementSession::clientDataBufferingTimerFired()
{
    INFO_LOG(LOGIDENTIFIER, "visible = ", m_element.elementIsHidden());

    updateClientDataBuffering();

#if PLATFORM(IOS_FAMILY)
    PlatformMediaSessionManager::sharedManager().configureWireLessTargetMonitoring();
#endif

    if (state() != Playing || !m_element.elementIsHidden())
        return;

    PlatformMediaSessionManager::SessionRestrictions restrictions = PlatformMediaSessionManager::sharedManager().restrictions(mediaType());
    if ((restrictions & PlatformMediaSessionManager::BackgroundTabPlaybackRestricted) == PlatformMediaSessionManager::BackgroundTabPlaybackRestricted)
        pauseSession();
}

void MediaElementSession::updateClientDataBuffering()
{
    if (m_clientDataBufferingTimer.isActive())
        m_clientDataBufferingTimer.stop();

    m_element.setShouldBufferData(dataBufferingPermitted());
}

void MediaElementSession::addBehaviorRestriction(BehaviorRestrictions restrictions)
{
    if (restrictions & ~m_restrictions)
        INFO_LOG(LOGIDENTIFIER, "adding ", restrictionNames(restrictions & ~m_restrictions));

    m_restrictions |= restrictions;

    if (restrictions & OverrideUserGestureRequirementForMainContent)
        m_mainContentCheckTimer.startRepeating(elementMainContentCheckInterval);
}

void MediaElementSession::removeBehaviorRestriction(BehaviorRestrictions restriction)
{
    if (restriction & RequireUserGestureToControlControlsManager) {
        m_mostRecentUserInteractionTime = MonotonicTime::now();
        if (auto page = m_element.document().page())
            page->setAllowsPlaybackControlsForAutoplayingAudio(true);
    }

    if (!(m_restrictions & restriction))
        return;

    INFO_LOG(LOGIDENTIFIER, "removing ", restrictionNames(m_restrictions & restriction));
    m_restrictions &= ~restriction;
}

#if PLATFORM(MAC)
static bool needsArbitraryUserGestureAutoplayQuirk(const Document& document)
{
    if (!document.settings().needsSiteSpecificQuirks())
        return false;

    auto loader = makeRefPtr(document.loader());
    return loader && loader->allowedAutoplayQuirks().contains(AutoplayQuirk::ArbitraryUserGestures);
}
#endif // PLATFORM(MAC)

SuccessOr<MediaPlaybackDenialReason> MediaElementSession::playbackPermitted() const
{
    if (m_element.isSuspended()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because element is suspended");
        return MediaPlaybackDenialReason::InvalidState;
    }

    auto& document = m_element.document();
    if (document.isMediaDocument() && !document.ownerElement())
        return { };

    if (pageExplicitlyAllowsElementToAutoplayInline(m_element))
        return { };

    if (requiresFullscreenForVideoPlayback() && !fullscreenPermitted()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because of fullscreen restriction");
        return MediaPlaybackDenialReason::FullscreenRequired;
    }

    if (m_restrictions & OverrideUserGestureRequirementForMainContent && updateIsMainContent())
        return { };

#if ENABLE(MEDIA_STREAM)
    if (m_element.hasMediaStreamSrcObject()) {
        if (document.isCapturing())
            return { };
        if (document.mediaState() & MediaProducer::IsPlayingAudio)
            return { };
    }
#endif

#if PLATFORM(MAC)
    // FIXME <https://webkit.org/b/175856>: Make this dependent on a runtime flag for desktop autoplay restrictions.
    const auto& topDocument = document.topDocument();
    if (topDocument.mediaState() & MediaProducer::HasUserInteractedWithMediaElement && topDocument.settings().needsSiteSpecificQuirks())
        return { };

    if (document.hasHadUserInteraction() && needsArbitraryUserGestureAutoplayQuirk(document))
        return { };
#endif

    if (m_restrictions & RequireUserGestureForVideoRateChange && m_element.isVideo() && !document.processingUserGestureForMedia()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because a user gesture is required for video rate change restriction");
        return MediaPlaybackDenialReason::UserGestureRequired;
    }

    if (m_restrictions & RequireUserGestureForAudioRateChange && (!m_element.isVideo() || m_element.hasAudio()) && !m_element.muted() && m_element.volume() && !document.processingUserGestureForMedia()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because a user gesture is required for audio rate change restriction");
        return MediaPlaybackDenialReason::UserGestureRequired;
    }

    if (m_restrictions & RequireUserGestureForVideoDueToLowPowerMode && m_element.isVideo() && !document.processingUserGestureForMedia()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because of video low power mode restriction");
        return MediaPlaybackDenialReason::UserGestureRequired;
    }

    return { };
}

bool MediaElementSession::autoplayPermitted() const
{
    const Document& document = m_element.document();
    if (document.pageCacheState() != Document::NotInPageCache)
        return false;
    if (document.activeDOMObjectsAreSuspended())
        return false;

    if (!hasBehaviorRestriction(MediaElementSession::InvisibleAutoplayNotPermitted))
        return true;

    // If the media element is audible, allow autoplay even when not visible as pausing it would be observable by the user.
    if ((!m_element.isVideo() || m_element.hasAudio()) && !m_element.muted() && m_element.volume())
        return true;

    auto* renderer = m_element.renderer();
    if (!renderer) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because element has no renderer");
        return false;
    }
    if (renderer->style().visibility() != Visibility::Visible) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because element is not visible");
        return false;
    }
    if (renderer->view().frameView().isOffscreen()) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because frame is offscreen");
        return false;
    }
    if (renderer->visibleInViewportState() != VisibleInViewportState::Yes) {
        ALWAYS_LOG(LOGIDENTIFIER, "Returning FALSE because element is not visible in the viewport");
        return false;
    }
    return true;
}

bool MediaElementSession::dataLoadingPermitted() const
{
    if (m_restrictions & OverrideUserGestureRequirementForMainContent && updateIsMainContent())
        return true;

    if (m_restrictions & RequireUserGestureForLoad && !m_element.document().processingUserGestureForMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::dataBufferingPermitted() const
{
    if (isSuspended())
        return false;

    if (state() == PlatformMediaSession::Playing)
        return true;

    if (shouldOverrideBackgroundLoadingRestriction())
        return true;

    if (m_elementIsHiddenUntilVisibleInViewport || m_elementIsHiddenBecauseItWasRemovedFromDOM || m_element.elementIsHidden())
        return false;

    return true;
}

bool MediaElementSession::fullscreenPermitted() const
{
    if (m_restrictions & RequireUserGestureForFullscreen && !m_element.document().processingUserGestureForMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::pageAllowsDataLoading() const
{
    Page* page = m_element.document().page();
    if (m_restrictions & RequirePageConsentToLoadMedia && page && !page->canStartMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::pageAllowsPlaybackAfterResuming() const
{
    Page* page = m_element.document().page();
    if (m_restrictions & RequirePageConsentToResumeMedia && page && !page->canStartMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::canShowControlsManager(PlaybackControlsPurpose purpose) const
{
    if (m_element.isSuspended() || !m_element.inActiveDocument()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: isSuspended()");
        return false;
    }

    if (m_element.isFullscreen()) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE: is fullscreen");
        return true;
    }

    if (m_element.muted()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: muted");
        return false;
    }

    if (m_element.document().isMediaDocument() && (m_element.document().frame() && m_element.document().frame()->isMainFrame())) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE: is media document");
        return true;
    }

    if (client().presentationType() == Audio) {
        if (!hasBehaviorRestriction(RequireUserGestureToControlControlsManager) || m_element.document().processingUserGestureForMedia()) {
            INFO_LOG(LOGIDENTIFIER, "returning TRUE: audio element with user gesture");
            return true;
        }

        if (m_element.isPlaying() && allowsPlaybackControlsForAutoplayingAudio()) {
            INFO_LOG(LOGIDENTIFIER, "returning TRUE: user has played media before");
            return true;
        }

        INFO_LOG(LOGIDENTIFIER, "returning FALSE: audio element is not suitable");
        return false;
    }

    if (purpose == PlaybackControlsPurpose::ControlsManager && !isElementRectMostlyInMainFrame(m_element)) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: not in main frame");
        return false;
    }

    if (!m_element.hasAudio() && !m_element.hasEverHadAudio()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: no audio");
        return false;
    }

    if (!playbackPermitted()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: playback not permitted");
        return false;
    }

    if (!hasBehaviorRestriction(RequireUserGestureToControlControlsManager) || m_element.document().processingUserGestureForMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE: no user gesture required");
        return true;
    }

    if (purpose == PlaybackControlsPurpose::ControlsManager && hasBehaviorRestriction(RequirePlaybackToControlControlsManager) && !m_element.isPlaying()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: needs to be playing");
        return false;
    }

    if (!m_element.hasEverNotifiedAboutPlaying()) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: hasn't fired playing notification");
        return false;
    }

#if ENABLE(FULLSCREEN_API)
    // Elements which are not descendents of the current fullscreen element cannot be main content.
    auto* fullscreenElement = m_element.document().webkitCurrentFullScreenElement();
    if (fullscreenElement && !m_element.isDescendantOf(*fullscreenElement)) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE: outside of full screen");
        return false;
    }
#endif

    // Only allow the main content heuristic to forbid videos from showing up if our purpose is the controls manager.
    if (purpose == PlaybackControlsPurpose::ControlsManager && m_element.isVideo()) {
        if (!m_element.renderer()) {
            INFO_LOG(LOGIDENTIFIER, "returning FALSE: no renderer");
            return false;
        }

        if (!m_element.hasVideo() && !m_element.hasEverHadVideo()) {
            INFO_LOG(LOGIDENTIFIER, "returning FALSE: no video");
            return false;
        }

        if (isLargeEnoughForMainContent(MediaSessionMainContentPurpose::MediaControls)) {
            INFO_LOG(LOGIDENTIFIER, "returning TRUE: is main content");
            return true;
        }
    }

    if (purpose == PlaybackControlsPurpose::NowPlaying) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE: potentially plays audio");
        return true;
    }

    INFO_LOG(LOGIDENTIFIER, "returning FALSE: no user gesture");
    return false;
}

bool MediaElementSession::isLargeEnoughForMainContent(MediaSessionMainContentPurpose purpose) const
{
    return isElementLargeEnoughForMainContent(m_element, purpose);
}

MonotonicTime MediaElementSession::mostRecentUserInteractionTime() const
{
    return m_mostRecentUserInteractionTime;
}

bool MediaElementSession::wantsToObserveViewportVisibilityForMediaControls() const
{
    return isLargeEnoughForMainContent(MediaSessionMainContentPurpose::MediaControls);
}

bool MediaElementSession::wantsToObserveViewportVisibilityForAutoplay() const
{
    return m_element.isVideo();
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void MediaElementSession::showPlaybackTargetPicker()
{
    INFO_LOG(LOGIDENTIFIER);

    auto& document = m_element.document();
    if (m_restrictions & RequireUserGestureToShowPlaybackTargetPicker && !document.processingUserGestureForMedia()) {
        INFO_LOG(LOGIDENTIFIER, "returning early because of permissions");
        return;
    }

    if (!document.page()) {
        INFO_LOG(LOGIDENTIFIER, "returning early because page is NULL");
        return;
    }

#if !PLATFORM(IOS_FAMILY)
    if (m_element.readyState() < HTMLMediaElementEnums::HAVE_METADATA) {
        INFO_LOG(LOGIDENTIFIER, "returning early because element is not playable");
        return;
    }
#endif

    auto& audioSession = AudioSession::sharedSession();
    document.showPlaybackTargetPicker(*this, is<HTMLVideoElement>(m_element), audioSession.routeSharingPolicy(), audioSession.routingContextUID());
}

bool MediaElementSession::hasWirelessPlaybackTargets() const
{
    INFO_LOG(LOGIDENTIFIER, "returning ", m_hasPlaybackTargets);

    return m_hasPlaybackTargets;
}

bool MediaElementSession::wirelessVideoPlaybackDisabled() const
{
    if (!m_element.document().settings().allowsAirPlayForMediaPlayback()) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE because of settings");
        return true;
    }

    if (m_element.hasAttributeWithoutSynchronization(HTMLNames::webkitwirelessvideoplaybackdisabledAttr)) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE because of attribute");
        return true;
    }

#if PLATFORM(IOS_FAMILY)
    auto& legacyAirplayAttributeValue = m_element.attributeWithoutSynchronization(HTMLNames::webkitairplayAttr);
    if (equalLettersIgnoringASCIICase(legacyAirplayAttributeValue, "deny")) {
        INFO_LOG(LOGIDENTIFIER, "returning TRUE because of legacy attribute");
        return true;
    }
    if (equalLettersIgnoringASCIICase(legacyAirplayAttributeValue, "allow")) {
        INFO_LOG(LOGIDENTIFIER, "returning FALSE because of legacy attribute");
        return false;
    }
#endif

    auto player = m_element.player();
    if (!player)
        return true;

    bool disabled = player->wirelessVideoPlaybackDisabled();
    INFO_LOG(LOGIDENTIFIER, "returning ", disabled, " because media engine says so");
    
    return disabled;
}

void MediaElementSession::setWirelessVideoPlaybackDisabled(bool disabled)
{
    if (disabled)
        addBehaviorRestriction(WirelessVideoPlaybackDisabled);
    else
        removeBehaviorRestriction(WirelessVideoPlaybackDisabled);

    auto player = m_element.player();
    if (!player)
        return;

    INFO_LOG(LOGIDENTIFIER, disabled);
    player->setWirelessVideoPlaybackDisabled(disabled);
}

void MediaElementSession::setHasPlaybackTargetAvailabilityListeners(bool hasListeners)
{
    INFO_LOG(LOGIDENTIFIER, hasListeners);

#if PLATFORM(IOS_FAMILY)
    m_hasPlaybackTargetAvailabilityListeners = hasListeners;
    PlatformMediaSessionManager::sharedManager().configureWireLessTargetMonitoring();
#else
    UNUSED_PARAM(hasListeners);
    m_element.document().playbackTargetPickerClientStateDidChange(*this, m_element.mediaState());
#endif
}

void MediaElementSession::setPlaybackTarget(Ref<MediaPlaybackTarget>&& device)
{
    m_playbackTarget = WTFMove(device);
    client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
}

void MediaElementSession::targetAvailabilityChangedTimerFired()
{
    client().wirelessRoutesAvailableDidChange();
}

void MediaElementSession::externalOutputDeviceAvailableDidChange(bool hasTargets)
{
    if (m_hasPlaybackTargets == hasTargets)
        return;

    INFO_LOG(LOGIDENTIFIER, hasTargets);

    m_hasPlaybackTargets = hasTargets;
    m_targetAvailabilityChangedTimer.startOneShot(0_s);
}

bool MediaElementSession::isPlayingToWirelessPlaybackTarget() const
{
#if !PLATFORM(IOS_FAMILY)
    if (!m_playbackTarget || !m_playbackTarget->hasActiveRoute())
        return false;
#endif

    return client().isPlayingToWirelessPlaybackTarget();
}

void MediaElementSession::setShouldPlayToPlaybackTarget(bool shouldPlay)
{
    INFO_LOG(LOGIDENTIFIER, shouldPlay);
    m_shouldPlayToPlaybackTarget = shouldPlay;
    client().setShouldPlayToPlaybackTarget(shouldPlay);
}

void MediaElementSession::mediaStateDidChange(MediaProducer::MediaStateFlags state)
{
    m_element.document().playbackTargetPickerClientStateDidChange(*this, state);
}
#endif

MediaPlayer::Preload MediaElementSession::effectivePreloadForElement() const
{
    MediaPlayer::Preload preload = m_element.preloadValue();

    if (pageExplicitlyAllowsElementToAutoplayInline(m_element))
        return preload;

    if (m_restrictions & MetadataPreloadingNotPermitted)
        return MediaPlayer::None;

    if (m_restrictions & AutoPreloadingNotPermitted) {
        if (preload > MediaPlayer::MetaData)
            return MediaPlayer::MetaData;
    }

    return preload;
}

bool MediaElementSession::requiresFullscreenForVideoPlayback() const
{
    if (pageExplicitlyAllowsElementToAutoplayInline(m_element))
        return false;

    if (is<HTMLAudioElement>(m_element))
        return false;

    if (m_element.document().isMediaDocument()) {
        ASSERT(is<HTMLVideoElement>(m_element));
        const HTMLVideoElement& videoElement = *downcast<const HTMLVideoElement>(&m_element);
        if (m_element.readyState() < HTMLVideoElement::HAVE_METADATA || !videoElement.hasEverHadVideo())
            return false;
    }

    if (m_element.isTemporarilyAllowingInlinePlaybackAfterFullscreen())
        return false;

    if (!m_element.document().settings().allowsInlineMediaPlayback())
        return true;

    if (!m_element.document().settings().inlineMediaPlaybackRequiresPlaysInlineAttribute())
        return false;

#if PLATFORM(IOS_FAMILY)
    if (IOSApplication::isIBooks())
        return !m_element.hasAttributeWithoutSynchronization(HTMLNames::webkit_playsinlineAttr) && !m_element.hasAttributeWithoutSynchronization(HTMLNames::playsinlineAttr);
    if (dyld_get_program_sdk_version() < DYLD_IOS_VERSION_10_0)
        return !m_element.hasAttributeWithoutSynchronization(HTMLNames::webkit_playsinlineAttr);
#endif

    if (m_element.document().isMediaDocument() && m_element.document().ownerElement())
        return false;

    return !m_element.hasAttributeWithoutSynchronization(HTMLNames::playsinlineAttr);
}

bool MediaElementSession::allowsAutomaticMediaDataLoading() const
{
    if (pageExplicitlyAllowsElementToAutoplayInline(m_element))
        return true;

    if (m_element.document().settings().mediaDataLoadsAutomatically())
        return true;

    return false;
}

void MediaElementSession::mediaEngineUpdated()
{
    INFO_LOG(LOGIDENTIFIER);

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_restrictions & WirelessVideoPlaybackDisabled)
        setWirelessVideoPlaybackDisabled(true);
    if (m_playbackTarget)
        client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
    if (m_shouldPlayToPlaybackTarget)
        client().setShouldPlayToPlaybackTarget(true);
#endif
    
}

void MediaElementSession::resetPlaybackSessionState()
{
    m_mostRecentUserInteractionTime = MonotonicTime();
    addBehaviorRestriction(RequireUserGestureToControlControlsManager | RequirePlaybackToControlControlsManager);
}

bool MediaElementSession::allowsPictureInPicture() const
{
    return m_element.document().settings().allowsPictureInPictureMediaPlayback();
}

#if PLATFORM(IOS_FAMILY)
bool MediaElementSession::requiresPlaybackTargetRouteMonitoring() const
{
    return m_hasPlaybackTargetAvailabilityListeners && !m_element.elementIsHidden();
}
#endif

#if ENABLE(MEDIA_SOURCE)
size_t MediaElementSession::maximumMediaSourceBufferSize(const SourceBuffer& buffer) const
{
    // A good quality 1080p video uses 8,000 kbps and stereo audio uses 384 kbps, so assume 95% for video and 5% for audio.
    const float bufferBudgetPercentageForVideo = .95;
    const float bufferBudgetPercentageForAudio = .05;

    size_t maximum = buffer.document().settings().maximumSourceBufferSize();

    // Allow a SourceBuffer to buffer as though it is audio-only even if it doesn't have any active tracks (yet).
    size_t bufferSize = static_cast<size_t>(maximum * bufferBudgetPercentageForAudio);
    if (buffer.hasVideo())
        bufferSize += static_cast<size_t>(maximum * bufferBudgetPercentageForVideo);

    // FIXME: we might want to modify this algorithm to:
    // - decrease the maximum size for background tabs
    // - decrease the maximum size allowed for inactive elements when a process has more than one
    //   element, eg. so a page with many elements which are played one at a time doesn't keep
    //   everything buffered after an element has finished playing.

    return bufferSize;
}
#endif

static bool isMainContentForPurposesOfAutoplay(const HTMLMediaElement& element)
{
    Document& document = element.document();
    if (!document.hasLivingRenderTree() || document.activeDOMObjectsAreStopped() || element.isSuspended() || !element.hasAudio() || !element.hasVideo())
        return false;

    // Elements which have not yet been laid out, or which are not yet in the DOM, cannot be main content.
    auto* renderer = element.renderer();
    if (!renderer)
        return false;

    if (!isElementLargeEnoughForMainContent(element, MediaSessionMainContentPurpose::Autoplay))
        return false;

    // Elements which are hidden by style, or have been scrolled out of view, cannot be main content.
    // But elements which have audio & video and are already playing should not stop playing because
    // they are scrolled off the page.
    if (renderer->style().visibility() != Visibility::Visible)
        return false;
    if (renderer->visibleInViewportState() != VisibleInViewportState::Yes && !element.isPlaying())
        return false;

    // Main content elements must be in the main frame.
    if (!document.frame() || !document.frame()->isMainFrame())
        return false;

    auto& mainFrame = document.frame()->mainFrame();
    if (!mainFrame.view() || !mainFrame.view()->renderView())
        return false;

    RenderView& mainRenderView = *mainFrame.view()->renderView();

    // Hit test the area of the main frame where the element appears, to determine if the element is being obscured.
    IntRect rectRelativeToView = element.clientRect();
    ScrollPosition scrollPosition = mainFrame.view()->documentScrollPositionRelativeToViewOrigin();
    IntRect rectRelativeToTopDocument(rectRelativeToView.location() + scrollPosition, rectRelativeToView.size());
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::AllowChildFrameContent | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowUserAgentShadowContent);
    HitTestResult result(rectRelativeToTopDocument.center());

    // Elements which are obscured by other elements cannot be main content.
    mainRenderView.hitTest(request, result);
    result.setToNonUserAgentShadowAncestor();
    RefPtr<Element> hitElement = result.targetElement();
    if (hitElement != &element)
        return false;

    return true;
}

static bool isElementRectMostlyInMainFrame(const HTMLMediaElement& element)
{
    if (!element.renderer())
        return false;

    auto documentFrame = makeRefPtr(element.document().frame());
    if (!documentFrame)
        return false;

    auto mainFrameView = documentFrame->mainFrame().view();
    if (!mainFrameView)
        return false;

    IntRect mainFrameRectAdjustedForScrollPosition = IntRect(-mainFrameView->documentScrollPositionRelativeToViewOrigin(), mainFrameView->contentsSize());
    IntRect elementRectInMainFrame = element.clientRect();
    auto totalElementArea = elementRectInMainFrame.area<RecordOverflow>();
    if (totalElementArea.hasOverflowed())
        return false;

    elementRectInMainFrame.intersect(mainFrameRectAdjustedForScrollPosition);

    return elementRectInMainFrame.area().unsafeGet() > totalElementArea.unsafeGet() / 2;
}

static bool isElementLargeRelativeToMainFrame(const HTMLMediaElement& element)
{
    static const double minimumPercentageOfMainFrameAreaForMainContent = 0.9;
    auto* renderer = element.renderer();
    if (!renderer)
        return false;

    auto documentFrame = makeRefPtr(element.document().frame());
    if (!documentFrame)
        return false;

    if (!documentFrame->mainFrame().view())
        return false;

    auto& mainFrameView = *documentFrame->mainFrame().view();
    auto maxVisibleClientWidth = std::min(renderer->clientWidth().toInt(), mainFrameView.visibleWidth());
    auto maxVisibleClientHeight = std::min(renderer->clientHeight().toInt(), mainFrameView.visibleHeight());

    return maxVisibleClientWidth * maxVisibleClientHeight > minimumPercentageOfMainFrameAreaForMainContent * mainFrameView.visibleWidth() * mainFrameView.visibleHeight();
}

static bool isElementLargeEnoughForMainContent(const HTMLMediaElement& element, MediaSessionMainContentPurpose purpose)
{
    static const double elementMainContentAreaMinimum = 400 * 300;
    static const double maximumAspectRatio = purpose == MediaSessionMainContentPurpose::MediaControls ? 3 : 1.8;
    static const double minimumAspectRatio = .5; // Slightly smaller than 9:16.

    // Elements which have not yet been laid out, or which are not yet in the DOM, cannot be main content.
    auto* renderer = element.renderer();
    if (!renderer)
        return false;

    double width = renderer->clientWidth();
    double height = renderer->clientHeight();
    double area = width * height;
    double aspectRatio = width / height;

    if (area < elementMainContentAreaMinimum)
        return false;

    if (aspectRatio >= minimumAspectRatio && aspectRatio <= maximumAspectRatio)
        return true;

    return isElementLargeRelativeToMainFrame(element);
}

void MediaElementSession::mainContentCheckTimerFired()
{
    if (!hasBehaviorRestriction(OverrideUserGestureRequirementForMainContent))
        return;

    updateIsMainContent();
}

bool MediaElementSession::updateIsMainContent() const
{
    if (m_element.isSuspended())
        return false;

    bool wasMainContent = m_isMainContent;
    m_isMainContent = isMainContentForPurposesOfAutoplay(m_element);

    if (m_isMainContent != wasMainContent)
        m_element.updateShouldPlay();

    return m_isMainContent;
}

bool MediaElementSession::allowsNowPlayingControlsVisibility() const
{
    auto page = m_element.document().page();
    return page && !page->isVisibleAndActive();
}

bool MediaElementSession::allowsPlaybackControlsForAutoplayingAudio() const
{
    auto page = m_element.document().page();
    return page && page->allowsPlaybackControlsForAutoplayingAudio();
}

}

#endif // ENABLE(VIDEO)
