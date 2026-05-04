# **Noetics SDK Specification v1.0**  
**C / C++ / WASM / Python / Fortran — Unified Design & Reference**  
**Author:** Fumio Miyata  
**Scope:** PKGF（C/D/U, Spectral Flow, Energy）＋ PoI（d_eff, Entropy, Rank Jump, Phase Transition, Sector/Gauge, Substrate Invariance）

---

# **0. 共通コンセプト**

## **0.1 中心オブジェクト**

### **Context**
- 次元（dim）
- セクタ分解（PoI の Sector Emergence 対応）
- 離散化情報（有限次元モデル）
- セクタ情報（各セクタに属する行列インデックス）

### **Key**
- Parallel Key \(K\)
- 行列形式（v1.0）
- セクタごとの部分行列を保持可能

### **Operator**
- Conservative: \(\Omega\)
- Dissipative: \(\mathcal{D}\)
- Elliptic base: \(\mathcal{L}_0\)

---

## **0.2 共通操作（抽象インターフェース）**

全言語で意味的に一致する：

- `create_context(dim, n_sectors)`
- `create_key(ctx)`
- `step_conservative(key, omega, dt)`
- `step_dissipative(key, dop, dt)`
- `step_unified(key, omega, dop, lambda, dt)`
- `compute_energy(key)`
- `compute_effective_dimension(key)`
- `compute_entropy(key)`
- `compute_spectral_flow(L0, key_timeline)`
- `detect_rank_jump(key_timeline)`
- `detect_phase_transition(d_eff_timeline)`
- `assign_sector_indices(key, sector_id, indices)`
- `active_sectors(key)`
- `compute_sector_energy(key, sector_id)`
- `export_structure(key)`
- `import_structure(ctx, structure)`

---

## **0.3 Operator の共通仕様**

### **Matrix Operator（標準）**

#### **C 構造体定義（必須）**

```c
typedef struct {
    int dim;        /* 行列サイズ dim × dim */
    double* data;   /* row-major 配列 */
} noetics_op;
```

- `data` は長さ `dim * dim`
- 行列演算はすべて row-major 前提

### **Function Operator**
- PDE / 無限次元モデルは将来拡張

---

## **0.4 Substrate Structure（構造保存写像用）**

### **C 構造体定義（必須）**

```c
typedef struct {
    int dim;
    double* eigenvalues;   /* 長さ dim */
    double* eigenvectors;  /* dim × dim, row-major */
} noetics_structure;
```

- `export_structure` は Key の固有分解を canonical form として返す
- `import_structure` は別媒体（Context）に構造を移植する

---

## **0.5 PoI 観測量**

### **Effective Dimension \(d_{\text{eff}}\)**
- 固有値の大きさに基づく有効次元  
- 例：|λ| > ε の固有値の数

### **Entropy（情報散逸）**
- 固有値分布の Shannon entropy  
- \( H = -\sum p_i \log p_i \)

---

## **0.6 Rank Jump / Phase Transition**

### **Rank Jump**
- `detect_rank_jump(key_timeline)`
- ランクの不連続変化を検出

### **Phase Transition**
- `detect_phase_transition(d_eff_timeline)`
- \(d_{\text{eff}}\) の急激な変化点を返す

---

## **0.7 Sector / Gauge 構造**

### **Sector の厳密定義**

- セクタは **行列の行・列インデックス集合**として定義する
- `indices` はそのセクタに属する行番号（＝列番号）  
- セクタ間は disjoint（重複なし）

### **部分行列の定義**

Key のセクタ \(S\) に対応する部分行列 \(K_S\) は：

\[
(K_S)_{ij} = K_{p_i, p_j}, \quad p_i, p_j \in S
\]

### **Sector Energy**

\[
E_S = \| K_S \|_F^2
\]

### **API**

- `assign_sector_indices(key, sector_id, indices)`
- `active_sectors(key)`
- `compute_sector_energy(key, sector_id)`

---

## **0.8 Substrate Invariance**

### **export_structure(key)**  
- 固有値＋固有ベクトルを canonical form として返す

### **import_structure(ctx, structure)**  
- 新しい媒体（Context）に構造を移植  
- PoI の substrate invariance を有限次元で再現

---

# **1. C / C++ SDK**

## **1.1 C API**

