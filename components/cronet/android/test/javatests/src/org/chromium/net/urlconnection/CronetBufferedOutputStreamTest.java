// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.urlconnection;

import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.test.util.Feature;

import org.chromium.net.CronetTestBase;
import org.chromium.net.NativeTestServer;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.net.URL;

/**
 * Tests the CronetBufferedOutputStream implementation.
 */
public class CronetBufferedOutputStreamTest extends CronetTestBase {
    private static final String UPLOAD_DATA_STRING = "Nifty upload data!";
    private static final byte[] UPLOAD_DATA = UPLOAD_DATA_STRING.getBytes();
    private static final int REPEAT_COUNT = 100000;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        launchCronetTestApp();
        assertTrue(NativeTestServer.startNativeTestServer(
                getInstrumentation().getTargetContext()));
    }

    @Override
    protected void tearDown() throws Exception {
        NativeTestServer.shutdownNativeTestServer();
        super.tearDown();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testGetOutputStreamAfterConnectionMade() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        assertEquals(200, connection.getResponseCode());
        try {
            connection.getOutputStream();
            fail();
        } catch (java.net.ProtocolException e) {
            // Expected.
        }
    }

    /**
     * Tests write after connect. Strangely, the default implementation allows
     * writing after being connected, so this test only runs against Cronet's
     * implementation.
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunCronetHttpURLConnection
    public void testWriteAfterConnect() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        connection.connect();
        try {
            // Attemp to write some more.
            out.write(UPLOAD_DATA);
            fail();
        } catch (IllegalStateException e) {
            assertEquals("Cannot write after being connected.", e.getMessage());
        }
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteAfterReadingResponse() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        assertEquals(200, connection.getResponseCode());
        try {
            out.write(UPLOAD_DATA);
            fail();
        } catch (Exception e) {
            // Default implementation gives an IOException and says that the
            // stream is closed. Cronet gives an IllegalStateException and
            // complains about write after connected.
        }
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithContentLength() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        byte[] largeData = getLargeData();
        connection.setRequestProperty("Content-Length",
                Integer.toString(largeData.length));
        OutputStream out = connection.getOutputStream();
        int totalBytesWritten = 0;
        // Number of bytes to write each time. It is doubled each time
        // to make sure that the buffer grows.
        int bytesToWrite = 683;
        while (totalBytesWritten < largeData.length) {
            if (bytesToWrite > largeData.length - totalBytesWritten) {
                // Do not write out of bound.
                bytesToWrite = largeData.length - totalBytesWritten;
            }
            out.write(largeData, totalBytesWritten, bytesToWrite);
            totalBytesWritten += bytesToWrite;
            bytesToWrite *= 2;
        }
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithContentLengthOneMassiveWrite() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        byte[] largeData = getLargeData();
        connection.setRequestProperty("Content-Length",
                Integer.toString(largeData.length));
        OutputStream out = connection.getOutputStream();
        out.write(largeData);
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithContentLengthWriteOneByte() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        byte[] largeData = getLargeData();
        connection.setRequestProperty("Content-Length",
                Integer.toString(largeData.length));
        OutputStream out = connection.getOutputStream();
        for (int i = 0; i < largeData.length; i++) {
            out.write(largeData[i]);
        }
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithZeroContentLength() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Content-Length", "0");
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        assertEquals("", getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostZeroByteWithoutContentLength() throws Exception {
        // Make sure both implementation sets the Content-Length header to 0.
        URL url = new URL(NativeTestServer.getEchoHeaderURL("Content-Length"));
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        assertEquals("0", getResponseAsString(connection));
        connection.disconnect();

        // Make sure the server echoes back empty body for both implementation.
        URL echoBody = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection2 =
                (HttpURLConnection) echoBody.openConnection();
        connection2.setDoOutput(true);
        connection2.setRequestMethod("POST");
        assertEquals(200, connection2.getResponseCode());
        assertEquals("OK", connection2.getResponseMessage());
        assertEquals("", getResponseAsString(connection2));
        connection2.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithoutContentLengthSmall() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        assertEquals(UPLOAD_DATA_STRING, getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithoutContentLength() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        byte[] largeData = getLargeData();
        OutputStream out = connection.getOutputStream();
        int totalBytesWritten = 0;
        // Number of bytes to write each time. It is doubled each time
        // to make sure that the buffer grows.
        int bytesToWrite = 683;
        while (totalBytesWritten < largeData.length) {
            if (bytesToWrite > largeData.length - totalBytesWritten) {
                // Do not write out of bound.
                bytesToWrite = largeData.length - totalBytesWritten;
            }
            out.write(largeData, totalBytesWritten, bytesToWrite);
            totalBytesWritten += bytesToWrite;
            bytesToWrite *= 2;
        }
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithoutContentLengthOneMassiveWrite() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        byte[] largeData = getLargeData();
        out.write(largeData);
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testPostWithoutContentLengthWriteOneByte() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        byte[] largeData = getLargeData();
        for (int i = 0; i < largeData.length; i++) {
            out.write(largeData[i]);
        }
        assertEquals(200, connection.getResponseCode());
        assertEquals("OK", connection.getResponseMessage());
        checkLargeData(getResponseAsString(connection));
        connection.disconnect();
    }

    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteLessThanContentLength()
            throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        // Set a content length that's 1 byte more.
        connection.setRequestProperty("Content-Length",
                Integer.toString(UPLOAD_DATA.length + 1));
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        try {
            connection.getResponseCode();
            fail();
        } catch (ProtocolException e) {
            // Expected.
        }
        connection.disconnect();
    }

    /**
     * Tests that if caller writes more than the content length provided,
     * an exception should occur.
     */
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteMoreThanContentLength() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        // Use a content length that is 1 byte shorter than actual data.
        connection.setRequestProperty("Content-Length",
                Integer.toString(UPLOAD_DATA.length - 1));
        OutputStream out = connection.getOutputStream();
        // Write a few bytes first.
        out.write(UPLOAD_DATA, 0, 3);
        try {
            // Write remaining bytes.
            out.write(UPLOAD_DATA, 3, UPLOAD_DATA.length - 3);
            fail();
        } catch (ProtocolException e) {
            assertEquals("exceeded content-length limit of "
                    + (UPLOAD_DATA.length - 1) + " bytes", e.getMessage());
        }
    }

    /**
     * Same as {@code testWriteMoreThanContentLength()}, but it only writes one byte
     * at a time.
     */
    @SmallTest
    @Feature({"Cronet"})
    @CompareDefaultWithCronet
    public void testWriteMoreThanContentLengthWriteOneByte() throws Exception {
        URL url = new URL(NativeTestServer.getEchoBodyURL());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        // Use a content length that is 1 byte shorter than actual data.
        connection.setRequestProperty("Content-Length",
                Integer.toString(UPLOAD_DATA.length - 1));
        OutputStream out = connection.getOutputStream();
        try {
            for (int i = 0; i < UPLOAD_DATA.length; i++) {
                out.write(UPLOAD_DATA[i]);
            }
            fail();
        } catch (java.net.ProtocolException e) {
            assertEquals("exceeded content-length limit of "
                    + (UPLOAD_DATA.length - 1) + " bytes", e.getMessage());
        }
    }

    /**
     * Tests that {@link CronetBufferedOutputStream} supports rewind in a
     * POST preserving redirect.
     * Use {@code OnlyRunCronetHttpURLConnection} as the default implementation
     * does not pass this test.
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunCronetHttpURLConnection
    public void testRewind() throws Exception {
        URL url = new URL(NativeTestServer.getRedirectToEchoBody());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Content-Length",
                Integer.toString(UPLOAD_DATA.length));
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        assertEquals(UPLOAD_DATA_STRING, getResponseAsString(connection));
        connection.disconnect();
    }

    /**
     * Like {@link #testRewind} but does not set Content-Length header.
     */
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunCronetHttpURLConnection
    public void testRewindWithoutContentLength() throws Exception {
        URL url = new URL(NativeTestServer.getRedirectToEchoBody());
        HttpURLConnection connection =
                (HttpURLConnection) url.openConnection();
        connection.setDoOutput(true);
        connection.setRequestMethod("POST");
        OutputStream out = connection.getOutputStream();
        out.write(UPLOAD_DATA);
        assertEquals(UPLOAD_DATA_STRING, getResponseAsString(connection));
        connection.disconnect();
    }

    /**
     * Helper method to extract response body as a string for testing.
     */
    private String getResponseAsString(HttpURLConnection connection)
            throws Exception {
        InputStream in = connection.getInputStream();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        int b;
        while ((b = in.read()) != -1) {
            out.write(b);
        }
        return out.toString();
    }

    /**
     * Produces a byte array that contains {@code REPEAT_COUNT} of
     * {@code UPLOAD_DATA_STRING}.
     */
    private byte[] getLargeData() {
        byte[] largeData = new byte[REPEAT_COUNT * UPLOAD_DATA.length];
        for (int i = 0; i < REPEAT_COUNT; i++) {
            for (int j = 0; j < UPLOAD_DATA.length; j++) {
                largeData[i * UPLOAD_DATA.length + j] = UPLOAD_DATA[j];
            }
        }
        return largeData;
    }

    /**
     * Helper function to check whether {@code data} is a concatenation of
     * {@code REPEAT_COUNT} {@code UPLOAD_DATA_STRING} strings.
     */
    private void checkLargeData(String data) {
        for (int i = 0; i < REPEAT_COUNT; i++) {
            assertEquals(UPLOAD_DATA_STRING, data.substring(
                    UPLOAD_DATA_STRING.length() * i,
                    UPLOAD_DATA_STRING.length() * (i + 1)));
        }
    }
}
