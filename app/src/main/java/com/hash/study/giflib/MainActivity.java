package com.hash.study.giflib;

import androidx.appcompat.app.AppCompatActivity;

import android.app.PendingIntent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;

import com.bumptech.glide.gifdecoder.StandardGifDecoder;
import com.bumptech.glide.load.resource.gif.StreamGifDecoder;
import com.hash.study.giflib.extension.GlideApp;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final ImageView display = findViewById(R.id.display);
        findViewById(R.id.showGif).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //使用giflib + framesequenceDrawable 加载
                GlideApp.with(MainActivity.this)
                        .asGif2()
                        .load(R.drawable.a11)
                        .into(display);
//                GlideApp.with(MainActivity.this)
//                        .asGif2()
//                        .load(R.drawable.app_gif_shared_element)
//                        .into(display);


            }
        });

    }
}
