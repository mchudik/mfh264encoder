# mfh264encoder
Media Foundation h.264 encoder GStreamer plugin

# Commands to run

gst-launch-1.0 -v -m autovideosrc ! videoscale ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! mfh264encoder ! video/x-h264 ! decodebin ! autovideosink

gst-launch-1.0 -v -m autovideosrc ! videoscale ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! mfh264encoder ! h264parse ! avimux ! filesink location=cameracapture.mp4
