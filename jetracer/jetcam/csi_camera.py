from .camera import Camera
import atexit
import cv2
import numpy as np
import threading
import traitlets


class CSICamera(Camera):

    capture_device = traitlets.Integer(default_value=0)
    capture_fps = traitlets.Integer(default_value=30)
    capture_width = traitlets.Integer(default_value=640)
    capture_height = traitlets.Integer(default_value=480)
    downsample = traitlets.Integer(default_value=1)

    def __init__(self, *args, **kwargs):
        super(CSICamera, self).__init__(*args, **kwargs)
        self.downsample = self.downsample or kwargs.get('downsample')
        self.capture_fps = self.capture_fps or kwargs.get('capture_fps')
        self.capture_width  = self.capture_width  or kwargs.get('capture_width')
        self.capture_height = self.capture_height or kwargs.get('capture_height')

        self.width  = self.capture_width  // self.downsample
        self.height = self.capture_height // self.downsample

        try:
            self.cap = cv2.VideoCapture(self._gst_str(), cv2.CAP_GSTREAMER)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

            # self.cap.release() will be called at exit
            atexit.register(self.cap.release)
        except:
            raise RuntimeError(
                'Could not initialize camera.  Please see error trace.')

    def _gst_str(self):
        gstreamer_pipeline = (
            "nvarguscamerasrc sensor-id=%d ! "
            "video/x-raw(memory:NVMM), width=(int)%d, height=(int)%d, framerate=(fraction)%d/1 ! "
            "nvvidconv interpolation-method=1 ! "
            "video/x-raw, width=(int)%d, height=(int)%d, format=(string)BGRx ! "
            "videoconvert ! "
            "video/x-raw, format=(string)BGR ! appsink max-buffers=1 drop=True"
            % (
                self.capture_device,
                self.capture_width,
                self.capture_height,
                self.capture_fps,
                self.width,
                self.height,
            )
        )
        return gstreamer_pipeline

    def _read(self):
        re, image = self.cap.read()
        if re:
            return image
        else:
            raise RuntimeError('Could not read image from camera')