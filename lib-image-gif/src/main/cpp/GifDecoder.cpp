//
//参考 Google
//http://androidxref.com/9.0.0_r3/xref/frameworks/ex/framesequence/jni/FrameSequence_gif.cpp
//http://androidxref.com/9.0.0_r3/xref/frameworks/ex/framesequence/jni/FrameSequence_webp.cpp
//

#include <malloc.h>
#include <string.h>
#include <android/bitmap.h>
#include "GifDecoder.h"
#include "utils/math.h"
#include "utils/log.h"


////////////////////////////////////////////////////////////////////////////////
// draw helpers
////////////////////////////////////////////////////////////////////////////////

static long getDelayMs(GraphicsControlBlock &gcb) {
    return gcb.DelayTime * 10;
}

static Color8888 gifColorToColor8888(const GifColorType &color) {
    return ARGB_TO_COLOR8888(0xff, color.Red, color.Green, color.Blue);
}

static bool willBeCleared(const GraphicsControlBlock &gcb) {
    return gcb.DisposalMode == DISPOSE_BACKGROUND || gcb.DisposalMode == DISPOSE_PREVIOUS;
}

// return true if area of 'target' is completely covers area of 'covered'
static bool checkIfCover(const GifImageDesc &target, const GifImageDesc &covered) {
    return target.Left <= covered.Left
           && covered.Left + covered.Width <= target.Left + target.Width
           && target.Top <= covered.Top
           && covered.Top + covered.Height <= target.Top + target.Height;
}

static void
copyLine(Color8888 *dst, const unsigned char *src, const ColorMapObject *cmap, int transparent,
         int width, int inSampleSize) {
    for (; width > 0; width--, src += inSampleSize, dst++) {
        if (*src != transparent && *src < cmap->ColorCount) {
            *dst = gifColorToColor8888(cmap->Colors[*src]);
        }
    }
}

static void setLineColor(Color8888 *dst, Color8888 color, int width) {
    for (; width > 0; width--, dst++) {
        *dst = color;
    }
}

static void getCopySize(const GifImageDesc &imageDesc, int maxWidth, int maxHeight,
                        GifWord &copyWidth, GifWord &copyHeight) {
    copyWidth = imageDesc.Width;
    if (imageDesc.Left + copyWidth > maxWidth) {
        copyWidth = maxWidth - imageDesc.Left;
    }
    copyHeight = imageDesc.Height;
    if (imageDesc.Top + copyHeight > maxHeight) {
        copyHeight = maxHeight - imageDesc.Top;
    }
}

static int streamReader(GifFileType *fileType, GifByteType *out, int size) {
    Stream *stream = (Stream *) fileType->UserData;
    return (int) stream->read(out, size);
}



GifDecoder::GifDecoder(char *filePath) {
    mGif = DGifOpenFileName(filePath, NULL);
    init();
}

GifDecoder::GifDecoder(Stream *stream) {
    mGif = DGifOpen(stream, streamReader, NULL);
    init();
}

void GifDecoder::init() {
    if (!mGif) {
        ALOGW("Gif load failed");
        DGifCloseFile(mGif, NULL);
        return;
    }
    if (DGifSlurp(mGif) != GIF_OK) {
        ALOGW("Gif slurp failed");
        DGifCloseFile(mGif, NULL);
        mGif = NULL;
        return;
    }

    int lastUnclearedFrame = -1;
    mPreservedFrames = new bool[mGif->ImageCount];
    mRestoringFrames = new int[mGif->ImageCount];

    GraphicsControlBlock gcb;
    for (int i = 0; i < mGif->ImageCount; i++) {
        const SavedImage &image = mGif->SavedImages[i];

        // find the loop extension pair
        for (int j = 0; (j + 1) < image.ExtensionBlockCount; j++) {
            ExtensionBlock *eb1 = image.ExtensionBlocks + j;
            ExtensionBlock *eb2 = image.ExtensionBlocks + j + 1;
            if (eb1->Function == APPLICATION_EXT_FUNC_CODE
                // look for "NETSCAPE2.0" app extension
                && eb1->ByteCount == 11
                && !memcmp((const char *) (eb1->Bytes), "NETSCAPE2.0", 11)
                // verify extension contents and get loop count
                && eb2->Function == CONTINUE_EXT_FUNC_CODE
                && eb2->ByteCount == 3
                && eb2->Bytes[0] == 1) {
                mLoopCount = (int) (eb2->Bytes[2] << 8) + (int) (eb2->Bytes[1]);
            }
        }

        DGifSavedExtensionToGCB(mGif, i, &gcb);

        // timing
        mDurationMs += getDelayMs(gcb);

        // preserve logic
        mPreservedFrames[i] = false;
        mRestoringFrames[i] = -1;
        if (gcb.DisposalMode == DISPOSE_PREVIOUS && lastUnclearedFrame >= 0) {
            mPreservedFrames[lastUnclearedFrame] = true;
            mRestoringFrames[i] = lastUnclearedFrame;
        }
        if (!willBeCleared(gcb)) {
            lastUnclearedFrame = i;
        }
    }

#if GIF_DEBUG
    ALOGI("GifDecoder created with size [%d, %d], frames is %d, duration is %ld",
          mGif->SWidth, mGif->SHeight, mGif->ImageCount, mDurationMs);
    for (int i = 0; i < mGif->ImageCount; i++) {
        DGifSavedExtensionToGCB(mGif, i, &gcb);
        LOGE("Frame %d - must preserve %d, restore point %d, trans color %d",
             i, mPreservedFrames[i], mRestoringFrames[i], gcb.TransparentColor);
    }
#endif

    // 解析 GIF 的背景色
    const ColorMapObject *cmap = mGif->SColorMap;
    if (cmap) {
        // calculate bg color
        GraphicsControlBlock gcb;
        DGifSavedExtensionToGCB(mGif, 0, &gcb);
        if (gcb.TransparentColor == NO_TRANSPARENT_COLOR
            && mGif->SBackGroundColor < cmap->ColorCount) {
            // 获取 GIF 的背景颜色
            mBgColor = gifColorToColor8888(cmap->Colors[mGif->SBackGroundColor]);
        }
    }

    // mark init success
    mHasInit = true;
}