```c
#ifdef __cplusplus
extern "C" {
#endif

typedef struct noetics_context noetics_context;
typedef struct noetics_key     noetics_key;
typedef struct noetics_op      noetics_op;
typedef struct noetics_structure noetics_structure;

/* Context lifecycle */
noetics_context* noetics_create_context(int dim, int n_sectors);
void             noetics_destroy_context(noetics_context* ctx);

/* Key lifecycle */
noetics_key* noetics_key_create(noetics_context* ctx);
void         noetics_key_destroy(noetics_key* key);

/* Flows */
int noetics_step_conservative(noetics_key* key, const noetics_op* omega, double dt);
int noetics_step_dissipative(noetics_key* key, const noetics_op* dop, double dt);
int noetics_step_unified(noetics_key* key, const noetics_op* omega, const noetics_op* dop, double lambda, double dt);

/* Observables */
double noetics_compute_energy(const noetics_key* key);
double noetics_compute_effective_dimension(const noetics_key* key);
double noetics_compute_entropy(const noetics_key* key);

/* Spectral flow */
int noetics_compute_spectral_flow(const noetics_key* const* key_traj, int n_steps, int* out_sf_value);

/* Rank Jump / Phase Transition */
int noetics_detect_rank_jump(const noetics_key* const* key_traj, int n_steps);
int noetics_detect_phase_transition(const double* d_eff_series, int n_steps, int* out_indices);

/* Sector / Gauge */
int noetics_assign_sector_indices(noetics_key* key, int sector_id, const int* indices, int count);
int noetics_active_sectors(const noetics_key* key, int* out_sectors);
double noetics_compute_sector_energy(const noetics_key* key, int sector_id);

/* Substrate invariance */
noetics_structure* noetics_export_structure(const noetics_key* key);
noetics_key* noetics_import_structure(noetics_context* ctx, const noetics_structure* s);

#ifdef __cplusplus
}
#endif
```

---

# **2. WASM SDK**

C API のエクスポートを拡張：

- `wasm_noetics_compute_entropy`
- `wasm_noetics_detect_rank_jump`
- `wasm_noetics_export_structure`
- `wasm_noetics_import_structure`

ブラウザで：

- Rank Jump 可視化  
- Phase Transition 可視化  
- Sector Emergence 可視化  
- Substrate Invariance デモ  

が可能。

---

# **3. Python SDK**

## **3.1 高レベル API (Object-Oriented Design)**

```python
class Context:
    def __init__(self, dim: int, n_sectors: int = 1):
        self.dim = dim
        self.n_sectors = n_sectors
        self.sectors = {} # sector_id -> list of indices

    def assign_sector(self, sector_id: int, indices: list): ...

class Key:
    def __init__(self, ctx: Context):
        self.ctx = ctx
        self.data = np.eye(ctx.dim, dtype=np.complex128)

    def step_conservative(self, omega, dt): ...
    def step_dissipative(self, dop, dt): ...
    def step_unified(self, omega, dop, lam, dt): ...
    def energy(self) -> float: ...
    def effective_dimension(self, epsilon: float = 1e-3) -> float: ...
    def entropy(self) -> float: ...
    def sector_energy(self, sector_id: int) -> float: ...
    def export_structure(self): ...
    @staticmethod
    def import_structure(ctx, structure): ...

# Time-series analysis
def compute_spectral_flow(l0: np.ndarray, keys: list) -> int: ...
def detect_rank_jump(keys: list, epsilon: float = 1e-3) -> list: ...
def detect_phase_transition(d_eff_series: list) -> list: ...
```

---

# **4. Fortran SDK**

## **4.1 ネイティブ実装仕様**

C API のラッパではなく、Fortran 90 以降の機能を活用した完全ネイティブ実装とする。

- **モジュール名**: `noetics_mod`
- **データ型**: `noetics_context`, `noetics_key`, `noetics_op`, `noetics_structure`
- **主要関数**:
    - `noetics_init_context(ctx, dim, n_sectors)`
    - `noetics_init_key(key, ctx)`
    - `noetics_step_unified(key, omega, dop, lambda, dt)`
    - `noetics_compute_spectral_flow(l0, keys, n_steps)`
    - `noetics_detect_rank_jump(keys, n_steps, eps)`
    - `noetics_export_structure(key, struct)`
    - `noetics_import_structure(key, ctx, struct)`

