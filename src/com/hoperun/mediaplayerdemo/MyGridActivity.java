package com.hoperun.mediaplayerdemo;

import android.app.Activity;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.os.Bundle;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.VideoView;
import com.hoperun.media.proxy.EventManager;
import com.hoperun.media.proxy.MediaProxy;
import com.hoperun.media.proxy.WeakHandler;

public class MyGridActivity extends Activity implements OnClickListener
{
    
    private static final String TAG = "MyGridActivity";
    
    private Button btn1, btn2, btn3, btn4, btn5, btn6, btn7, btn8, btn9, btn10,
            btn11, btn12;
    
    // private EditText urlinput;
    // SurfaceHolder holder;
    // MediaPlayer mp;
    VideoView videoView;
    
    //    static {
    //        System.loadLibrary("hlsjni");
    //    }
    // ------------------------------------------------------------------------------------------------------------------
    //    private static Thread mHLSThread;
    
    //    // native method
    //    public static native void hlsProxyInit(byte[] url);
    //
    //    public static native void hlsProxyUninit();
    //
    //    public static native void hlsProxyPrepare(byte[] url);
    //
    //    private native void setEventManager(EventManager eventManager);
    //
    //    private native void detachEventManager();
    
    // 播放列表
    // sohu
    static byte[] guangdong = "http://gslb.tv.sohu.com/live?cid=60&type=hls".getBytes();
    
    // static byte[] jiangsu=
    // "http://gslb.tv.sohu.com/live?cid=51&type=hls".getBytes();
    static byte[] jiangsu = "http://zb.v.qq.com:1863/?progid=2674956498&ostype=ios".getBytes(); // QQTV
    
    static byte[] dongfang = "http://gslb.tv.sohu.com/live?cid=3&type=hls".getBytes();
    
    static byte[] zhejiang = "http://gslb.tv.sohu.com/live?cid=31&type=hls".getBytes();
    
    static byte[] bejing = "http://gslb.tv.sohu.com/live?cid=36&type=hls".getBytes();
    
    static byte[] liaoning = "http://gslb.tv.sohu.com/live?cid=10&type=hls".getBytes();
    
    static byte[] anhui = "http://gslb.tv.sohu.com/live?cid=13&type=hls".getBytes();
    
    static byte[] dongnan = "http://gslb.tv.sohu.com/live?cid=7&type=hls".getBytes();
    
    // PPTV
    static byte[] PPTV_beijing = "http://web-play.pptv.com/web-m3u8-300166.m3u8".getBytes();
    
    static byte[] PPTV_shanxi = "http://web-play.pptv.com/web-m3u8-300167.m3u8".getBytes();
    
    static byte[] PPTV_jiangxi = "http://web-play.pptv.com/web-m3u8-300168.m3u8".getBytes();
    
    static byte[] PPTV_xiamen = "http://web-play.pptv.com/web-m3u8-300169.m3u8".getBytes();
    
    // ------------------------------------------------------------------------------------------------------------------
    
    // public static void startHLSProxy() {
    // // Start up the C app thread
    // if (mHLSThread == null) {
    // mHLSThread = new Thread(new mHLSProxy(), "HLSThread");
    // mHLSThread.start();
    // }
    // }
    
    // /**
    // * Simple nativeInit() runnable
    // */
    // static class mHLSProxy implements Runnable {
    // public void run() {
    // MyGridActivity.hlsProxyInit(guangdong);
    // Log.d(TAG,
    // "----------------------------------------------------------------------------------------------");
    // }
    // }
    private static MediaProxy mediaProxy;
    
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        
        //        setEventManager(EventManager.getInstance());
        //
        //        MyGridActivity.hlsProxyInit(guangdong);
        EventManager em = EventManager.getInstance();
        em.addHandler(eventHandler);
        try
        {
            mediaProxy = MediaProxy.getInstance();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            //            mHandler.sendEmptyMessage(ConstantValue.INIT_VLCLIB_FAILED);
        }
        
        setContentView(R.layout.main04);
        btn1 = (Button) findViewById(R.id.pause1_Button);
        btn2 = (Button) findViewById(R.id.pause2_Button);
        btn3 = (Button) findViewById(R.id.pause3_Button);
        btn4 = (Button) findViewById(R.id.pause4_Button);
        btn5 = (Button) findViewById(R.id.pause5_Button);
        btn6 = (Button) findViewById(R.id.pause6_Button);
        btn7 = (Button) findViewById(R.id.pause7_Button);
        btn8 = (Button) findViewById(R.id.pause8_Button);
        btn9 = (Button) findViewById(R.id.pause9_Button);
        btn10 = (Button) findViewById(R.id.pause10_Button);
        btn11 = (Button) findViewById(R.id.pause11_Button);
        btn12 = (Button) findViewById(R.id.pause12_Button);
        
        videoView = (VideoView) findViewById(R.id.mysurfaceView1);
        videoView.setOnPreparedListener(onPreparedListener);
        
