# Event-Driven Architecture Refactor Blueprint

## Overview

This document outlines a comprehensive plan to replace blocking waits, polling loops, and arbitrary delays with properly engineered event-driven solutions throughout the HeadlessWeb codebase. The current architecture relies heavily on `sleep()`, `wait()`, and polling patterns that create race conditions, performance issues, and unreliable behavior in headless environments.

**Status**: 🔴 Planning Phase  
**Priority**: Critical - Core Architecture Issue  
**Approach**: Comprehensive refactoring with proper engineering solutions (NO quick fixes)

## Problem Analysis

### Current Issues
- **~100+ instances** of blocking waits throughout codebase
- **Polling loops** instead of event-driven waiting
- **Arbitrary delays** after DOM operations
- **Race conditions** in headless environments
- **Test reliability issues** due to timing dependencies
- **Performance degradation** from unnecessary blocking

### Root Causes
1. **Missing event infrastructure** for browser state changes
2. **Lack of completion signals** for async operations
3. **No proper readiness detection** for DOM/network states
4. **Inadequate WebKit integration** for native events
5. **Legacy patterns** carried over without refactoring

## Refactor Plan

### Phase 1: Foundation - Event Infrastructure (Week 1-2)

#### 1.1 Core Event System
**Files**: `src/Browser/Events.cpp`, `src/Browser/EventLoopManager.cpp`

**Current Issues**:
- Mixed event-driven and polling approaches
- Fallback to `std::this_thread::sleep_for()` in EventLoopManager.cpp:77
- JavaScript-side polling in event scripts (Events.cpp:362, 404, 441, 457, 537, 564, 590)

**Solutions**:
- Implement proper WebKit signal handlers for all browser events
- Create unified event bus for internal state changes
- Add MutationObserver integration for DOM changes
- Implement network request completion signals
- Remove all fallback polling patterns

**New Components**:
```cpp
class BrowserEventBus {
    // Unified event system for all browser state changes
    void subscribe(EventType type, std::function<void(Event)> handler);
    void emit(EventType type, Event event);
};

class MutationTracker {
    // Proper DOM change detection using WebKit MutationObserver
    void observeElement(std::string selector, std::function<void()> callback);
    void observeSubtree(std::string selector, std::function<void()> callback);
};

class NetworkEventTracker {
    // Track network requests and responses
    void onRequestStart(std::function<void(NetworkRequest)> callback);
    void onRequestComplete(std::function<void(NetworkResponse)> callback);
    void onNetworkIdle(std::function<void()> callback);
};
```

#### 1.2 Browser Readiness Detection
**Files**: `src/Browser/Core.cpp`, `src/Browser/Wait.cpp`

**Current Issues**:
- No unified readiness state management
- Arbitrary waits for page/DOM readiness
- Missing framework-specific readiness detection

**Solutions**:
- Implement comprehensive browser state tracking
- Add DOM readiness signals (DOMContentLoaded, load, interactive)
- Create framework-aware readiness detection (React, Vue, Angular)
- Add viewport and rendering completion signals

**New Components**:
```cpp
enum class BrowserState {
    LOADING, DOM_READY, RESOURCES_LOADED, FULLY_READY, FRAMEWORK_READY
};

class BrowserStateManager {
    BrowserState getCurrentState();
    void waitForState(BrowserState state, int timeout_ms);
    void onStateChange(std::function<void(BrowserState)> callback);
};
```

### Phase 2: Core Browser Operations (Week 3-4)

#### 2.1 DOM Operations Refactor
**File**: `src/Browser/DOM.cpp`

**Current Blocking Patterns**:
- Line 40: `wait(5)` after filling input → **Replace with input event completion**
- Line 162: `wait(5)` for value processing → **Replace with value change signal**
- Line 192: `wait(25)` after retry → **Replace with retry completion callback**
- Line 312: `wait(10)` before select operations → **Replace with element ready signal**
- Line 349: `wait(5)` after select operations → **Replace with selection event**
- Lines 390, 414, 433, 457, 743, 799, 850: Similar patterns throughout

**Refactor Strategy**:
1. **Input Operations**: Wait for `input`, `change`, `blur` events instead of fixed delays
2. **Selection Operations**: Wait for `change` and `focus` events
3. **Click Operations**: Wait for `click` event completion and any triggered actions
4. **Form Operations**: Wait for form validation and submission events

**New DOM API**:
```cpp
class AsyncDOMOperations {
    std::future<bool> fillInputAsync(std::string selector, std::string value);
    std::future<bool> clickElementAsync(std::string selector);
    std::future<bool> selectOptionAsync(std::string selector, std::string value);
    std::future<bool> submitFormAsync(std::string selector);
    
    // Event-driven waiting
    void waitForElementEvent(std::string selector, std::string event, std::function<void()> callback);
    void waitForElementReady(std::string selector, std::function<void()> callback);
};
```

