# HeadlessWeb Documentation

**Complete documentation for HeadlessWeb - Production-ready browser automation**

## 📚 Documentation Index

### **Getting Started**
- **[Quick Reference](quick-reference.md)** - Fast API cheatsheet and common patterns
- **[Installation Guide](../README.md#installation)** - System setup and dependencies

### **Core Documentation**
- **[C++ CLI Usage](../README.md)** - Command-line interface and core features
- **[Architecture](../README.md#architecture)** - System design and components

### **JavaScript/Node.js Integration**
- **[Node.js Guide](nodejs.md)** - Complete JavaScript API documentation
- **[NPM Package](npm-package.md)** - Package installation and usage
- **[Quick Reference](quick-reference.md)** - API cheatsheet

### **Advanced Topics**
- **[Sandbox & user namespaces](sandbox-and-userns.md)** - Fixing `bwrap: setting up uid map: Permission denied` on AppArmor-restricted hosts
- **[Node.js Integration Blueprint](../spec/NODEJS-INTEGRATION-BLUEPRINT.md)** - Technical implementation details
- **[100% Test Success Report](../spec/FINAL-ACHIEVEMENT-REPORT.md)** - Quality assurance documentation
- **[Test Validity Audit](../spec/TEST-VALIDITY-AUDIT-REPORT.md)** - Comprehensive test validation

### **Examples**
- **[C++ Examples](../examples/)** - Command-line usage examples
- **[JavaScript Examples](../examples/js/)** - Node.js automation examples

## 🎯 Quick Navigation

### **I want to...**

**Use HeadlessWeb from command line:**
- [C++ CLI Documentation](../README.md)
- [Installation Guide](../README.md#installation)

**Use HeadlessWeb from JavaScript:**
- [Node.js Quick Start](nodejs.md#quick-start)
- [API Reference](nodejs.md#api-reference)
- [NPM Package Guide](npm-package.md)

**Integrate with testing frameworks:**
- [Testing Framework](nodejs.md#testing-framework)
- [Jest Integration](npm-package.md#jest-integration)
- [Example Test Suites](../examples/js/testing-suite.js)

**Build from source:**
- [Development Setup](nodejs.md#development)
- [Build System](../README.md#installation)

**Understand the architecture:**
- [System Architecture](../README.md#architecture)
- [Node.js Integration Design](../spec/NODEJS-INTEGRATION-BLUEPRINT.md)

## 🚀 Feature Overview

### **Core Capabilities**
- ✅ **WebKitGTK 6.0** - Modern web standards support
- ✅ **Comprehensive test suite** - High reliability
- ✅ **Session persistence** - Maintain state across runs
- ✅ **Screenshot capture** - Visual verification
- ✅ **Form automation** - Complete form interaction
- ✅ **JavaScript execution** - Custom script support

### **JavaScript API Features**
- ✅ **Dual API design** - Async operations + sync queries
- ✅ **Fluent interface** - Chainable operations
- ✅ **Built-in testing** - Assertions and reporting
- ✅ **Event monitoring** - Comprehensive event system
- ✅ **TypeScript support** - Complete type definitions
- ✅ **CLI interface** - Command-line automation

### **Platform Support**
- ✅ **Linux** (Ubuntu, Debian, CentOS, etc.)
- ✅ **macOS** (10.15+)
- ✅ **Node.js** (16+)
- ✅ **Docker** containers
- ✅ **CI/CD** integration (GitHub Actions, etc.)

## 📖 Documentation Structure

```
docs/
├── README.md              # This index
├── nodejs.md              # Complete Node.js guide
├── npm-package.md         # NPM package documentation
└── quick-reference.md     # API cheatsheet

spec/
├── NODEJS-INTEGRATION-BLUEPRINT.md    # Technical design
├── FINAL-ACHIEVEMENT-REPORT.md         # 100% test success
└── TEST-VALIDITY-AUDIT-REPORT.md       # Quality validation

examples/
├── cpp/                   # C++ CLI examples
└── js/                    # JavaScript examples
    ├── basic-usage.js     # Getting started
    └── testing-suite.js   # Testing framework
```

## 🎓 Learning Path

### **Beginner**
1. Start with [Quick Reference](quick-reference.md)
2. Try [JavaScript Examples](../examples/js/basic-usage.js)
3. Read [Node.js Quick Start](nodejs.md#quick-start)

### **Intermediate**
1. Explore [API Reference](nodejs.md#api-reference)
2. Learn [Testing Framework](nodejs.md#testing-framework)
3. Check [NPM Package Guide](npm-package.md)

### **Advanced**
1. Study [Integration Blueprint](../spec/NODEJS-INTEGRATION-BLUEPRINT.md)
2. Review [Architecture](../README.md#architecture)
3. Contribute via [Development Guide](nodejs.md#development)

## 🔧 Common Use Cases

### **Web Testing**
```javascript
const test = browser.createTest('Login Test');
await test
    .navigate('/login')
    .type('#username', 'user')
    .assertExists('.dashboard');
```
📖 **Learn more**: [Testing Framework](nodejs.md#testing-framework)

### **Data Extraction**
```javascript
await browser.navigate('https://api.com');
const data = browser.getText('body');
const parsed = JSON.parse(data);
```
📖 **Learn more**: [DOM Interaction](nodejs.md#dom-interaction-sync)

### **Form Automation**
```javascript
await browser.type('#email', 'user@example.com');
await browser.click('#submit');
const success = browser.exists('.success');
```
📖 **Learn more**: [Form Automation](npm-package.md#form-interaction)

### **Screenshot Capture**
```javascript
await browser.navigate('https://example.com');
await browser.screenshot('homepage.png');
```
📖 **Learn more**: [Screenshots](nodejs.md#screenshots)

## 🤝 Contributing

### **Documentation**
- Improve existing guides
- Add new examples
- Update API references
- Fix typos and clarity

### **Code**
- Enhance JavaScript API
- Add new features
- Fix bugs
- Improve tests

### **Community**
- Help users in issues
- Share usage examples
- Write blog posts
- Create tutorials

**Contribution Guide**: [Development Setup](nodejs.md#development)

## 📞 Support

### **Getting Help**
- **Quick questions**: [Quick Reference](quick-reference.md)
- **Detailed usage**: [Node.js Guide](nodejs.md)
- **Package issues**: [NPM Package Guide](npm-package.md)
- **Bug reports**: [GitHub Issues](https://github.com/user/headlessweb/issues)

### **Community**
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and community support
- **Examples**: Real-world usage patterns

## 🎯 Related Projects

### **Similar Tools**
- **Puppeteer** - Chrome DevTools Protocol
- **Playwright** - Multi-browser automation
- **Selenium** - WebDriver protocol

### **HeadlessWeb Advantages**
- ✅ **Native performance** (C++ core)
- ✅ **WebKit focus** (Safari compatibility)
- ✅ **Minimal dependencies** (system WebKit)
- ✅ **Session persistence** (automatic state saving)
- ✅ **100% test reliability** (proven stability)

---

## 🏆 Project Status

**Production Ready** - HeadlessWeb has comprehensive test coverage and is ready for production use.

### **Recent Milestones**
- ✅ **HTTP header capture/export** - Session headers stored and exportable to JSON
- ✅ **Complete Node.js integration** - Full JavaScript API
- ✅ **Comprehensive documentation** - Production-ready guides
- ✅ **NPM package ready** - Easy installation and usage

### **Quality Metrics**
- **Platform Support**: Linux, macOS
- **Language Support**: C++, JavaScript/Node.js
- **Documentation**: Complete guides and examples

---

**Ready to automate the web? Start with the [Quick Reference](quick-reference.md)!** 🚀