GifDecoder::~GifDecoder() {
    if (mGif) {
        DGifCloseFile(mGif, NULL);
    }
    delete[] mPreservedFrames;
    delete[] mRestoringFrames;
    ALOGE("GifDecoder release.");
}

long
GifDecoder::drawFrame(int frameNr, Color8888 *outputPtr, int outputPixelStride, int previousFrameNr,
                      int inSampleSize) {
    if (!mHasInit) {
        return -1;
    }

    GifFileType *gif = mGif;
#if GIF_DEBUG
    ALOGD("      drawFrame on %p nr %d on addr %p, previous frame nr %d",
          this, frameNr, outputPtr, previousFrameNr);
#endif

    const int requestedWidth = mGif->SWidth / inSampleSize;
    const int requestedHeight = mGif->SHeight / inSampleSize;

    GraphicsControlBlock gcb;

    int start = max(previousFrameNr + 1, 0);

    for (int i = max(start - 1, 0); i < frameNr; i++) {
        int neededPreservedFrame = getRestoringFrame(i);
        if (neededPreservedFrame >= 0 &&
            (mPreserveBufferFrame != neededPreservedFrame)) {
#if GIF_DEBUG
            ALOGD("frame %d needs frame %d preserved, but %d is currently, so drawing from scratch",
                    i, neededPreservedFrame, mPreserveBufferFrame);
#endif
            start = 0;
        }
    }

    for (int i = start; i <= frameNr; i++) {
        DGifSavedExtensionToGCB(gif, i, &gcb);
        const SavedImage &frame = gif->SavedImages[i];

#if GIF_DEBUG
        bool frameOpaque = gcb.TransparentColor == NO_TRANSPARENT_COLOR;
        ALOGD("producing frame %d, drawing frame %d (opaque %d, disp %d, del %d)",
                frameNr, i, frameOpaque, gcb.DisposalMode, gcb.DelayTime);
#endif
        if (i == 0) {
            // clear bitmap
            Color8888 bgColor = mBgColor;
            for (int y = 0; y < requestedHeight; y++) {
                for (int x = 0; x < requestedWidth; x++) {
                    outputPtr[y * outputPixelStride + x] = bgColor;
                }
            }
        } else {
            GraphicsControlBlock prevGcb;
            DGifSavedExtensionToGCB(gif, i - 1, &prevGcb);
            const SavedImage &prevFrame = gif->SavedImages[i - 1];
            bool prevFrameDisposed = willBeCleared(prevGcb);

            bool newFrameOpaque = gcb.TransparentColor == NO_TRANSPARENT_COLOR;
            bool prevFrameCompletelyCovered = newFrameOpaque
                                              && checkIfCover(frame.ImageDesc,
                                                              prevFrame.ImageDesc);

            if (prevFrameDisposed && !prevFrameCompletelyCovered) {
                switch (prevGcb.DisposalMode) {
                    case DISPOSE_BACKGROUND: {
                        // 填充背景色
                        Color8888 *dst = outputPtr + (prevFrame.ImageDesc.Left / inSampleSize) +
                                         (prevFrame.ImageDesc.Top / inSampleSize) *
                                         outputPixelStride;
                        GifWord copyWidth, copyHeight;
                        getCopySize(prevFrame.ImageDesc, requestedWidth, requestedHeight, copyWidth,
                                    copyHeight);
                        for (; copyHeight > 0; copyHeight--) {
                            setLineColor(dst, TRANSPARENT, copyWidth);
                            dst += outputPixelStride;
                        }
                        break;
                    }
                    case DISPOSE_PREVIOUS: {
                        // 从上一帧中恢复数据
                        restorePreserveBuffer(outputPtr, outputPixelStride, inSampleSize);
                        break;
                    }
                }
            }

            if (getPreservedFrame(i - 1)) {
                // 保存上一帧的数据
                savePreserveBuffer(outputPtr, outputPixelStride, i - 1, inSampleSize);
            }
        }

        bool willBeCleared = gcb.DisposalMode == DISPOSE_BACKGROUND
                             || gcb.DisposalMode == DISPOSE_PREVIOUS;
        if (i == frameNr || !willBeCleared) {
            // 使用全局色表为默认色表
            const ColorMapObject *cmap = gif->SColorMap;
            // 若存在局部色表, 则使用局部色表
            if (frame.ImageDesc.ColorMap) {
                cmap = frame.ImageDesc.ColorMap;
            }
            if (cmap) {
                // 填充当前帧的颜色
                const unsigned char *src = frame.RasterBits;
                Color8888 *dst = outputPtr + (frame.ImageDesc.Left / inSampleSize) +
                                 (frame.ImageDesc.Top / inSampleSize) * outputPixelStride;
                GifWord copyWidth, copyHeight;
                getCopySize(frame.ImageDesc, requestedWidth, requestedHeight, copyWidth,
                            copyHeight);
                for (; copyHeight > 0; copyHeight--) {
                    copyLine(dst, src, cmap, gcb.TransparentColor, copyWidth, inSampleSize);
                    src += frame.ImageDesc.Width * inSampleSize;
                    dst += outputPixelStride;
                }
            } else {
                ALOGI("Color map not available, ignore this frame %d", frameNr);
            }
        }
    }
    // return last frame's delay
    const int maxFrame = gif->ImageCount;
    const int lastFrame = (frameNr + maxFrame - 1) % maxFrame;
    DGifSavedExtensionToGCB(gif, lastFrame, &gcb);
    return getDelayMs(gcb);
}

