package com.group3.safetyhelmet;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;


import android.Manifest;
import android.app.Activity;
import android.app.Application;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelUuid;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.card.MaterialCardView;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.UUID;


public class ActivityConfig extends AppCompatActivity {
    private static final String TAG = "ActivityConfig";
    private BluetoothAdapter bluetoothAdapter;

    BluetoothConnectionService btConnService = null;

    final int SEND_SMS_PERMISSION_REQUEST_CODE = 1;
    private static String phoneNum = "9549078034";
    private static String message = "EMERGENCY:\nThis is a message coming from the SAFETY HELMET system.\n";

    private static Context context;

    static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
    BluetoothDevice btDevice;

    private HashSet<String> bluetoothDevices = new HashSet<String>();
    private String btDeviceAddress = "00:14:03:05:FE:90";
    private String btDevicePasskey = "1234";

    private Button btnStartConn;
    private Button btnSendOff;
    private Button btnSendOn;
    private Switch tempSwitch;
    private String tempUnit = "F";
    private Switch bluetoothSwitch;
    private String blueOnState = "OFF";

    // Create a BroadcastReceiver for ACTION_FOUND.
    private final BroadcastReceiver broadcastReceiverBTEnable = new BroadcastReceiver() {
        @RequiresApi(api = Build.VERSION_CODES.M)
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                final int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, bluetoothAdapter.ERROR);
                switch (state) {
                    case BluetoothAdapter.STATE_OFF:
                        Log.d(TAG, "STATE OFF.");
                        break;
                    case BluetoothAdapter.STATE_TURNING_OFF:
                        Log.d(TAG, "STATE TURNING OFF.");
                        break;
                    case BluetoothAdapter.STATE_ON:
                        Log.d(TAG, "STATE ON.");
                        discoverBluetoothDevices();
                        break;
                    case BluetoothAdapter.STATE_TURNING_ON:
                        Log.d(TAG, "STATE TURNING ON.");
                        break;
                }
            }
        }
    };

    private final BroadcastReceiver broadcastReceiverBTDiscovered = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            switch (action) {
                case BluetoothAdapter.ACTION_DISCOVERY_STARTED:
                    Log.d(TAG, "Searching for devices.");
                    break;
                case BluetoothAdapter.ACTION_DISCOVERY_FINISHED:
                    Log.d(TAG, "Discovery Finished.");
                    break;
                case BluetoothDevice.ACTION_FOUND:
                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                    if (!bluetoothDevices.contains(device.getAddress())) {
                        bluetoothDevices.add(device.getAddress());
                    }

                    // Check if desired device has been found.
                    if (bluetoothDevices.contains(btDeviceAddress)) {
                        Log.d(TAG, "Found Device: " + btDeviceAddress);

                        btDevice = device;
                        bluetoothAdapter.cancelDiscovery();
                        pairDevice(btDevice);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private final BroadcastReceiver broadcastReceiverBTPair = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (BluetoothDevice.ACTION_PAIRING_REQUEST.equals(intent.getAction())) {
                final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                int type = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, BluetoothDevice.ERROR);

                if (type == BluetoothDevice.PAIRING_VARIANT_PIN) {
                    Log.d(TAG,"Entering pin: " + btDevicePasskey);
                    device.setPin(btDevicePasskey.getBytes());
                    abortBroadcast();
                } else {
                    Log.d(TAG,"Unexpected pairing type: " + type);
                }
            }
        }
    };

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "On Destroy.");
        // Don't forget to unregister the ACTION_FOUND receiver.
        try {
            unregisterReceiver(broadcastReceiverBTEnable);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
        try {
            unregisterReceiver(broadcastReceiverBTDiscovered);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
        try {
            unregisterReceiver(broadcastReceiverBTPair);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_config);

        Toolbar toolbar = findViewById(R.id.config_toolbar);
        toolbar.setTitle(R.string.config_toolbar_title);
        setSupportActionBar(toolbar);

        IntentFilter intentFilter = new IntentFilter(BluetoothDevice.ACTION_PAIRING_REQUEST);
        intentFilter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY);
        registerReceiver(broadcastReceiverBTPair,intentFilter);

        context = getApplicationContext();

        if (!checkPermission(Manifest.permission.SEND_SMS)) {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.SEND_SMS},
                    SEND_SMS_PERMISSION_REQUEST_CODE);
        }

        tempSwitch = (Switch) findViewById(R.id.tempSwitch);
        tempSwitch.setText(tempUnit);
        tempSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    setTempUnit("C");
                    tempSwitch.setText(getTempUnit());
                    // TODO convert temperature to C and update temperature value
                } else {
                    setTempUnit("F");
                    tempSwitch.setText(getTempUnit());
                    // TODO convert temperature to F and update temperature value
                }
            }
        });


        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        bluetoothSwitch = (Switch) findViewById(R.id.bluetoothOnSwitch);
        bluetoothSwitch.setText(blueOnState);

        //TODO set Bluetooth switch to ON if already paired and connected
        bluetoothSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @RequiresApi(api = Build.VERSION_CODES.M)
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    setBlueOnState("ON");
                    bluetoothSwitch.setText(getBlueOnState());
                    // Enable blue.
                    enableBluetooth();
                } else {
                    setBlueOnState("OFF");
                    bluetoothSwitch.setText(getBlueOnState());

                    disableBluetooth();

                    // Confirm to user that bluetooth as been disabled.
                    Toast toast = Toast.makeText(ActivityConfig.this, R.string.toast_bluetooth_disabled, Toast.LENGTH_SHORT);
                    toast.show();

                    final TextView helloTextView = (TextView) findViewById(R.id.bluetoothConfigState);
                    helloTextView.setText(R.string.device_status_not_connected);
                }
            }
        });

        btnStartConn = (Button) findViewById(R.id.button);
        btnSendOff = (Button) findViewById(R.id.button2);
        btnSendOn = (Button) findViewById(R.id.button3);

        btnStartConn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startConn();
            }
        });

        btnSendOn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String data = "A";
                byte[] bytes = data.getBytes();
                btConnService.writeOut(bytes);
            }
        });

        btnSendOff.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String data = "B";
                byte[] bytes = data.getBytes();
                btConnService.writeOut(bytes);
            }
        });
    }

    public void startConn() {
        startBTConnection(btDevice, MY_UUID);
    }

    public void startBTConnection(BluetoothDevice device, UUID uuid) {
        Log.d(TAG, "Starting bluetooth connection.");

        if (btConnService == null) {
            Log.d(TAG, "DEBUG: NULL");
            btConnService = new BluetoothConnectionService(getApplicationContext());
        }
        btConnService.startClient(device, uuid);
    }

    private void pairDevice(BluetoothDevice device) {
        try {
            Log.d(TAG, "Start Pairing... with: " + device.getName());
            device.createBond();
            Log.d(TAG, "Pairing finished.");
            final TextView helloTextView = (TextView) findViewById(R.id.bluetoothConfigState);
            helloTextView.setText(R.string.device_status_connected);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public void checkDiscoveryPermissions() {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            Log.d(TAG, "ERROR: Permissions.");
            int permissionCheck = this.checkSelfPermission("Manifest.permission.ACCESS_FINE_LOCATION");
            permissionCheck += this.checkSelfPermission("Manifest.permission.ACCESS_COARSE_LOCATION");
            if (permissionCheck != 0) {
                this.requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION}, 1001);
            } else {
                Log.d(TAG, "Permissions satisfied.");
            }
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public void discoverBluetoothDevices() {
            IntentFilter filter = new IntentFilter();
            filter.addAction(BluetoothDevice.ACTION_FOUND);
            registerReceiver(broadcastReceiverBTDiscovered, filter);

            filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
            registerReceiver(broadcastReceiverBTDiscovered, filter);

            filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
            registerReceiver(broadcastReceiverBTDiscovered, filter);

            checkDiscoveryPermissions();
            bluetoothAdapter.startDiscovery();
    }

    public void disableBluetooth() {
        if (bluetoothAdapter == null) {
            Log.d(TAG, "ERROR: Device is not bluetooth capable.");

        } else if (!bluetoothAdapter.isEnabled()) {
            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            registerReceiver(broadcastReceiverBTEnable, intentFilter);
        } else if (bluetoothAdapter.isEnabled()) {
            if (btConnService != null) {
                btConnService.cancelService();
                btConnService = null;
            }

            bluetoothAdapter.disable();
            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            registerReceiver(broadcastReceiverBTEnable, intentFilter);
        }
    }

    public void enableBluetooth() {
        if (bluetoothAdapter == null) {
            Log.d(TAG, "ERROR: Device is not bluetooth capable.");

        } else if (!bluetoothAdapter.isEnabled()) {
            Intent intentBluetoothEnable = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivity(intentBluetoothEnable);

            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            registerReceiver(broadcastReceiverBTEnable, intentFilter);
        } else if (bluetoothAdapter.isEnabled()) {
            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            registerReceiver(broadcastReceiverBTEnable, intentFilter);
        }
    }

    public static void receivedData(String data) {
        Log.d(TAG, "Trying to send message...");
        if (data.compareTo("sSS") == 0 && checkPermission(Manifest.permission.SEND_SMS)) {
            Log.d(TAG, "Sending SMS...");
            SmsManager smsManager = SmsManager.getDefault();
            smsManager.sendTextMessage(phoneNum, null, message,
                    null, null);
        }
    }

    public static boolean checkPermission(String permission) {
        Log.d(TAG, "Checking permissions...");
        int check = ContextCompat.checkSelfPermission(context, permission);
        return (check == PackageManager.PERMISSION_GRANTED);
    }

    public void setTempUnit(String unit) {
        byte[] bytes = unit.getBytes();
        btConnService.writeOut(bytes);
        tempUnit = unit;
    }

    public String getTempUnit() {
        return tempUnit;
    }

    public void setBlueOnState(String state) {
        blueOnState = state;
    }

    public String getBlueOnState() {
        return blueOnState;
    }
}
