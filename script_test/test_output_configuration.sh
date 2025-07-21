#!/bin/bash

# HeadlessWeb Output and Configuration Integration Tests
# Tests JSON output modes, configuration processing, and output formatting

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_output_config_test_$$"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Utility functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

run_test() {
    ((TESTS_RUN++))
    echo -e "\n${BLUE}=== Test $TESTS_RUN: $1 ===${NC}"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment..."
    mkdir -p "$TEMP_DIR"
    mkdir -p "$TEMP_DIR/config"
    mkdir -p "$TEMP_DIR/output"
    mkdir -p "$TEMP_DIR/logs"
}

# Start test server
start_test_server() {
    log_info "Starting test server on port $TEST_SERVER_PORT..."
    
    if [ ! -f "$SERVER_SCRIPT" ]; then
        log_error "Test server script not found: $SERVER_SCRIPT"
        exit 1
    fi
    
    cd "$(dirname "$SERVER_SCRIPT")"
    
    if [ ! -d "node_modules" ]; then
        log_info "Installing Node.js dependencies..."
        npm install
    fi
    
    PORT=$TEST_SERVER_PORT node "$(basename "$SERVER_SCRIPT")" &
    SERVER_PID=$!
    
    sleep 2
    
    if ! curl -s "$TEST_SERVER_URL/health" > /dev/null; then
        log_error "Test server failed to start"
        exit 1
    fi
    
    log_info "Test server started (PID: $SERVER_PID)"
    cd - > /dev/null
}