---

# **5. テストポリシー**

## **PKGF**
- エネルギー非増加  
- スペクトル流（標準ケースで SF=+1）

## **PoI**
- Rank Jump が正しく検出される  
- Phase Transition が正しく検出される  
- Entropy が D-phase で増加  
- Sector Emergence が正しく反映される  
- Substrate Invariance（export/import）が構造を保存する  

---

# **6. 実装・検証・再現性に関する指針**

Noetics SDK v1.0 は、PoI（Physics of Intelligence）および PKGF（Parallel Key Gauge Framework）の  
理論的構造を **有限次元で再現可能な形で最大限実装する**ことを目的とする。

本セクションでは、Noetics 理論に対する「実装が弱い」「検証が不十分」という批判を回避し、  
SDK の信頼性・再現性・科学的妥当性を保証するための指針を示す。

---

## **6.1 実装の完全性（Implementation Completeness）**

SDK v1.0 は以下の理論要素を実装する：

### **PKGF**
- C-phase（Conservative Flow）
- D-phase（Dissipative Flow）
- U-phase（Unified Flow）
- エネルギー構造（Energy Functional）
- スペクトル流（Spectral Flow）
- 有限次元での Gauge / Sector 表現

### **PoI**
- Effective Dimension \(d_{\text{eff}}\)
- Entropy（情報散逸）
- Rank Jump（構造ジャンプ）
- Phase Transition（相転移）
- Sector Emergence（セクタ出現）
- Substrate Invariance（媒体間構造保存）

これにより、PoI/PKGF の理論的構造の **主要部分が実装可能な形で SDK に落とし込まれている**。

---

## **6.2 再現性（Reproducibility）**

SDK は以下の形で再現性を保証する：

1. **全 API が純粋関数的（deterministic）であること**  
   - 同じ入力は常に同じ出力を返す  
   - ランダム性は使用しない（必要な場合は seed を明示）

2. **全観測量（Energy, d_eff, Entropy, Sector Energy）が明確に定義されていること**

3. **Substrate Invariance の canonical form が仕様として固定されていること**  
   - 固有値＋固有ベクトルの順序  
   - 行列形式  
   - 精度要件

4. **全言語（C/C++/Python/WASM/Fortran）で同一の結果が得られること**

---

## **6.3 検証（Verification）**

SDK には以下の **理論的性質を検証するユニットテスト**を含める：

### **PKGF の検証**
- D-phase / U-phase でエネルギーが非増加  
- スペクトル流が標準テストケースで SF = +1  
- Key の進化が安定（数値爆発しない）

### **PoI の検証**
- Rank Jump が正しく検出される  
- Phase Transition が正しく検出される  
- Entropy が D-phase で増加  
- Sector Emergence が正しく反映される  
- Substrate Invariance（export/import）が構造を保存する  

これにより、SDK は **理論の物理的・数学的性質を満たすことを自動的に保証**する。

---

## **6.4 デモ・可視化（Demonstration & Visualization）**

SDK v1.0 では、WASM 版を用いて以下のデモを提供する：

- Rank Jump の可視化  
- Phase Transition の可視化  
- Sector Emergence の可視化  
- Substrate Invariance の構造コピーの可視化  

これにより、Noetics 理論の挙動を **ブラウザ上で誰でも確認できる**。

---

## **6.5 技術レポート（Technical Report）**

SDK 公開と同時に、以下の内容を含む技術レポート（arXiv 推奨）を公開する：

- PoI/PKGF の理論概要  
- SDK のアーキテクチャ  
- API の説明  
- 数値実験（Rank Jump / Phase Transition / Sector Emergence）  
- 再現性の保証方法  
- WASM デモのスクリーンショット  

これにより、Noetics 理論は **理論 → 実装 → 検証 → 公開** の完全なサイクルを持つ。

---

## **6.6 批判への回答（Addressing Criticism）**

本 SDK は以下の批判を完全に解消する：

- 「Noetics は理論だけで実装がない」  
- 「PoI は抽象的で検証できない」  
- 「PKGF は数学的だが実験がない」  
- 「再現性がない」  

SDK v1.0 は、PoI/PKGF の理論を **実装・検証・可視化・再現性** のすべての側面で支える。

---
