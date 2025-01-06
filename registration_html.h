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
            font-family: Arial;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
        }
        .camera-container {
            position: relative;
            width: fit-content;
            margin: 20px auto;
        }
        #cameraView {
            width: 100%;
            max-width: 400px;
            display: block;
            border-radius: 8px;
        }
        #faceBox {
            position: absolute;
            border: 3px solid #4CAF50;
            display: none;
        }
        .form-group { margin-bottom: 20px; }
        button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 12px;
            width: 100%;
            border-radius: 4px;
            font-size: 16px;
            cursor: pointer;
        }
        button:disabled {
            background: #ccc;
            cursor: not-allowed;
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
        #enrollStatus {
            text-align: center;
            margin: 10px 0;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h2 style="text-align: center;">Owner Registration</h2>
    <form id="regForm">
        <div class="form-group">
            <input type="text" id="ownerName" placeholder="Owner Name" required>
            <input type="password" id="password" placeholder="Password" required>
            <input type="text" id="deviceId" readonly>
            <input type="text" id="location" placeholder="Location" required>
        </div>
        
        <div class="form-group">
            <h3 style="text-align: center;">Face Enrollment</h3>
            <div class="camera-container">
                <img id="cameraView" src="/stream">
                <div id="faceBox"></div>
            </div>
            <button type="button" id="enrollFace">Start Face Enrollment</button>
            <div id="enrollStatus">Look at the camera and press Start</div>
        </div>
        
        <button type="submit" id="submitBtn" disabled>Complete Registration</button>
    </form>
<script>
    // Server endpoints
    const hostname = window.location.hostname;
    const MAIN_SERVER = `http://${hostname}:80`;
    const STREAM_SERVER = `http://${hostname}:81`;

    // Initialize camera stream
    document.getElementById('cameraView').src = `${STREAM_SERVER}/stream`;

    // Global variables
    let samplesCollected = 0;
    const TOTAL_SAMPLES = 5;
    let enrollmentTimer = null;
    let faceSamples = [];

    // Set device ID
    document.getElementById('deviceId').value = `${hostname}_${Date.now()}`;

    // Face enrollment handler
    document.getElementById('enrollFace').onclick = async () => {
        console.log('Starting face enrollment...');
        const status = document.getElementById('enrollStatus');
        const enrollBtn = document.getElementById('enrollFace');
        const faceBox = document.getElementById('faceBox');
        
        try {
            // Enable face detection
            const detectRes = await fetch(`${MAIN_SERVER}/control?var=face_detect&val=1`);
            if (!detectRes.ok) throw new Error('Failed to enable detection');
            
            // Enable enrollment
            const enrollRes = await fetch(`${MAIN_SERVER}/control?var=face_enroll&val=1`);
            if (!enrollRes.ok) throw new Error('Failed to start enrollment');
            
            // Update UI
            enrollBtn.disabled = true;
            status.textContent = 'Starting enrollment... Position your face';
            faceBox.style.display = 'block';
            
            // Start capture loop
            captureLoop();
            
        } catch(err) {
            console.error('Enrollment error:', err);
            status.textContent = err.message;
            enrollBtn.disabled = false;
            faceBox.style.display = 'none';
        }
    };

    // Face capture loop
   async function captureLoop() {
    const status = document.getElementById('enrollStatus');
    
    try {
        while (samplesCollected < TOTAL_SAMPLES) {
            const statusRes = await fetch(`${MAIN_SERVER}/status`);
            const responseText = await statusRes.text();
            
            try {
                const data = JSON.parse(responseText);
                console.log('Status:', data);
                
                if (data.face_detect === 1) {
                    const captureRes = await fetch(`${MAIN_SERVER}/capture`);
                    if (captureRes.ok) {
                        const blob = await captureRes.blob();
                        faceSamples.push(await blobToBase64(blob));
                        samplesCollected++;
                        status.textContent = `Sample ${samplesCollected}/${TOTAL_SAMPLES} captured`;
                        await new Promise(resolve => setTimeout(resolve, 1000));
                    }
                }
            } catch (parseError) {
                console.error('Status response:', responseText);
                console.error('Parse error:', parseError);
                continue; // Skip this iteration on parse error
            }
            
            await new Promise(resolve => setTimeout(resolve, 500));
        }
        
        status.textContent = 'Face enrollment complete!';
        document.getElementById('submitBtn').disabled = false;
        document.getElementById('faceBox').style.display = 'none';
        
    } catch(err) {
        console.error('Capture error:', err);
        status.textContent = 'Enrollment failed: ' + err.message;
        document.getElementById('enrollFace').disabled = false;
    }
}

    // Blob to base64 converter
    function blobToBase64(blob) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onloadend = () => resolve(reader.result.split(',')[1]);
            reader.onerror = reject;
            reader.readAsDataURL(blob);
        });
    }

    // Form submission handler
    document.getElementById('regForm').onsubmit = async (e) => {
        e.preventDefault();
        
        if (samplesCollected < TOTAL_SAMPLES) {
            alert('Please complete face enrollment first');
            return;
        }
        
        const data = {
            ownerName: document.getElementById('ownerName').value,
            password: document.getElementById('password').value,
            deviceId: document.getElementById('deviceId').value,
            location: document.getElementById('location').value,
            faceSamples: faceSamples,
            enrolled: true
        };

        try {
            const res = await fetch(`${MAIN_SERVER}/register`, {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            });
            
            const result = await res.json();
            if (result.success) {
                alert('Registration successful!');
                window.location.href = '/';
            } else {
                throw new Error(result.message || 'Registration failed');
            }
        } catch(err) {
            console.error('Registration error:', err);
            alert('Registration failed: ' + err.message);
        }
    };
</script>
</body>
</html>
)rawliteral";

#endif // REGISTRATION_HTML_H