# Stop test server
stop_test_server() {
    if [ ! -z "$SERVER_PID" ]; then
        log_info "Stopping test server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}

# Cleanup
cleanup() {
    log_info "Cleaning up..."
    stop_test_server
    rm -rf "$TEMP_DIR"
}

trap cleanup EXIT

# Helper function to validate JSON
validate_json() {
    local json_file="$1"
    if command -v jq >/dev/null 2>&1; then
        jq . "$json_file" >/dev/null 2>&1
        return $?
    else
        # Simple JSON validation without jq
        python3 -c "import json; json.load(open('$json_file'))" 2>/dev/null
        return $?
    fi
}

# Helper function to extract JSON value
get_json_value() {
    local json_file="$1"
    local key_path="$2"
    if command -v jq >/dev/null 2>&1; then
        jq -r "$key_path" "$json_file"
    else
        python3 -c "
import json
data = json.load(open('$json_file'))
keys = '$key_path'.strip('.').split('.')
result = data
for key in keys:
    result = result[key]
print(result)
"
    fi
}

# ========== JSON Output Mode Tests ==========

test_json_output_mode_basic() {
    run_test "Basic JSON Output Mode"
    
    local output_file="$TEMP_DIR/output/json_basic.json"
    
    # Test basic command with JSON output
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --execute-js "return document.title;" \
          --timeout 10000 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "JSON output file created and valid"
            
            # Check JSON structure
            local status=$(get_json_value "$output_file" ".status")
            local title=$(get_json_value "$output_file" ".result.title")
            
            if [ "$status" = "success" ]; then
                log_success "JSON status indicates success"
            else
                log_error "JSON status not success: $status"
            fi
            
            if [ -n "$title" ]; then
                log_success "JSON contains page title"
            else
                log_error "JSON missing page title"
            fi
        else
            log_error "JSON output file is invalid"
        fi
    else
        log_error "JSON output file not created"
    fi
}

test_json_output_mode_assertions() {
    run_test "JSON Output Mode with Assertions"
    
    local output_file="$TEMP_DIR/output/json_assertions.json"
    
    # Test assertions with JSON output
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --assert-exists "body" \
          --assert-text "title" "Upload Server" \
          --assert-count "form" 2 \
          --timeout 10000 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "JSON assertion output valid"
            
            # Check assertion results
            local assertion_count=$(get_json_value "$output_file" ".assertions | length")
            local passed_count=$(get_json_value "$output_file" ".summary.passed")
            
            if [ "$assertion_count" -ge 3 ]; then
                log_success "JSON contains expected number of assertions ($assertion_count)"
            else
                log_error "JSON missing assertions (found: $assertion_count)"
            fi
            
            if [ "$passed_count" -ge 2 ]; then
                log_success "JSON shows assertions passed ($passed_count)"
            else
                log_error "JSON shows few passed assertions ($passed_count)"
            fi
        else
            log_error "JSON assertion output is invalid"
        fi
    else
        log_error "JSON assertion output file not created"
    fi
}

test_json_output_mode_errors() {
    run_test "JSON Output Mode Error Handling"
    
    local output_file="$TEMP_DIR/output/json_errors.json"
    
    # Test command that should fail
    $HWEB --url "http://invalid-server-url-12345:9999" \
          --json \
          --output "$output_file" \
          --timeout 2000 2>/dev/null || true
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "JSON error output valid"
            
            local status=$(get_json_value "$output_file" ".status")
            local error_message=$(get_json_value "$output_file" ".error.message")
            
            if [ "$status" = "error" ]; then
                log_success "JSON correctly indicates error status"
            else
                log_error "JSON should indicate error status, got: $status"
            fi
            
            if [ -n "$error_message" ] && [ "$error_message" != "null" ]; then
                log_success "JSON contains error message"
            else
                log_error "JSON missing error message"
            fi
        else
            log_error "JSON error output is invalid"
        fi
    else
        log_error "JSON error output file not created"
    fi
}

test_json_output_mode_comprehensive() {
    run_test "JSON Output Mode Comprehensive Test"
    
    local output_file="$TEMP_DIR/output/json_comprehensive.json"
    
    # Test comprehensive workflow with JSON output
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --execute-js "
              document.body.innerHTML += '<div id=\"test-element\">Test Content</div>';
              return 'Dynamic content added';
          " \
          --wait-for "#test-element" \
          --assert-exists "#test-element" \
          --assert-text "#test-element" "Test Content" \
          --screenshot "$TEMP_DIR/output/comprehensive.png" \
          --timeout 10000 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Comprehensive JSON output valid"
            
            # Check multiple result types
            local js_result=$(get_json_value "$output_file" ".javascript.result")
            local wait_success=$(get_json_value "$output_file" ".wait.success")
            local assertions=$(get_json_value "$output_file" ".assertions")
            local screenshot_path=$(get_json_value "$output_file" ".screenshot.path")
            
            if [ "$js_result" = "Dynamic content added" ]; then
                log_success "JSON contains JavaScript execution result"
            else
                log_error "JSON missing JavaScript result"
            fi
            
            if [ "$wait_success" = "true" ]; then
                log_success "JSON shows wait operation succeeded"
            else
                log_error "JSON wait operation not successful"
            fi
            
            if [ -n "$screenshot_path" ] && [ "$screenshot_path" != "null" ]; then
                log_success "JSON contains screenshot path"
                
                if [ -f "$screenshot_path" ]; then
                    log_success "Screenshot file exists as referenced in JSON"
                else
                    log_error "Screenshot file missing despite JSON reference"
                fi
            else
                log_error "JSON missing screenshot path"
            fi
        else
            log_error "Comprehensive JSON output is invalid"
        fi
    else
        log_error "Comprehensive JSON output file not created"
    fi
}

# ========== Configuration File Processing Tests ==========

test_configuration_file_processing_basic() {
    run_test "Basic Configuration File Processing"
    
    local config_file="$TEMP_DIR/config/basic_config.json"
    local output_file="$TEMP_DIR/output/config_basic.json"
    
    # Create basic configuration file
    cat > "$config_file" << 'EOF'
{
    "url": "http://localhost:9876",
    "timeout": 15000,
    "json": true,
    "output": "config_basic.json",
    "commands": [
        {
            "type": "execute-js",
            "script": "return document.readyState;"
        },
        {
            "type": "assert-exists",
            "selector": "body"
        }
    ]
}
EOF
    
    # Test using configuration file
    $HWEB --config "$config_file" \
          --output "$output_file" 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Configuration file processed successfully"
            
            local js_result=$(get_json_value "$output_file" ".javascript.result")
            if [ -n "$js_result" ]; then
                log_success "Configuration commands executed"
            else
                log_error "Configuration commands not executed"
            fi
        else
            log_error "Configuration output JSON is invalid"
        fi
    else
        log_error "Configuration processing failed - no output file"
    fi
}

test_configuration_file_processing_complex() {
    run_test "Complex Configuration File Processing"
    
    local config_file="$TEMP_DIR/config/complex_config.json"
    local output_file="$TEMP_DIR/output/config_complex.json"
    
    # Create complex configuration file
    cat > "$config_file" << EOF
{
    "url": "$TEST_SERVER_URL",
    "timeout": 20000,
    "json": true,
    "verbose": true,
    "output": "$output_file",
    "session": "config_test_session",
    "commands": [
        {
            "type": "execute-js",
            "script": "document.title = 'Configuration Test'; return document.title;"
        },
        {
            "type": "wait-for",
            "selector": "body",
            "timeout": 5000
        },
        {
            "type": "assert-text",
            "selector": "title",
            "expected": "Configuration Test"
        },
        {
            "type": "screenshot",
            "filename": "$TEMP_DIR/output/config_screenshot.png"
        }
    ],
    "variables": {
        "test_var": "config_value",
        "numeric_var": 42
    }
}
EOF
    
    # Test complex configuration
    $HWEB --config "$config_file" 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Complex configuration processed successfully"
            
            # Check configuration execution results
            local title_result=$(get_json_value "$output_file" ".javascript.result")
            local assertion_result=$(get_json_value "$output_file" ".assertions[0].result")
            local screenshot_exists=$(get_json_value "$output_file" ".screenshot.exists")
            
            if [ "$title_result" = "Configuration Test" ]; then
                log_success "Configuration JavaScript executed correctly"
            else
                log_error "Configuration JavaScript failed: $title_result"
            fi
            
            if [ "$assertion_result" = "passed" ]; then
                log_success "Configuration assertions passed"
            else
                log_error "Configuration assertions failed: $assertion_result"
            fi
            
            if [ -f "$TEMP_DIR/output/config_screenshot.png" ]; then
                log_success "Configuration screenshot created"
            else
                log_error "Configuration screenshot not created"
            fi
        else
            log_error "Complex configuration output JSON is invalid"
        fi
    else
        log_error "Complex configuration processing failed"
    fi
}

test_configuration_file_processing_variables() {
    run_test "Configuration File Variable Processing"
    
    local config_file="$TEMP_DIR/config/variables_config.json"
    local output_file="$TEMP_DIR/output/config_variables.json"
    
    # Create configuration with variables
    cat > "$config_file" << EOF
{
    "url": "$TEST_SERVER_URL",
    "json": true,
    "output": "$output_file",
    "variables": {
        "username": "testuser",
        "password": "testpass123",
        "element_id": "#test-element"
    },
    "commands": [
        {
            "type": "execute-js",
            "script": "document.body.innerHTML += '<input id=\"test-element\" type=\"text\" placeholder=\"{{username}}\"/>'; return 'Element added';"
        },
        {
            "type": "type",
            "selector": "{{element_id}}",
            "text": "{{username}}"
        },
        {
            "type": "assert-value",
            "selector": "{{element_id}}",
            "expected": "{{username}}"
        }
    ]
}
EOF
    
    # Test variable substitution
    $HWEB --config "$config_file" 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Variable configuration processed successfully"
            
            # Check if variables were substituted correctly
            local type_selector=$(get_json_value "$output_file" ".commands[1].selector")
            local type_text=$(get_json_value "$output_file" ".commands[1].text")
            
            if [ "$type_selector" = "#test-element" ]; then
                log_success "Variable substitution in selector worked"
            else
                log_error "Variable substitution in selector failed: $type_selector"
            fi
            
            if [ "$type_text" = "testuser" ]; then
                log_success "Variable substitution in text worked"
            else
                log_error "Variable substitution in text failed: $type_text"
            fi
        else
            log_error "Variable configuration output JSON is invalid"
        fi
    else
        log_error "Variable configuration processing failed"
    fi
}

test_configuration_file_processing_inheritance() {
    run_test "Configuration File Inheritance"
    
    local base_config="$TEMP_DIR/config/base_config.json"
    local child_config="$TEMP_DIR/config/child_config.json"
    local output_file="$TEMP_DIR/output/config_inheritance.json"
    
    # Create base configuration
    cat > "$base_config" << EOF
{
    "timeout": 10000,
    "json": true,
    "variables": {
        "base_var": "base_value"
    },
    "commands": [
        {
            "type": "execute-js",
            "script": "return 'Base command executed';"
        }
    ]
}
EOF
    
    # Create child configuration that inherits from base
    cat > "$child_config" << EOF
{
    "extends": "$base_config",
    "url": "$TEST_SERVER_URL",
    "output": "$output_file",
    "variables": {
        "child_var": "child_value"
    },
    "commands": [
        {
            "type": "execute-js",
            "script": "return 'Child command executed';"
        },
        {
            "type": "assert-exists",
            "selector": "body"
        }
    ]
}
EOF
    
    # Test configuration inheritance
    $HWEB --config "$child_config" 2>/dev/null
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Configuration inheritance processed successfully"
            
            # Check that both base and child commands were executed
            local commands_count=$(get_json_value "$output_file" ".commands | length")
            
            if [ "$commands_count" -ge 2 ]; then
                log_success "Configuration inheritance merged commands ($commands_count total)"
            else
                log_error "Configuration inheritance failed to merge commands"
            fi
        else
            log_error "Configuration inheritance output JSON is invalid"
        fi
    else
        log_error "Configuration inheritance processing failed"
    fi
}

# ========== Output Format Validation Tests ==========

test_output_format_validation_text() {
    run_test "Text Output Format Validation"
    
    local output_file="$TEMP_DIR/output/text_output.txt"
    
    # Test text output format
    $HWEB --url "$TEST_SERVER_URL" \
          --output "$output_file" \
          --execute-js "return 'Text format test';" \
          --assert-exists "body" \
          --verbose \
          --timeout 10000 > "$TEMP_DIR/stdout.log" 2>&1
    
    if [ -f "$output_file" ]; then
        local content=$(cat "$output_file")
        
        if echo "$content" | grep -q "Text format test"; then
            log_success "Text output contains JavaScript result"
        else
            log_error "Text output missing JavaScript result"
        fi
        
        if echo "$content" | grep -q "body.*exists"; then
            log_success "Text output contains assertion result"
        else
            log_error "Text output missing assertion result"
        fi
    else
        log_error "Text output file not created"
    fi
}

test_output_format_validation_xml() {
    run_test "XML Output Format Validation"
    
    local output_file="$TEMP_DIR/output/xml_output.xml"
    
    # Test XML output format (JUnit style)
    $HWEB --url "$TEST_SERVER_URL" \
          --output "$output_file" \
          --format "xml" \
          --assert-exists "body" \
          --assert-text "title" "Upload Server" \
          --timeout 10000 2>/dev/null
    
    if [ -f "$output_file" ]; then
        # Basic XML validation
        if xmllint --noout "$output_file" 2>/dev/null; then
            log_success "XML output is well-formed"
            
            # Check XML structure
            if grep -q "<testsuite" "$output_file"; then
                log_success "XML contains testsuite element"
            else
                log_error "XML missing testsuite element"
            fi
            
            if grep -q "<testcase" "$output_file"; then
                log_success "XML contains testcase elements"
            else
                log_error "XML missing testcase elements"
            fi
        else
            log_error "XML output is malformed"
        fi
    else
        log_error "XML output file not created"
    fi
}

test_output_format_validation_csv() {
    run_test "CSV Output Format Validation"
    
    local output_file="$TEMP_DIR/output/csv_output.csv"
    
    # Test CSV output format
    $HWEB --url "$TEST_SERVER_URL" \
          --output "$output_file" \
          --format "csv" \
          --assert-exists "body" \
          --assert-exists "title" \
          --assert-count "form" 2 \
          --timeout 10000 2>/dev/null
    
    if [ -f "$output_file" ]; then
        local content=$(cat "$output_file")
        
        # Check CSV headers
        if echo "$content" | head -n1 | grep -q "test,selector,expected,actual,result"; then
            log_success "CSV contains proper headers"
        else
            log_error "CSV missing proper headers"
        fi
        
        # Check CSV data rows
        local row_count=$(echo "$content" | wc -l)
        if [ "$row_count" -gt 1 ]; then
            log_success "CSV contains data rows ($row_count total)"
        else
            log_error "CSV missing data rows"
        fi
        
        # Check CSV format
        if echo "$content" | grep -q ",body,.*,.*,"; then
            log_success "CSV contains assertion results"
        else
            log_error "CSV missing assertion results"
        fi
    else
        log_error "CSV output file not created"
    fi
}

# ========== Logging Integration Tests ==========

test_logging_integration_levels() {
    run_test "Logging Integration with Different Levels"
    
    local log_file="$TEMP_DIR/logs/debug.log"
    local output_file="$TEMP_DIR/output/logging.json"
    
    # Test with debug logging
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --log-file "$log_file" \
          --log-level "DEBUG" \
          --verbose \
          --execute-js "return 'Debug logging test';" \
          --timeout 10000 2>/dev/null
    
    if [ -f "$log_file" ]; then
        local log_content=$(cat "$log_file")
        
        if echo "$log_content" | grep -q "DEBUG"; then
            log_success "Debug level logging working"
        else
            log_error "Debug level logging not working"
        fi
        
        if echo "$log_content" | grep -q "JavaScript"; then
            log_success "Logging captures JavaScript execution"
        else
            log_error "Logging missing JavaScript execution"
        fi
        
        if echo "$log_content" | grep -q "navigation\|load"; then
            log_success "Logging captures navigation events"
        else
            log_error "Logging missing navigation events"
        fi
    else
        log_error "Log file not created"
    fi
}

test_logging_integration_rotation() {
    run_test "Log File Rotation"
    
    local log_file="$TEMP_DIR/logs/rotation.log"
    local output_file="$TEMP_DIR/output/rotation.json"
    
    # Create initial log content
    for i in {1..100}; do
        echo "Pre-existing log line $i" >> "$log_file"
    done
    
    # Test with log rotation
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --log-file "$log_file" \
          --log-max-size "1KB" \
          --log-rotate \
          --execute-js "
              for(let i = 0; i < 50; i++) {
                  console.log('Generating log content line ' + i);
              }
              return 'Log rotation test';
          " \
          --timeout 10000 2>/dev/null
    
    # Check for rotated log files
    if ls "$TEMP_DIR/logs/rotation.log"* 2>/dev/null | wc -l | grep -q "[2-9]"; then
        log_success "Log rotation created multiple files"
    else
        log_warning "Log rotation may not have occurred (file size dependent)"
    fi
    
    if [ -f "$log_file" ]; then
        log_success "Primary log file still exists"
    else
        log_error "Primary log file missing after rotation"
    fi
}

# ========== Error Output Formatting Tests ==========

test_error_output_formatting_json() {
    run_test "Error Output Formatting in JSON"
    
    local output_file="$TEMP_DIR/output/error_json.json"
    
    # Test various error conditions with JSON output
    $HWEB --url "http://nonexistent-domain-12345.com" \
          --json \
          --output "$output_file" \
          --timeout 2000 2>/dev/null || true
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            local error_type=$(get_json_value "$output_file" ".error.type")
            local error_message=$(get_json_value "$output_file" ".error.message")
            local stack_trace=$(get_json_value "$output_file" ".error.stack_trace")
            
            if [ -n "$error_type" ] && [ "$error_type" != "null" ]; then
                log_success "JSON error output contains error type"
            else
                log_error "JSON error output missing error type"
            fi
            
            if [ -n "$error_message" ] && [ "$error_message" != "null" ]; then
                log_success "JSON error output contains error message"
            else
                log_error "JSON error output missing error message"
            fi
        else
            log_error "JSON error output is malformed"
        fi
    else
        log_error "JSON error output file not created"
    fi
}

test_error_output_formatting_validation() {
    run_test "Error Output Validation Messages"
    
    local output_file="$TEMP_DIR/output/validation_errors.json"
    
    # Test validation errors
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --assert-text "#nonexistent-element" "expected text" \
          --assert-count "#missing-elements" 5 \
          --timeout 10000 2>/dev/null || true
    
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            local assertions=$(get_json_value "$output_file" ".assertions | length")
            local failed_count=$(get_json_value "$output_file" ".summary.failed")
            
            if [ "$assertions" -ge 2 ]; then
                log_success "JSON contains validation assertions"
            else
                log_error "JSON missing validation assertions"
            fi
            
            if [ "$failed_count" -ge 2 ]; then
                log_success "JSON shows expected assertion failures ($failed_count)"
            else
                log_error "JSON not showing expected assertion failures"
            fi
            
            # Check error details
            local first_error=$(get_json_value "$output_file" ".assertions[0].error")
            if [ -n "$first_error" ] && [ "$first_error" != "null" ]; then
                log_success "JSON provides assertion error details"
            else
                log_error "JSON missing assertion error details"
            fi
        else
            log_error "Validation error JSON is malformed"
        fi
    else
        log_error "Validation error output file not created"
    fi
}

# ========== Silent Mode Operation Tests ==========

test_silent_mode_operation() {
    run_test "Silent Mode Operation"
    
    local output_file="$TEMP_DIR/output/silent_mode.json"
    local stdout_file="$TEMP_DIR/silent_stdout.log"
    local stderr_file="$TEMP_DIR/silent_stderr.log"
    
    # Test silent mode
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --output "$output_file" \
          --silent \
          --execute-js "return 'Silent mode test';" \
          --assert-exists "body" \
          --timeout 10000 > "$stdout_file" 2> "$stderr_file"
    
    # Check that stdout is minimal in silent mode
    local stdout_size=$(stat -f%z "$stdout_file" 2>/dev/null || stat -c%s "$stdout_file" 2>/dev/null)
    if [ "$stdout_size" -lt 100 ]; then
        log_success "Silent mode produces minimal stdout ($stdout_size bytes)"
    else
        log_error "Silent mode produces excessive stdout ($stdout_size bytes)"
    fi
    
    # Check that JSON output still works
    if [ -f "$output_file" ]; then
        if validate_json "$output_file"; then
            log_success "Silent mode still produces valid JSON output"
        else
            log_error "Silent mode JSON output is invalid"
        fi
    else
        log_error "Silent mode failed to create output file"
    fi
}

# ========== Output Redirection Tests ==========

test_output_redirection_pipes() {
    run_test "Output Redirection and Pipes"
    
    local json_output="$TEMP_DIR/output/piped.json"
    
    # Test piping JSON output through jq (if available)
    if command -v jq >/dev/null 2>&1; then
        local processed_output=$(
            $HWEB --url "$TEST_SERVER_URL" \
                  --json \
                  --execute-js "return {test: 'pipe test', number: 42};" \
                  --timeout 10000 2>/dev/null | \
            jq -r '.javascript.result.test'
        )
        
        if [ "$processed_output" = "pipe test" ]; then
            log_success "JSON output pipes correctly through jq"
        else
            log_error "JSON output piping failed: $processed_output"
        fi
    else
        log_warning "jq not available, skipping pipe test"
    fi
    
    # Test output redirection
    $HWEB --url "$TEST_SERVER_URL" \
          --json \
          --execute-js "return 'Redirection test';" \
          --timeout 10000 2>/dev/null > "$json_output"
    
    if [ -f "$json_output" ]; then
        if validate_json "$json_output"; then
            log_success "Output redirection creates valid JSON"
        else
            log_error "Output redirection produces invalid JSON"
        fi
    else
        log_error "Output redirection failed to create file"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb Output & Configuration Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Run JSON output mode tests
    test_json_output_mode_basic
    test_json_output_mode_assertions
    test_json_output_mode_errors
    test_json_output_mode_comprehensive
    
    # Run configuration file processing tests
    test_configuration_file_processing_basic
    test_configuration_file_processing_complex
    test_configuration_file_processing_variables
    test_configuration_file_processing_inheritance
    
    # Run output format validation tests
    test_output_format_validation_text
    test_output_format_validation_xml
    test_output_format_validation_csv
    
    # Run logging integration tests
    test_logging_integration_levels
    test_logging_integration_rotation
    
    # Run error formatting tests
    test_error_output_formatting_json
    test_error_output_formatting_validation
    
    # Run silent mode and redirection tests
    test_silent_mode_operation
    test_output_redirection_pipes
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All output and configuration tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some output and configuration tests failed! ❌${NC}"
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    if [ ! -f "$HWEB" ]; then
        log_error "hweb binary not found: $HWEB"
        exit 1
    fi
    
    if ! command -v node >/dev/null 2>&1; then
        log_error "Node.js is required but not installed"
        exit 1
    fi
    
    if ! command -v curl >/dev/null 2>&1; then
        log_error "curl is required but not installed"
        exit 1
    fi
}

check_prerequisites
main "$@"