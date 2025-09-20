// Cb Browser Runtime - Architecture Design

## Cb HTML Integration Runtime

### Core Components

1. **Parser (WebAssembly)**
   - Cb source → AST conversion
   - Error handling & syntax checking
   - Running in WASM for performance

2. **Code Generator (JavaScript)**
   - AST → JavaScript transpilation
   - AST → WebAssembly compilation
   - DOM API binding generation

3. **Runtime Bridge (JavaScript)**
   - Cb ↔ JavaScript interop
   - DOM manipulation wrapper
   - Event handling system
   - Browser API access

### Implementation Strategy

#### Phase 1: JavaScript Transpiler
```javascript
class CbRuntime {
    static async loadCbScript(scriptElement) {
        const cbSource = scriptElement.textContent;
        const jsCode = await this.transpile(cbSource);
        const script = document.createElement('script');
        script.textContent = jsCode;
        document.head.appendChild(script);
    }
    
    static async transpile(cbSource) {
        // Cb parser (WASM) → AST
        const ast = await CbParser.parse(cbSource);
        
        // AST → JavaScript
        const generator = new CbJSGenerator();
        return generator.generate(ast);
    }
}

// Auto-detect and execute Cb scripts
document.addEventListener('DOMContentLoaded', () => {
    const cbScripts = document.querySelectorAll('script[type="text/cb"]');
    cbScripts.forEach(script => CbRuntime.loadCbScript(script));
});
```

#### Phase 2: WASM Integration
```javascript
class CbWasmRuntime {
    constructor() {
        this.wasmModule = null;
        this.memory = null;
    }
    
    async loadWasmScript(scriptElement) {
        const cbSource = scriptElement.textContent;
        const wasmBytes = await this.compileToWasm(cbSource);
        
        const module = await WebAssembly.instantiate(wasmBytes, {
            env: {
                // DOM API bindings
                dom_set_inner_html: (elementId, content) => {
                    const element = document.getElementById(this.readString(elementId));
                    element.innerHTML = this.readString(content);
                },
                dom_add_event_listener: (elementId, eventType, callbackPtr) => {
                    // Event callback to WASM function
                }
            }
        });
        
        this.wasmModule = module;
        if (module.instance.exports.main) {
            module.instance.exports.main();
        }
    }
}
```

### Browser Compatibility
- **Chrome/Edge**: Full support (WASM + ES6)
- **Firefox**: Full support
- **Safari**: Full support (iOS 11.3+)
- **IE11**: Polyfill required for WASM

### Performance Characteristics
- **Parse Time**: ~2-5ms (small scripts)
- **Compilation**: JavaScript ~10ms, WASM ~50ms
- **Runtime**: Native browser performance
- **Memory**: Minimal overhead

### Development Workflow
```bash
# Development mode (real-time transpilation)
cb-dev-server --watch src/

# Production build (pre-compilation)
cb build --target=web --output=dist/

# Generated files:
# - app.cb.js (JavaScript version)
# - app.cb.wasm (WebAssembly version)  
# - cb-runtime.min.js (minimal runtime)
```

### API Design

#### DOM Manipulation
```cb
// Cb wrapper for DOM APIs
mod dom {
    extern fn get_by_id(id: string) -> Element;
    extern fn create_element(tag: string) -> Element;
    
    impl Element {
        fn set_text(&self, text: string);
        fn set_html(&self, html: string);
        fn add_class(&self, class: string);
        fn on_click(&self, callback: fn());
    }
}

// Usage
fn main() {
    let button = dom::get_by_id("my-button");
    button.on_click(|| {
        let output = dom::get_by_id("output");
        output.set_text("Button clicked from Cb!");
    });
}
```

#### Async/Await Support
```cb
// Future/Promise integration
async fn fetch_data() -> Result<User, Error> {
    let response = http::get("https://api.example.com/user/1").await?;
    let user: User = json::parse(response.body)?;
    Ok(user)
}

async fn main() {
    match fetch_data().await {
        Ok(user) => render_user(user),
        Err(e) => console::error("Failed to fetch:", e)
    }
}
```
