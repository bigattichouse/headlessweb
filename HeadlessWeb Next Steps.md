# HeadlessWeb Next Steps Blueprint

## Current Status

### ✅ Completed Features
- Core navigation and session persistence
- Cookie management
- localStorage persistence (with file:// limitations)
- Form state preservation (input, textarea, select, checkbox)
- Scroll position tracking
- User agent customization
- Custom variable storage
- Session restoration after ending
- Command chaining
- History navigation (back/forward)

### ⚠️ Known Limitations
- sessionStorage on file:// URLs (browser security restriction)
- Screenshot functionality not implemented
- No binary session format (using JSON)
- No test assertion framework
- Debug output always on (needs --debug flag conversion)

## Priority 1: Essential Fixes (1-2 days)

### 1.1 Debug Flag Conversion
```blueprint
Task ConvertDebugFlag {
  description: "Change from --quiet to --debug flag for explicit debug output",
  
  changes: [
    // Global flag change
    Change GlobalFlag {
      from: "bool g_quiet = false",
      to: "bool g_debug = false",
      files: ["main.cpp"]
    },
    
    // Argument parsing
    Change ArgumentParsing {
      from: "--quiet suppresses output",
      to: "--debug enables debug output",
      update_help_text: true
    },
    
    // Output functions
    Change DebugOutput {
      create_function: "debug_output()",
      behavior: "only outputs when g_debug is true",
      replace_all: "std::cerr << 'Debug:' with debug_output()"
    }
  ],
  
  testing: "Verify no debug output by default, only with --debug"
}
```

### 1.2 Screenshot Implementation
```blueprint
Task ImplementScreenshot {
  description: "Add WebKit screenshot functionality",
  
  implementation: {
    method: "webkit_web_view_get_snapshot()",
    format: "PNG",
    options: ["full_page", "visible_area"],
    async: true
  },
  
  command: "--screenshot [filename]",
  default_filename: "screenshot.png",
  
  error_handling: [
    "Invalid path",
    "Permission denied",
    "WebKit snapshot failure"
  ]
}
```

## Priority 2: Testing Framework (2-3 days)

### 2.1 Assertion Commands
```blueprint
Feature AssertionCommands {
  description: "Add testing assertions for CI/CD integration",
  
  commands: [
    AssertExists {
      syntax: "--assert-exists <selector>",
      output: "PASS: Element exists" | "FAIL: Element not found",
      exit_code: 0 | 1
    },
    
    AssertText {
      syntax: "--assert-text <selector> <expected>",
      comparison: "exact_match",
      output: "PASS/FAIL with actual vs expected"
    },
    
    AssertContains {
      syntax: "--assert-contains <selector> <substring>",
      comparison: "substring_match"
    },
    
    AssertCount {
      syntax: "--assert-count <selector> <number>",
      comparison: "element_count"
    },
    
    AssertJS {
      syntax: "--assert-js <expression>",
      evaluation: "JavaScript boolean expression"
    }
  ],
  
  modifiers: {
    "--message": "Custom assertion message",
    "--pass-fail": "Always output PASS/FAIL"
  }
}
```

### 2.2 Test Reporting
```blueprint
Feature TestReporting {
  description: "Track and report test results",
  
  commands: [
    TestReportStart {
      syntax: "--test-report start <name>",
      action: "Begin tracking assertions"
    },
    
    TestReportEnd {
      syntax: "--test-report end",
      action: "Output summary and exit with appropriate code"
    },
    
    TestReportStatus {
      syntax: "--test-report status",
      action: "Show current test results"
    }
  ],
  
  output_formats: [
    "text" (default),
    "json",
    "junit" (for CI integration)
  ],
  
  tracking: {
    total_tests: int,
    passed: int,
    failed: int,
    assertions: TestResult[]
  }
}
```

## Priority 3: Output Enhancements (1-2 days)

### 3.1 JSON Output Mode
```blueprint
Feature JSONOutput {
  flag: "--json",
  description: "Output all results as JSON",
  
  structure: {
    success: boolean,
    command: string,
    result: any,
    error?: string,
    session_state?: {
      url: string,
      cookies: int,
      storage_items: int
    }
  },
  
  affects: [
    "All query commands (--text, --attr, etc.)",
    "Error messages",
    "Session info"
  ]
}
```

### 3.2 Format Options
```blueprint
Feature OutputFormat {
  flag: "--format <type>",
  types: ["text", "json", "csv", "raw"],
  
  examples: {
    "--count --format json": '{"count": 5}',
    "--text --format raw": "unprocessed text with newlines",
    "--cookies --format csv": "name,value,domain,path"
  }
}
```

## Priority 4: Advanced Features (3-5 days)

### 4.1 Recording and Replay
```blueprint
Feature ActionRecording {
  description: "Record user actions for replay",
  
  Recording {
    start: "--record-start [name]",
    stop: "--record-stop",
    
    captures: [
      "click events with selectors",
      "form inputs with values",
      "navigation changes",
      "wait times between actions"
    ],
    
    storage: {
      format: "JSON array of actions",
      location: "session or separate file"
    }
  },
  
  Replay {
    command: "--replay <name>",
    options: {
      "--replay-speed": "1x, 2x, 0.5x",
      "--replay-stop-on-error": boolean
    }
  }
}
```

### 4.2 Advanced Waiting
```blueprint
Feature EnhancedWaiting {
  commands: [
    WaitForText {
      syntax: "--wait-text <text>",
      search: "anywhere in page body",
      timeout: 10000
    },
    
    WaitForNetworkIdle {
      syntax: "--wait-network-idle",
      condition: "no requests for 500ms"
    },
    
    WaitForFunction {
      syntax: "--wait-function <js_function>",
      polling: "every 100ms until true"
    }
  ]
}
```

### 4.3 File Operations
```blueprint
Feature FileOperations {
  Download {
    command: "--download-wait <filename>",
    behavior: "Wait for download to complete",
    storage: "~/.headlessweb/downloads/{session}/"
  },
  
  Upload {
    command: "--upload <selector> <filepath>",
    validation: "Check file exists",
    types: "input[type=file]"
  }
}
```

## Priority 5: Performance & Architecture (5-7 days)

### 5.1 Binary Session Format
```blueprint
Feature BinarySessionFormat {
  description: "Replace JSON with efficient binary format",
  
  format: {
    header: {
      version: uint32,
      flags: uint32,
      sections: SectionOffset[]
    },
    
    sections: {
      metadata: "URLs, timestamps",
      cookies: "Compressed cookie data",
      storage: "Key-value pairs",
      form_state: "Compressed form data",
      custom: "User variables"
    },
    
    compression: "ZSTD for each section"
  },
  
  benefits: [
    "5-10x smaller files",
    "Faster load/save",
    "Partial loading capability"
  ]
}
```

### 5.2 Shared Cache
```blueprint
Feature SharedWebKitCache {
  description: "Share cache between sessions for performance",
  
  structure: {
    cache_dir: "~/.headlessweb/cache/shared/",
    session_isolation: "Cookies and storage remain separate",
    
    shared_resources: [
      "Images",
      "CSS files", 
      "JavaScript files",
      "Fonts"
    ]
  },
  
  management: {
    size_limit: "500MB default",
    cleanup: "--clear-cache command",
    auto_cleanup: "LRU when limit reached"
  }
}
```

## Priority 6: Nice-to-Have Features (Future)

### 6.1 Multi-Element Operations
```blueprint
Feature MultiElementOps {
  commands: [
    "--click-all <selector>",
    "--type-all <selector> <text>",
    "--check-all <selector>"
  ]
}
```

### 6.2 Visual Debugging
```blueprint
Feature VisualDebug {
  commands: [
    "--highlight <selector>",
    "--screenshot-diff <file1> <file2>",
    "--show-clicks" (visual markers)
  ]
}
```

### 6.3 Network Control
```blueprint
Feature NetworkControl {
  commands: [
    "--block <pattern>",
    "--throttle <speed>",
    "--offline",
    "--intercept <pattern> <response>"
  ]
}
```

## Implementation Schedule

### Week 1
- Day 1: Debug flag conversion
- Day 2: Screenshot implementation  
- Day 3-4: Basic assertion commands
- Day 5: Test reporting framework

### Week 2
- Day 1-2: JSON output mode
- Day 3-4: Recording/replay basics
- Day 5: Enhanced waiting commands

### Week 3+
- Binary session format
- Performance optimizations
- Advanced features as needed

## Success Criteria

1. **No breaking changes** to existing functionality
2. **All tests pass** including new assertion tests
3. **Performance maintained** (<400ms startup time)
4. **Documentation updated** for all new features
5. **CI/CD ready** with proper exit codes and reporting
