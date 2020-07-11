package com.group3.safetyhelmet;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;

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
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import org.w3c.dom.Text;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private static Context context;
    private Button setEmergContBtn;
    private Button setConfigBtn;
    private static TextView tempDataView;
    private static TextView deviceStatusView;
    private static TextView emergencyContactView;
    private static TextView addEmergAlert;
    private static TextView bluetoothConnAlert;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        context = getApplicationContext();

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

        deviceStatusView = (TextView) findViewById(R.id.deviceStatus);
        tempDataView = (TextView) findViewById(R.id.tempValue);
        emergencyContactView = (TextView) findViewById(R.id.emergContValue);

        addEmergAlert = (TextView) findViewById(R.id.warningEmergMesg);
        bluetoothConnAlert = (TextView) findViewById(R.id.warningConnectDevice);
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

    public static void updateTempView(String temp) {
        tempDataView.setText(temp);
    }

    public static void updateDeviceStatus(int code) {
        if (code == 1) {
            deviceStatusView.setTextColor(ContextCompat.getColor(context, R.color.colorDeviceStatConnected));
            tempDataView.setTextColor(ContextCompat.getColor(context, R.color.colorDeviceStatConnected));
            deviceStatusView.setText("Connected");
            bluetoothConnAlert.setText("");
            tempDataView.setText("84");
        } else if (code == 0){
            deviceStatusView.setTextColor(ContextCompat.getColor(context, R.color.colorDeviceStatNotConnected));
            deviceStatusView.setText("Not Connected");
            bluetoothConnAlert.setText(R.string.warning_connect_device);
        }
    }

    public static void updateEmergContactInfoView(String name, String phoneNum) {
        StringBuilder reformatPhoneNum = new StringBuilder("(").append(phoneNum.substring(0, 3));
        reformatPhoneNum.append(") ").append(phoneNum.substring(3, 6)).append("-").append(phoneNum.substring(6)).toString();
        emergencyContactView.setTextColor(ContextCompat.getColor(context, R.color.colorDeviceStatConnected));
        emergencyContactView.setText(name + " : " + reformatPhoneNum);
        addEmergAlert.setText("");
    }
}
