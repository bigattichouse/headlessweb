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

module.exports = app;