import cv2
import dlib 
import numpy as np
import logging

detector = dlib.get_frontal_face_detector()
predictor = dlib.shape_predictor("shape_predictor_68_face_landmarks.dat")
tracker = None
tracking = False

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# ESP32-CAM streaming URL
url = "http://192.168.1.123:81/stream"

SCALE_FACTOR = 1.5

# Load Haar Cascade for face detection
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")

def generate_frames():
    global tracker, tracking
    cap = cv2.VideoCapture(url)
    logging.info(f"Attempting to connect to stream at {url}")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            logging.error("Failed to capture frame")
            break

        # Resize frame
        height, width = frame.shape[:2]
        new_height = int(height * SCALE_FACTOR)
        new_width = int(width * SCALE_FACTOR)
        frame = cv2.resize(frame, (new_width, new_height))

        # Convert to RGB for dlib
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        if not tracking:
            # Detect faces using dlib
            faces = detector(rgb_frame)
            if len(faces) > 0:
                # Start tracking the first detected face
                face = faces[0]
                tracker = dlib.correlation_tracker()
                rect = dlib.rectangle(face.left(), face.top(), face.right(), face.bottom())
                tracker.start_track(rgb_frame, rect)
                tracking = True
                logging.info("Started tracking face")
        else:
            # Update tracker
            tracking_quality = tracker.update(rgb_frame)
            
            if tracking_quality >= 8.0:  # Good tracking confidence
                tracked_position = tracker.get_position()
                
                # Get coordinates and draw rectangle
                x = int(tracked_position.left())
                y = int(tracked_position.top())
                w = int(tracked_position.width())
                h = int(tracked_position.height())
                
                cv2.rectangle(frame, (x, y), (x+w, y+h), (255, 0, 0), 2)
                logging.debug(f"Tracking face at position (x={x}, y={y}, w={w}, h={h})")
            else:
             # Lost track, reset tracking
                tracking = False
                logging.info("Lost face tracking")

        # Encode the frame in JPEG format
        ret, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')