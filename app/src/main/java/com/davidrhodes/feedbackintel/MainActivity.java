package com.davidrhodes.feedbackintel;

import android.app.Activity;
import android.content.ContentValues;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class MainActivity extends Activity {
    static { System.loadLibrary("feedbackintel"); }
    private static final String HOME = "http://127.0.0.1:8787/";
    private WebView web;

    private native void nativeStart(String dir);

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        nativeStart(getFilesDir().getAbsolutePath());
        web = new WebView(this);
        setContentView(web);
        WebSettings s = web.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        web.setWebViewClient(new WebViewClient());
        web.setDownloadListener((url, ua, cd, mime, len) -> save(url, mime));
        new Thread(() -> {
            for (int i = 0; i < 40; i++) {
                try {
                    HttpURLConnection c = (HttpURLConnection) new URL(HOME + "files").openConnection();
                    c.setConnectTimeout(500);
                    if (c.getResponseCode() == 200) break;
                } catch (Exception e) {
                    try { Thread.sleep(300); } catch (InterruptedException ie) { }
                }
            }
            runOnUiThread(() -> web.loadUrl(HOME));
        }).start();
    }

    private void save(String url, String mime) {
        new Thread(() -> {
            try {
                String f = Uri.parse(url).getQueryParameter("f");
                String name = f == null ? "report" : f.replace('/', '_');
                HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
                InputStream in = c.getInputStream();
                OutputStream out;
                if (Build.VERSION.SDK_INT >= 29) {
                    ContentValues v = new ContentValues();
                    v.put(MediaStore.Downloads.DISPLAY_NAME, name);
                    v.put(MediaStore.Downloads.MIME_TYPE, mime == null ? "application/octet-stream" : mime);
                    v.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
                    Uri u = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, v);
                    out = getContentResolver().openOutputStream(u);
                } else {
                    File dir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
                    out = new FileOutputStream(new File(dir, name));
                }
                byte[] buf = new byte[8192];
                int n;
                while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                out.close();
                in.close();
                runOnUiThread(() -> Toast.makeText(this, "Saved " + name + " to Downloads", Toast.LENGTH_LONG).show());
            } catch (Exception e) {
                runOnUiThread(() -> Toast.makeText(this, "Could not save the file", Toast.LENGTH_LONG).show());
            }
        }).start();
    }

    @Override
    public void onBackPressed() {
        if (web.canGoBack()) web.goBack(); else moveTaskToBack(true);
    }
}
