
#pragma
#include <jni.h>
#include "giflib/gif_lib.h"
#include "Color.h"
#include "stream/Stream.h"

class GifDecoder {

private:
    GifFileType *mGif;
    // array of bool per frame - if true, frame data is used by a later DISPOSE_PREVIOUS frame
    bool *mPreservedFrames = NULL;
    // array of ints per frame - if >= 0, points to the index of the preserve that frame needs
    int *mRestoringFrames = NULL;
    // 缓存 Gif 的背景色
    Color8888 mBgColor = TRANSPARENT;

    // 缓存上一帧的 Bitmap 数据
    Color8888 *mPreserveBuffer = NULL;
    // 缓存上一帧的 SampleSize
    int mPreserveSampleSize = 1;
    // 上一帧的 FrameNumber
    int mPreserveBufferFrame;

    int mLoopCount = 1;
    long mDurationMs = 0l;
    bool mHasInit = false;

public:
    /**
     *
     * @param stream 处理原始GIf流信息
     */
    GifDecoder(Stream *stream);
    /**
     *
     * @param filePath 处理Gif文件
     */
    GifDecoder(char *filePath);

    ~GifDecoder();
    // 是否初始化
    bool hasInit() {
        return mHasInit;
    }
    // gif数据帧的宽度
    int getWidth() {
        return mHasInit ? mGif->SWidth : 0;
    }

    int getHeight() { return mHasInit ? mGif->SHeight : 0; }
    //是否透明
    bool isOpaque() {
        return (mBgColor & COLOR_8888_ALPHA_MASK) == COLOR_8888_ALPHA_MASK;
    }
    // 获取Gif数据帧的个数
    int getFrameCount() { return mHasInit ? mGif->ImageCount : 0; }
    //循环次数
    int getLooperCount() {
        return mLoopCount;
    }

    long getDuration() {
        return mDurationMs;
    }

    long drawFrame(int frameNr, Color8888 *outputPtr, int outputPixelStride, int previousFrameNr,
                   int inSampleSize);

private:
    void init();
    // 获取上一帧数据
    bool getPreservedFrame(int frameIndex) const { return mPreservedFrames[frameIndex]; }

    int getRestoringFrame(int frameIndex) const { return mRestoringFrames[frameIndex]; }

    // 缓存上一帧的数据
    void savePreserveBuffer(Color8888 *outputPtr, int outputPixelStride, int frameNr, int inSampleSize);

    // 从上一帧中恢复数据
    void restorePreserveBuffer(Color8888 *outputPtr, int outputPixelStride, int inSampleSize);

};

jint GifDecoder_OnLoad(JNIEnv *env);

