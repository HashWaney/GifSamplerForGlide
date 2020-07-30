package com.hash.study.gif;

import android.graphics.Bitmap;
import android.util.Log;

import androidx.annotation.Nullable;

import java.io.InputStream;
import java.nio.ByteBuffer;

public final class GifDecoder {

    private static final String TAG = GifDecoder.class.getSimpleName();

    // /////////////////////////////////////////// Get instance //////////////////////////////////////////////////

    /**
     * Get an instance of GifDecoder
     *
     * @param filePath a gif file path.
     * @return an instance of GifDecoder, if decode failed will return null.
     */
    @Nullable
    public static GifDecoder decodeFilePath(String filePath) {
        return nativeDecodeFile(filePath);
    }

    /**
     * Get an instance of GifDecoder
     *
     * @param stream a gif stream
     * @return an instance of GifDecoder, if decode failed will return null.
     */
    @Nullable
    public static GifDecoder decodeStream(InputStream stream) {
        if (stream == null) {
            throw new IllegalArgumentException();
        }
        // use buffer pool
        byte[] tempStorage = new byte[16 * 1024];
        return nativeDecodeStream(stream, tempStorage);
    }

    /**
     * Get an instance of GifDecoder
     *
     * @param data a gif byte array.
     * @return an instance of GifDecoder, if decode failed will return null.
     */
    @Nullable
    public static GifDecoder decodeByteArray(byte[] data) {
        return decodeByteArray(data, 0, data.length);
    }

    /**
     * Get an instance of GifDecoder
     *
     * @param data a gif byte array.
     * @return an instance of GifDecoder, if decode failed will return null.
     */
    @Nullable
    public static GifDecoder decodeByteArray(byte[] data, int offset, int length) {
        if (data == null) {
            throw new IllegalArgumentException();
        }
        if (offset < 0 || length < 0 || (offset + length > data.length)) {
            throw new IllegalArgumentException("invalid offset/length parameters");
        }
        return nativeDecodeByteArray(data, offset, length);
    }

    /**
     * Get an instance of GifDecoder
     *
     * @param buffer a gif native buffer.
     * @return an instance of GifDecoder, if decode failed will return null.
     */
    @Nullable
    public static GifDecoder decodeByteBuffer(ByteBuffer buffer) {
        if (buffer == null) {
            throw new IllegalArgumentException();
        }
        if (!buffer.isDirect()) {
            if (buffer.hasArray()) {
                byte[] byteArray = buffer.array();
                return decodeByteArray(byteArray, buffer.position(), buffer.remaining());
            } else {
                throw new IllegalArgumentException("Cannot have non-direct ByteBuffer with no byte array");
            }
        }
        return nativeDecodeByteBuffer(buffer, buffer.position(), buffer.remaining());
    }

    // /////////////////////////////////////////// Inner Method. //////////////////////////////////////////////////

    private long mNativePtr;
    private final int mWidth, mHeight, mFrameCount, mLooperCount;
    private final boolean mIsOpaque;
    private final long mDuration;

    // invoke at native
    private GifDecoder(long nativePtr, int width, int height, boolean isOpaque, int frameCount, int looperCount, long duration) {
        this.mNativePtr = nativePtr;
        this.mWidth = width;
        this.mHeight = height;
        this.mIsOpaque = isOpaque;
        this.mFrameCount = frameCount;
        this.mLooperCount = looperCount;
        this.mDuration = duration;
        if (BuildConfig.DEBUG) {
            Log.e(TAG, toString());
        }
    }

    @Override
    public String toString() {
        return "GifDecoder{" +
                "Width=" + mWidth + ", " +
                "Height=" + mHeight + ", " +
                "IsOpaque=" + mIsOpaque + ", " +
                "FrameCount=" + mFrameCount + ", " +
                "LooperCount=" + mLooperCount + ", " +
                "Duration=" + mDuration + "ms " +
                '}';
    }

    /**
     * Get Bitmap at require frame.
     *
     * @param frameNr         the frame that u wanted.
     * @param output          in and out args, will fill pixels at native.
     * @param previousFrameNr previous frame number, u can pass -1.
     * @param inSampleSize    do sample size, is power of 2.
     * @return next frame duration. Unit is ms
     */
    public long getFrame(int frameNr, Bitmap output, int previousFrameNr, int inSampleSize) {
        return nativeGetFrame(mNativePtr, frameNr, output, previousFrameNr, inSampleSize);
    }

    /**
     * Get gif width.
     *
     * @return gif width.
     */
    public int getWidth() {
        return mWidth;
    }

    /**
     * Get gif height.
     *
     * @return gif height.
     */
    public int getHeight() {
        return mHeight;
    }

    /**
     * Get gif frame count.
     *
     * @return gif frame count.
     */
    public int getFrameCount() {
        return mFrameCount;
    }

    /**
     * Get Looper count;
     */
    public int getLooperCount() {
        return mLooperCount;
    }

    /**
     * Get gif background color have opaque or not.
     *
     * @return true is have.
     */
    public boolean isOpaque() {
        return mIsOpaque;
    }

    /**
     * Get the duration associated with this gif.
     *
     * @return Unit is ms.
     */
    public long getDuration() {
        return mDuration;
    }

    /**
     * Destroy resource.
     */
    public void destroy() {
        nativeDestroy(mNativePtr);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (mNativePtr != 0) {
                nativeDestroy(mNativePtr);
                mNativePtr = 0;
            }
        } finally {
            super.finalize();
        }
    }

    // /////////////////////////////////////////// Native Method. //////////////////////////////////////////////////
    static {
        System.loadLibrary("giftool");
    }

    private static native GifDecoder nativeDecodeFile(String filePath);

    private static native GifDecoder nativeDecodeStream(InputStream stream, byte[] tempStorage);

    private static native GifDecoder nativeDecodeByteArray(byte[] data, int offset, int length);

    private static native GifDecoder nativeDecodeByteBuffer(ByteBuffer buffer, int position, int remaining);

    private static native long nativeGetFrame(long decoder, int frameNr, Bitmap output, int previousFrameNr, int inSampleSize);

    private static native void nativeDestroy(long nativePtr);
}
