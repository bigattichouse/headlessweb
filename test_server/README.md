# HeadlessWeb Upload Test Server

This Node.js server provides a controlled environment for testing file upload functionality in HeadlessWeb.

## Features

- Single file uploads
- Multiple file uploads
- Drag & drop file uploads
- Upload progress monitoring
- Upload status tracking
- File size and type validation
- CORS enabled for cross-origin requests

## Usage

### Starting the Server

```bash
./start_test_server.sh
```

Or manually:

```bash
cd test_server
npm install
node upload-server.js
```

The server will start on port 9876: http://localhost:9876

### API Endpoints

- `GET /` - Main upload test page
- `POST /upload/single` - Single file upload endpoint
- `POST /upload/multiple` - Multiple file upload endpoint  
- `GET /upload/status/:uploadId` - Check upload status
- `GET /uploads` - List all uploads
- `DELETE /uploads` - Clear upload history
- `GET /health` - Health check

### Test Integration

The C++ upload manager tests automatically use this server when running. Make sure to start the server before running upload tests:

```bash
# Terminal 1: Start the test server
./test_server/start_test_server.sh

# Terminal 2: Run upload tests
cd tests && ./hweb_tests --gtest_filter="UploadManagerTest.*"
```

### File Limits

- Maximum file size: 10MB
- Maximum files per upload: 5
- All file types accepted for testing

## Dependencies

- express: Web server framework
- multer: File upload middleware
- cors: Cross-origin resource sharing

## Future Improvements

The long-term goal is to replace this Node.js server with a C++ implementation that integrates directly with the HeadlessWeb test suite.