#### 2.2 Navigation and Page Loading
**Files**: `src/Browser/Core.cpp`, `src/Browser/Wait.cpp`

**Current Issues**:
- Fixed timeouts for navigation completion
- No proper page load state tracking
- Missing SPA navigation detection

**Solutions**:
- Implement navigation promise pattern
- Add proper page load event handling
- Create SPA navigation detection using URL changes and framework signals
- Add resource loading completion tracking

### Phase 3: Session and State Management (Week 5)

#### 3.1 Session Restoration Refactor
**File**: `src/Browser/Session.cpp`

**Current Blocking Patterns**:
- Line 16: `wait(100)` for user agent → **Replace with browser config completion**
- Line 71: `wait(500)` for cookies → **Replace with cookie set completion**
- Line 87: `wait(500)` for localStorage → **Replace with storage operation callback**
- Line 93: `wait(500)` for sessionStorage → **Replace with storage operation callback**
- Line 111: `wait(500)` for form state → **Replace with form restoration completion**
- Line 124: `wait(200)` for active elements → **Replace with element focus events**
- Line 136: `wait(500)` for custom attributes → **Replace with DOM mutation completion**
- Line 148: `wait(200)` for custom state → **Replace with state application completion**
- Line 167: `wait(500)` final settlement → **Replace with full restoration completion signal**

**Refactor Strategy**:
1. **Sequential Restoration Chain**: Each restoration step waits for previous completion
2. **Operation Callbacks**: Each operation provides completion notification
3. **State Verification**: Verify each restoration step completed successfully
4. **Rollback Capability**: Handle restoration failures gracefully

**New Session API**:
```cpp
class AsyncSessionRestoration {
    std::future<bool> restoreUserAgent(std::string userAgent);
    std::future<bool> restoreCookies(std::vector<Cookie> cookies);
    std::future<bool> restoreStorage(StorageType type, std::map<std::string, std::string> data);
    std::future<bool> restoreFormState(std::vector<FormField> fields);
    std::future<bool> restoreFullState(Session session);
};
```

### Phase 4: Advanced Wait Patterns (Week 6)

#### 4.1 Network and Resource Waiting
**File**: `src/Browser/Wait.cpp`

**Current Polling Patterns**:
- Lines 219-235: `waitForNetworkIdle()` polling loop
- Lines 351-367: `waitForNetworkRequest()` polling loop
- Lines 415-433: `waitForElementCount()` polling loop
- Lines 478-496: `waitForAttribute()` polling loop
- Lines 643-648: `waitForSPANavigation()` polling loop
- Lines 696-714: `waitForFrameworkReady()` polling loop
- Lines 767-783: `waitForDOMChange()` polling loop
- Lines 861-877: `waitForContentChange()` polling loop

**Event-Driven Solutions**:
1. **Network Idle**: Monitor active requests via WebKit network events
2. **Element Count**: Use MutationObserver for DOM changes
3. **Attribute Changes**: Use attribute mutation observers
4. **SPA Navigation**: Monitor URL changes and framework routing
5. **Framework Ready**: Integrate with framework-specific ready events
6. **Content Changes**: Use text/content mutation observers

**New Wait API**:
```cpp
class EventDrivenWaiting {
    std::future<bool> waitForNetworkIdle(int idle_time_ms, int timeout_ms);
    std::future<bool> waitForElementCount(std::string selector, int count, ComparisonOperator op, int timeout_ms);
    std::future<bool> waitForAttributeValue(std::string selector, std::string attr, std::string value, int timeout_ms);
    std::future<bool> waitForSPANavigation(std::string route_pattern, int timeout_ms);
    std::future<bool> waitForFrameworkReady(FrameworkType type, int timeout_ms);
    std::future<bool> waitForContentChange(std::string selector, std::string expected_content, int timeout_ms);
};
```

### Phase 5: File Operations (Week 7)

#### 5.1 Download Manager Refactor
**File**: `src/FileOps/DownloadManager.cpp`

**Current Polling Patterns**:
- Line 161: `std::this_thread::sleep_for()` in wait loop
- Lines 325, 350, 376, 387, 419, 482, 585, 891: Multiple polling loops
- Lines 79-83: Fallback to polling when native watching fails

**Event-Driven Solutions**:
1. **Filesystem Watching**: Use proper inotify/ReadDirectoryChangesW APIs
2. **Download Completion**: Monitor browser download events
3. **File Integrity**: Use checksums and file lock detection
4. **Progress Tracking**: Real-time progress events instead of polling

