// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.rlz;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.JNINamespace;
import org.chromium.chrome.browser.ChromeMobileApplication;
import org.chromium.chrome.browser.Tab;

import java.util.concurrent.atomic.AtomicReference;

/**
 * Utility class for managing revenue sharing information.
 */
@JNINamespace("chrome::android")
public class RevenueStats {
    private static final String PREF_RLZ_NOTIFIED = "rlz_first_search_notified";

    // Use an AtomicReference since getInstance() can be called from multiple threads.
    private static AtomicReference<RevenueStats> sInstance = new AtomicReference<RevenueStats>();

    /**
     * Returns the singleton instance of ExternalAuthUtils, creating it if needed.
     */
    public static RevenueStats getInstance() {
        if (sInstance.get() == null) {
            ChromeMobileApplication application =
                    (ChromeMobileApplication) ApplicationStatus.getApplicationContext();
            sInstance.compareAndSet(null, application.createRevenueStatsInstance());
        }
        return sInstance.get();
    }

    /**
     * Notifies tab creation event.
     */
    public void tabCreated(Tab chromeTab) {}

    /**
     * Retrieves the client ID string.
     */
    public String retrieveClientId() {
        return null;
    }

    /**
     * Returns whether the RLZ provider has been notified that the first search has occurred.
     */
    protected static boolean getRlzNotified(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getBoolean(
                PREF_RLZ_NOTIFIED, false);
    }

    /**
     * Stores whether the RLZ provider has been notified that the first search has occurred as
     * shared preference.
     */
    protected static void setRlzNotified(Context context, boolean notified) {
        SharedPreferences.Editor sharedPreferencesEditor =
                PreferenceManager.getDefaultSharedPreferences(context).edit();
        sharedPreferencesEditor.putBoolean(PREF_RLZ_NOTIFIED, notified);
        sharedPreferencesEditor.apply();
    }

    /**
     * Sets search client id.
     */
    protected static void setSearchClient(String client) {
        nativeSetSearchClient(client);
    }

    /**
     * Sets rlz value.
     */
    protected static void setRlzParameterValue(String rlz) {
        nativeSetRlzParameterValue(rlz);
    }

    private static native void nativeSetSearchClient(String client);
    private static native void nativeSetRlzParameterValue(String rlz);
}
