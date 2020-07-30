# 参看文章

https://juejin.im/post/5f1d4470e51d45348e27c24b

# Feature

支持圆形GIF加载 配置可查看GifResourceDecoder.java
FrameSequenceDrawable.java


```java
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


```

```java
 /**
     * Pass true to mask the shape of the animated drawing content to a circle.
     *
     * <p> The masking circle will be the largest circle contained in the Drawable's bounds.
     * Masking is done with BitmapShader, incurring minimal additional draw cost.
     */
    public final void setCircleMaskEnabled(boolean circleMaskEnabled) {
        if (mCircleMaskEnabled != circleMaskEnabled) {
            mCircleMaskEnabled = circleMaskEnabled;
            // Anti alias only necessary when using circular mask
            mPaint.setAntiAlias(circleMaskEnabled);
            invalidateSelf();
        }
    }


```