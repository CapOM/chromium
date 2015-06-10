// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.os.AsyncTask;

import org.chromium.base.PowerMonitor;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.bookmarkswidget.BookmarkThumbnailWidgetProviderBase;
import org.chromium.chrome.browser.crash.CrashFileManager;
import org.chromium.chrome.browser.crash.MinidumpUploadService;
import org.chromium.chrome.browser.download.DownloadManagerService;
import org.chromium.chrome.browser.media.MediaNotificationService;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksShim;
import org.chromium.chrome.browser.precache.PrecacheLauncher;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.preferences.privacy.PrivacyPreferencesManager;
import org.chromium.chrome.browser.share.ShareHelper;

/**
 * Handler for application level tasks to be completed on deferred startup.
 */
public class DeferredStartupHandler {
    private static DeferredStartupHandler sDeferredStartupHandler;
    private boolean mDeferredStartupComplete;

    /**
     * This class is an application specific object that handles the deferred startup.
     * @return The singleton instance of {@link DeferredStartupHandler}.
     */
    public static DeferredStartupHandler getInstance() {
        if (sDeferredStartupHandler == null) {
            sDeferredStartupHandler = new DeferredStartupHandler();
        }
        return sDeferredStartupHandler;
    }

    private DeferredStartupHandler() { }

    /**
     * Handle application level deferred startup tasks that can be lazily done after all
     * the necessary initialization has been completed. Any calls requiring network access should
     * probably go here.
     * @param application The application object to use for context.
     * @param crashDumpUploadingDisabled Whether crash dump uploading should be disabled.
     */
    public void onDeferredStartup(final ChromeMobileApplication application,
            final boolean crashDumpUploadingDisabled) {
        if (mDeferredStartupComplete) return;
        ThreadUtils.assertOnUiThread();

        // Punt all tasks that may block the UI thread off onto a background thread.
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                try {
                    TraceEvent.begin("ChromeBrowserInitializer.onDeferredStartup.doInBackground");
                    if (crashDumpUploadingDisabled) {
                        PrivacyPreferencesManager.getInstance(application).disableCrashUploading();
                    } else {
                        MinidumpUploadService.tryUploadAllCrashDumps(application);
                    }
                    CrashFileManager crashFileManager =
                            new CrashFileManager(application.getCacheDir());
                    crashFileManager.cleanOutAllNonFreshMinidumpFiles();

                    MinidumpUploadService.storeBreakpadUploadAttemptsInUma(
                                ChromePreferenceManager.getInstance(application));

                    // Force a widget refresh in order to wake up any possible zombie widgets.
                    // This is needed to ensure the right behavior when the process is suddenly
                    // killed.
                    BookmarkThumbnailWidgetProviderBase.refreshAllWidgets(application);

                    // Initialize whether or not precaching is enabled.
                    PrecacheLauncher.updatePrecachingEnabled(
                            PrivacyPreferencesManager.getInstance(application), application);

                    return null;
                } finally {
                    TraceEvent.end("ChromeBrowserInitializer.onDeferredStartup.doInBackground");
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

        // TODO(aruslan): http://b/6397072 This will be moved elsewhere
        PartnerBookmarksShim.kickOffReading(application);

        PowerMonitor.create(application);

        // Starts syncing with GSA.
        application.createGsaHelper().startSync();

        DownloadManagerService.getDownloadManagerService(application)
                .clearPendingDownloadNotifications();

        application.initializeSharedClasses();

        ShareHelper.clearSharedScreenshots(application);

        // Clear any media notifications that existed when Chrome was last killed.
        MediaNotificationService.clearMediaNotifications(application);

        mDeferredStartupComplete = true;
    }

    /**
    * @return Whether deferred startup has been completed.
    */
    @VisibleForTesting
    public boolean isDeferredStartupComplete() {
        return mDeferredStartupComplete;
    }
}
