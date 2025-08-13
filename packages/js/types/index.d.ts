// TypeScript definitions for @headlessweb/js

declare module '@headlessweb/js' {
    import { EventEmitter } from 'events';

    /**
     * Browser configuration options
     */
    export interface BrowserOptions {
        /** Session name for persistence (default: 'default') */
        session?: string;
        /** Run in headless mode (default: true) */
        headless?: boolean;
        /** Default timeout in milliseconds (default: 30000) */
        timeout?: number;
        /** Browser width (default: 1024) */
        width?: number;
        /** Browser height (default: 768) */
        height?: number;
    }

    /**
     * Page information object
     */
    export interface PageInfo {
        url: string;
        title: string;
        readyState: string;
    }

    /**
     * Test step information
     */
    export interface TestStep {
        action: string;
        details: Record<string, any>;
        success: boolean;
        duration: number;
        message: string;
        error: string;
        timestamp: number;
    }

    /**
     * Test report summary
     */
    export interface TestSummary {
        passed: number;
        failed: number;
        total: number;
        success: boolean;
        duration: number;
    }

    /**
     * Test report object
     */
    export interface TestReport {
        name: string;
        summary: TestSummary;
        steps: TestStep[];
        timing: {
            startTime: number;
            endTime: number;
            totalTime: number;
        };
    }

    /**
     * System information object
     */
    export interface SystemInfo {
        platform: string;
        arch: string;
        webkit_version: string;
    }

    /**
     * Browser automation class
     */
    export class Browser extends EventEmitter {
        constructor(options?: BrowserOptions);

        readonly session: string;
        readonly headless: boolean;
        readonly timeout: number;
        readonly width: number;
        readonly height: number;

        // Navigation methods
        navigate(url: string): Promise<boolean>;
        navigateSync(url: string): boolean;
        getCurrentUrl(): string;

        // DOM interaction methods (async)
        click(selector: string): Promise<boolean>;
        type(selector: string, text: string): Promise<boolean>;
        executeJavaScript(code: string): Promise<string>;
        screenshot(filename?: string): Promise<boolean>;

        // DOM interaction methods (sync)
        clickSync(selector: string): boolean;
        typeSync(selector: string, text: string): boolean;
        executeJavaScriptSync(code: string): string;
        screenshotSync(filename?: string): boolean;

        // Element query methods
        exists(selector: string): boolean;
        getText(selector: string): string;
        getHtml(selector: string): string;
        getAttribute(selector: string, attribute: string): string;
        count(selector: string): number;

        // Utility methods
        chain(): BrowserChain;
        createTest(name: string): TestSuite;
        wait(ms: number): Promise<void>;
        getPageInfo(): PageInfo;
        destroy(): void;

        // Events
        on(event: 'created', listener: (data: { session: string }) => void): this;
        on(event: 'navigated', listener: (data: { url: string }) => void): this;
        on(event: 'clicked', listener: (data: { selector: string }) => void): this;
        on(event: 'typed', listener: (data: { selector: string; text: string }) => void): this;
        on(event: 'screenshot', listener: (data: { filename: string }) => void): this;
        on(event: 'destroyed', listener: () => void): this;
        on(event: 'error', listener: (error: Error) => void): this;
    }

    /**
     * Fluent API chain for batch operations
     */
    export class BrowserChain {
        constructor(browser: Browser);

        navigate(url: string): BrowserChain;
        click(selector: string): BrowserChain;
        type(selector: string, text: string): BrowserChain;
        wait(ms: number): BrowserChain;
        screenshot(filename?: string): BrowserChain;
        execute(): Promise<any[]>;
    }

    /**
     * Session management class
     */
    export class Session {
        constructor(sessionName?: string, sessionsDir?: string);

        readonly sessionName: string;
        readonly sessionsDir: string;

        getName(): string;
        save(): Promise<boolean>;
        load(): Promise<boolean>;
        delete(): Promise<boolean>;
        list(): string[];
        exists(): boolean;
    }

    /**
     * Test suite for browser automation testing
     */
    export class TestSuite {
        constructor(browser: Browser, name: string);

        readonly browser: Browser;
        readonly name: string;
        readonly steps: TestStep[];
        readonly startTime: number;

        // Navigation
        navigate(url: string): Promise<TestSuite>;

        // DOM interactions
        click(selector: string): Promise<TestSuite>;
        type(selector: string, text: string): Promise<TestSuite>;
        screenshot(filename?: string): Promise<TestSuite>;

        // Assertions
        assertExists(selector: string): Promise<TestSuite>;
        assertNotExists(selector: string): Promise<TestSuite>;
        assertText(selector: string, expectedText: string): Promise<TestSuite>;
        assertAttribute(selector: string, attribute: string, expectedValue: string): Promise<TestSuite>;
        assertUrl(pattern: string | RegExp): Promise<TestSuite>;

        // Utilities
        wait(ms: number): Promise<TestSuite>;
        message(message: string): TestSuite;

        // Reporting
        generateReport(): TestReport;
        generateTextReport(): string;
    }

    /**
     * Create a new Browser instance (convenience factory)
     */
    export function create(options?: BrowserOptions): Browser;

    /**
     * Get system information
     */
    export function getSystemInfo(): SystemInfo;

    /**
     * Package version
     */
    export const version: string;

    /**
     * Core C++ library version
     */
    export const coreVersion: string;
}