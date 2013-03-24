package com.hoperun.media.proxy;

import android.util.Log;

public class MediaProxy
{
    private static final String TAG = "LibActivate";
    
    private static MediaProxy sInstance;
    
    static
    {
        try
        {
            System.loadLibrary("hlsjni");
        }
        catch (Exception e)
        {
            Log.e(TAG, "Can't load hlsjni library: " + e); // FIXME Alert
                                                           // user
            System.exit(1);
        }
        
    }
    
    /**
     * Singleton constructor Without surface and vout to create the thumbnail
     * and get information e.g. on the MediaLibraryAcitvity
     * 
     * @return
     * @throws LibVlcException
     */
    public synchronized static MediaProxy getInstance()
    {
        if (sInstance == null)
        {
            /* First call */
            sInstance = new MediaProxy();
            sInstance.init();
        }
        return sInstance;
    }
    
    /**
     * Constructor It is private because this class is a singleton.
     */
    private MediaProxy()
    {
        Log.v(TAG, "MediaProxy()...... ");
    }
    
    private void init()
    {
        setEventManager(EventManager.getInstance());
        hlsProxyInit();
    }
    
    /**
     * Destroy this libVLC instance
     * 
     * @note You must call it before exiting
     */
    public void unInit()
    {
        Log.v(TAG, "Destroying LibVLC instance");
        //        nativeDestroy();
        hlsProxyUninit();
        detachEventManager();
        //        mIsInitialized = false;
    }
    
    public void request(byte[] url)
    {
        hlsProxyPrepare(url);
    }
    
    // native method
    private static native void hlsProxyInit();
    
    private static native void hlsProxyUninit();
    
    private static native void hlsProxyPrepare(byte[] url);
    
    private native void setEventManager(EventManager eventManager);
    
    private native void detachEventManager();
}
