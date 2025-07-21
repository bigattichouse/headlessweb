const express = require('express');
const multer = require('multer');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 9876;

// Enable CORS for all routes
app.use(cors());

// Parse JSON bodies
app.use(express.json());

// Serve static files from public directory
app.use(express.static(path.join(__dirname, 'public')));

// Configure multer for file uploads
const storage = multer.diskStorage({
  destination: function (req, file, cb) {
    const uploadDir = path.join(__dirname, 'uploads');
    if (!fs.existsSync(uploadDir)) {
      fs.mkdirSync(uploadDir, { recursive: true });
    }
    cb(null, uploadDir);
  },
  filename: function (req, file, cb) {
    // Generate unique filename with timestamp
    const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
    cb(null, file.fieldname + '-' + uniqueSuffix + path.extname(file.originalname));
  }
});

const upload = multer({ 
  storage: storage,
  limits: {
    fileSize: 10 * 1024 * 1024, // 10MB limit
    files: 5 // Maximum 5 files
  },
  fileFilter: function (req, file, cb) {
    // Accept all file types for testing
    cb(null, true);
  }
});

// Upload status tracking
const uploadStatus = new Map();

// Routes

// Test page route
app.get('/', (req, res) => {
  res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>Upload Test Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .upload-area { border: 2px dashed #ccc; padding: 20px; margin: 20px 0; }
        .upload-area:hover { border-color: #999; }
        .status { margin: 10px 0; font-weight: bold; }
        .success { color: green; }
        .error { color: red; }
        .progress-bar { width: 100%; height: 20px; background: #f0f0f0; margin: 10px 0; }
        .progress-fill { height: 100%; background: #4CAF50; width: 0%; transition: width 0.3s; }
    </style>
</head>
<body>
    <h1>HeadlessWeb Upload Test Server</h1>
    
    <h2>Single File Upload</h2>
    <form id="single-upload-form" enctype="multipart/form-data">
        <input type="file" id="file-input" name="file" />
        <button type="submit">Upload</button>
    </form>
    
    <h2>Multiple File Upload</h2>
    <form id="multiple-upload-form" enctype="multipart/form-data">
        <input type="file" id="multiple-input" name="files" multiple />
        <button type="submit">Upload Multiple</button>
    </form>
    
    <h2>Drag & Drop Upload</h2>
    <div id="drop-zone" class="upload-area">
        Drop files here or click to select
        <input type="file" id="drop-input" name="files" multiple style="display: none;" />
    </div>
    
    <div id="upload-status" class="status">Ready</div>
    <div class="progress-bar">
        <div id="progress-fill" class="progress-fill"></div>
    </div>
    
    <h2>Upload History</h2>
    <div id="upload-history"></div>

    <script>
        function updateStatus(message, isError = false) {
            const statusDiv = document.getElementById('upload-status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + (isError ? 'error' : 'success');
        }
        
        function updateProgress(percent) {
            document.getElementById('progress-fill').style.width = percent + '%';
        }
        
        function addToHistory(filename, status) {
            const historyDiv = document.getElementById('upload-history');
            const entry = document.createElement('div');
            entry.innerHTML = \`<strong>\${filename}</strong>: \${status}\`;
            historyDiv.insertBefore(entry, historyDiv.firstChild);
        }
        
        // Single file upload
        document.getElementById('single-upload-form').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData();
            const fileInput = document.getElementById('file-input');
            
            if (fileInput.files.length === 0) {
                updateStatus('Please select a file', true);
                return;
            }
            
            formData.append('file', fileInput.files[0]);
            uploadFile('/upload/single', formData);
        });
        
        // Multiple file upload
        document.getElementById('multiple-upload-form').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData();
            const fileInput = document.getElementById('multiple-input');
            
            if (fileInput.files.length === 0) {
                updateStatus('Please select files', true);
                return;
            }
            
            for (let file of fileInput.files) {
                formData.append('files', file);
            }
            uploadFile('/upload/multiple', formData);
        });
        
        // Drag and drop functionality
        const dropZone = document.getElementById('drop-zone');
        const dropInput = document.getElementById('drop-input');
        
        dropZone.addEventListener('click', () => dropInput.click());
        
        dropZone.addEventListener('dragover', function(e) {
            e.preventDefault();
            dropZone.style.backgroundColor = '#f0f0f0';
        });
        
        dropZone.addEventListener('dragleave', function(e) {
            dropZone.style.backgroundColor = 'white';
        });
        
        dropZone.addEventListener('drop', function(e) {
            e.preventDefault();
            dropZone.style.backgroundColor = 'white';
            
            const files = e.dataTransfer.files;
            if (files.length > 0) {
                const formData = new FormData();
                for (let file of files) {
                    formData.append('files', file);
                }
                updateStatus('Files dropped, uploading...');
                uploadFile('/upload/multiple', formData);
            }
        });
        
        function uploadFile(endpoint, formData) {
            updateStatus('Uploading...');
            updateProgress(0);
            
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    updateProgress(percentComplete);
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    const response = JSON.parse(xhr.responseText);
                    updateStatus('Upload complete');
                    updateProgress(100);
                    
                    if (response.files) {
                        response.files.forEach(file => {
                            addToHistory(file.originalname, 'Success');
                        });
                    } else if (response.file) {
                        addToHistory(response.file.originalname, 'Success');
                    }
                } else {
                    updateStatus('Upload failed', true);
                    updateProgress(0);
                }
            });
            
            xhr.addEventListener('error', function() {
                updateStatus('Upload error', true);
                updateProgress(0);
            });
            
            xhr.open('POST', endpoint);
            xhr.send(formData);
        }
        
        // Utility functions for testing
        function simulateUploadComplete() {
            updateStatus('Upload complete');
            return true;
        }
        
        function getUploadProgress() {
            const progressFill = document.getElementById('progress-fill');
            return parseInt(progressFill.style.width) || 0;
        }
        
        function hasFileInputs() {
            return document.querySelectorAll('input[type="file"]').length > 0;
        }
        
        function findFileInputs() {
            const inputs = Array.from(document.querySelectorAll('input[type="file"]'));
            return JSON.stringify(inputs.map(input => '#' + input.id));
        }
        
        function elementExists(selector) {
            return document.querySelector(selector) !== null;
        }
    </script>
</body>
</html>
  `);
});

// Single file upload endpoint
app.post('/upload/single', upload.single('file'), (req, res) => {
  try {
    if (!req.file) {
      return res.status(400).json({ error: 'No file uploaded' });
    }
    
    const uploadId = Date.now().toString();
    uploadStatus.set(uploadId, {
      status: 'complete',
      filename: req.file.originalname,
      size: req.file.size,
      timestamp: new Date()
    });
    
    res.json({
      success: true,
      uploadId: uploadId,
      file: {
        originalname: req.file.originalname,
        filename: req.file.filename,
        size: req.file.size,
        mimetype: req.file.mimetype
      }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Multiple file upload endpoint
app.post('/upload/multiple', upload.array('files', 5), (req, res) => {
  try {
    if (!req.files || req.files.length === 0) {
      return res.status(400).json({ error: 'No files uploaded' });
    }
    
    const uploadId = Date.now().toString();
    uploadStatus.set(uploadId, {
      status: 'complete',
      fileCount: req.files.length,
      timestamp: new Date()
    });
    
    res.json({
      success: true,
      uploadId: uploadId,
      files: req.files.map(file => ({
        originalname: file.originalname,
        filename: file.filename,
        size: file.size,
        mimetype: file.mimetype
      }))
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Upload status endpoint
app.get('/upload/status/:uploadId', (req, res) => {
  const uploadId = req.params.uploadId;
  const status = uploadStatus.get(uploadId);
  
  if (!status) {
    return res.status(404).json({ error: 'Upload not found' });
  }
  
  res.json(status);
});

// List all uploads endpoint
app.get('/uploads', (req, res) => {
  const uploads = Array.from(uploadStatus.entries()).map(([id, status]) => ({
    id,
    ...status
  }));
  
  res.json({ uploads });
});

// Clear upload history endpoint
app.delete('/uploads', (req, res) => {
  uploadStatus.clear();
  res.json({ success: true, message: 'Upload history cleared' });
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date(),
    uploadsCount: uploadStatus.size 
  });
});

// Error handling middleware
app.use((error, req, res, next) => {
  if (error instanceof multer.MulterError) {
    if (error.code === 'LIMIT_FILE_SIZE') {
      return res.status(400).json({ error: 'File too large' });
    }
    if (error.code === 'LIMIT_FILE_COUNT') {
      return res.status(400).json({ error: 'Too many files' });
    }
  }
  
  res.status(500).json({ error: error.message });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({ error: 'Route not found' });
});

// Start server
app.listen(PORT, () => {
  console.log(`Upload test server running on http://localhost:${PORT}`);
  console.log(`Upload endpoint: http://localhost:${PORT}/upload/single`);
  console.log(`Multiple upload endpoint: http://localhost:${PORT}/upload/multiple`);
  console.log(`Health check: http://localhost:${PORT}/health`);
});

// ========== Advanced Testing Endpoints ==========

// Slow response simulation endpoint
app.get('/slow/:delay', (req, res) => {
  const delay = parseInt(req.params.delay) || 1000;
  const maxDelay = 10000; // 10 second maximum
  const actualDelay = Math.min(delay, maxDelay);
  
  setTimeout(() => {
    res.json({ 
      message: 'Delayed response completed', 
      delay: actualDelay,
      timestamp: new Date().toISOString()
    });
  }, actualDelay);
});

// Memory stress testing endpoint
app.get('/large-content/:size', (req, res) => {
  const size = parseInt(req.params.size) || 1024;
  const maxSize = 10240; // 10MB maximum for safety
  const actualSize = Math.min(size, maxSize);
  
  // Generate content of specified size in KB
  const chunk = 'x'.repeat(1024); // 1KB chunk
  let content = '';
  for (let i = 0; i < actualSize; i++) {
    content += chunk;
  }
  
  res.send(`
    <!DOCTYPE html>
    <html>
    <head>
        <title>Large Content Test (${actualSize}KB)</title>
    </head>
    <body>
        <h1>Large Content Test</h1>
        <p>Content size: ${actualSize}KB</p>
        <div id="large-content">
            <pre>${content}</pre>
        </div>
        <script>
            // Add some interactivity to large content
            document.getElementById('large-content').onclick = function() {
                this.style.backgroundColor = this.style.backgroundColor === 'yellow' ? 'white' : 'yellow';
            };
        </script>
    </body>
    </html>
  `);
});

// Complex form validation endpoint
app.post('/validate-form', express.urlencoded({ extended: true }), (req, res) => {
  const errors = [];
  const warnings = [];
  
  // Username validation
  if (!req.body.username) {
    errors.push('Username is required');
  } else if (req.body.username.length < 3) {
    errors.push('Username must be at least 3 characters');
  } else if (req.body.username.length > 20) {
    errors.push('Username must be less than 20 characters');
  } else if (!/^[a-zA-Z0-9_]+$/.test(req.body.username)) {
    errors.push('Username can only contain letters, numbers, and underscores');
  }
  
  // Email validation
  if (!req.body.email) {
    errors.push('Email is required');
  } else if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(req.body.email)) {
    errors.push('Email format is invalid');
  }
  
  // Password validation
  if (!req.body.password) {
    errors.push('Password is required');
  } else {
    if (req.body.password.length < 8) {
      errors.push('Password must be at least 8 characters');
    }
    if (!/(?=.*[a-z])/.test(req.body.password)) {
      warnings.push('Password should contain lowercase letters');
    }
    if (!/(?=.*[A-Z])/.test(req.body.password)) {
      warnings.push('Password should contain uppercase letters');
    }
    if (!/(?=.*\d)/.test(req.body.password)) {
      warnings.push('Password should contain numbers');
    }
    if (!/(?=.*[@$!%*?&])/.test(req.body.password)) {
      warnings.push('Password should contain special characters');
    }
  }
  
  // Confirm password validation
  if (req.body.password && req.body.confirm_password && 
      req.body.password !== req.body.confirm_password) {
    errors.push('Passwords do not match');
  }
  
  // Country/State validation
  if (req.body.country === 'us' || req.body.country === 'ca') {
    if (!req.body.state) {
      errors.push('State/Province is required for selected country');
    }
  }
  
  const response = {
    valid: errors.length === 0,
    errors: errors,
    warnings: warnings,
    data: req.body,
    validation_timestamp: new Date().toISOString()
  };
  
  // Return appropriate HTTP status
  if (errors.length > 0) {
    res.status(400).json(response);
  } else {
    res.json(response);
  }
});

// Session management and testing endpoint
app.get('/session-test', (req, res) => {
  res.send(`
    <!DOCTYPE html>
    <html>
    <head>
        <title>Session Management Test</title>
    </head>
    <body>
        <h1>Session Management Test</h1>
        <div id="session-status">Initializing...</div>
        
        <button onclick="setSessionData()">Set Session Data</button>
        <button onclick="getSessionData()">Get Session Data</button>
        <button onclick="clearSessionData()">Clear Session Data</button>
        
        <h2>Current Session Data:</h2>
        <div id="session-data"></div>
        
        <h2>Test Actions:</h2>
        <button onclick="simulateFormData()">Simulate Form Data</button>
        <button onclick="simulateMultiplePages()">Simulate Page Navigation</button>
        
        <div id="form-container" style="display:none;">
            <h3>Test Form</h3>
            <form id="session-form">
                <input type="text" id="session-name" placeholder="Name" value="">
                <input type="email" id="session-email" placeholder="Email" value="">
                <select id="session-preference">
                    <option value="option1">Option 1</option>
                    <option value="option2">Option 2</option>
                    <option value="option3">Option 3</option>
                </select>
                <textarea id="session-notes" placeholder="Notes"></textarea>
            </form>
        </div>
        
        <script>
            function updateStatus(message) {
                document.getElementById('session-status').textContent = message;
            }
            
            function setSessionData() {
                // Set various types of session data
                localStorage.setItem('test-key', 'test-value-' + Date.now());
                localStorage.setItem('user-preferences', JSON.stringify({
                    theme: 'dark',
                    language: 'en',
                    notifications: true
                }));
                
                sessionStorage.setItem('session-key', 'session-value-' + Date.now());
                sessionStorage.setItem('temp-data', JSON.stringify({
                    page: 'session-test',
                    visited: new Date().toISOString()
                }));
                
                // Set cookies
                document.cookie = 'test-cookie=cookie-value-' + Date.now() + '; path=/';
                document.cookie = 'persistent-cookie=persistent-value; path=/; max-age=3600';
                
                updateStatus('Session data set successfully');
                getSessionData(); // Refresh display
            }
            
            function getSessionData() {
                const sessionData = {
                    localStorage: {
                        'test-key': localStorage.getItem('test-key'),
                        'user-preferences': localStorage.getItem('user-preferences')
                    },
                    sessionStorage: {
                        'session-key': sessionStorage.getItem('session-key'),
                        'temp-data': sessionStorage.getItem('temp-data')
                    },
                    cookies: document.cookie
                };
                
                document.getElementById('session-data').innerHTML = 
                    '<pre>' + JSON.stringify(sessionData, null, 2) + '</pre>';
                updateStatus('Session data retrieved');
            }
            
            function clearSessionData() {
                localStorage.clear();
                sessionStorage.clear();
                
                // Clear cookies
                document.cookie.split(";").forEach(function(c) { 
                    document.cookie = c.replace(/^ +/, "").replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/"); 
                });
                
                updateStatus('Session data cleared');
                document.getElementById('session-data').innerHTML = '';
            }
            
            function simulateFormData() {
                document.getElementById('form-container').style.display = 'block';
                document.getElementById('session-name').value = 'Test User ' + Math.floor(Math.random() * 100);
                document.getElementById('session-email').value = 'test' + Math.floor(Math.random() * 100) + '@example.com';
                document.getElementById('session-preference').value = 'option2';
                document.getElementById('session-notes').value = 'Test notes created at ' + new Date().toLocaleString();
                
                updateStatus('Form data simulated');
            }
            
            function simulateMultiplePages() {
                // Simulate navigation through multiple pages
                let pageCounter = 1;
                const maxPages = 5;
                
                const simulatePageVisit = () => {
                    sessionStorage.setItem('current-page', pageCounter);
                    sessionStorage.setItem('page-history', JSON.stringify(
                        JSON.parse(sessionStorage.getItem('page-history') || '[]').concat(['page-' + pageCounter])
                    ));
                    
                    updateStatus('Simulated visit to page ' + pageCounter + ' of ' + maxPages);
                    
                    pageCounter++;
                    if (pageCounter <= maxPages) {
                        setTimeout(simulatePageVisit, 500);
                    } else {
                        updateStatus('Page navigation simulation complete');
                        getSessionData();
                    }
                };
                
                sessionStorage.removeItem('page-history');
                simulatePageVisit();
            }
            
            // Initialize on load
            window.onload = function() {
                updateStatus('Session test page loaded');
                getSessionData();
            };
        </script>
    </body>
    </html>
  `);
});

// Dynamic content testing endpoint
app.get('/dynamic-content', (req, res) => {
  res.send(`
    <!DOCTYPE html>
    <html>
    <head>
        <title>Dynamic Content Test</title>
        <style>
            .content-block { 
                border: 1px solid #ccc; 
                margin: 10px; 
                padding: 10px; 
                min-height: 50px;
            }
            .loading { background-color: #ffffcc; }
            .loaded { background-color: #ccffcc; }
            .error { background-color: #ffcccc; }
        </style>
    </head>
    <body>
        <h1>Dynamic Content Test</h1>
        
        <button onclick="loadDynamicContent()">Load Dynamic Content</button>
        <button onclick="simulateSlowLoad()">Simulate Slow Load</button>
        <button onclick="simulateError()">Simulate Error</button>
        <button onclick="clearContent()">Clear Content</button>
        
        <div id="dynamic-container">
            <div class="content-block" id="content-1">Content Block 1 - Static</div>
        </div>
        
        <div id="status">Ready</div>
        
        <script>
            let contentCounter = 1;
            
            function loadDynamicContent() {
                contentCounter++;
                const container = document.getElementById('dynamic-container');
                
                const newBlock = document.createElement('div');
                newBlock.className = 'content-block loading';
                newBlock.id = 'content-' + contentCounter;
                newBlock.textContent = 'Loading content block ' + contentCounter + '...';
                
                container.appendChild(newBlock);
                
                // Simulate async loading
                setTimeout(() => {
                    newBlock.className = 'content-block loaded';
                    newBlock.textContent = 'Content Block ' + contentCounter + ' - Loaded at ' + new Date().toLocaleTimeString();
                    updateStatus('Content block ' + contentCounter + ' loaded');
                }, Math.random() * 1000 + 500);
            }
            
            function simulateSlowLoad() {
                contentCounter++;
                const container = document.getElementById('dynamic-container');
                
                const slowBlock = document.createElement('div');
                slowBlock.className = 'content-block loading';
                slowBlock.id = 'content-' + contentCounter;
                slowBlock.textContent = 'Loading slow content block ' + contentCounter + '...';
                
                container.appendChild(slowBlock);
                updateStatus('Starting slow load...');
                
                // Simulate very slow loading (5 seconds)
                setTimeout(() => {
                    slowBlock.className = 'content-block loaded';
                    slowBlock.textContent = 'Slow Content Block ' + contentCounter + ' - Finally loaded after 5 seconds!';
                    updateStatus('Slow content loaded');
                }, 5000);
            }
            
            function simulateError() {
                contentCounter++;
                const container = document.getElementById('dynamic-container');
                
                const errorBlock = document.createElement('div');
                errorBlock.className = 'content-block loading';
                errorBlock.id = 'content-' + contentCounter;
                errorBlock.textContent = 'Loading content block ' + contentCounter + '...';
                
                container.appendChild(errorBlock);
                
                // Simulate loading failure
                setTimeout(() => {
                    errorBlock.className = 'content-block error';
                    errorBlock.textContent = 'Error: Failed to load content block ' + contentCounter;
                    updateStatus('Content load failed');
                }, Math.random() * 1000 + 500);
            }
            
            function clearContent() {
                const container = document.getElementById('dynamic-container');
                // Keep only the first static block
                while (container.children.length > 1) {
                    container.removeChild(container.lastChild);
                }
                contentCounter = 1;
                updateStatus('Content cleared');
            }
            
            function updateStatus(message) {
                document.getElementById('status').textContent = message + ' at ' + new Date().toLocaleTimeString();
            }
        </script>
    </body>
    </html>
  `);
});

// Complex form testing endpoint
app.get('/complex-form', (req, res) => {
  res.send(`
    <!DOCTYPE html>
    <html>
    <head>
        <title>Complex Form Test</title>
        <style>
            .form-step { display: none; }
            .form-step.active { display: block; }
            .error { color: red; font-size: 12px; }
            .valid { border: 2px solid green; }
            .invalid { border: 2px solid red; }
            fieldset { margin: 10px 0; }
        </style>
    </head>
    <body>
        <h1>Complex Multi-Step Form</h1>
        
        <div id="progress-indicator">
            Step <span id="current-step">1</span> of 3
        </div>
        
        <form id="complex-form" action="/validate-form" method="post">
            <!-- Step 1: Personal Information -->
            <div class="form-step active" id="step-1">
                <h2>Personal Information</h2>
                <fieldset>
                    <label for="username">Username:</label>
                    <input type="text" id="username" name="username" required>
                    <div class="error" id="username-error"></div>
                    
                    <label for="email">Email:</label>
                    <input type="email" id="email" name="email" required>
                    <div class="error" id="email-error"></div>
                    
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" required>
                    <div class="error" id="password-error"></div>
                    
                    <label for="confirm_password">Confirm Password:</label>
                    <input type="password" id="confirm_password" name="confirm_password" required>
                    <div class="error" id="confirm_password-error"></div>
                </fieldset>
                
                <button type="button" onclick="nextStep(2)">Next</button>
            </div>
            
            <!-- Step 2: Address Information -->
            <div class="form-step" id="step-2">
                <h2>Address Information</h2>
                <fieldset>
                    <label for="country">Country:</label>
                    <select id="country" name="country" onchange="updateStates()" required>
                        <option value="">Select Country</option>
                        <option value="us">United States</option>
                        <option value="ca">Canada</option>
                        <option value="uk">United Kingdom</option>
                    </select>
                    
                    <div id="state-container" style="display:none;">
                        <label for="state">State/Province:</label>
                        <select id="state" name="state">
                            <option value="">Select State/Province</option>
                        </select>
                    </div>
                    
                    <label for="address">Address:</label>
                    <input type="text" id="address" name="address" required>
                    
                    <label for="city">City:</label>
                    <input type="text" id="city" name="city" required>
                    
                    <label for="postal">Postal Code:</label>
                    <input type="text" id="postal" name="postal" required>
                </fieldset>
                
                <button type="button" onclick="prevStep(1)">Previous</button>
                <button type="button" onclick="nextStep(3)">Next</button>
            </div>
            
            <!-- Step 3: Preferences -->
            <div class="form-step" id="step-3">
                <h2>Preferences</h2>
                <fieldset>
                    <legend>Newsletter Subscriptions:</legend>
                    <label><input type="checkbox" name="newsletters[]" value="general"> General News</label><br>
                    <label><input type="checkbox" name="newsletters[]" value="tech"> Tech Updates</label><br>
                    <label><input type="checkbox" name="newsletters[]" value="promotions"> Promotions</label><br>
                </fieldset>
                
                <fieldset>
                    <legend>Communication Preference:</legend>
                    <label><input type="radio" name="communication" value="email" checked> Email</label><br>
                    <label><input type="radio" name="communication" value="sms"> SMS</label><br>
                    <label><input type="radio" name="communication" value="phone"> Phone</label><br>
                </fieldset>
                
                <label for="bio">Bio (optional):</label>
                <textarea id="bio" name="bio" rows="4" cols="50"></textarea>
                
                <button type="button" onclick="prevStep(2)">Previous</button>
                <button type="submit">Submit Form</button>
            </div>
        </form>
        
        <div id="form-result" style="display:none;">
            <h2>Form Submission Result</h2>
            <div id="result-content"></div>
        </div>
        
        <script>
            function showStep(stepNumber) {
                // Hide all steps
                document.querySelectorAll('.form-step').forEach(step => {
                    step.classList.remove('active');
                });
                
                // Show target step
                document.getElementById('step-' + stepNumber).classList.add('active');
                document.getElementById('current-step').textContent = stepNumber;
            }
            
            function nextStep(stepNumber) {
                if (validateCurrentStep()) {
                    showStep(stepNumber);
                }
            }
            
            function prevStep(stepNumber) {
                showStep(stepNumber);
            }
            
            function validateCurrentStep() {
                const currentStep = parseInt(document.getElementById('current-step').textContent);
                let isValid = true;
                
                if (currentStep === 1) {
                    isValid = validatePersonalInfo();
                } else if (currentStep === 2) {
                    isValid = validateAddressInfo();
                }
                
                return isValid;
            }
            
            function validatePersonalInfo() {
                let isValid = true;
                
                // Clear previous errors
                document.querySelectorAll('.error').forEach(error => error.textContent = '');
                document.querySelectorAll('input').forEach(input => {
                    input.classList.remove('valid', 'invalid');
                });
                
                // Username validation
                const username = document.getElementById('username').value;
                if (!username || username.length < 3) {
                    document.getElementById('username-error').textContent = 'Username must be at least 3 characters';
                    document.getElementById('username').classList.add('invalid');
                    isValid = false;
                } else {
                    document.getElementById('username').classList.add('valid');
                }
                
                // Email validation
                const email = document.getElementById('email').value;
                const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
                if (!email || !emailRegex.test(email)) {
                    document.getElementById('email-error').textContent = 'Valid email is required';
                    document.getElementById('email').classList.add('invalid');
                    isValid = false;
                } else {
                    document.getElementById('email').classList.add('valid');
                }
                
                // Password validation
                const password = document.getElementById('password').value;
                const confirmPassword = document.getElementById('confirm_password').value;
                
                if (!password || password.length < 8) {
                    document.getElementById('password-error').textContent = 'Password must be at least 8 characters';
                    document.getElementById('password').classList.add('invalid');
                    isValid = false;
                } else {
                    document.getElementById('password').classList.add('valid');
                }
                
                if (password !== confirmPassword) {
                    document.getElementById('confirm_password-error').textContent = 'Passwords do not match';
                    document.getElementById('confirm_password').classList.add('invalid');
                    isValid = false;
                } else if (confirmPassword) {
                    document.getElementById('confirm_password').classList.add('valid');
                }
                
                return isValid;
            }
            
            function validateAddressInfo() {
                let isValid = true;
                const country = document.getElementById('country').value;
                
                if (!country) {
                    isValid = false;
                }
                
                if ((country === 'us' || country === 'ca') && !document.getElementById('state').value) {
                    isValid = false;
                }
                
                return isValid;
            }
            
            function updateStates() {
                const country = document.getElementById('country').value;
                const stateContainer = document.getElementById('state-container');
                const stateSelect = document.getElementById('state');
                
                if (country === 'us') {
                    stateContainer.style.display = 'block';
                    stateSelect.innerHTML = \`
                        <option value="">Select State</option>
                        <option value="ca">California</option>
                        <option value="ny">New York</option>
                        <option value="tx">Texas</option>
                        <option value="fl">Florida</option>
                    \`;
                } else if (country === 'ca') {
                    stateContainer.style.display = 'block';
                    stateSelect.innerHTML = \`
                        <option value="">Select Province</option>
                        <option value="on">Ontario</option>
                        <option value="bc">British Columbia</option>
                        <option value="ab">Alberta</option>
                        <option value="qc">Quebec</option>
                    \`;
                } else {
                    stateContainer.style.display = 'none';
                    stateSelect.innerHTML = '<option value="">N/A</option>';
                }
            }
            
            // Form submission handling
            document.getElementById('complex-form').onsubmit = function(e) {
                e.preventDefault();
                
                const formData = new FormData(this);
                
                fetch('/validate-form', {
                    method: 'POST',
                    body: formData
                })
                .then(response => response.json())
                .then(data => {
                    document.getElementById('form-result').style.display = 'block';
                    document.getElementById('result-content').innerHTML = \`
                        <h3>Validation Result</h3>
                        <p><strong>Valid:</strong> \${data.valid ? 'Yes' : 'No'}</p>
                        \${data.errors.length > 0 ? '<h4>Errors:</h4><ul>' + data.errors.map(e => '<li>' + e + '</li>').join('') + '</ul>' : ''}
                        \${data.warnings.length > 0 ? '<h4>Warnings:</h4><ul>' + data.warnings.map(w => '<li>' + w + '</li>').join('') + '</ul>' : ''}
                        <pre>\${JSON.stringify(data.data, null, 2)}</pre>
                    \`;
                    
                    // Scroll to result
                    document.getElementById('form-result').scrollIntoView();
                })
                .catch(error => {
                    document.getElementById('form-result').style.display = 'block';
                    document.getElementById('result-content').innerHTML = '<p style="color: red;">Error: ' + error.message + '</p>';
                });
            };
        </script>
    </body>
    </html>
  `);
});

// Performance testing endpoint
app.get('/performance-test/:operations?', (req, res) => {
  const operations = parseInt(req.params.operations) || 100;
  const maxOperations = 1000; // Safety limit
  const actualOperations = Math.min(operations, maxOperations);
  
  res.send(`
    <!DOCTYPE html>
    <html>
    <head>
        <title>Performance Test (${actualOperations} operations)</title>
    </head>
    <body>
        <h1>Performance Test Page</h1>
        <p>Target operations: ${actualOperations}</p>
        
        <button onclick="runPerformanceTest()">Run Performance Test</button>
        <button onclick="runMemoryTest()">Run Memory Test</button>
        <button onclick="resetTest()">Reset</button>
        
        <div id="performance-results">
            <h2>Results</h2>
            <div id="operation-count">Operations completed: 0</div>
            <div id="elapsed-time">Elapsed time: 0ms</div>
            <div id="operations-per-second">Operations/second: 0</div>
            <div id="memory-usage">Memory usage: N/A</div>
        </div>
        
        <div id="test-area"></div>
        
        <script>
            let testData = [];
            let testStartTime;
            
            function runPerformanceTest() {
                const targetOps = ${actualOperations};
                testStartTime = performance.now();
                testData = [];
                
                const testArea = document.getElementById('test-area');
                testArea.innerHTML = '';
                
                for (let i = 0; i < targetOps; i++) {
                    // Simulate DOM operations
                    const element = document.createElement('div');
                    element.id = 'test-element-' + i;
                    element.textContent = 'Test element ' + i + ' created at ' + Date.now();
                    element.style.display = i % 10 === 0 ? 'block' : 'none'; // Only show every 10th
                    
                    testArea.appendChild(element);
                    testData.push({
                        id: i,
                        timestamp: performance.now(),
                        data: 'test-data-' + i
                    });
                    
                    // Update progress periodically
                    if (i % 10 === 0 || i === targetOps - 1) {
                        updatePerformanceResults(i + 1, targetOps);
                    }
                }
                
                const endTime = performance.now();
                const totalTime = endTime - testStartTime;
                const opsPerSecond = (targetOps / totalTime) * 1000;
                
                document.getElementById('operation-count').textContent = 'Operations completed: ' + targetOps;
                document.getElementById('elapsed-time').textContent = 'Elapsed time: ' + totalTime.toFixed(2) + 'ms';
                document.getElementById('operations-per-second').textContent = 'Operations/second: ' + opsPerSecond.toFixed(2);
                
                updateMemoryUsage();
            }
            
            function runMemoryTest() {
                // Create large objects to test memory usage
                const largeArray = [];
                const chunkSize = 10000;
                
                for (let i = 0; i < 10; i++) {
                    const chunk = new Array(chunkSize);
                    for (let j = 0; j < chunkSize; j++) {
                        chunk[j] = {
                            id: i * chunkSize + j,
                            data: 'memory-test-data-' + Math.random().toString(36),
                            timestamp: Date.now(),
                            randomValues: new Array(10).fill().map(() => Math.random())
                        };
                    }
                    largeArray.push(chunk);
                    
                    // Update memory usage
                    setTimeout(() => updateMemoryUsage(), 100 * i);
                }
                
                // Store reference to prevent garbage collection during test
                window.memoryTestData = largeArray;
            }
            
            function resetTest() {
                document.getElementById('test-area').innerHTML = '';
                testData = [];
                window.memoryTestData = null;
                
                document.getElementById('operation-count').textContent = 'Operations completed: 0';
                document.getElementById('elapsed-time').textContent = 'Elapsed time: 0ms';
                document.getElementById('operations-per-second').textContent = 'Operations/second: 0';
                updateMemoryUsage();
            }
            
            function updatePerformanceResults(completed, total) {
                const currentTime = performance.now();
                const elapsedTime = currentTime - testStartTime;
                const opsPerSecond = (completed / elapsedTime) * 1000;
                
                document.getElementById('operation-count').textContent = 'Operations completed: ' + completed + '/' + total;
                document.getElementById('elapsed-time').textContent = 'Elapsed time: ' + elapsedTime.toFixed(2) + 'ms';
                document.getElementById('operations-per-second').textContent = 'Operations/second: ' + opsPerSecond.toFixed(2);
            }
            
            function updateMemoryUsage() {
                if (performance.memory) {
                    const memory = performance.memory;
                    const memoryInfo = \`
                        Used: \${(memory.usedJSHeapSize / 1024 / 1024).toFixed(2)} MB, 
                        Total: \${(memory.totalJSHeapSize / 1024 / 1024).toFixed(2)} MB, 
                        Limit: \${(memory.jsHeapSizeLimit / 1024 / 1024).toFixed(2)} MB
                    \`;
                    document.getElementById('memory-usage').textContent = 'Memory usage: ' + memoryInfo;
                } else {
                    document.getElementById('memory-usage').textContent = 'Memory usage: Not available in this browser';
                }
            }
            
            // Update memory usage on page load
            window.onload = updateMemoryUsage;
        </script>
    </body>
    </html>
  `);
});

module.exports = app;