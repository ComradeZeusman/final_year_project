import cv2
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# ESP32-CAM streaming URL
url = "http://192.168.1.123:81/stream"

# Load Haar Cascade for face detection
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")

def generate_frames():
    cap = cv2.VideoCapture(url)
    logging.info(f"Attempting to connect to stream at {url}")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            logging.error("Failed to capture frame")
            break

        # Convert to grayscale for face detection
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        try:
            # Detect faces
            faces = face_cascade.detectMultiScale(gray, 1.1, 4)
            logging.info(f"Detected {len(faces)} faces in frame")

            # Draw rectangles around faces
            for (x, y, w, h) in faces:
                cv2.rectangle(frame, (x, y), (x+w, y+h), (255, 0, 0), 2)
                logging.debug(f"Face detected at position (x={x}, y={y}, w={w}, h={h})")

        except Exception as e:
            logging.error(f"Error during face detection: {str(e)}")
            continue

        # Encode the frame in JPEG format
        ret, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')