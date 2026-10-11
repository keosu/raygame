package org.raygame;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import java.lang.ref.WeakReference;
import java.util.Locale;

// JNI polls state/error; binder and UI callbacks never call into destroyed native objects.
public final class SpeechBridge implements Application.ActivityLifecycleCallbacks {
    private final Handler main = new Handler(Looper.getMainLooper());
    private final WeakReference<Activity> owner;
    private final Application application;
    private TextToSpeech engine;
    private volatile int state = 1; // Matches SpeechState: initializing=1, ready=2, speaking=3.
    private volatile String error = "";
    private volatile boolean closed;
    private boolean paused;
    private int generation;

    public SpeechBridge(Activity activity) {
        owner = new WeakReference<>(activity);
        application = activity.getApplication();
        final Context context = activity.getApplicationContext();
        main.post(() -> {
            if (closed) return;
            application.registerActivityLifecycleCallbacks(this);
            try {
                engine = new TextToSpeech(context, result -> main.post(() -> initialized(result)));
            } catch (RuntimeException e) {
                fail("Cannot initialize Android TTS: " + e.getMessage(), true);
            }
        });
    }

    private synchronized void initialized(int result) {
        if (closed) return;
        if (result != TextToSpeech.SUCCESS || engine == null) {
            fail("Android TTS engine is unavailable", true);
            return;
        }
        int listener = engine.setOnUtteranceProgressListener(new UtteranceProgressListener() {
            @Override public void onStart(String id) { }
            @Override public void onDone(String id) { complete(id, ""); }
            @Override public void onError(String id) { complete(id, "Android speech synthesis failed"); }
            @Override public void onError(String id, int code) {
                complete(id, "Android speech synthesis failed: " + code);
            }
            @Override public void onStop(String id, boolean interrupted) { complete(id, ""); }
        });
        if (listener != TextToSpeech.SUCCESS) fail("Cannot observe Android speech progress", true);
        else state = 2;
    }

    private synchronized void complete(String id, String message) {
        if (closed || !Integer.toString(generation).equals(id)) return;
        error = message;
        state = 2;
    }

    private synchronized void fail(String message, boolean unavailable) {
        error = message;
        state = unavailable ? 4 : 2;
    }

    public int getState() { return state; }
    public String getError() { return error; }

    public synchronized boolean speak(String text, String language, float rate, float volume) {
        if (closed || paused || (state != 2 && state != 3)) {
            error = "Android speech is not ready or activity is paused";
            return false;
        }
        final int ticket = ++generation;
        error = "";
        state = 3;
        main.post(() -> {
            synchronized (SpeechBridge.this) {
                if (closed || ticket != generation) return;
                try {
                    Locale locale = language.isEmpty() ? Locale.getDefault() : Locale.forLanguageTag(language);
                    int supported = engine.setLanguage(locale);
                    if (supported == TextToSpeech.LANG_MISSING_DATA || supported == TextToSpeech.LANG_NOT_SUPPORTED) {
                        engine.stop();
                        fail("No installed Android voice data for " + locale.toLanguageTag(), false);
                        return;
                    }
                    Bundle parameters = new Bundle();
                    parameters.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, volume);
                    if (engine.setSpeechRate(rate) != TextToSpeech.SUCCESS ||
                        engine.speak(text, TextToSpeech.QUEUE_FLUSH, parameters, Integer.toString(ticket)) != TextToSpeech.SUCCESS)
                        fail("Android TTS rejected the utterance", false);
                } catch (RuntimeException e) {
                    fail("Android TTS: " + e.getMessage(), false);
                }
            }
        });
        return true;
    }

    public synchronized void stop() {
        final int ticket = ++generation;
        if (state == 3) state = 2;
        main.post(() -> {
            synchronized (SpeechBridge.this) {
                if (!closed && ticket == generation && engine != null) {
                    try { engine.stop(); }
                    catch (RuntimeException e) { fail("Cannot stop Android TTS: " + e.getMessage(), true); }
                }
            }
        });
    }

    public synchronized void close() {
        if (closed) return;
        closed = true;
        ++generation;
        state = 0;
        main.post(() -> {
            application.unregisterActivityLifecycleCallbacks(this);
            if (engine != null) {
                try { engine.shutdown(); }
                catch (RuntimeException e) { error = "Cannot close Android TTS: " + e.getMessage(); }
                finally { engine = null; }
            }
        });
    }

    @Override public synchronized void onActivityPaused(Activity activity) {
        if (activity == owner.get()) { paused = true; stop(); }
    }
    @Override public synchronized void onActivityResumed(Activity activity) {
        if (activity == owner.get()) paused = false;
    }
    @Override public void onActivityDestroyed(Activity activity) {
        if (activity == owner.get()) close();
    }
    @Override public void onActivityCreated(Activity activity, Bundle data) { }
    @Override public void onActivityStarted(Activity activity) { }
    @Override public void onActivityStopped(Activity activity) { }
    @Override public void onActivitySaveInstanceState(Activity activity, Bundle data) { }
}
