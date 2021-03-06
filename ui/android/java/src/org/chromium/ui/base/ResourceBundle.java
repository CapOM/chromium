// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.ResourceExtractor;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.SuppressFBWarnings;

import java.io.File;
import java.util.Locale;

/**
 * This class provides the resource bundle related methods for the native
 * library.
 */
@JNINamespace("ui")
public class ResourceBundle {
    private static ResourceExtractor.ResourceEntry[] sActiveLocaleResources;

    /**
     * Applies the reverse mapping done by locale_pak_resources.py.
     */
    private static String toChromeLocaleName(String srcFileName) {
        srcFileName = srcFileName.replace(".lpak", ".pak");
        String[] parts = srcFileName.split("_");
        if (parts.length > 1) {
            int dotIdx = parts[1].indexOf('.');
            return parts[0] + "-" + parts[1].substring(0, dotIdx).toUpperCase(Locale.ENGLISH)
                    + parts[1].substring(dotIdx);
        }
        return srcFileName;
    }

    /**
     * Initializes the list of locale packs for the active locale that are
     * present within the apk.
     *
     * @param context Any context
     * @param localePaksResId Resource ID locale_paks (generated by
     *            locale_pak_resources.py)
     */
    @SuppressFBWarnings("LI_LAZY_INIT_UPDATE_STATIC")  // Not thread-safe.
    public static void initializeLocalePaks(Context context, int localePaksResId) {
        ThreadUtils.assertOnUiThread();
        assert sActiveLocaleResources == null;
        Resources resources = context.getResources();
        TypedArray resIds = resources.obtainTypedArray(localePaksResId);
        try {
            int len = resIds.length();
            sActiveLocaleResources = new ResourceExtractor.ResourceEntry[len];
            for (int i = 0; i < len; ++i) {
                int resId = resIds.getResourceId(i, 0);
                String resPath = resources.getString(resId);
                String srcBaseName = new File(resPath).getName();
                String dstBaseName = toChromeLocaleName(srcBaseName);
                sActiveLocaleResources[i] = new ResourceExtractor.ResourceEntry(resId, resPath,
                        dstBaseName);
            }
        } finally {
            resIds.recycle();
        }
    }

    @SuppressFBWarnings("MS_EXPOSE_REP")  // Don't modify the array.
    public static ResourceExtractor.ResourceEntry[] getActiveLocaleResources() {
        return sActiveLocaleResources;
    }

    @CalledByNative
    private static String getLocalePakResourcePath(String locale) {
        if (sActiveLocaleResources == null) {
            return null;
        }
        String fileName = locale + ".pak";
        for (ResourceExtractor.ResourceEntry entry : sActiveLocaleResources) {
            if (fileName.equals(entry.extractedFileName)) {
                return entry.pathWithinApk;
            }
        }
        return null;
    }
}
