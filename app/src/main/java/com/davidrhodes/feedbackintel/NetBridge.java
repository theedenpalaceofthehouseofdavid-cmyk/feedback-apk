package com.davidrhodes.feedbackintel;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;
import java.util.Map;

public class NetBridge {
    public static Object[] fetch(String url, String agent, String[] hdrs, boolean head, int timeout) {
        try {
            URL u = new URL(url);
            for (int hop = 0; hop < 8; hop++) {
                HttpURLConnection c = (HttpURLConnection) u.openConnection();
                c.setInstanceFollowRedirects(false);
                c.setConnectTimeout(timeout * 1000);
                c.setReadTimeout(timeout * 1000);
                c.setRequestMethod(head ? "HEAD" : "GET");
                c.setRequestProperty("User-Agent", agent);
                for (String h : hdrs) {
                    int i = h.indexOf(':');
                    if (i > 0) c.setRequestProperty(h.substring(0, i).trim(), h.substring(i + 1).trim());
                }
                int code = c.getResponseCode();
                String loc = c.getHeaderField("Location");
                if (code >= 300 && code < 400 && loc != null) {
                    u = new URL(u, loc);
                    c.disconnect();
                    continue;
                }
                StringBuilder hs = new StringBuilder("HTTP/1.1 " + code + "\r\n");
                for (Map.Entry<String, List<String>> e : c.getHeaderFields().entrySet()) {
                    if (e.getKey() == null) continue;
                    for (String v : e.getValue()) hs.append(e.getKey()).append(": ").append(v).append("\r\n");
                }
                hs.append("\r\n");
                ByteArrayOutputStream bo = new ByteArrayOutputStream();
                if (!head) {
                    InputStream in = code >= 400 ? c.getErrorStream() : c.getInputStream();
                    if (in != null) {
                        byte[] buf = new byte[8192];
                        int n;
                        while ((n = in.read(buf)) > 0 && bo.size() < 6 * 1024 * 1024) bo.write(buf, 0, n);
                        in.close();
                    }
                }
                String ct = c.getContentType() == null ? "" : c.getContentType();
                return new Object[]{String.valueOf(code), u.toString(), ct, hs.toString(), bo.toByteArray()};
            }
        } catch (Exception e) {
        }
        return null;
    }
}
