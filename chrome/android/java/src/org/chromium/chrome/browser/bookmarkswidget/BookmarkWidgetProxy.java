// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarkswidget;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * Proxy that responds to tapping on the Bookmarks widget.
 */
public class BookmarkWidgetProxy extends BroadcastReceiver {
    private static final String TAG = "BookmarkWidgetProxy";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (BookmarkThumbnailWidgetService.getChangeFolderAction(context)
                .equals(intent.getAction())) {
            BookmarkThumbnailWidgetService.changeFolder(context, intent);
        } else {
            Intent view = new Intent(intent);
            view.setComponent(null);
            startActivity(context, view);
        }
    }

    void startActivity(Context context, Intent intent) {
        try {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        } catch (Exception e) {
            Log.w(TAG, "Failed to start intent activity", e);
        }
    }
}
