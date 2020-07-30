package com.hash.study.giflib.extension;

import android.graphics.Bitmap;
import android.util.Log;

import androidx.annotation.NonNull;

import com.bumptech.glide.load.Options;
import com.bumptech.glide.load.ResourceDecoder;
import com.bumptech.glide.load.engine.bitmap_recycle.BitmapPool;
import com.bumptech.glide.load.resource.drawable.DrawableResource;
import com.hash.study.gif.BuildConfig;
import com.hash.study.gif.FrameSequenceDrawable;
import com.hash.study.gif.GifDecoder;


import java.io.IOException;
import java.io.InputStream;

/**
 * 定义 GIF Decoder.
 */
public class GifResourceDecoder implements ResourceDecoder<InputStream, FrameSequenceDrawable> {

    private static final String TAG = GifResourceDecoder.class.getSimpleName();
    private final FrameSequenceDrawable.BitmapProvider mProvider;

    GifResourceDecoder(final BitmapPool bitmapPool) {
        this.mProvider = new FrameSequenceDrawable.BitmapProvider() {
            @Override
            public Bitmap acquireBitmap(int minWidth, int minHeight) {
                return bitmapPool.getDirty(minWidth, minHeight, Bitmap.Config.ARGB_8888);
            }

            @Override
            public void releaseBitmap(Bitmap bitmap) {
                bitmapPool.put(bitmap);
            }
        };
    }

    @Override
    public boolean handles(@NonNull InputStream source, @NonNull Options options) throws IOException {
        return true;
    }

    /**
     * 将 GIF 的 InputStream 转为 GifDrawableResource
     */
    @Override
    public GifDrawableResource decode(@NonNull InputStream source, int width, int height, @NonNull Options options) throws IOException {
        GifDecoder decoder = GifDecoder.decodeStream(source);
        if (decoder == null) {
            return null;
        }
        int inSampleSize = calcSampleSize(decoder.getWidth(), decoder.getHeight(), width, height);
        FrameSequenceDrawable drawable = new FrameSequenceDrawable(decoder, mProvider, inSampleSize);
        // 圆形Gif显示开关
        drawable.setCircleMaskEnabled(false);
        return new GifDrawableResource(drawable);
    }

    private int calcSampleSize(int sourceWidth, int sourceHeight, int requestedWidth, int requestedHeight) {
        int exactSampleSize = Math.min(sourceWidth / requestedWidth,
                sourceHeight / requestedHeight);
        int powerOfTwoSampleSize = exactSampleSize == 0 ? 0 : Integer.highestOneBit(exactSampleSize);
        // Although functionally equivalent to 0 for BitmapFactory, 1 is a safer default for our code
        // than 0.
        int sampleSize = Math.max(1, powerOfTwoSampleSize);
        if (BuildConfig.DEBUG) {
            Log.i(TAG, "Downsampling GIF"
                    + ", sampleSize: " + sampleSize
                    + ", target dimens: [" + requestedWidth + "x" + requestedHeight + "]"
                    + ", actual dimens: [" + sourceWidth + "x" + sourceHeight + "]"
            );
        }
        return sampleSize;
    }

    /**
     * 创建一个用于加载 GIF 的 Glide 的 Resource
     */
    private static class GifDrawableResource extends DrawableResource<FrameSequenceDrawable> {

        private GifDrawableResource(FrameSequenceDrawable drawable) {
            super(drawable);
        }

        @NonNull
        @Override
        public Class<FrameSequenceDrawable> getResourceClass() {
            return FrameSequenceDrawable.class;
        }

        @Override
        public int getSize() {
            return 0;
        }

        @Override
        public void recycle() {
            drawable.stop();
            drawable.destroy();
        }
    }

}
