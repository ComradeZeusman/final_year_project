#ifndef REGISTRATION_HTML_H
#define REGISTRATION_HTML_H

const char REGISTRATION_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Owner Registration</title>
    <style>
        body {
            margin: 40px;
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        input {
            display: block;
            margin: 10px 0;
            padding: 12px;
            width: 100%;
            box-sizing: border-box;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        input[readonly] {
            background: #f5f5f5;
        }
        button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 12px;
            cursor: pointer;
            width: 100%;
            border-radius: 4px;
            font-size: 16px;
        }
        button:disabled {
            background: #cccccc;
            cursor: not-allowed;
        }
        #cameraView {
            width: 100%;
            max-width: 400px;
            margin: 20px auto;
            display: block;
            border-radius: 8px;
        }
        .error {
            color: #ff0000;
            display: none;
            margin-top: 5px;
            font-size: 14px;
        }
        #enrollStatus {
            display: block;
            margin: 10px 0;
            font-weight: bold;
            text-align: center;
            color: #333;
        }
        .camera-container {
            position: relative;
            width: fit-content;
            margin: 20px auto;
        }
        #faceBox {
            position: absolute;
            border: 3px solid #4CAF50;
            display: none;
        }
        h2, h3 {
            color: #333;
            text-align: center;
        }
    </style>
</head>
<body>
    <h2>Owner Registration</h2>
    <form id="regForm">
        <div class="form-group">
            <input type="text" id="ownerName" placeholder="Owner Name" required>
        </div>
        <div class="form-group">
            <input type="password" id="password" placeholder="Password" required>
            <input type="password" id="confirmPassword" placeholder="Confirm Password" required>
            <span id="passwordError" class="error">Passwords do not match</span>
        </div>
        <div class="form-group">
            <input type="text" id="deviceId" readonly>
            <input type="text" id="location" placeholder="Location" required>
        </div>
        <div class="form-group">
            <h3>Face Enrollment</h3>
            <div class="camera-container">
                <img id="cameraView" src="/stream">
                <div id="faceBox"></div>
            </div>
            <button type="button" id="enrollFace">Start Face Enrollment</button>
            <span id="enrollStatus">Look at the camera and press Start</span>
        </div>
        <button type="submit" id="submitBtn" disabled>Complete Registration</button>
    </form>

    <script>
        let samplesCollected = 0;
        const TOTAL_SAMPLES = 5;
        let enrollmentTimer = null;

        // Generate device ID using timestamp and random string
        const timestamp = new Date().getTime().toString(36);
        const random = Math.random().toString(36).substr(2, 5);
        document.getElementById('deviceId').value = `ESP32_${timestamp}${random}`;

        // Password validation
        document.getElementById('confirmPassword').onkeyup = () => {
            const pwd = document.getElementById('password').value;
            const confirm = document.getElementById('confirmPassword').value;
            const error = document.getElementById('passwordError');
            error.style.display = pwd !== confirm ? 'block' : 'none';
        };

        // Face enrollment process
        document.getElementById('enrollFace').onclick = async () => {
            const status = document.getElementById('enrollStatus');
            const enrollBtn = document.getElementById('enrollFace');
            const faceBox = document.getElementById('faceBox');
            
            try {
                // Enable face detection and recognition
                await fetch('/control?var=face_detect&val=1');
                await fetch('/control?var=face_recognize&val=1');
                
                // Start enrollment
                enrollBtn.disabled = true;
                status.textContent = 'Starting enrollment...';
                faceBox.style.display = 'block';
                
                const res = await fetch('/control?var=face_enroll&val=1');
                if(res.ok) {
                    status.textContent = `Look directly at the camera (0/${TOTAL_SAMPLES} samples)`;
                    checkEnrollmentProgress();
                } else {
                    throw new Error('Failed to start enrollment');
                }
            } catch(err) {
                console.error('Enrollment error:', err);
                status.textContent = 'Enrollment failed. Please try again.';
                enrollBtn.disabled = false;
                faceBox.style.display = 'none';
            }
        };

        // Check enrollment progress
        async function checkEnrollmentProgress() {
            const status = document.getElementById('enrollStatus');
            const submitBtn = document.getElementById('submitBtn');
            const faceBox = document.getElementById('faceBox');
            
            try {
                const res = await fetch('/status');
                const data = await res.json();
                console.log('Enrollment status:', data);
                
                if(data.face_enroll === 0) {
                    status.textContent = 'Face enrolled successfully!';
                    submitBtn.disabled = false;
                    faceBox.style.display = 'none';
                    if(enrollmentTimer) clearTimeout(enrollmentTimer);
                    return;
                }
                
                if(data.samples_collected !== samplesCollected) {
                    samplesCollected = data.samples_collected;
                    status.textContent = `Keep looking at camera (${samplesCollected}/${TOTAL_SAMPLES} samples)`;
                }
                
                enrollmentTimer = setTimeout(checkEnrollmentProgress, 1000);
            } catch(err) {
                console.error('Status check error:', err);
                status.textContent = 'Error checking enrollment status';
                faceBox.style.display = 'none';
                if(enrollmentTimer) clearTimeout(enrollmentTimer);
            }
        }

        // Form submission
        document.getElementById('regForm').onsubmit = async (e) => {
            e.preventDefault();
            
            if(enrollmentTimer) {
                clearTimeout(enrollmentTimer);
            }

            const data = {
                ownerName: document.getElementById('ownerName').value,
                password: document.getElementById('password').value,
                deviceId: document.getElementById('deviceId').value,
                location: document.getElementById('location').value,
                faceEnrolled: true
            };

            try {
                const res = await fetch('/register', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(data)
                });
                if(res.ok) {
                    alert('Registration successful!');
                    window.location.href = '/';
                } else {
                    throw new Error('Registration failed');
                }
            } catch(err) {
                console.error('Registration error:', err);
                alert('Registration failed. Please try again.');
            }
        };
    </script>
</body>
</html>
)rawliteral";

#endif // REGISTRATION_HTML_H