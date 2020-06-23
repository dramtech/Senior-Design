package com.group3.safetyhelmet;

import android.content.Context;
import android.content.SharedPreferences;

public class Storage {
    private SharedPreferences sp;
    private SharedPreferences.Editor editor;

    public Storage(SharedPreferences sp) {
        this.sp = sp;

        if (this.sp != null) {
            editor = sp.edit();
        }
    }

    public void add(String key, String value) {
        editor.putString(key, value);
        editor.apply();
    }

    public String getValue(String key) {
        return sp.getString(key, "");
    }

    public void remove(String key) {

    }

}
