# RSA Algorithms in C++

An educational C++ lab exploring arbitrary-precision arithmetic and number theory operations used in RSA. The programs implement their own unsigned big-integer representation using `std::vector<uint64_t>`, without an external big-integer library.

The original lab source files are preserved unchanged. This repository reorganizes the files and adds build instructions; it does not refactor or correct the algorithms.

## Programs

| Executable | Purpose | Original lab task |
| --- | --- | --- |
| `primality_check` | Miller–Rabin probable-prime check using 12 fixed bases | `project_01_01` |
| `rsa_private_exponent` | Compute `d = e^-1 mod ((p - 1)(q - 1))`, or write `-1` when the GCD check fails | `project_01_02` |
| `modular_exponentiation` | Compute `y = x^k mod N` | `project_01_03` |

Topics covered include limb-based big integers, carry and borrow handling, bit shifts, modular arithmetic, binary GCD, and modular inversion.

## Repository structure

```text
.
├── CMakeLists.txt
├── README.md
├── .gitignore
├── src/
│   ├── primality/main.cpp
│   ├── private_exponent/main.cpp
│   └── modular_exponentiation/main.cpp
└── docs/
    └── 23127448_Report.pdf
```

The three programs are independent executables. Their big-integer implementations remain in their original source files.

## Build

Use a C++11-compatible compiler. Run commands from the repository root.

### Direct build with GCC / MinGW

Create a `build` directory, then run:

```sh
g++ -std=c++11 -O2 src/primality/main.cpp -o build/primality_check
g++ -std=c++11 -O2 src/private_exponent/main.cpp -o build/rsa_private_exponent
g++ -std=c++11 -O2 src/modular_exponentiation/main.cpp -o build/modular_exponentiation
```

On Windows, GCC produces executables with the `.exe` extension. Create the directory using `New-Item -ItemType Directory -Force build` in PowerShell or `mkdir -p build` in a POSIX shell.

### CMake (optional)

Install CMake 3.16 or later and a compatible C++ build toolchain:

```sh
cmake -S . -B build
cmake --build build --config Release
```

For MinGW, select its generator on the first configuration:

```sh
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Executable locations depend on the generator: commonly `build/` for single-configuration generators and `build/Release/` for Visual Studio.

## Input format

All numeric inputs use **hexadecimal digits in reverse order**. This is digit-level reversal, not byte-level little-endian encoding.

| Decimal value | Conventional hex | Input used here |
| --- | --- | --- |
| 61 | `3D` | `D3` |
| 65 | `41` | `14` |
| 3233 | `CA1` | `1AC` |

Use one value per line. Uppercase and lowercase hex digits are accepted. Inputs should satisfy the mathematical preconditions of each operation.

### Primality check

```sh
./build/primality_check input.inp result.out
```

Input: one line containing `n`. Output: `1` if the fixed-base Miller–Rabin test accepts the number, otherwise `0`.

Example input representing 61:

```text
D3
```

### RSA private exponent

```sh
./build/rsa_private_exponent input.inp
```

Input: three lines containing `p`, `q`, and `e`, respectively. Use distinct primes `p` and `q` and a valid public exponent. The output path is derived from the input path: `input.inp` becomes `input.out`.

Example input representing `p = 61`, `q = 53`, `e = 17`:

```text
D3
53
11
```

### Modular exponentiation

```sh
./build/modular_exponentiation input.inp result.out
```

Input: three lines containing `N`, `k`, and `x`, respectively. Use `N > 1`, `k >= 0`, and `0 <= x < N`. Output uses the same reversed-hex format.

Example input representing `N = 3233`, `k = 17`, `x = 65`:

```text
1AC
11
14
```

For PowerShell, the first command above can be written as `.\build\primality_check.exe input.inp result.out`; use the corresponding `.exe` names for the other programs.

## Scope and limitations

This is an academic implementation, not a production cryptographic library. It does not provide padding, secure key generation, or a complete secure messaging system. The fixed Miller–Rabin bases are not a general proof of primality for arbitrary-size inputs.

The original algorithms have not undergone comprehensive correctness testing. Known issues identified during source review include borrow propagation in big-integer subtraction, factor restoration in binary GCD, and a potential unsigned boundary overflow in the division search. Invalid input handling is also limited. These issues remain in the preserved lab code.

## Lab report

The original report is available in [docs/23127448_Report.pdf](docs/23127448_Report.pdf).
