package com.group3.safetyhelmet;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ConnectException;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.UUID;

public class BluetoothConnectionService {
    private static final String TAG = "BluetoothConnService";
    private static final String appName = "MYAPP";
    private static final UUID MY_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");

    private final BluetoothAdapter bluetoothAdapter;
    private AcceptThread acceptThread;
    private ConnectThread connectThread;
    private BluetoothDevice btDevice;
    private UUID deviceUUID;
    private ConnectedThread connectedThread;
    private boolean STOP = false;
    Context mcontext;

    public BluetoothConnectionService(Context context) {
        mcontext = context;
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        start();
    }

    private class AcceptThread extends Thread {
        private final BluetoothServerSocket serverSocket;

        public AcceptThread() {
            BluetoothServerSocket temp = null;

            try {
                temp = bluetoothAdapter.listenUsingInsecureRfcommWithServiceRecord(appName, MY_UUID);
                Log.d(TAG, "Accept thread setting up server socket.");
            } catch (IOException e) {
                Log.d(TAG, "Accept Thread: " + e.getMessage());
            }
            serverSocket = temp;
        }

        public void run() {
            Log.d(TAG, "Accept thread running...");
            BluetoothSocket socket = null;

            try {
                Log.d(TAG, "Run: Server socket started...");
                // Blocking call.
                socket = serverSocket.accept();
                Log.d(TAG, "Run: Connection successful.");
            } catch (IOException e) {
                Log.d(TAG, "Run: " + e.getMessage());
            }

            if (socket == null) {
                connected(socket, btDevice);
            }
            Log.i(TAG, "End Accept Thread.");
        }

        public void cancel() {
            Log.d(TAG, "Accept Cancel: Cancelling Accept Thread.");

            try {
                serverSocket.close();
            } catch (IOException e) {
                Log.d(TAG, "Accept Thread: Socket close failed.");
            }
        }
    }

    private class ConnectThread extends Thread{
        private BluetoothSocket btSocket;

        public ConnectThread(BluetoothDevice device, UUID uuid) {
            Log.d(TAG, "Connect Thread Started...");
            btDevice = device;
            deviceUUID = uuid;
        }

        public void run() {
            BluetoothSocket temp = null;
            Log.d(TAG, "Running Connect Thread...");

            try {
                Log.d(TAG, "Trying to create comm socket...");
                temp = btDevice.createRfcommSocketToServiceRecord(deviceUUID);
            } catch (IOException e) {
                Log.d(TAG, "Could not create com socket.");
            }

            btSocket = temp;
            bluetoothAdapter.cancelDiscovery();

            try {
                // Blocking call.
                btSocket.connect();
                Log.d(TAG, "Connect Thread: Connected");
            } catch (IOException e) {
                try {
                    Log.d(TAG, "Connect Thread: Closing socket.");
                    btSocket.close();
                } catch (IOException i) {
                    Log.d(TAG, "Connect Thread: Failed to close socket.");
                }
            }
            connected(btSocket, btDevice);
        }

        public void cancel() {
            Log.d(TAG, "Connect Cancel: Closing socket.");

            try {
                btSocket.close();
            } catch (IOException e) {
                Log.d(TAG, "Connect Thread: Socket close failed.");
            }
        }
    }

    public synchronized void start() {
        Log.d(TAG, "Starting...");
        STOP = false;

        if (connectThread != null) {
            connectThread.cancel();
            connectThread = null;
        }

        if (acceptThread == null) {
            acceptThread = new AcceptThread();
            acceptThread.start();
        }
    }

    public void startClient(BluetoothDevice device, UUID uuid) {
        Log.d(TAG, "Start Client: Started.");
        connectThread = new ConnectThread(device, uuid);
        connectThread.start();
    }

    private class ConnectedThread extends Thread {
        private final BluetoothSocket socket;
        private final InputStream inStream;
        private final OutputStream outStream;

        public void stopStream() {
            try {
                inStream.close();
                outStream.close();
                socket.close();

                Log.d(TAG, "Streams closed successfully.");
            } catch (IOException e) {
                Log.d(TAG, "Could not close streams.");
            }
        }
        public ConnectedThread(BluetoothSocket socket) {

            Log.d(TAG, "Connected Thread: Starting");
            this.socket = socket;
            InputStream tempInStream = null;
            OutputStream tempOutStream = null;

            if (!STOP) {
                try {
                    tempInStream = this.socket.getInputStream();
                    tempOutStream = this.socket.getOutputStream();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            inStream = tempInStream;
            outStream = tempOutStream;
        }

        public void run() {
            byte[] buffer = new byte[2048];
            int bytes;
            StringBuilder data = new StringBuilder();

            while (!STOP) {
                try {
                    bytes = inStream.read(buffer);
                    String dataIn = new String(buffer, 0, bytes);

                    data.append(dataIn);

                    if (data.length() == 3) {
                        Log.d(TAG, data.toString());
                        ActivityConfig.receivedData(data.toString());
                        data.setLength(0);
                    }

                } catch (IOException e) {
                    e.printStackTrace();
                    break;
                }
            }
        }

        public void write(byte[] bytes) {
            String outData = new String(bytes, Charset.defaultCharset());
            Log.d(TAG, "Write: " + outData);

            try {
                Log.d(TAG, "Writing to device...");
                outStream.write(bytes);
            } catch (IOException e) {
                Log.d(TAG, "Error writing data: " + e.getMessage());
            }
        }

        public void cancel() {
            try {
                socket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public void writeOut(byte[] bytes) {
        ConnectThread connThread;
        Log.d(TAG, "Write: Write Called.");
        connectedThread.write(bytes);
    }

    private void connected(BluetoothSocket socket, BluetoothDevice device) {
        Log.d(TAG, "Connected: Starting.");
        connectedThread = new ConnectedThread(socket);
        connectedThread.start();
    }

    public void cancelService() {
        try {
            Log.d(TAG, "Cancelling service...");
            STOP = true;

            if (connectedThread != null) {
                connectedThread.stopStream();
                connectedThread.cancel();
            }
            Log.d(TAG, "Threads cancelled successfully.");
        } catch (Exception e) {
            Log.d(TAG, "Could not cancel threads: " + e.getMessage());
        }
    }
}
