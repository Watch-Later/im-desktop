#pragma once

namespace Features
{
    bool isNicksEnabled();
    QString getProfileDomain();
    QString getProfileDomainAgent();
    size_t maximumUndoStackSize();
    bool useAppleEmoji();
    bool opensOnClick();
    bool phoneAllowed();
    bool externalPhoneAttachment();
    bool showNotificationsText();
    bool showNotificationsTextSettings();

    QString dataVisibilityLink();
    QString passwordRecoveryLink();
    QString securityCallLink();
    QString attachPhoneUrl();
    QString updateAppUrl();

    enum class LoginMethod
    {
        Basic,
        OTPViaEmail
    };

    LoginMethod loginMethod();
    bool isOAuth2LoginAllowed();
    size_t getSMSResultTime();

    bool showAttachPhoneNumberPopup();
    bool closeBtnAttachPhoneNumberPopup();
    int showTimeoutAttachPhoneNumberPopup();
    QString textAttachPhoneNumberPopup();
    QString titleAttachPhoneNumberPopup();

    bool isSmartreplyEnabled();
    bool isSmartreplyForQuoteEnabled();
    std::chrono::milliseconds smartreplyHideTime();
    int smartreplyMsgidCacheSize();

    bool isSuggestsEnabled();
    std::chrono::milliseconds getSuggestTimerInterval();
    bool isServerSuggestsEnabled();
    int maxAllowedLocalSuggestChars();
    int maxAllowedSuggestChars();
    int maxAllowedLocalSuggestWords();
    int maxAllowedSuggestWords();

    bool betaUpdateAllowed();
    bool updateAllowed();
    bool avatarChangeAllowed();
    bool clRemoveContactsAllowed();

    bool changeNameAllowed();
    bool changeInfoAllowed();

    bool pollsEnabled();

    int getFsIDLength();

    bool isSpellCheckEnabled();
    size_t spellCheckMaxSuggestCount();

    QString favoritesImageIdEnglish();
    QString favoritesImageIdRussian();

    int getAsyncResponseTimeout();

    int getVoipCallUserLimit();
    int getVoipVideoUserLimit();
    int getVoipBigConferenceBoundary();

    bool isVcsCallByLinkEnabled();
    bool isVcsWebinarEnabled();

    QString getVcsRoomList();

    std::string getReactionsJson();
    bool reactionsEnabled();

    std::string getStatusJson();
    bool isStatusEnabled();
    bool isCustomStatusEnabled();

    bool isGlobalContactSearchAllowed();

    bool forceCheckMacUpdates();
    bool callRoomInfoEnabled();

    bool areYourInvitesButtonVisible();
    bool isGroupInvitesBlacklistEnabled();

    bool IvrLoginEnabled();
    int IvrResendCountToShow();

    bool isContactUsViaBackend();
    bool isUpdateFromBackendEnabled();

    bool isWebpScreenshotEnabled();
    qint64 maxFileSizeForWebpConvert();

    bool isInviteBySmsEnabled();
    bool shouldShowSmsNotifySetting();

    bool isAnimatedStickersInPickerAllowed();
    bool isAnimatedStickersInChatAllowed();

    bool isContactListSmoothScrollingEnabled();

    bool isBackgroundPttPlayEnabled();

    bool removeDeletedFromNotifications();

    bool longPathTooltipsAllowed();

    bool isFormattingInBubblesEnabled();

    bool isFormattingInInputEnabled();

    bool isBlockFormattingInInputEnabled();

    QString statusBannerEmojis();

    bool isAppsNavigationBarVisible();
    bool isMessengerTabBarVisible();
    bool isStatusInAppsNavigationBar();

    bool isOrganizationStructureEnabled();
    bool isTasksEnabled();
    bool isCalendarEnabled();

    bool isRecentsPinnedItemsEnabled();
    bool isScheduledMessagesEnabled();
    bool isThreadsEnabled();
    bool isRemindersEnabled();

    bool isSharedFederationStickerpacksSupported();

    std::chrono::milliseconds threadResubscribeTimeout();

    bool isDraftEnabled();
    std::chrono::seconds draftTimeout();
    int draftMaximumLength();

    bool isMessageCornerMenuEnabled();
    bool isTaskCreationInChatEnabled();

    bool isRestrictedFilesEnabled();
    bool trustedStatusDefault();

    bool isAntivirusCheckEnabled();
    bool isAntivirusCheckProgressVisible();

    bool isExpandedGalleryEnabled();
    bool hasWebEngine();

    std::chrono::seconds webPageReloadInterval();
}
