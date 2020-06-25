package com.group3.safetyhelmet;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class activity_emergContact extends AppCompatActivity {
    private static final String TAG = "activityContact";

    private EditText contactFirstName;
    private EditText contactLastName;
    private EditText contactPhoneNum;
    private Button saveBtn;
    private SharedPreferences sp;
    private Storage storage;

    // Emergency Contact Test:
    private Button emergencyBtn;
    final int SEND_SMS_PERMISSION_REQUEST_CODE = 1;

    private static String phoneNum = "9549078034";
    private static String message = "Hello, this is a test.";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Establish shared preference access.
        sp = getSharedPreferences("contactInfo", Context.MODE_PRIVATE);
        storage = new Storage(sp);

        setContentView(R.layout.activity_emerg_contact);

        Toolbar toolbar = findViewById(R.id.nav_toolbar);
        toolbar.setTitle(R.string.emergency_cont_title);
        setSupportActionBar(toolbar);

        contactFirstName = (EditText) findViewById(R.id.emergContName);
        contactLastName = (EditText) findViewById(R.id.emergContLastName);
        contactPhoneNum = (EditText) findViewById(R.id.emergContNumber);
        saveBtn = (Button) findViewById(R.id.saveButton);

        emergencyBtn = (Button) findViewById(R.id.signalEmerg);
        if (!checkPermission(Manifest.permission.SEND_SMS)) {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.SEND_SMS},
                                                SEND_SMS_PERMISSION_REQUEST_CODE);
        }

        saveBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                // Retrieve contact info from view.
                String firstName = contactFirstName.getText().toString();
                String lastName = contactLastName.getText().toString();
                String phoneNum = contactPhoneNum.getText().toString();

                // Check if contact info entered is valid.
                if (firstName == null || lastName == null ||
                        firstName.length() == 0 || lastName.length() == 0 || phoneNum.length() != 10) {
                    Log.d(TAG, "Error: User inputted invalid contact information.");
                    return;
                }

                // Save emergency contact in Shared Preferences.
                storage.add("ContactFirstName1", firstName);
                storage.add("ContactLastName1", lastName);
                storage.add("ContactPhoneNum1", phoneNum);

                // Let user know contact info has been added.
                Toast toast = Toast.makeText(getApplicationContext(), R.string.toast_contact_added, Toast.LENGTH_SHORT);
                toast.show();

                // Clear input form.
                contactFirstName.setText("");
                contactLastName.setText("");
                contactPhoneNum.setText("");

                Log.d(TAG, storage.getValue("ContactFirstName1"));
                Log.d(TAG, storage.getValue("ContactLastName1"));
                Log.d(TAG, storage.getValue("ContactPhoneNum1"));
            }
        });

        // SMS emergency message test.
        emergencyBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (checkPermission(Manifest.permission.SEND_SMS)) {
                    Log.d(TAG, "Sending SMS...");
                    SmsManager smsManager = SmsManager.getDefault();
                    smsManager.sendTextMessage(phoneNum, null, message,
                                                null, null);
                }
            }
        });
    }

    public boolean checkPermission(String permission) {
        int check = ContextCompat.checkSelfPermission(this, permission);
        return (check == PackageManager.PERMISSION_GRANTED);
    }

}