        btn1.setOnClickListener(this);
        btn2.setOnClickListener(this);
        btn3.setOnClickListener(this);
        btn4.setOnClickListener(this);
        btn5.setOnClickListener(this);
        btn6.setOnClickListener(this);
        btn7.setOnClickListener(this);
        btn8.setOnClickListener(this);
        btn9.setOnClickListener(this);
        btn10.setOnClickListener(this);
        btn11.setOnClickListener(this);
        btn12.setOnClickListener(this);
    }
    
    private OnPreparedListener onPreparedListener = new OnPreparedListener()
    {
        @Override
        public void onPrepared(MediaPlayer mp)
        {
            videoView.start();
        }
    };
    
    @Override
    public void onClick(View v)
    {
        // TODO Auto-generated method stub
        if (v.getId() == R.id.pause1_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(guangdong);
            }
        }
        else if (v.getId() == R.id.pause2_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(jiangsu);
            }
        }
        else if (v.getId() == R.id.pause3_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(dongfang);
            }
        }
        else if (v.getId() == R.id.pause4_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(zhejiang);
            }
        }
        else if (v.getId() == R.id.pause5_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(bejing);
            }
        }
        else if (v.getId() == R.id.pause6_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(liaoning);
            }
        }
        else if (v.getId() == R.id.pause7_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(anhui);
            }
        }
        else if (v.getId() == R.id.pause8_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(dongnan);
            }
        }
        else if (v.getId() == R.id.pause9_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(PPTV_beijing);
            }
        }
        else if (v.getId() == R.id.pause10_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(PPTV_shanxi);
            }
        }
        else if (v.getId() == R.id.pause11_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(PPTV_jiangxi);
            }
        }
        else if (v.getId() == R.id.pause12_Button)
        {
            if (videoView.isPlaying())
            {
                mediaProxy.request(PPTV_xiamen);
            }
        }
        
    }
    
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        mediaProxy.unInit();
        //        hlsProxyUninit();
        //        detachEventManager();
        EventManager em = EventManager.getInstance();
        em.removeHandler(eventHandler);
    }
    
    /**
     * Handle libvlc asynchronous events
     */
    private final VideoPlayerEventHandler eventHandler = new VideoPlayerEventHandler(
            this);
    
    private static class VideoPlayerEventHandler extends
            WeakHandler<MyGridActivity>
    {
        public VideoPlayerEventHandler(MyGridActivity owner)
        {
            super(owner);
        }
        
        @Override
        public void handleMessage(Message msg)
        {
            MyGridActivity activity = getOwner();
            if (activity == null)
                return;
            // int event = msg.getData().getInt("event");
            int event = msg.arg1;
            log("VideoPlayerEventHandler---->eventHandler=" + event);
            switch (event)
            {
                case EventManager.MediaPlayerInit:
                    log("eventHandler------>MediaPlayerInit------------");
                    mediaProxy.request(guangdong);
                    break;
                case EventManager.MediaPlayerBuffering:
                    log("eventHandler------>MediaPlayerBuffering------------");
                    // add for information of buffer Percent 达到100%时，播放加载完成
                    float bufPercent = msg.getData().getFloat("item_float");
                    log("eventHandler------>MediaPlayerBuffering------------"
                            + bufPercent + "%");
                    
                    // if (activity.isFristBuffer && bufPercent == 100) {
                    // activity.mediaPlayerPrepared();
                    // activity.isFristBuffer = false;
                    // }
                    break;
                case EventManager.MediaPlayerPlaying:
                    log("eventHandler------>MediaPlayerPlaying");
                    // activity.mediaPlayerPrepared();
                    String pathUri = "http://localhost:8080/livetv.ts";
                    Log.d(TAG, "pathUri=" + pathUri);
                    if (!TextUtils.isEmpty(pathUri))
                    {
                        if (activity.videoView.isPlaying())
                            activity.videoView.stopPlayback();
                        activity.videoView.setVideoPath(pathUri);
                    }
                    break;
                case EventManager.MediaPlayerPaused:
                    log("eventHandler------>MediaPlayerPaused");
                    break;
                case EventManager.MediaPlayerStopped:
                    log("eventHandler------>MediaPlayerStopped");
                    // activity.removeStopLoad();
                    break;
                case EventManager.MediaPlayerEndReached:
                    log("eventHandler------>MediaPlayerEndReached");
                    // activity.mediaPlayerEndReached();
                    // activity.endReached();
                    break;
                case EventManager.MediaPlayerVout:
                    // vlc音频与视频切换发送这个消息
                    // activity.handleVout(msg);
                    break;
                // add item for "INPUT_EVENT_DEAD" defined in vlc_input.h
                // 20121110
                case EventManager.MediaPlayerDead:
                    // 播放异常终止Error
                    // activity.mediaPlayerError();
                    break;
                // add item end 20121110
                default:
                    log("eventHandler------>"
                            + String.format("Event not handled (0x%x)",
                                    msg.getData().getInt("event")));
                    break;
            }
        }
    };
    
    private static void log(String msg)
    {
        Log.d(TAG, msg);
    }
    
}
