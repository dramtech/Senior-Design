package com.group3.safetyhelmet;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.Toolbar;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import org.w3c.dom.Text;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private Button setEmergContBtn;
    private Button setConfigBtn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Setting app the tool bar
        Toolbar myToolbar = (Toolbar) findViewById(R.id.main_toolbar);
        setSupportActionBar(myToolbar);

        // App theme always in night mode
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

        setEmergContBtn = findViewById(R.id.setEmergContBtn);
        setEmergContBtn.setOnClickListener(this);
        setConfigBtn = findViewById(R.id.setConfigBtn);
        setConfigBtn.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {

            case R.id.setEmergContBtn:
                loadEmergContForm();
                break;
            case R.id.setConfigBtn:
                loadConfig();
                break;
            default:
                break;
        }
    }

    private void loadEmergContForm() {
        // TODO load a form to add emergency contact
        Intent intent = new Intent(this, activity_emergContact.class);
        startActivity(intent);
    }


    private void loadConfig() {
        Intent intent = new Intent(this, ActivityConfig.class);
        startActivity(intent);
    }

    private void loadNav() {
        Intent intent = new Intent(this, ActivityNav.class);
        startActivity(intent);
    }
}