void
GifDecoder::restorePreserveBuffer(Color8888 *outputPtr, int outputPixelStride, int inSampleSize) {
    // 判断是否可以从上一帧中获取数据
    if (!mPreserveBuffer || inSampleSize != mPreserveSampleSize) {
        ALOGI("preserve buffer not available.");
        return;
    }
    // 从上一帧的 Buffer 中拷贝数据
    const int requestWidth = mGif->SWidth / inSampleSize;
    const int requestHeight = mGif->SHeight / inSampleSize;
    for (int y = 0; y < requestHeight; y++) {
        memcpy(outputPtr + outputPixelStride * y, mPreserveBuffer + requestWidth * y,
               requestWidth * 4);
    }
}

void
GifDecoder::savePreserveBuffer(Color8888 *outputPtr, int outputPixelStride, int frameNr,
                               int inSampleSize) {
    if (frameNr == mPreserveBufferFrame) {
        return;
    }
    mPreserveBufferFrame = frameNr;
    mPreserveSampleSize = inSampleSize;
    const int width = mGif->SWidth / inSampleSize;
    const int height = mGif->SHeight / inSampleSize;
    if (!mPreserveBuffer) {
        mPreserveBuffer = new Color8888[width * height];
    }
    for (int y = 0; y < height; y++) {
        memcpy(mPreserveBuffer + width * y, outputPtr + outputPixelStride * y, width * 4);
    }
}

////////////////////////////////////////////////////////////////////////////////
// JNILoader
////////////////////////////////////////////////////////////////////////////////

static jobject createJavaGifDecoder(JNIEnv *env, jclass jclazz, GifDecoder *decoder) {
    if (!decoder || !decoder->hasInit()) {
        ALOGE("Gif parsed failed. Please check input source and try again.");
        return NULL;
    }
    // Create Java method.<init>是每个对象创建走的第一个方法。
    jmethodID jCtr = env->GetMethodID(jclazz, "<init>", "(JIIZIIJ)V");
    // 在C++层构建Java层的GifDecoder
    return env->NewObject(
            jclazz, jCtr,
            reinterpret_cast<jlong>(decoder),
            decoder->getWidth(),
            decoder->getHeight(),
            decoder->isOpaque(),
            decoder->getFrameCount(),
            decoder->getLooperCount(),
            static_cast<jlong>(decoder->getDuration())
    );
}

namespace gifdecoder {

