ChromeCast-Arduino-Button
=========================

<p><img src="http://chromecast.entertailion.com/chromecastanimation100.gif"/></p>

<p>This Arduino sketch allows you to control media playing on a <a href="https://www.google.com/intl/en/chrome/devices/chromecast/">ChromeCast</a> device.</p>

<p>The sketch can control all the existing Google apps that support ChromeCast including YouTube, Google Music and Google Movies. Netflix is not supported since it uses a proprietary protocol for media control.</p>

<p>This sketch requires an Arduino Ethernet shield and a button. The sketch has been tested with an Arduino Uno and a <a href="https://www.adafruit.com/products/367">Tactile Button switch</a> and
a <a href="https://www.adafruit.com/products/1185">Massive Arcade Button with LED</a>.</p>

<p>The sketch requires this <a href="https://github.com/entertailion/Arduino-Websocket">websocket library</a> which has to be installed in the libraries directory of your Arduino IDE. The websocket library has been modified to work with ChromeCast devices.
Use the Arduino IDE to upload the code onto an Arduino device. </p>

<p>To customize the sketch for your ChromeCast device, simply update ChromeCast device IP address in the sketch. The sketch assumes a button is on pin A0. Uncomment the loop() code if you want to use pin 2 for the button.
You can also put a LED on pin 7 to get feedback when the network is initialized and the button is pressed.</p>
 
<p>To control playback, the sketch makes a websocket connection to the current app running on the ChromeCast device.
 These steps are done in order:
<ol>
<li>Check if there is an app running on the ChromeCast device and get its app id.</li>
<li>Get the app status to get the connection service URL.</li>
<li>Use the connection service URL to get the websocket address.</li>
<li>Create a websocket connection.</li>
<li>Send RAMP PLAY/STOP messages to control media playback.</li>
<li>Close the websocket connection.</li>
</ol>
The code toggles between the play and pause state every time the button is pressed.
</p>

<p>Watch this <a href="http://youtu.be/NliuFTY-xhc">video</a> to see the sketch in action.</p>

<p>Other apps developed by Entertailion:
<ul>
<li><a href="https://github.com/entertailion/ChromeCast-Arduino">ChromeCast-Arduino</a>: Use an Arduino to send YouTube videos to a ChromeCast device when a motion sensor is triggered</li>
<li><a href="https://github.com/entertailion/Fling">Fling</a>: Share local media files with a ChromeCast device</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.tvremote">Able Remote for Google TV</a>: The ultimate Google TV remote</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.launcher">Open Launcher for Google TV</a>: The ultimate Google TV launcher</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.overlay">Overlay for Google TV</a>: Live TV effects for Google TV</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.overlaynews">Overlay News for Google TV</a>: News headlines over live TV</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.videowall">Video Wall</a>: Wall-to-Wall Youtube videos</li>
<li><a href="https://play.google.com/store/apps/details?id=com.entertailion.android.tasker">GTV Tasker Plugin</a>: Control your Google TV with Tasker actions</li>
</ul>
</p>