**New File API**:
```cpp
class AsyncFileOperations {
    std::future<FileInfo> waitForDownload(std::string filename_pattern, int timeout_ms);
    std::future<std::vector<FileInfo>> waitForMultipleDownloads(std::vector<DownloadRequest> requests, int timeout_ms);
    void onDownloadProgress(std::function<void(DownloadProgress)> callback);
    void onDownloadComplete(std::function<void(FileInfo)> callback);
};
```

### Phase 6: Testing Infrastructure (Week 8)

#### 6.1 Test Environment Fixes
**Files**: All test files with blocking patterns

**Current Issues**:
- Hundreds of `std::this_thread::sleep_for()` calls in tests
- Fixed delays instead of condition-based waiting
- Race conditions in assertion tests

**Solutions**:
1. **Condition-Based Waiting**: Replace all fixed delays with condition checks
2. **Test Event Synchronization**: Proper browser readiness before assertions
3. **Deterministic Testing**: Eliminate timing-dependent test failures
4. **Performance Testing**: Measure actual performance vs. fixed delays

**New Test Utilities**:
```cpp
class TestWaitUtilities {
    void waitForCondition(std::function<bool()> condition, int timeout_ms);
    void waitForBrowserReady(int timeout_ms);
    void waitForAssertion(std::function<bool()> assertion, int timeout_ms);
    void synchronizeWithBrowser();
};
```

## Implementation Strategy

### Engineering Principles
1. **No Quick Fixes**: All solutions must be properly engineered, event-driven patterns
2. **Backward Compatibility**: Maintain existing API while transitioning internals
3. **Comprehensive Testing**: Each refactored component gets full test coverage
4. **Performance Monitoring**: Measure improvements vs. current blocking patterns
5. **Documentation**: Update all documentation to reflect new patterns

### Development Approach
1. **Feature Flags**: Use feature flags to enable new event-driven patterns gradually
2. **A/B Testing**: Compare old vs. new patterns for reliability and performance
3. **Incremental Rollout**: Phase-by-phase replacement with fallback options
4. **Monitoring**: Add extensive logging to track pattern effectiveness

### Success Criteria
- [ ] **Zero polling loops** in core browser operations
- [ ] **Sub-100ms response times** for all DOM operations
- [ ] **99%+ test reliability** in headless environments
- [ ] **50%+ performance improvement** in typical workflows
- [ ] **Complete event coverage** for all browser state changes

## Risk Mitigation

### Technical Risks
1. **WebKit Integration Complexity**: Some WebKit signals may be difficult to access
   - **Mitigation**: Research WebKit documentation, create minimal test cases
2. **Timing Edge Cases**: Event-driven patterns may miss rapid state changes
   - **Mitigation**: Comprehensive event coverage, proper event ordering
3. **Framework Compatibility**: Different frameworks may need different approaches
   - **Mitigation**: Framework-specific adapters, extensive testing

### Project Risks
1. **Large Scope**: This is a major architectural change
   - **Mitigation**: Incremental rollout, feature flags, thorough testing
2. **Breaking Changes**: Some internal APIs will change significantly
   - **Mitigation**: Maintain compatibility layers during transition
3. **Testing Complexity**: New patterns need new testing approaches
   - **Mitigation**: Develop testing utilities first, comprehensive test suites

## Timeline

- **Week 1-2**: Foundation - Event Infrastructure
- **Week 3-4**: Core Browser Operations  
- **Week 5**: Session and State Management
- **Week 6**: Advanced Wait Patterns
- **Week 7**: File Operations
- **Week 8**: Testing Infrastructure
- **Week 9**: Integration Testing and Performance Validation
- **Week 10**: Documentation and Deployment

## Files Requiring Changes

### Critical Priority
- `src/Browser/Wait.cpp` - Complete overhaul of waiting patterns
- `src/Browser/DOM.cpp` - All DOM operations need event-driven completion
- `src/Browser/Session.cpp` - Session restoration chain needs proper signals
- `src/Browser/Events.cpp` - Remove fallback polling, add comprehensive events
- `src/Browser/EventLoopManager.cpp` - Eliminate `sleep_for()` fallbacks

### High Priority  
- `src/FileOps/DownloadManager.cpp` - Replace all polling with filesystem events
- `src/Browser/EnhancedFormInteraction.cpp` - Form operations need event completion
- `src/Browser/Utilities.cpp` - Core `wait()` method pattern needs refactoring

### Medium Priority
- All test files - Replace fixed delays with condition-based waiting
- `src/Assertion/Manager.cpp` - Ensure assertions use proper browser readiness

### Documentation Updates
- Update all spec documents to reflect new event-driven patterns
- Create developer guide for new event-driven APIs
- Add performance benchmarking documentation

---

**Note**: This refactor addresses fundamental architectural issues. The goal is to create a robust, high-performance, event-driven browser automation system that works reliably in all environments. No shortcuts or quick fixes will be implemented - only properly engineered solutions.