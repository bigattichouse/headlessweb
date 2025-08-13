module.exports = {
    // Test environment
    testEnvironment: 'node',
    
    // Test file patterns
    testMatch: [
        '<rootDir>/test/**/*.test.js',
        '<rootDir>/test/**/*.spec.js'
    ],
    
    // Setup files
    setupFilesAfterEnv: ['<rootDir>/test/helpers/setup.js'],
    globalTeardown: '<rootDir>/test/helpers/teardown.js',
    
    // Coverage configuration
    collectCoverage: true,
    coverageDirectory: 'coverage',
    coverageReporters: ['text', 'lcov', 'html'],
    collectCoverageFrom: [
        'lib/**/*.js',
        'index.js',
        '!lib/**/*.test.js',
        '!test/**/*'
    ],
    
    // Coverage thresholds (reduced for mock testing)
    coverageThreshold: {
        global: {
            branches: 20,
            functions: 20,
            lines: 20,
            statements: 20
        }
    },
    
    // Module paths
    moduleDirectories: ['node_modules', '<rootDir>'],
    
    // Test timeout
    testTimeout: 30000,
    
    // Verbose output
    verbose: true,
    
    // Handle native modules gracefully
    moduleNameMapper: {
        '^.*\\.node$': '<rootDir>/test/mocks/addon-mock.js'
    },
    
    // Transform configuration
    transform: {},
    
    // Test environment options
    testEnvironmentOptions: {
        // Ensure clean environment for each test
    }
};