/**
 * Noetics WASM SDK Wrapper v1.0 - Direct Binding Edition
 * Reconstructed for verified symbol access.
 */
class NoeticsSDK {
    constructor(wasmModule) {
        this.module = wasmModule;
        
        // Direct binding to verified symbols
        const bindDirect = (jsName, cNameWithUnderscore) => {
            const f = this.module[cNameWithUnderscore];
            if (typeof f !== 'function') {
                throw new Error(`CRITICAL: Symbol ${cNameWithUnderscore} not found in WASM module.`);
            }
            this[jsName] = f;
        };

        // Core lifecycle & PKGF flows
        bindDirect('_createContext', '_noetics_create_context');
        bindDirect('_createKey', '_noetics_key_create');
        bindDirect('_stepUnified', '_noetics_step_unified');
        bindDirect('_computeEnergy', '_noetics_compute_energy');
        
        // Analytical functions
        bindDirect('_computeEntropy', '_wasm_noetics_compute_entropy');
        bindDirect('_detectRankJump', '_wasm_noetics_detect_rank_jump');
        bindDirect('_exportStructure', '_wasm_noetics_export_structure');
        bindDirect('_importStructure', '_wasm_noetics_import_structure');
        bindDirect('_keyDataPtr', '_wasm_noetics_key_data_ptr');
        bindDirect('_assignSector', '_wasm_noetics_assign_sector_indices');
        bindDirect('_computeSectorEnergy', '_wasm_noetics_compute_sector_energy');
    }

    createContext(dim, nSectors) { return this._createContext(dim, nSectors); }
    createKey(ctxPtr) { return this._createKey(ctxPtr); }
    stepUnified(keyPtr, oPtr, dPtr, l, dt) { return this._stepUnified(keyPtr, oPtr, dPtr, l, dt); }
    energy(keyPtr) { return this._computeEnergy(keyPtr); }
    entropy(keyPtr) { return this._computeEntropy(keyPtr); }
    keyDataPtr(keyPtr) { return this._keyDataPtr(keyPtr); }

    assignSector(keyPtr, sectorId, indices) {
        const count = indices.length;
        const ptr = this.module._malloc(count * 4);
        this.module.HEAP32.set(new Int32Array(indices), ptr >> 2);
        const res = this._assignSector(keyPtr, sectorId, ptr, count);
        this.module._free(ptr);
        return res;
    }

    sectorEnergy(keyPtr, sectorId) {
        return this._computeSectorEnergy(keyPtr, sectorId);
    }

    detectRankJump(keyPtrs) {
        const count = keyPtrs.length;
        const ptr = this.module._malloc(count * 4);
        this.module.HEAP32.set(new Uint32Array(keyPtrs), ptr >> 2);
        const res = this._detectRankJump(ptr, count);
        this.module._free(ptr);
        return res;
    }

    exportStructure(keyPtr) {
        const structPtr = this._exportStructure(keyPtr);
        const dim = this.module.HEAP32[structPtr >> 2];
        const evalsPtr = this.module.HEAP32[(structPtr + 4) >> 2];
        const evecsPtr = this.module.HEAP32[(structPtr + 8) >> 2];
        const eigenvalues = Array.from(new Float64Array(this.module.HEAPF64.buffer, evalsPtr, dim));
        const eigenvectors = Array.from(new Float64Array(this.module.HEAPF64.buffer, evecsPtr, dim * dim));
        return { dim, eigenvalues, eigenvectors, _ptr: structPtr };
    }

    importStructure(ctxPtr, structure) {
        return this._importStructure(ctxPtr, structure._ptr);
    }
}

if (typeof module !== 'undefined') {
    module.exports = NoeticsSDK;
}
