/**
 * Created by Hash on 2020/7/30.
 * http://androidxref.com/9.0.0_r3/xref/frameworks/ex/framesequence/jni/Color.h#24
 */

#pragma

typedef uint32_t Color8888;

static const Color8888 COLOR_8888_ALPHA_MASK = 0xff000000; // TODO: handle endianness
static const Color8888 TRANSPARENT = 0x0;// TODO: handle endianness
#define ARGB_TO_COLOR8888(a, r, g, b) \
   ((a) << 24 | (b) << 16 | (g) << 8 | (r))
