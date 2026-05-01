/**
 * Noetics WASM SDK Wrapper
 * Provides a high-level JS interface to the PKGF engine.
 */
class NoeticsSDK {
    constructor(wasmModule) {
        this.module = wasmModule;
        
        // Wrap C functions
        this._createContext = this.module.cwrap('noetics_create_context', 'number', ['number', 'number']);
        this._createKey = this.module.cwrap('noetics_key_create', 'number', ['number']);
        this._stepUnified = this.module.cwrap('noetics_step_unified', 'number', ['number', 'number', 'number', 'number', 'number']);
        this._computeEnergy = this.module.cwrap('noetics_compute_energy', 'number', ['number']);
        this._computeEntropy = this.module.cwrap('wasm_noetics_compute_entropy', 'number', ['number']);
    }

    createContext(dim, nSectors) {
        return this._createContext(dim, nSectors);
    }

    createKey(ctxPtr) {
        return this._createKey(ctxPtr);
    }

    stepUnified(keyPtr, omegaPtr, dopPtr, lambda, dt) {
        return this._stepUnified(keyPtr, omegaPtr, dopPtr, lambda, dt);
    }

    energy(keyPtr) {
        return this._computeEnergy(keyPtr);
    }

    entropy(keyPtr) {
        return this._computeEntropy(keyPtr);
    }

    // Substrate Invariance Demo Helper
    exportStructure(keyPtr) {
        const structPtr = this.module._wasm_noetics_export_structure(keyPtr);
        // JS extraction logic for eigenvalues/vectors from WASM heap
        return structPtr;
    }
}

if (typeof module !== 'undefined') {
    module.exports = NoeticsSDK;
}