    jobject _nativeDecodeFile(JNIEnv *env, jclass jclazz, jstring file_path) {
        char *filePath = const_cast<char *>(env->GetStringUTFChars(file_path, NULL));
        GifDecoder *decoder = new GifDecoder(filePath);
        env->ReleaseStringUTFChars(file_path, filePath);
        return createJavaGifDecoder(env, jclazz, decoder);
    }

    jobject _nativeDecodeStream(JNIEnv *env, jclass jclazz, jobject istream,
                               jbyteArray byteArray) {
        JavaInputStream stream(env, istream, byteArray);
        GifDecoder *decoder = new GifDecoder(&stream);
        return createJavaGifDecoder(env, jclazz, decoder);
    }

    jobject _nativeDecodeByteArray(JNIEnv *env, jclass jclazz,
                                  jbyteArray byteArray,
                                  jint offset, jint length) {
        jbyte *bytes = reinterpret_cast<jbyte *>(env->GetPrimitiveArrayCritical(byteArray, NULL));
        if (bytes == NULL) {
            ALOGE("couldn't read array bytes");
            return NULL;
        }
        MemoryStream stream(bytes + offset, length, NULL);
        GifDecoder *decoder = new GifDecoder(&stream);
        env->ReleasePrimitiveArrayCritical(byteArray, bytes, 0);
        return createJavaGifDecoder(env, jclazz, decoder);
    }

    jobject _nativeDecodeByteBuffer(JNIEnv *env, jclass jclazz, jobject buf,
                                   jint offset, jint limit) {
        jobject globalBuf = env->NewGlobalRef(buf);
        JavaVM *vm;
        env->GetJavaVM(&vm);
        MemoryStream stream(
                (reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(globalBuf))) + offset,
                limit,
                globalBuf);
        GifDecoder *decoder = new GifDecoder(&stream);
        //创建GifDecoder
        return createJavaGifDecoder(env, jclazz, decoder);
    }

    jlong _nativeGetFrame(JNIEnv *env, jobject, jlong handle,
                         jint frameNr, jobject bitmap, jint prevFrameNr, jint inSampleSize) {
        GifDecoder *decoder = reinterpret_cast<GifDecoder *>(handle);
        AndroidBitmapInfo info;
        void *pixels;
        AndroidBitmap_getInfo(env, bitmap, &info);
        AndroidBitmap_lockPixels(env, bitmap, &pixels);
        // 获取一行的像素数数量  每行字节数/4  = 一行的像素的个数乘以4
        // （rgba）像素由rgba四个分量组成，像素数量是等于每一行的字节数除以4 因为像素有四个分量每个分量占用一个字节
        int pixelStride = info.stride >> 2;
        jlong delayMs = decoder->drawFrame(frameNr, (Color8888 *) pixels, pixelStride,
                                           prevFrameNr, inSampleSize);
        AndroidBitmap_unlockPixels(env, bitmap);
        return delayMs;
    }

    void _nativeDestroy(JNIEnv *, jobject, jlong native_ptr) {
        GifDecoder *decoder = reinterpret_cast<GifDecoder *>(native_ptr);
        delete (decoder);
    }

}

static JNINativeMethod gGifDecoderMethods[] = {
        // 动态注册的方式注册Java层的native方法
        {"nativeDecodeFile",       "(Ljava/lang/String;)Lcom/hash/study/gif/GifDecoder;",      (void *) gifdecoder::_nativeDecodeFile},
        {"nativeDecodeStream",     "(Ljava/io/InputStream;[B)Lcom/hash/study/gif/GifDecoder;", (void *) gifdecoder::_nativeDecodeStream},
        {"nativeDecodeByteArray",  "([BII)Lcom/hash/study/gif/GifDecoder;",                    (void *) gifdecoder::_nativeDecodeByteArray},
        {"nativeDecodeByteBuffer", "(Ljava/nio/ByteBuffer;II)Lcom/hash/study/gif/GifDecoder;", (void *) gifdecoder::_nativeDecodeByteBuffer},
        // other method.
        {"nativeGetFrame",         "(JILandroid/graphics/Bitmap;II)J",                         (void *) gifdecoder::_nativeGetFrame},
        {"nativeDestroy",          "(J)V",                                                     (void *) gifdecoder::_nativeDestroy},
};

// 通过
jint GifDecoder_OnLoad(JNIEnv *env) {
    jclass jclsGifDecoder = env->FindClass("com/hash/study/gif/GifDecoder");
    jclsGifDecoder = reinterpret_cast<jclass>(env->NewGlobalRef(jclsGifDecoder));
    return env->RegisterNatives(
            jclsGifDecoder,
            gGifDecoderMethods,
            sizeof(gGifDecoderMethods) / sizeof(gGifDecoderMethods[0])
    );
}