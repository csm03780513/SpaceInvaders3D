package com.csm.spaceinvaders3d;

import android.app.NativeActivity;
import android.media.AudioManager;
import android.os.Bundle;

public class MyGameActivity extends NativeActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
    }
}
