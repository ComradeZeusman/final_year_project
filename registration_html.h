const char REGISTRATION_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Face Registration</title>
    <style>
        body { font-family: Arial; max-width: 800px; margin: 0 auto; padding: 20px; background: #f5f5f5; }
        .camera-container { position: relative; width: fit-content; margin: 20px auto; }
        #cameraView { width: 100%; max-width: 400px; display: block; border-radius: 8px; }
        button { background: #4CAF50; color: white; border: none; padding: 12px; width: 100%; 
                 border-radius: 4px; cursor: pointer; margin: 10px 0; }
        button:disabled { background: #ccc; cursor: not-allowed; }
        input { display: block; margin: 10px 0; padding: 12px; width: 100%; border: 1px solid #ddd; }
        #status { text-align: center; margin: 10px 0; padding: 10px; }
        .loading { opacity: 0.7; pointer-events: none; }
        .spinner { display: none; position: absolute; top: 50%; left: 50%; 
                  transform: translate(-50%, -50%); width: 40px; height: 40px;
                  border: 4px solid #f3f3f3; border-top: 4px solid #4CAF50;
                  border-radius: 50%; animation: spin 1s linear infinite; }
        .success { color: #4CAF50; }
        .error { color: #f44336; }
        @keyframes spin { 
            0% { transform: translate(-50%, -50%) rotate(0deg); }
            100% { transform: translate(-50%, -50%) rotate(360deg); }
        }
    </style>
</head>
<body>
    <form id="regForm">
        <h2>Face Registration</h2>
        <input type="text" id="ownerName" placeholder="Owner Name" required>
        <input type="password" id="password" placeholder="Password" required>
        
        <div class="camera-container">
            <div class="spinner" id="spinner"></div>
            <img id="cameraView" src="/stream">
        </div>
        <div id="status">Position your face and click Capture</div>
        <button type="button" id="captureBtn">Capture Face</button>
        <button type="submit" id="submitBtn" disabled>Complete Registration</button>
    </form>

<script>
const MAIN_SERVER = `http://${window.location.hostname}:80`;
let faceImage = null;

function setUI(loading) {
    document.getElementById('captureBtn').disabled = loading;
    document.getElementById('submitBtn').disabled = !faceImage || loading;
    document.getElementById('spinner').style.display = loading ? 'block' : 'none';
    document.getElementById('cameraView').classList.toggle('loading', loading);
}

function showStatus(msg, type = '') {
    const status = document.getElementById('status');
    status.textContent = msg;
    status.className = type;
}

async function captureFace() {
    try {
        setUI(true);
        showStatus('Initializing detection...');

        // Reset and enable detection
        await fetch(`${MAIN_SERVER}/control?var=face_detect&val=0`);
        await new Promise(resolve => setTimeout(resolve, 100));
        await fetch(`${MAIN_SERVER}/control?var=face_detect&val=1`);
        await new Promise(resolve => setTimeout(resolve, 500));

        showStatus('Looking for face...');
        const capture = await fetch(`${MAIN_SERVER}/capture`);
        
        if (!capture.ok) {
            const error = await capture.text();
            throw new Error(error || 'Capture failed');
        }

        faceImage = await capture.blob().then(blob => new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => resolve(reader.result.split(',')[1]);
            reader.onerror = reject;
            reader.readAsDataURL(blob);
        }));

        showStatus('Face captured!', 'success');
        document.getElementById('submitBtn').disabled = false;
    } catch (err) {
        showStatus(err.message, 'error');
        console.error('Capture error:', err);
    } finally {
        setUI(false);
    }
}

async function submitRegistration(e) {
    e.preventDefault();
    if (!faceImage) {
        showStatus('Please capture face first', 'error');
        return;
    }

    try {
        setUI(true);
        showStatus('Registering...');

        const res = await fetch(`${MAIN_SERVER}/register`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                ownerName: document.getElementById('ownerName').value,
                password: document.getElementById('password').value,
                deviceId: window.location.hostname,
                faceData: faceImage
            })
        });

        const data = await res.json();
        if (data.success) {
            showStatus('Success! Redirecting...', 'success');
            setTimeout(() => window.location.href = '/', 1500);
        } else {
            throw new Error(data.message || 'Registration failed');
        }
    } catch (err) {
        showStatus(err.message, 'error');
        console.error(err);
    } finally {
        setUI(false);
    }
}

document.getElementById('captureBtn').onclick = captureFace;
document.getElementById('regForm').onsubmit = submitRegistration;
</script>
</body>
</html>
)rawliteral";